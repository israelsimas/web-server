/**************************************************************************
 * misch.h
 *
 *  Create on: 31/08/2017
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform SIP Intelbras
 *
 * Copyrights Intelbras, 2017
 *
 **************************************************************************/
#ifndef MISC_H_
#define MISC_H_

/**************************************************************************
 * INCLUDES
 **************************************************************************/
#include <syslog.h>
#include <stdio.h>

#define SYSLOG_NAME			"web-server"
#define LOG_FACILITY    LOG_LOCAL4
#define OPEN_LOG 			  openlog(SYSLOG_NAME, 0, LOG_FACILITY);

#define LOG(str,...)      	        PRINT_LOG((LOG_DEBUG | LOG_FACILITY),str,##__VA_ARGS__)
#define LOG_ERROR(str,...)          PRINT_ERROR_LOG(LOG_ERR,str,##__VA_ARGS__)
#define LOG_ERROR_ASSERT(str,...)   PRINT_ERROR_ASSERT_LOG(LOG_ERR,str,##__VA_ARGS__)

#ifdef PLATFORM_X86 // SYSLOG or PRINTF
  #define PRINT_LOG(out,str,...)               syslog(out, "%s | %s() | "str" | %d",THIS_FILE,__FUNCTION__,##__VA_ARGS__,__LINE__)
  #define PRINT_ERROR_LOG(out,str,...)         syslog(out, "## %s | %s() | "str" | %d",THIS_FILE,__FUNCTION__,##__VA_ARGS__,__LINE__)
  #define PRINT_ERROR_ASSERT_LOG(out,str,...)  syslog(out, "## %s | %s() | "str" | %d",THIS_FILE,__FUNCTION__,##__VA_ARGS__,__LINE__); ASSERT(0)
#else
	#define PRINT_LOG(out,str,...)                printf("%s | %s() | "str" | %d\n",THIS_FILE,__FUNCTION__,##__VA_ARGS__,__LINE__)
  #define PRINT_ERROR_LOG(out,str,...)          printf("## %s | %s() | "str" | %d\n",THIS_FILE,__FUNCTION__,##__VA_ARGS__,__LINE__)
  #define PRINT_ERROR_ASSERT_LOG(out,str,...)   printf("## %s | %s() | "str" | %d\n",THIS_FILE,__FUNCTION__,##__VA_ARGS__,__LINE__); ASSERT(0)
#endif

#define ERROR	  -1
#define SUCCESS	0

#define POINTER_NULL    '\0'

#endif
