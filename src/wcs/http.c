/*
 ============================================================================
 Name        : http.c
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description :
 ============================================================================
 */

#include "http.h"
#include "../cJSON/cJSON.h"
#include <curl/curl.h>

#if defined(_WIN32)
#pragma comment(lib, "curllib.lib")
#endif

/*============================================================================*/
/* type wcs_Mutex */

#if defined(_WIN32)

void wcs_Mutex_Init (wcs_Mutex * self)
{
	InitializeCriticalSection (self);
}

void wcs_Mutex_Cleanup (wcs_Mutex * self)
{
	DeleteCriticalSection (self);
}

void wcs_Mutex_Lock (wcs_Mutex * self)
{
	EnterCriticalSection (self);
}

void wcs_Mutex_Unlock (wcs_Mutex * self)
{
	LeaveCriticalSection (self);
}

#else

void wcs_Mutex_Init (wcs_Mutex * self)
{
	pthread_mutex_init (self, NULL);
}

void wcs_Mutex_Cleanup (wcs_Mutex * self)
{
	pthread_mutex_destroy (self);
}

void wcs_Mutex_Lock (wcs_Mutex * self)
{
	pthread_mutex_lock (self);
}

void wcs_Mutex_Unlock (wcs_Mutex * self)
{
	pthread_mutex_unlock (self);
}

#endif

/*============================================================================*/
/* Global */
char *userAgent = NULL;
const char *version = "1.0.0";

void wcs_Buffer_formatInit ();

void wcs_Global_Init (long flags)
{
	wcs_Buffer_formatInit ();
	userAgent = wcs_String_Concat3("User-Agent: WCS-C-SDK-", version,"(http://wcs.chinanetcenter.com/)");
	curl_global_init (CURL_GLOBAL_ALL);
}

void wcs_Global_Cleanup ()
{
	curl_global_cleanup ();
}

/*============================================================================*/
/* func wcs_call */

static const char g_statusCodeError[] = "http status code is not OK";

void wcs_Curl_cleanup(CURL *curl)
{
	if (NULL != curl)
	{
		curl_easy_cleanup ((CURL *) curl);
	}
}

wcs_Error wcs_callex (CURL * curl, wcs_Buffer * resp, wcs_Json ** ret, wcs_Bool simpleError, wcs_Buffer * resph)
{
	wcs_Error err;
	CURLcode curlCode;
	long httpCode;
	wcs_Json *root;

	curlCode = curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, wcs_Buffer_Fwrite);
	curlCode = curl_easy_setopt (curl, CURLOPT_WRITEDATA, resp);
	if (resph != NULL)
	{
		curlCode = curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, wcs_Buffer_Fwrite);
		curlCode = curl_easy_setopt (curl, CURLOPT_WRITEHEADER, resph);
	}

	curlCode = curl_easy_perform (curl);
	if (curlCode == 0)
	{
		curlCode = curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &httpCode);
		if (wcs_Buffer_Len (resp) != 0)
		{
			root = cJSON_Parse (wcs_Buffer_CStr (resp));
		}
		else
		{
			root = NULL;
			LOG_TRACE("resph = %s", resph->buf);
		}
		LOG_TRACE("resp = %s", resp->buf);
		if (NULL != ret)
		{
			*ret = root;
		}
		err.code = (int) httpCode;
		if (httpCode / 100 != 2)
		{
			if (simpleError)
			{
				err.message = g_statusCodeError;
			}
			else
			{
				err.message = wcs_Json_GetString (root, "error", g_statusCodeError);
			}
			if (err.message)
			{
		            LOG_TRACE("httpCode is invalid , httpCode = %d, err.message: %s", httpCode, err.message);
		        }
		}
		else
		{
			err.message = "OK";
		}
	}
	else
	{
		*ret = NULL;
		err.code = curlCode;
		err.message = "curl_easy_perform error";
	}
	//printf("Server return value: %s", resp);
	return err;
}

/*============================================================================*/
/* type wcs_Json */

const char *wcs_Json_GetString (wcs_Json * self, const char *key, const char *defval)
{
	wcs_Json *sub;
	if (self == NULL)
	{
		return defval;
	}
	sub = cJSON_GetObjectItem (self, key);
	if (sub != NULL && sub->type == cJSON_String)
	{
		return sub->valuestring;
	}
	else
	{
		return defval;
	}
}

const char *wcs_Json_GetStringAt (wcs_Json * self, int n, const char *defval)
{
	wcs_Json *sub;
	if (self == NULL)
	{
		return defval;
	}
	sub = cJSON_GetArrayItem (self, n);
	if (sub != NULL && sub->type == cJSON_String)
	{
		return sub->valuestring;
	}
	else
	{
		return defval;
	}
}

wcs_Int64 wcs_Json_GetInt64 (wcs_Json * self, const char *key, wcs_Int64 defval)
{
	wcs_Json *sub;
	if (self == NULL)
	{
		return defval;
	}
	sub = cJSON_GetObjectItem (self, key);
	if (sub != NULL && sub->type == cJSON_Number)
	{
		return (wcs_Int64) sub->valuedouble;
	}
	else
	{
		return defval;
	}
}

int wcs_Json_GetBoolean (wcs_Json * self, const char *key, int defval)
{
	wcs_Json *sub;
	if (self == NULL)
	{
		return defval;
	}
	sub = cJSON_GetObjectItem (self, key);
	if (sub != NULL)
	{
		if (sub->type == cJSON_False)
		{
			return 0;
		}
		else if (sub->type == cJSON_True)
		{
			return 1;
		}
	}
	return defval;
}

wcs_Json *wcs_Json_GetObjectItem (wcs_Json * self, const char *key, wcs_Json * defval)
{
	wcs_Json *sub;
	if (self == NULL)
	{
		return defval;
	}
	sub = cJSON_GetObjectItem (self, key);
	if (sub != NULL)
	{
		return sub;
	}
	else
	{
		return defval;
	}
}

wcs_Json *wcs_Json_GetArrayItem (wcs_Json * self, int n, wcs_Json * defval)
{
	wcs_Json *sub;
	if (self == NULL)
	{
		return defval;
	}
	sub = cJSON_GetArrayItem (self, n);
	if (sub != NULL)
	{
		return sub;
	}
	else
	{
		return defval;
	}
}

void wcs_Json_Destroy (wcs_Json * self)
{
	cJSON_Delete (self);
}

wcs_Uint32 wcs_Json_GetInt (wcs_Json * self, const char *key, wcs_Uint32 defval)
{
	wcs_Json *sub;
	if (self == NULL)
	{
		return defval;
	}
	sub = cJSON_GetObjectItem (self, key);
	if (sub != NULL && sub->type == cJSON_Number)
	{
		return (wcs_Uint32) sub->valueint;
	}
	else
	{
		return defval;
	}
}

/*============================================================================*/
/* type wcs_Client */

wcs_Auth wcs_NoAuth = {
	NULL,
	NULL
};

void wcs_Client_InitEx (wcs_Client * self, wcs_Auth auth, size_t bufSize)
{
	self->curl = curl_easy_init ();
	self->root = NULL;
	self->auth = auth;

	wcs_Buffer_Init (&self->b, bufSize);
	wcs_Buffer_Init (&self->respHeader, bufSize);

	self->boundNic = NULL;

	self->lowSpeedLimit = 0;
	self->lowSpeedTime = 0;
#if 0
	self->regionTable = wcs_Rgn_Table_Create ();
#endif
}

void wcs_Client_InitNoAuth (wcs_Client * self, size_t bufSize)
{
	wcs_Client_InitEx (self, wcs_NoAuth, bufSize);
}

void wcs_Client_Cleanup (wcs_Client * self)
{
	if (self->auth.itbl != NULL)
	{
		self->auth.itbl->Release (self->auth.self);
		self->auth.itbl = NULL;
	}
	if (self->curl != NULL)
	{
		curl_easy_cleanup ((CURL *) self->curl);
		self->curl = NULL;
	}
	if (self->root != NULL)
	{
		cJSON_Delete (self->root);
		self->root = NULL;
	}
	wcs_Buffer_Cleanup (&self->b);
	wcs_Buffer_Cleanup (&self->respHeader);
}

void wcs_Client_BindNic (wcs_Client * self, const char *nic)
{
	self->boundNic = nic;
}
void wcs_Client_SetLowSpeedLimit (wcs_Client * self, long lowSpeedLimit, long lowSpeedTime)
{
	self->lowSpeedLimit = lowSpeedLimit;
	self->lowSpeedTime = lowSpeedTime;
}

CURL *wcs_Client_reset (wcs_Client * self, CURL *mycurl)
{
	CURL *curl = NULL;

	if (NULL != mycurl)
	{
		curl = mycurl;
	}
	else
	{
		curl = (CURL *) self->curl;
	}

	curl_easy_reset (curl);
	wcs_Buffer_Reset (&self->b);
	wcs_Buffer_Reset (&self->respHeader);
	if (self->root != NULL)
	{
		cJSON_Delete (self->root);
		self->root = NULL;
	}

	curl_easy_setopt (curl, CURLOPT_NOSIGNAL, 1);

	return curl;
}

static CURL *wcs_Client_initcall (wcs_Client * self, const char *url, CURL *mycurl)
{
	CURL *curl = wcs_Client_reset (self, mycurl);

	if (HTTP_METHOD_GET == self->method)
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
	}
	else
	{
		curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST, "POST");
	}
	curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt (curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt (curl, CURLOPT_URL, url);
	//wcs_Log_Warn ("wcs_Client_initcall: url = %s, curlRet = %d\n", url, curlRet);
	return curl;
}

static wcs_Error wcs_Client_callWithBody (wcs_Client * self, wcs_Json ** ret, 
	const char *url, const char *body, wcs_Int64 bodyLen, const char *mimeType, 
	wcs_Common_Header * extraHeader)
{
	int retCode = 0;
	wcs_Error err;
	const char *ctxType;
	const char *uploadBatch = NULL;
	const char *key = NULL;
	char ctxLength[64];
	wcs_Json *root = NULL;
	wcs_Header *headers = NULL;
	CURL *curl = NULL;

	if (NULL == self)
	{
		err.code = 9899;
		err.message = "wcs_Client_callWithBody param self is invalid";
		LOG_ERROR("wcs_Client_callWithBody err.message = %s", err.message);
		return err;
	}
	
	if ((NULL != extraHeader) && (NULL != extraHeader->curl))
	{
		curl = extraHeader->curl;
	}
	else
	{
		curl = (CURL *) self->curl;
	}

	// Bind the NIC for sending packets.
	if (self->boundNic != NULL)
	{
		retCode = curl_easy_setopt (curl, CURLOPT_INTERFACE, self->boundNic);
		if (retCode == CURLE_INTERFACE_FAILED)
		{
			err.code = 9994;
			err.message = "Can not bind the given NIC";
			LOG_ERROR("wcs_Client_callWithBody boundNic err.message = %s", err.message);
			return err;
		}
	}

	curl_easy_setopt (curl, CURLOPT_POST, 1);

	if (mimeType == NULL)
	{
		ctxType = "Content-Type: application/octet-stream";
	}
	else
	{
		ctxType = wcs_String_Concat2 ("Content-Type: ", mimeType);
	}

	if (self->auth.itbl != NULL)
	{
		if (body == NULL)
		{
			err = self->auth.itbl->Auth (self->auth.self, &headers, url, NULL, 0);
		}
		else
		{
			err = self->auth.itbl->Auth (self->auth.self, &headers, url, body, (size_t) bodyLen);
		}

		if (err.code != 200)
		{
			return err;
		}
	}

	wcs_snprintf (ctxLength, 64, "Content-Length: %lld", bodyLen);
	headers = curl_slist_append (headers, ctxLength);
	headers = curl_slist_append (headers, ctxType);
	headers = curl_slist_append (headers, "Expect:");

	if (extraHeader != NULL)
	{
		if (extraHeader->uploadBatch != NULL)
		{
			uploadBatch = wcs_String_Concat2 ("UploadBatch: ", (const char *)extraHeader->uploadBatch);
			headers = curl_slist_append (headers, uploadBatch);
		}
		if (extraHeader->key != NULL)
		{
			key = wcs_String_Concat2 ("Key: ", extraHeader->key);
			headers = curl_slist_append (headers, key);
		}

	}
	headers = curl_slist_append (headers, userAgent);
	curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);

	if ((wcs_True == self->multiTask) && (NULL !=  extraHeader->resph) && 
		(NULL != extraHeader->resp))
	{
		err = wcs_callex (curl, extraHeader->resp, &root, wcs_False, extraHeader->resph);
	}
	else
	{
		err = wcs_callex (curl, &self->b, &root, wcs_False, &self->respHeader);
	}

	curl_slist_free_all (headers);
	if (mimeType != NULL)
	{
		free ((void *) ctxType);
	}

	if (NULL != key)
	{
		free((void *)key);
		key = NULL;
	}
	if (NULL != uploadBatch)
	{
		free((void *)uploadBatch);
		uploadBatch = NULL;
	}	
	
	if (NULL == root)
	{
		LOG_WARN("Root is null in wcs_Client_callWithBody");
	}
	if ((NULL != ret) && (NULL != root))
	{
		*ret = root;
	}
	return err;
}

wcs_Error wcs_Client_CallWithBinary (wcs_Client * self, wcs_Json ** ret, const char *url, wcs_Reader body, wcs_Int64 bodyLen, const char *mimeType, wcs_Common_Header * extraHeader)
{
	CURL *curl = NULL;
	curl = wcs_Client_initcall (self, url, extraHeader->curl);
	if (NULL != extraHeader->resph)
	{
		wcs_Buffer_Reset(extraHeader->resph);
	}
	if (NULL != extraHeader->resp)
	{
		wcs_Buffer_Reset(extraHeader->resp);
	}

	curl_easy_setopt (curl, CURLOPT_INFILESIZE_LARGE, bodyLen);
	curl_easy_setopt (curl, CURLOPT_READFUNCTION, body.Read);
	curl_easy_setopt (curl, CURLOPT_READDATA, body.self);
	LOG_TRACE("wcs_Client_CallWithBinary: %s\n", url);

	return wcs_Client_callWithBody (self, ret, url, NULL, bodyLen, mimeType, extraHeader);
}

wcs_Error wcs_Client_CallWithBuffer (wcs_Client * self, wcs_Json ** ret, const char *url, const char *body, size_t bodyLen, const char *mimeType, wcs_Common_Header * extraHeader)
{
	CURL *curl = wcs_Client_initcall (self, url, NULL);

	curl_easy_setopt (curl, CURLOPT_POSTFIELDSIZE, bodyLen);
	curl_easy_setopt (curl, CURLOPT_POSTFIELDS, body);
	LOG_TRACE("wcs_Client_CallWithBinary: %s, %s\n", url, body);

	return wcs_Client_callWithBody (self, ret, url, body, bodyLen, mimeType, extraHeader);
}

wcs_Error wcs_Client_CallWithBuffer2 (wcs_Client * self, wcs_Json ** ret, const char *url, const char *body, size_t bodyLen, const char *mimeType)
{
	CURL *curl = wcs_Client_initcall (self, url, NULL);

	curl_easy_setopt (curl, CURLOPT_POSTFIELDSIZE, bodyLen);
	curl_easy_setopt (curl, CURLOPT_POSTFIELDS, body);
	LOG_TRACE("wcs_Client_CallWithBinary: %s\n", url);

	return wcs_Client_callWithBody (self, ret, url, NULL, bodyLen, mimeType, NULL);
}

wcs_Error wcs_Client_Call (wcs_Client * self, wcs_Json ** ret, const char *url)
{
	wcs_Error err;
	wcs_Header *headers = NULL;
	CURL *curl = wcs_Client_initcall (self, url, NULL);

	if (self->auth.itbl != NULL)
	{
		err = self->auth.itbl->Auth (self->auth.self, &headers, url, NULL, 0);
		if (err.code != 200)
		{
			return err;
		}
	}
	headers = curl_slist_append (headers, userAgent);
	curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);

	err = wcs_callex (curl, &self->b, &self->root, wcs_False, &self->respHeader);
	curl_slist_free_all (headers);
	if ((NULL != ret) && (NULL != self->root))
	{
		*ret = self->root;
	} 
	return err;
}


