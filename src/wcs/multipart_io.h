/*
 ============================================================================
 Name        : multipart_io.h
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description :
 ============================================================================
 */

#ifndef WCS_RESUMABLE_IO_H
#define WCS_RESUMABLE_IO_H

#include "http.h"
#include "io.h"
#include <curl/curl.h>

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/

#define wcs_Multipart_InvalidCtx			701
#define wcs_Multipart_UnmatchedChecksum		9900
#define wcs_Multipart_InvalidPutProgress	9901
#define wcs_Multipart_PutFailed				9902
#define wcs_Multipart_PutInterrupted		9903
#define wcs_Multipart_MaxThreadNum	8	
//#define wcs_Multipart_MaxQueueNum		256

#define Default_ConfigFile "./PatchUpload.ini"
#define FILENAME_LEN 512
#define BUCKET_LEN 128
#define KEY_LEN 512
#define TOKEN_LEN 1500

/*============================================================================*/
/* type wcs_Multipart_WaitGroup */

	typedef struct _wcs_Multipart_WaitGroup_Itbl
	{
		void (*Add) (void *self, int n);
		void (*Done) (void *self);
		void (*Wait) (void *self);
		void (*Release) (void *self);
	} wcs_Multipart_WaitGroup_Itbl;

	typedef struct _wcs_Multipart_WaitGroup
	{
		void *self;
		wcs_Multipart_WaitGroup_Itbl *itbl;
	} wcs_Multipart_WaitGroup;

#if defined(_WIN32)

	wcs_Multipart_WaitGroup wcs_Multipart_MTWG_Create (void);

#endif


/*============================================================================*/
/* type wcs_Multipart_ThreadModel */

	typedef struct _wcs_Multipart_ThreadModel_Itbl
	{
		wcs_Multipart_WaitGroup (*WaitGroup) (void *self);
		wcs_Client *(*ClientTls) (void *self, wcs_Client * mc);
		int (*RunTask) (void *self, void (*task) (void *params), void *params);
	} wcs_Multipart_ThreadModel_Itbl;

	typedef struct _wcs_Multipart_ThreadModel
	{
		void *self;
		wcs_Multipart_ThreadModel_Itbl *itbl;
	} wcs_Multipart_ThreadModel;


	WCS_DLLAPI extern wcs_Multipart_ThreadModel wcs_Multipart_ST;

/*============================================================================*/
/* type wcs_Multipart_Settings */

	typedef struct _wcs_Multipart_Settings
	{
		int taskQsize;
		int workers;
		int chunkSize;
		int tryTimes;
		wcs_Multipart_ThreadModel threadModel;
	} wcs_Multipart_Settings;

	WCS_DLLAPI extern void wcs_Multipart_SetSettings (wcs_Multipart_Settings * v);

/*============================================================================*/
/* type wcs_Multipart_PutExtra */

	typedef struct _wcs_Multipart_BlkputRet
	{
		const char *ctx;
		const char *checksum;
		wcs_Uint32 crc32;
		wcs_Uint32 offset;
		wcs_Buffer *resph;
		wcs_Buffer *resp;
	} wcs_Multipart_BlkputRet;

#define WCS_RIO_NOTIFY_OK 0
#define WCS_RIO_NOTIFY_EXIT 1

	typedef int (*wcs_Multipart_FnNotify) (void *recvr, int blkIdx, int blkSize, wcs_Multipart_BlkputRet * ret);
	typedef int (*wcs_Multipart_FnNotifyErr) (void *recvr, int blkIdx, int blkSize, wcs_Error err);

         typedef struct _wcs_PatchInfo
        {
           //char uploadBatchId[UPLOADBATCHID_LEN+1];
           unsigned char *uploadBatchId;
           unsigned int blockSize;
           unsigned int chunkSize;
           unsigned int succNum;
           unsigned int varCount;
           int **blocks;
           char *bucket;
           char *key;
           char *uploadFile;
           char *upHost;
           wcs_Io_PutExtraParam *param;
        }wcs_PatchInfo;

	typedef struct _wcs_Multipart_PutExtra
	{
		const char *callbackParams;
		const char *bucket;
		const char *customMeta;
		const char *mimeType;
		int chunkSize;
		int blockSize;
		int tryTimes;
		int blkIdx;				//BlockOrder
		void *notifyRecvr;
		wcs_Multipart_FnNotify notify;
		wcs_Multipart_FnNotifyErr notifyErr;
		wcs_Multipart_BlkputRet *progresses;
		//CURL *multiCurl;
		size_t blockCnt;
		wcs_Multipart_ThreadModel threadModel;

		// For those file systems that save file name as Unicode strings,
		// use this field to name the local file name in UTF-8 format for CURL.
		const char *localFileName;

		// For those who want to invoke a upload callback on the business server
		// which returns a JSON object.
		void *callbackRet;
		  wcs_Error (*callbackRetParser) (void *, wcs_Json *);

		// Use xVarsList to pass user defined variables and xVarsCount to pass the count of them.
		//
		// (extra->xVarsList[i])[0] set as the variable name, e.g. "x:Price".
		// **NOTICE**: User defined variable's name MUST starts with a prefix string "x:".
		//
		// (extra->xVarsList[i])[1] set as the value, e.g. "priceless".
		//const char *(*xVarsList)[2];
		//int xVarsCount;
		wcs_Io_PutExtraParam *params;

		// For those who want to send request to specific host.
		const char *upHost;
		wcs_Uint32 upHostFlags;
		char *upBucket;
		char *accessKey;
		char *uptoken;
		char *key;
		unsigned char *uploadBatch;
		wcs_PatchInfo *patchInfo;
	} wcs_Multipart_PutExtra;

 /*============================================================================*/
 /* type wcs_Multipart_task */
  
 typedef struct _wcs_Multipart_task
 {
	wcs_ReaderAt f;
	wcs_Client *mc;
	wcs_Multipart_PutExtra *extra;
	wcs_Multipart_WaitGroup wg;
	int *nfails;
	wcs_Count *ninterrupts;
	unsigned int *succNum;
	int blkIdx;
	int blkSize1;
	void *multiCurl; 
	threadpool_t *threadPool;
	wcs_Buffer resp;
	wcs_Buffer resph;
 } wcs_Multipart_task;



void wcs_GenerateUUID (unsigned char *result, unsigned int len);
int getPatchInfo(const char *file, wcs_PatchInfo *patchInfo);
void getXVarList(wcs_Io_PutExtraParam *param, const char *file);
int getSuccNum(const char *file);
int getVarCount(const char *file);

/*============================================================================*/
/* type wcs_Multipart_PutRet */

	typedef wcs_Io_PutRet wcs_Multipart_PutRet;

/*============================================================================*/
/* func wcs_Multipart_BlockCount */

	WCS_DLLAPI extern int wcs_Multipart_BlockCount (wcs_Int64 fsize, wcs_Int64 blockSize);
	WCS_DLLAPI extern void wcs_Multipart_putRetCleanUp(wcs_Multipart_PutRet *ret);

/*============================================================================*/
/* func wcs_Multipart_PutXXX */

#ifndef WCS_UNDEFINED_KEY
#define WCS_UNDEFINED_KEY		NULL
#endif

	WCS_DLLAPI extern wcs_Error wcs_Multipart_Put (wcs_Client * self, wcs_Multipart_PutRet * ret, const char *uptoken, const char *key, wcs_ReaderAt f, wcs_Int64 fsize, wcs_Multipart_PutExtra * extra);

	WCS_DLLAPI extern wcs_Error wcs_Multipart_PutFile (wcs_Client * self, wcs_Multipart_PutRet * ret, const char *uptoken, const char *key, const char *localFile, wcs_Multipart_PutExtra * extra);

    WCS_DLLAPI extern wcs_Error wcs_Multipart_UploadCheck(const char *configFile, wcs_Client * self, wcs_Multipart_PutRet* ret, wcs_Multipart_PutExtra* extra);

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif							// WCS_RESUMABLE_IO_H
