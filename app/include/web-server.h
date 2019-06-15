/**************************************************************************
 * web-server.h
 *
 *  Create on: 07/06/2019
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform Sirius Intelbras
 *
 * Copyrights Intelbras, 2019
 *
 **************************************************************************/

#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_

/**************************************************************************
 * INCLUDES
 **************************************************************************/
#include <syslog.h>

/**************************************************************************
 * DEFINITIONS
 **************************************************************************/

#define DATABASE_PATH   "/data/database.sql"

#define POST_SIZE_MAX   1024
#define PORT_DEFAULT    8080

#define STATIC_FOLDER   "/var/www"
#define INDEX_HTML      "/index.html"

#define TEMPLATE_FILE_NAME_GZIP_PATH   "%s%s.gz"
#define TEMPLATE_FILE_NAME_PATH   "%s%s"

#define DATABASE_REQUEST  "/db.cgi"
#define STATUS_REQUEST    "/status.cgi"
#define ENDPOINT_STATUS_REQUEST "/endpoint_status.cgi"
#define STATUS_REGISTER_REQUEST "/statusRegister.cgi"
#define STATUS_GENERAL_REQUEST  "/status_general.cgi"
#define SUPPORT_GIGA_REQUEST    "/support_giga.cgi"
#define VERSIO_REQUEST          "/version.cgi"
#define RESTART_REQUEST         "/restart.cgi"
#define RESTART_SYSLOG_REQUEST  "/restart_syslog.cgi"
#define BURN_STATUS_REQUEST     "/burn_status.cgi"
#define FACTORY_RESET_REQUEST   "/factoryReset.cgi"
#define LOGO_RESET_REQUEST      "/logoReset.cgi"
#define SET_LANGUAGE_REQUEST    "/setLanguage.cgi"
#define NOTIFY_REQUEST          "/notify.cgi"
#define AUTOPROV_LOG_REQUEST    "/autoprov.log"

#define NOT_FOUND_MESSAGE "Page not found, do what you want"

#define NUM_MAX_QUERY_COMANDS       10
#define NUMBER_WHITE_LIST_COMMANDS  7

#define HEADER_JSON_DB  100

#define JSON1_CONTENT "[\n   {\n   \"rows\":[\n               {\n         \"SYSPhoneLanguage\":\"pt_BR\"\n         }\n      ]\n   }\n]"
#define JSON2_CONTENT "[\
   {\
   \"rows\":[\
               {\
         \"weekStart\":1,\
         \"SYSTimeNTPSecondAddress\":\"b.ntp.br\",\
         \"hourStop\":0,\
         \"SYSTimeManual\":0,\
         \"SYSTimeEnableNTP\":2,\
         \"weekDayStop\":1,\
         \"dayStart\":1,\
         \"SYSTimeNTPFirstAddress\":\"a.ntp.br\",\
         \"monthStart\":1,\
         \"offsetDaylightSaving\":1,\
         \"SYSTimeUpdateInterval\":10,\
         \"dayStop\":1,\
         \"hourStart\":0,\
         \"SYSTimeDayligthSaving\":0,\
         \"monthStop\":2,\
         \"weekStop\":1,\
         \"weekDayStart\":1,\
         \"SYSTimeDateFormat\":\"DD/MM/AA\",\
         \"SYSTimeTimeZone\":-181,\
         \"SYSTimeFormat\":\"HH:MM:ss\"\
         }\
      ]\
   }\
]"

#define STATUS_CONTENT "{\n\
\"account\":   {\n\
   \"user4\":\"0\",\n\
   \"user3\":\"0\",\n\
   \"user2\":\"0\",\n\
   \"user1\":\"200\"\n\
   },\n\
\"net\":   {\n\
   \"add_ipv6\":\"\",\n\
   \"add_ipv4\":\"10.1.39.66\",\n\
   \"netmask\":\"255.255.255.0\",\n\
   \"gateway_ipv6\":\"\",\n\
   \"dns2_ipv6\":\"\",\n\
   \"dns1_ipv6\":\"\",\n\
   \"type_ipv6\":\"\",\n\
   \"gateway_ipv4\":\"10.1.39.1\",\n\
   \"dns1_ipv4\":\"8.8.8.8\",\n\
   \"type_ipv4\":0,\n\
   \"mac\":\"00:1A:3F:01:09:19\",\n\
   \"dns2_ipv4\":\"\",\n\
   \"prot_mode\":0\n\
   },\n\
\"system\":   {\n\
   \"swMinor\":\"1\",\n\
   \"server_host\":\"10.1.39.66\",\n\
   \"tmp_op\":\"6674\",\n\
   \"time12h\":\"01:50:36\",\n\
   \"date\":\"01/01/2017\",\n\
   \"time24h\":\"01:50:36\",\n\
   \"swMajor\":\"0\",\n\
   \"user\":\"admin\",\n\
   \"host_name\":\"intelbras\",\n\
   \"freePartition\":1,\n\
   \"swPatch\":\"1\",\n\
   \"hwVersion\":\"21\",\n\
   \"tmp_ntp\":1483246236\n\
   },\n\
\"product\":\"tip1001\",\n\
\"branch\":\"D\"\n\
}"

#define ENDPOINT_CONTENT "{\n  \"endpointsFree\": 1\n  }\n   "
#define REGISTER_STATUS_CONTENT "{\n  \"RegisterStatus\": \"200\", \n \"RegisterICIP\": \"0\"  }\n   "

/**************************************************************************
 * TYPEDEFS
 **************************************************************************/

/**
 * 	@enum E_HTTP_STATUS_CODE
 */
typedef enum {
	HTTP_SC_TRYING 												  = 100,
	HTTP_SC_RINGING 												= 180,
	HTTP_SC_CALL_BEING_FORWARDED 					  = 181,
	HTTP_SC_QUEUED 												  = 182,
	HTTP_SC_PROGRESS 											  = 183,

	HTTP_SC_OK 														  = 200,
	HTTP_SC_ACCEPTED 											  = 202,

	HTTP_SC_MULTIPLE_CHOICES 							  = 300,
	HTTP_SC_MOVED_PERMANENTLY 							= 301,
	HTTP_SC_MOVED_TEMPORARILY 							= 302,
	HTTP_SC_USE_PROXY 											= 305,
	HTTP_SC_ALTERNATIVE_SERVICE 						= 380,

	HTTP_SC_BAD_REQUEST 										= 400,
	HTTP_SC_UNAUTHORIZED 									  = 401,
	HTTP_SC_PAYMENT_REQUIRED 							  = 402,
	HTTP_SC_FORBIDDEN 											= 403,
	HTTP_SC_NOT_FOUND 											= 404,
	HTTP_SC_METHOD_NOT_ALLOWED 						  = 405,
	HTTP_SC_NOT_ACCEPTABLE 								  = 406,
	HTTP_SC_PROXY_AUTHENTICATION_REQUIRED 	= 407,
	HTTP_SC_REQUEST_TIMEOUT 								= 408,
	HTTP_SC_GONE 													  = 410,
	HTTP_SC_REQUEST_ENTITY_TOO_LARGE 		 	  = 413,
	HTTP_SC_REQUEST_URI_TOO_LONG 				 	  = 414,
	HTTP_SC_UNSUPPORTED_MEDIA_TYPE 				  = 415,
	HTTP_SC_UNSUPPORTED_URI_SCHEME 				  = 416,
	HTTP_SC_BAD_EXTENSION 									= 420,
	HTTP_SC_EXTENSION_REQUIRED 						  = 421,
	HTTP_SC_SESSION_TIMER_TOO_SMALL 				= 422,
	HTTP_SC_INTERVAL_TOO_BRIEF 						  = 423,
	HTTP_SC_TEMPORARILY_UNAVAILABLE 				= 480,
	HTTP_SC_CALL_TSX_DOES_NOT_EXIST 				= 481,
	HTTP_SC_LOOP_DETECTED 									= 482,
	HTTP_SC_TOO_MANY_HOPS 									= 483,
	HTTP_SC_ADDRESS_INCOMPLETE 						  = 484,
	HTTP_SC_AMBIGUOUS 											= 485,
	HTTP_SC_BUSY_HERE 											= 486,
	HTTP_SC_REQUEST_TERMINATED 						  = 487,
	HTTP_SC_NOT_ACCEPTABLE_HERE 						= 488,
	HTTP_SC_BAD_EVENT 											= 489,
	HTTP_SC_REQUEST_UPDATED 								= 490,
	HTTP_SC_REQUEST_PENDING 								= 491,
	HTTP_SC_UNDECIPHERABLE 								  = 493,

	HTTP_SC_INTERNAL_SERVER_ERROR 					= 500,
	HTTP_SC_NOT_IMPLEMENTED 								= 501,
	HTTP_SC_BAD_GATEWAY 										= 502,
	HTTP_SC_SERVICE_UNAVAILABLE 						= 503,
	HTTP_SC_SERVER_TIMEOUT 								  = 504,
	HTTP_SC_VERSION_NOT_SUPPORTED 					= 505,
	HTTP_SC_MESSAGE_TOO_LARGE 							= 513,
	HTTP_SC_PRECONDITION_FAILURE 					  = 580,

	HTTP_SC_BUSY_EVERYWHERE 								= 600,
	HTTP_SC_DECLINE 												= 603,
	HTTP_SC_DOES_NOT_EXIST_ANYWHERE 				= 604,
	HTTP_SC_NOT_ACCEPTABLE_ANYWHERE 				= 606,

	HTTP_SC_TSX_TIMEOUT 										= HTTP_SC_REQUEST_TIMEOUT,
	HTTP_SC_TSX_TRANSPORT_ERROR 						= HTTP_SC_SERVICE_UNAVAILABLE,

} E_HTTP_STATUS_CODE;

/**************************************************************************
 * INTERNAL FUNCTIONS
 **************************************************************************/

/*
 * Callback function used to get data from database
 */ 
int callback_database(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used to get status system
 */ 
int callback_status(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used to get status from Endpoint
 */ 
int callback_endpoint_status(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used to get register status from Endpoint
 */ 
int callback_status_register(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used to get status general from system
 */ 
int callback_status_general(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used to get statusfor giga support
 */ 
int callback_support_giga(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used to get version 
 */ 
int callback_version(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used to notify tables changes 
 */ 
int callback_notify(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used to get Autoprov logs 
 */ 
int callback_autoprov_log(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used to get version 
 */ 
int callback_burn_status(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used reboot system
 */ 
int callback_restart(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used restart syslog
 */ 
int callback_restart_syslog(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used factory Reset
 */ 
int callback_factory_reset(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used Reset the logo
 */ 
int callback_logo_reset(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function usedset language
 */
int callback_set_language(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Callback function used to serve static files that are present in the static folder
 */ 
int callback_static_file(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Default callback function called if no endpoint has a match
 */
int callback_default(const struct _u_request *request, struct _u_response *response, void *user_data);

/*
 * Return the filename extension
 */
const char *getFilenameExt(const char *pchPath);

/*
 * Return content from a file
 */
char *readFile(const char *pchFilename);

#endif
