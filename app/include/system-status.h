/**************************************************************************
 * system-status.h
 *
 *  Create on: 07/06/2019
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform Sirius Intelbras
 *
 * Copyrights Intelbras, 2019
 *
 **************************************************************************/

#ifndef SYTEM_STATUS_H_
#define SYTEM_STATUS_H_

#ifdef __APPLE__
  #define DEFAULT_INTERFACE "en0"
#elif __linux__
  #if 1
  #define DEFAULT_INTERFACE "eth0"
  #else
  #define DEFAULT_INTERFACE "wlp3s0"
  #endif
#endif

#define INVALID_IP        "0"
#define SIZE_STR_GATEWAY  40
#define SIZE_STR_MAC      40
#define SIZE_STR_STATUS_SYS      40
#define BUFFER_REG_LENGHT 		   15
#define PORT_SERVER_REG_STATUS   4849
#define ENDPOINT_BUSY     0

#define BUFFER_ENDP_LENGHT 	20
#define ENDPOINT_STATUS 		"CMD_STATUS"
#define PORT_SERVER_ENDPOINT_STATUS   4860

#define PERCENTAGE_STR_LEN  5

#define LOGIN_TIMEOUT     300

#define AUTHENTICATE_REQUEST(req, resp) \
                              do { \
                                if (!isAuthenticated(req, resp)) { \
                                    return U_CALLBACK_UNAUTHORIZED; \
                                } \
                              } while(0)

/**************************************************************************
 * TYPEDEFS
 **************************************************************************/
typedef struct SYSTEM_GENERAL {
  char *pchProduct;
  char *pchBranch;
  char *pchDatabasePath;
  char *pchVersion;
  WORD wMajor;
  WORD wMinor;
  WORD wPatch;   
  int accountNumber;
  int dev_id;
  time_t loginTime;
  char *pchAdminUser;
  char *pchAdminPwd;  
} SYSTEM_GENERAL;

typedef enum {
  IP_ADDR_TYPE_NONE,
  IP_ADDR_TYPE_IPV4,
  IP_ADDR_TYPE_IPV6,
  IP_ADDR_TYPE_IPV4_FQDN,
  IP_ADDR_TYPE_IPV6_FQDN,
	IP_ADDR_TYPE_IPV4_IPV6,
} E_IP_ADDR_TYPE;

/**************************************************************************
 * INTERNAL FUNCTIONS
 **************************************************************************/

void initSystemGeneral(struct _h_connection *pConn);

SYSTEM_GENERAL *getSystemGeneral();

char *getActiveInterface();

char *getMac();

BOOL getRegisterStatusAccount(json_t ** j_result, WORD wAccount);

BOOL getEndpointFreeStatus(json_t ** j_result);

BOOL getStatusAccount(json_t ** j_result);

BOOL getStatusNetwork(json_t ** j_result);

BOOL getStatusSystem(json_t ** j_result);

BOOL getGeneralStatus(json_t **j_result);

BOOL getGigaSupport(json_t **j_result);

BOOL getVersionStatus(json_t **j_result);

BOOL getFwCloudVersion(json_t **j_result);

char *addIPv6Brackets(char *pchIpAddr);

char *removeBracketsAddr(char *pchIpAddress);

E_IP_ADDR_TYPE getIPAddrType(char *pchIpAddress);

#endif
