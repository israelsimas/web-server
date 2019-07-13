/**************************************************************************
 *
 * system-request.c
 *
 *    Get general information to web service
 *
 * Copyright 2019 Intelbras
 *
 **************************************************************************/

#include <base64.h>
#include <hoel.h>
#include <config.h>
#include <jansson.h>
#include <misc.h>
#include <system-status.h>
#include <system-request.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <middleware.h>

#define THIS_FILE "system-request.c"

void restartSystem() {

  int status = system("sync && sleep 3 && reboot");
  if (WEXITSTATUS(status) != SUCCESS) {
    LOG_ERROR("Error to execute command");
  }
}

void restartSyslog() {
  
  int status = system("/etc/init.d/S60syslog restart");
  if (WEXITSTATUS(status) != SUCCESS) {
    LOG_ERROR("Error to execute command");
  }  
}

void factoryReset() {

  system("rm -r /data/*");
  system("rm -rf /run/firmware");
  system("rm -rf /run/logs");
  system("sync");
  system("/etc/rc5.d/S60syslog stop");
  system("/etc/rc5.d/S25files_default start");
  system("/etc/rc5.d/S28mac start");
  system("killall control-call");

  // TODO
  //setSelfProvisioning('update_enable_config=1&update_url_server=undefined&update_path=&update_authType=2&update_username=&update_password=&update_protocol=3&update_turn_on=1&update_repeat=0&update_interval=undefined&update_weekly=0&update_begin_hour=&update_begin_minutes=&update_end_hour=&update_end_minutes=&weeks_mask=127', lighty.stat)	
}

void logoReset() {

  system("rm /data/images/*");
  system("cp /etc/images/logo.bmp /data/images/logo.bmp"); 	
	system("sync"); 
}

void setLanguage() {
  // TODO
  // Not necessary to set language because Table update will change the language
}

void notifyTables(const char *pchTables) {
  sendMiddlewareMessage(pchTables);
}

void generalNotify(const char *pchTables) {
  sendMiddlewareMessage(pchTables);
}

void setNTPDateTime() {
  
  system("/etc/rc5.d/S96ntpdate stop");
  system("sleep 2 && /etc/rc5.d/S96ntpdate update");  
}

void setManualDateTime(const char *pchDate) {
    
  system("/etc/rc5.d/S96ntpdate start 2>&1 >> /dev/null"); 
}

void setChangeBootPartition(int bootPartition) {
  
  char *pchComand;
  char *pchPartition = PARTITION_1;

  if (bootPartition == 2) {
    pchPartition = PARTITION_2;
  }

  pchComand = msprintf("fw_setenv rootfspart %s && fw_setenv linux_bootargs root=mtd:%s rootfstype=jffs2", pchPartition, pchPartition);
  system(pchComand);
  o_free(pchComand); 
}

static void createAutopFile(struct _u_map *map_url) {

  int i, size;
  char *pchContent, *pchParam, *pchValue;
  const char **ppKeys;
  FILE *pFile = fopen("/data/autop_params.lua", "rb");
  BOOL bValidParam = FALSE;

  system("rm /data/autop_params.lua");

  ppKeys = u_map_enum_keys(map_url);

  pFile = fopen("/data/autop_params.lua", "w+");
  if (pFile != NULL) {

    fwrite("params = {}\n", o_strlen("params = {}\n"), 1, pFile);

    pchContent = msprintf("\nparams.thisFile = \"src/magnet/request_to_os.lua\" \nparams.webconfDir = \"/var/www/\"");
    size = o_strlen(pchContent);
    fwrite(pchContent, size, 1, pFile);
    o_free(pchContent);

    for (i = 0; i < map_url->nb_values; i++) {

      bValidParam = TRUE;
      pchParam = ppKeys[i];
      pchValue = u_map_get(map_url, ppKeys[i]);

      if (!strcmp(pchParam, "update_repeat")) {
        if (!strcmp(pchValue, "1"))  {
          pchContent = msprintf("\nparams.periodic = true");
        } else {
           pchContent = msprintf("\nparams.periodic = false");
        }
      } else if (!strcmp(pchParam, "update_weekly")) {
        if (!strcmp(pchValue, "1"))  {
          pchContent = msprintf("\nparams.weekly = true");
        } else {
           pchContent = msprintf("\nparams.weekly = false");
        }
      } else if (!strcmp(pchParam, "weeks_mask")) {
        pchContent = msprintf("\nparams.weekdays = %s", pchValue);
      } else if (!strcmp(pchParam, "update_interval")) {
        pchContent = msprintf("\nparams.period = %s", pchValue);
      } else if (!strcmp(pchParam, "update_turn_on")) {
        if (!strcmp(pchValue, "1"))  {
          pchContent = msprintf("\nparams.onBoot = true");
        } else {
           pchContent = msprintf("\nparams.onBoot = false");
        }
      } else if (!strcmp(pchParam, "update_enable_config")) {
        if (!strcmp(pchValue, "1"))  {
          pchContent = msprintf("\nparams.enable = true");
        } else {
           pchContent = msprintf("\nparams.enable = false");
        }
      } else if (!strcmp(pchParam, "update_protocol")) {
        pchContent = msprintf("\nparams.protocol = %s", pchValue);
      } else if (!strcmp(pchParam, "update_authType")) {
        pchContent = msprintf("\nparams.authTye = %s", pchValue);
      } else if (!strcmp(pchParam, "update_username")) {
        pchContent = msprintf("\nparams.username = %s", pchValue);   
      } else if (!strcmp(pchParam, "update_password")) {
        pchContent = msprintf("\nparams.password = %s", pchValue);     
      } else {
        bValidParam = FALSE;
      }

      if (bValidParam) {
        size = o_strlen(pchContent);
        fwrite(pchContent, size, 1, pFile);
        o_free(pchContent);
      }
    }

    pchContent = msprintf("\nparams.macaddress = %s\n", getMac());   
    size = o_strlen(pchContent);
    fwrite(pchContent, size, 1, pFile);
    o_free(pchContent);

    fclose(pFile);
  }
}

void setSelfProvision(struct _u_map *map_url) {
  createAutopFile(map_url);
}

void startCaptureLog() {

  char *pchCommand, *pchInterface;
  int lenghtLog   = 1; 
  int numFilesLog = 2;    

  system("mkdir -p /data/logs");
  system("rm /data/logs/*");
  system("rm -r /data/firmware/*");
  system("sync");
  system("killall tcpdump");

  pchInterface = getActiveInterface();

  pchCommand = msprintf("tcpdump -i %s -C %d -W %d -w /data/logs/log &", pchInterface, lenghtLog, numFilesLog);

  system(pchCommand); 

  o_free(pchInterface);
  o_free(pchCommand);
}

void stopCaptureLog() {
  system("killall tcpdump");
}

int getAutoprovXML(char **ppchBuffer) {

  FILE *pf;
  int body_length = 0;
  long fsize;
  char *pchCmd;

  system("lua /var/www/src/web_system_request.lua export_autoprov");

  pf = fopen("cat /data/autoprov_exported.xml", "rb");
  if (pf) {

    fseek(pf, 0, SEEK_END);
    fsize = ftell(pf);
    *ppchBuffer = o_malloc(fsize + 1);
    fread(*ppchBuffer, 1, fsize, pf);

    *ppchBuffer[fsize] = 0;
    body_length = o_strlen(*ppchBuffer);
    fclose(pf);
  }

  return body_length;
}
