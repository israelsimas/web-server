/**************************************************************************
 *
 * system-request.c
 *
 *    Get general information to web service
 *
 * Copyright 2019 Intelbras
 *
 **************************************************************************/

#include <database.h>
#include <jansson.h>
#include <utils.h>
#include <system-status.h>
#include <system-request.h>
#include <middleware.h>

#define THIS_FILE "system-request.c"

extern middleware_conn conn;
extern struct _db_connection *connDB;

void restartSystem() {

  int status = system("sync && sleep 3 && reboot");
  if (WEXITSTATUS(status) != SUCCESS) {
    log_error("Error to execute command");
  }
}

void restartSyslog() {
  
  int status = system("/etc/init.d/S60syslog restart");
  if (WEXITSTATUS(status) != SUCCESS) {
    log_error("Error to execute command");
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

  // TODO: setSelfProvisioning('update_enable_config=1&update_url_server=undefined&update_path=&update_authType=2&update_username=&update_password=&update_protocol=3&update_turn_on=1&update_repeat=0&update_interval=undefined&update_weekly=0&update_begin_hour=&update_begin_minutes=&update_end_hour=&update_end_minutes=&weeks_mask=127', lighty.stat)	
}

void logoReset() {

  system("rm /data/images/*");
  system("cp /etc/images/logo.bmp /data/images/logo.bmp"); 	
	system("sync"); 
}

void setLanguage() {
  // TODO: Not necessary to set language because Table update will change the language
}

void notifyTables(const char *pchTables) {
  middleware_publish(conn, "status/database", pchTables);
}

void generalNotify(const char *pchTables) {
  middleware_publish(conn, "status/database", pchTables);
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
  char *pchContent;
  const char **ppKeys, *pchParam, *pchValue;
  FILE *pFile = fopen("/data/autop_params.lua", "rb");
  bool bValidParam = false;

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

      bValidParam = true;
      pchParam = ppKeys[i];
      pchValue = u_map_get(map_url, ppKeys[i]);

      if (!o_strcmp(pchParam, "update_repeat")) {
        if (!o_strcmp(pchValue, "1"))  {
          pchContent = msprintf("\nparams.periodic = true");
        } else {
           pchContent = msprintf("\nparams.periodic = false");
        }
      } else if (!o_strcmp(pchParam, "update_weekly")) {
        if (!o_strcmp(pchValue, "1"))  {
          pchContent = msprintf("\nparams.weekly = true");
        } else {
           pchContent = msprintf("\nparams.weekly = false");
        }
      } else if (!o_strcmp(pchParam, "weeks_mask")) {
        pchContent = msprintf("\nparams.weekdays = %s", pchValue);
      } else if (!o_strcmp(pchParam, "update_interval")) {
        pchContent = msprintf("\nparams.period = %s", pchValue);
      } else if (!o_strcmp(pchParam, "update_turn_on")) {
        if (!o_strcmp(pchValue, "1"))  {
          pchContent = msprintf("\nparams.onBoot = true");
        } else {
           pchContent = msprintf("\nparams.onBoot = false");
        }
      } else if (!o_strcmp(pchParam, "update_enable_config")) {
        if (!o_strcmp(pchValue, "1"))  {
          pchContent = msprintf("\nparams.enable = true");
        } else {
           pchContent = msprintf("\nparams.enable = false");
        }
      } else if (!o_strcmp(pchParam, "update_protocol")) {
        pchContent = msprintf("\nparams.protocol = %s", pchValue);
      } else if (!o_strcmp(pchParam, "update_authType")) {
        pchContent = msprintf("\nparams.authTye = %s", pchValue);
      } else if (!o_strcmp(pchParam, "update_username")) {
        pchContent = msprintf("\nparams.username = %s", pchValue);   
      } else if (!o_strcmp(pchParam, "update_password")) {
        pchContent = msprintf("\nparams.password = %s", pchValue);     
      } else {
        bValidParam = false;
      }

      if (bValidParam) {
        size = o_strlen(pchContent);
        fwrite(pchContent, size, 1, pFile);
        o_free(pchContent);
      }
    }

    pchContent = msprintf("\nparams.macaddress = %s\n", ntw_get_mac(DEFAULT_INTERFACE, false));   
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

  pchInterface = ntw_get_active_interface_name(connDB);

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

  system("rm /data/autoprov_exported.xml");

  return body_length;
}

int getContactXML(char **ppchBuffer, const char *pchIds) {

  FILE *pf;
  int body_length = 0;
  long fsize;
  char *pchCmd;

  pchCmd = msprintf("lua /var/www/src/web_system_request.lua export_autoprov %s", pchIds);
  system(pchCmd);

  pf = fopen("cat /data/contacts_exported.xml", "rb");
  if (pf) {

    fseek(pf, 0, SEEK_END);
    fsize = ftell(pf);
    *ppchBuffer = o_malloc(fsize + 1);
    fread(*ppchBuffer, 1, fsize, pf);

    *ppchBuffer[fsize] = 0;
    body_length = o_strlen(*ppchBuffer);
    fclose(pf);
  }

  system("rm /data/contacts_exported.xml");

  return body_length;
}
