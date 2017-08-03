/*
 ============================================================================
 Name        : io.h
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */

#ifndef WCS_IO_H
#define WCS_IO_H

#include "http.h"
#include "reader.h"
#include "../cJSON/cJSON.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* type wcs_Io_PutExtra */

	typedef struct _wcs_Io_PutExtraParam
	{
		char *key;
		char *value;
		struct _wcs_Io_PutExtraParam *next;
	} wcs_Io_PutExtraParam;

	typedef struct _wcs_Io_PutExtra
	{
		wcs_Io_PutExtraParam *params;
		const char *mimeType;
		wcs_Uint32 crc32;
		wcs_Uint32 checkCrc32;

		// For those file systems that save file name as Unicode strings,
		// use this field to name the local file name in UTF-8 format for CURL.
		const char *localFileName;

		// For those who want to invoke a upload callback on the business server
		// which returns a JSON object.
		void *callbackRet;
		  wcs_Error (*callbackRetParser) (void *, wcs_Json *);

		// For those who want to send request to specific host.
		const char *upHost;
		wcs_Uint32 upHostFlags;
		const char *upBucket;
		const char *accessKey;
		const char *uptoken;

		// For those who want to abort uploading data to server.
		void *upAbortUserData;
		wcs_Rd_FnAbort upAbortCallback;

		// Use the following field to specify the size of an uploading file definitively.
		size_t upFileSize;
	} wcs_Io_PutExtra;

/*============================================================================*/
/* type wcs_Io_PutRet */

	typedef struct _wcs_Io_PutRet
	{
		const char *hash;
		const char *key;
		const char *persistentId;
	} wcs_Io_PutRet;

	typedef size_t (*rdFunc) (void *buffer, size_t size, size_t n, void *rptr);

/*============================================================================*/
/* func wcs_Io_PutXXX */

#ifndef WCS_UNDEFINED_KEY
#define WCS_UNDEFINED_KEY		NULL
#endif

	WCS_DLLAPI extern wcs_Error wcs_Io_PutFile (wcs_Client * self, cJSON ** ret, const char *uptoken, const char *key, const char *localFile, wcs_Io_PutExtra * extra);

	WCS_DLLAPI extern wcs_Error wcs_Io_PutBuffer (wcs_Client * self, cJSON ** ret, const char *uptoken, const char *key, const char *buf, size_t fsize, wcs_Io_PutExtra * extra);

	WCS_DLLAPI extern wcs_Error wcs_Io_PutStream (wcs_Client * self, wcs_Io_PutRet * ret, const char *uptoken, const char *key, void *ctx,	// 'ctx' is the same as rdr's last param
		size_t fsize, rdFunc rdr, wcs_Io_PutExtra * extra);

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif							// WCS_IO_H
