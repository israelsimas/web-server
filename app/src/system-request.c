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

void setSelfProvision(const char *pchParameters) {

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

void exportLog() {
  // // -- Adiciona extensao .pcap para todos os arquivos da pasta logs antes de compactar
  // f = io.popen('ls ' .. prefixPathLogs..'/logs/')
  // for name in f:lines() do
  //   os.execute('mv '..prefixPathLogs..'/logs/'.. name .. ' '..prefixPathLogs..'/logs/' ..name.. '.pcap')
  //   os.execute('sync')
  // end

  // os.execute('cd '..prefixPathLogs..'/logs/ && tar -cvf '..prefixPathLogs..'/logs/logs.tar *')
  // local logFile = io.open(prefixPathLogs..'/logs/logs.tar')

  // if (logFile) then

  //   local content = logFile:read("*all")
  //   logFile:close()
  //   lighty.header["Content-Type"] = "application/octet-stream"
  //   lighty.header["Content-Disposition"] = "Attachment;filename=logs.tar"
  //   lighty.content = { content }

  //   return 200    
  // else
  //   return 404
  // end 
}
