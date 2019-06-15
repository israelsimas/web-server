/**************************************************************************
 * middleware.h
 *
 *  Create on: 02/05/2019
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform SIP Intelbras
 *
 * Copyrights Intelbras, 2019
 *
 **************************************************************************/

#ifndef MIDDLEWARE_H_
#define MIDDLEWARE_H_

/**************************************************************************
 * INCLUDES
 **************************************************************************/
#include <misc.h>
#include <defTypes.h>

/**************************************************************************
 * DEFINITIONS
 **************************************************************************/
#define ZMQ_PUBLISHER_ADDR   			    "tcp://*:5561"
#define ZMQ_SUBSCRIBER_ADDR   			  "tcp://localhost:5570"

#define PARAM_TABLES            "tables"
#define PARAM_APP_ID  	        "app_id"
#define ENVELOPE_ZMQ_DATABASE  	"database"

/**************************************************************************
 * INTERNAL FUNCTIONS
 **************************************************************************/
BOOL openMiddleware();

int stopZeroMQ();

void sendMiddlewareMessage(char *pchTales);

#endif
