/*
 ============================================================================
 Name        : http.h
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */

#ifndef WCS_HTTP_H
#define WCS_HTTP_H

#include "base.h"
#include "conf.h"
#include "../base/threadpool.h"
#include "../base/log.h"
#include <curl/curl.h>
#include <pthread/pthread.h>
/*============================================================================*/
/* Global */

#ifdef __cplusplus
extern "C"
{
#endif
extern char *userAgent;
extern const char *version;
extern void wcs_Global_Init (long flags);
extern void wcs_Global_Cleanup ();

extern void wcs_MacAuth_Init ();
extern void wcs_MacAuth_Cleanup ();

extern void wcs_Servend_Init (long flags);
extern void wcs_Servend_Cleanup ();

#ifdef __cplusplus
}
#endif

/*============================================================================*/
/* type wcs_Mutex */

#if defined(_WIN32_NO)
#include <windows.h>
typedef CRITICAL_SECTION wcs_Mutex;
typedef pthread_cond_t wcs_Cond;
#else
#include <pthread.h>
typedef pthread_mutex_t wcs_Mutex;
typedef pthread_cond_t wcs_Cond;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

WCS_DLLAPI extern void wcs_Mutex_Init (wcs_Mutex * self);
WCS_DLLAPI extern void wcs_Mutex_Cleanup (wcs_Mutex * self);

WCS_DLLAPI extern void wcs_Mutex_Lock (wcs_Mutex * self);
WCS_DLLAPI extern void wcs_Mutex_Unlock (wcs_Mutex * self);

/*============================================================================*/
/* type wcs_Json */

typedef struct cJSON wcs_Json;

WCS_DLLAPI extern const char *wcs_Json_GetString (wcs_Json * self, const char *key, const char *defval);
WCS_DLLAPI extern const char *wcs_Json_GetStringAt (wcs_Json * self, int n, const char *defval);
WCS_DLLAPI extern wcs_Int64 wcs_Json_GetInt64 (wcs_Json * self, const char *key, wcs_Int64 defval);
WCS_DLLAPI extern int wcs_Json_GetBoolean (wcs_Json * self, const char *key, int defval);
WCS_DLLAPI extern wcs_Json *wcs_Json_GetObjectItem (wcs_Json * self, const char *key, wcs_Json * defval);
WCS_DLLAPI extern wcs_Json *wcs_Json_GetArrayItem (wcs_Json * self, int n, wcs_Json * defval);
WCS_DLLAPI extern void wcs_Json_Destroy (wcs_Json * self);

/*============================================================================*/
/* type wcs_Auth */

#pragma pack(1)

typedef struct curl_slist wcs_Header;

typedef struct _wcs_Auth_Itbl
{
	wcs_Error (*Auth) (void *self, wcs_Header ** header, const char *url, const char *addition, size_t addlen);
	void (*Release) (void *self);
} wcs_Auth_Itbl;

typedef struct _wcs_Auth
{
	void *self;
	wcs_Auth_Itbl *itbl;
} wcs_Auth;

typedef enum _wcs_Http_Method
{
	HTTP_METHOD_POST = 0,
	HTTP_METHOD_GET
}wcs_Http_Method;

typedef struct _wcs_Common_Headers
{
	char *auth;
	unsigned char *uploadBatch;
	char *key;
	CURL *curl;
	wcs_Buffer *resph;
	wcs_Buffer *resp;
} wcs_Common_Header;
typedef struct _wcs_multiThread_info
{
	unsigned int totalBlkNum;
	unsigned int doneBlkNum;
	wcs_Mutex totalDoneMutex;
	wcs_Mutex doneBlkNumMutex;
	wcs_Cond notify; //
}wcs_multiThread_info;


typedef struct _wcs_Common_Param
{
	char *prefix;
	unsigned int limit;
	char *mode;
	unsigned long marker;
} wcs_Common_Param;


WCS_DLLAPI extern wcs_Auth wcs_NoAuth;

/*============================================================================*/
/* type wcs_Client */

struct _wcs_Rgn_RegionTable;

#define UPLOADBATCHID_LEN 32


typedef struct _wcs_Client
{
	void *curl;
	wcs_Auth auth;
	wcs_Json *root;
	wcs_Buffer b;
	wcs_Buffer respHeader;

	// Use the following field to specify which NIC to use for sending packets.
	const char *boundNic;

	// Use the following field to specify the average transfer speed in bytes per second (Bps)
	// that the transfer should be below during lowSpeedTime seconds for this SDK to consider
	// it to be too slow and abort.
	long lowSpeedLimit;

	// Use the following field to specify the time in number seconds that
	// the transfer speed should be below the logSpeedLimit for this SDK to consider it
	// too slow and abort.
	long lowSpeedTime;

	// Use the following field to manange information of multi-region.
	struct _wcs_Rgn_RegionTable *regionTable;
	wcs_Http_Method method;
	wcs_multiThread_info *threadInfo;
	threadpool_t *threadPool; 
	wcs_Mutex nfailsMutex;
	wcs_Mutex ninterruptsMutex;
	wcs_Mutex writeFileMutex;
	wcs_Bool multiTask; //Default is wcs_True
	wcs_Bool isPatchUpload;
	char *patchInfoFile;
} wcs_Client;

WCS_DLLAPI extern void wcs_Client_InitEx (wcs_Client * self, wcs_Auth auth, size_t bufSize);
WCS_DLLAPI extern void wcs_Client_Cleanup (wcs_Client * self);
WCS_DLLAPI extern void wcs_Client_BindNic (wcs_Client * self, const char *nic);
WCS_DLLAPI extern void wcs_Client_SetLowSpeedLimit (wcs_Client * self, long lowSpeedLimit, long lowSpeedTime);

WCS_DLLAPI extern wcs_Error wcs_Client_Call (wcs_Client * self, wcs_Json ** ret, const char *url);
WCS_DLLAPI extern wcs_Error wcs_Client_CallNoRet (wcs_Client * self, const char *url);
WCS_DLLAPI extern wcs_Error wcs_Client_CallWithBinary (wcs_Client * self, wcs_Json ** ret, const char *url,
	wcs_Reader body, wcs_Int64 bodyLen, const char *mimeType, wcs_Common_Header * extraHeader);
WCS_DLLAPI extern wcs_Error wcs_Client_CallWithBuffer (wcs_Client * self, wcs_Json ** ret, const char *url,
	const char *body, size_t bodyLen, const char *mimeType, wcs_Common_Header * extraHeader);

WCS_DLLAPI extern wcs_Error wcs_Client_CallWithBuffer2 (wcs_Client * self, wcs_Json ** ret, const char *url, const char *body, size_t bodyLen, const char *mimeType);

/*============================================================================*/
/* func wcs_Client_InitNoAuth/InitMacAuth  */

typedef struct _wcs_Mac
{
	const char *accessKey;
	const char *secretKey;
} wcs_Mac;

wcs_Auth wcs_MacAuth (wcs_Mac * mac);

WCS_DLLAPI extern char *wcs_Mac_Sign (wcs_Mac * self, char *data);
WCS_DLLAPI extern char *wcs_Mac_SignToken (wcs_Mac * self, char *data);

WCS_DLLAPI extern void wcs_Client_InitNoAuth (wcs_Client * self, size_t bufSize);
WCS_DLLAPI extern void wcs_Client_InitMacAuth (wcs_Client * self, size_t bufSize, wcs_Mac * mac);

WCS_DLLAPI wcs_Error wcs_callex (CURL * curl, wcs_Buffer * resp, wcs_Json ** ret, wcs_Bool simpleError, wcs_Buffer * resph);

void wcs_Curl_cleanup(CURL *curl);
/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif							/* WCS_HTTP_H */
