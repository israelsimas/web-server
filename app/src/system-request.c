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
