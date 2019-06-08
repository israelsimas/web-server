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


/**************************************************************************
 * DEFINITIONS
 **************************************************************************/

#define PORT          8080
#define PREFIX        "/test"
#define PREFIXJSON    "/testjson"
#define PREFIXCOOKIE  "/testcookie"
#define STATIC_FOLDER "/var/www"

#define JSON1_URL "/db.cgi"
#define JSON2_URL "/db.cgi"
#define JSON3_URL "/status.cgi"

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

#define JSON3_CONTENT "{\n\
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

/**************************************************************************
 * INTERNAL FUNCTIONS
 **************************************************************************/

/**
 * callback functions declaration
 */
int callback_get_test (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_get_empty_response (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_post_test (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_all_test_foo (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_get_cookietest (const struct _u_request * request, struct _u_response * response, void * user_data);

///////////////
int callback_json1_test(const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_json2_test(const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_json3_test(const struct _u_request * request, struct _u_response * response, void * user_data);
///////////////

// Callback function used to serve static files that are present in the static folder
int callback_static_file (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data);

#endif
