/*
 ============================================================================
 Name        : rs.c
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */
#include "rs.h"
#include "../cJSON/cJSON.h"
#include <time.h>
#include "fop.h"

/*============================================================================*/
/* type wcs_RS_PutPolicy/GetPolicy */

char *wcs_RS_PutPolicy_Token (wcs_RS_PutPolicy * auth, wcs_Mac * mac)
{
	int expires = 0;
	time_t deadline;
	char *authstr = NULL;
	char *token = NULL;
	cJSON *root = NULL;

	if ((NULL == auth) || (NULL == mac)  || (NULL == mac->accessKey) || (NULL == mac->secretKey))
	{
		return NULL;
	}

	root = cJSON_CreateObject ();

	if (auth->scope)
	{
		cJSON_AddStringToObject (root, "scope", auth->scope);
	}

	if (auth->expires)
	{
		expires = auth->expires;
	}
	else
	{
		expires = 3600;			// 1小时
	}
	time (&deadline);
	deadline += expires;
	deadline *= 1000;
	char tmpDeadline[14];
	sprintf (tmpDeadline, "%ld", deadline);
	cJSON_AddStringToObject (root, "deadline", tmpDeadline);

	if (auth->saveKey)
	{
		cJSON_AddStringToObject (root, "saveKey", auth->saveKey);
	}
	if (auth->fsizeLimit)
	{
		cJSON_AddNumberToObject (root, "fsizeLimit", auth->fsizeLimit);
	}

	cJSON_AddNumberToObject (root, "overwrite", auth->overwrite);

	if (auth->returnUrl)
	{
		cJSON_AddStringToObject (root, "returnUrl", auth->returnUrl);
	}
	if (auth->returnBody)
	{
		cJSON_AddStringToObject (root, "returnBody", auth->returnBody);
	}

	if (auth->callbackUrl)
	{
		cJSON_AddStringToObject (root, "callbackUrl", auth->callbackUrl);
	}
	if (auth->callbackBody)
	{
		cJSON_AddStringToObject (root, "callbackBody", auth->callbackBody);
	}
	if (auth->persistentNotifyUrl)
	{
		cJSON_AddStringToObject (root, "persistentNotifyUrl", auth->persistentNotifyUrl);
	}
	if (auth->persistentOps)
	{
		cJSON_AddStringToObject (root, "persistentOps", auth->persistentOps);
	}
	if (auth->contentDetect)
	{
		cJSON_AddStringToObject (root, "contentDetect", auth->contentDetect);
	}
	if (auth->detectNotifyURL)
	{
		cJSON_AddStringToObject (root, "detectNotifyURL", auth->detectNotifyURL);
	}
	if (auth->detectNotifyRule)
	{
		cJSON_AddStringToObject (root, "detectNotifyRule", auth->detectNotifyRule);
	}

	//cJSON_AddNumberToObject(root, "instant", 0);
	cJSON_AddNumberToObject (root, "separate", auth->separate);

	authstr = cJSON_PrintUnformatted (root);
	cJSON_Delete (root);
	if ((NULL != mac->accessKey) && (NULL != mac->secretKey) && (NULL != authstr))
	{
		LOG_TRACE("authstr: %s", authstr);
		token = wcs_Mac_SignToken (mac, authstr);
		wcs_Free (authstr);
	}

	return token;
}

char *wcs_RS_GetPolicy_MakeRequest (wcs_RS_GetPolicy * auth, const char *baseUrl, wcs_Mac * mac)
{
	int expires;
	time_t deadline;
	char e[11];
	char *authstr;
	char *token;
	char *request;

	if ((NULL == auth) || (NULL == baseUrl) || (NULL == mac))
	{
		return NULL;
	}
	if (auth->expires)
	{
		expires = auth->expires;
	}
	else
	{
		expires = 3600;			// 1小时
	}
	time (&deadline);
	deadline += expires;
	sprintf (e, "%u", (unsigned int) deadline);

	if (strchr (baseUrl, '?') != NULL)
	{
		authstr = wcs_String_Concat3 (baseUrl, "&e=", e);
	}
	else
	{
		authstr = wcs_String_Concat3 (baseUrl, "?e=", e);
	}

	token = wcs_Mac_Sign (mac, authstr);

	request = wcs_String_Concat3 (authstr, "&token=", token);

	wcs_Free (token);
	wcs_Free (authstr);

	return request;
}

char *wcs_RS_MakeBaseUrl (const char *domain, const char *key)
{
	wcs_Bool fesc;
	char *baseUrl;
	char *escapedKey = wcs_PathEscape (key, &fesc);

	baseUrl = wcs_String_Concat ("http://", domain, "/", escapedKey, NULL);

	if (fesc)
	{
		wcs_Free (escapedKey);
	}

	return baseUrl;
}

/*============================================================================*/
/* func wcs_RS_Stat */

wcs_Error wcs_RS_Stat (wcs_Client * self, cJSON ** ret, const char *tableName, const char *key, const char *mgrHost)
{
	wcs_Error err;
	cJSON *root = NULL;

	wcs_Zero(err);
	
	if ((NULL == self) || (NULL == tableName) || (NULL == key))
	{
		err.code = 9899;
		err.message = "wcs_RS_Stat Param is NULL";
		return err;
	}
	
	char *url = NULL;
	char *entryURI = wcs_String_Concat3 (tableName, ":", key);
	char *entryURIEncoded = wcs_String_Encode (entryURI);
	if (NULL != mgrHost)
	{
		url = wcs_String_Concat3 (mgrHost, "/stat/", entryURIEncoded);
	}
	else
	{
		url = wcs_String_Concat3 (WCS_RS_HOST, "/stat/", entryURIEncoded);
	}

	wcs_Free (entryURI);
	wcs_Free (entryURIEncoded);
	self->method = HTTP_METHOD_GET;

	err = wcs_Client_Call (self, &root, url);
	wcs_Free (url);
	if (NULL != ret)
	{
		*ret = root;
	}
	return err;
}

wcs_Error wcs_RS_List (wcs_Client * self, cJSON ** ret, const char *bucketName, wcs_Common_Param * param, const char *mgrHost)
{
	wcs_Error err;
	cJSON *root = NULL;
	//wcs_Common_Param localParam;
	char *url = NULL;
	char *tmpUrl = NULL;
	char tmpMarker[512];
	char *encode = NULL;

	if ((NULL == bucketName) || (NULL ==self))
	{
		err.code = 9899;
		err.message = "wcs_RS_List param is null";
		return err;
	}
	if (NULL != mgrHost)
	{
		url = wcs_String_Concat3 (mgrHost, "/list?bucket=", bucketName);
	}
	else
	{
		url = wcs_String_Concat3 (WCS_RS_HOST, "/list?bucket=", bucketName);
	}
	if (NULL != param)
	{
		if (param->limit > 0)
		{
			wcs_Zero(tmpMarker);
			sprintf(tmpMarker, "limit=%d", param->limit);
			tmpUrl = wcs_String_Concat3 (url, "&", tmpMarker);
			wcs_Free (url);
			url = tmpUrl;
		}
		if (NULL != param->prefix)
		{
		        wcs_Zero(tmpMarker);
		        encode = wcs_String_Encode(param->prefix);
			sprintf(tmpMarker, "prefix=%s", encode);
			tmpUrl = wcs_String_Concat3 (url, "&", tmpMarker);
			wcs_Free (url);
			wcs_Free(encode);
			url = tmpUrl;

		}

		if (NULL != param->mode)
		{
		        wcs_Zero(tmpMarker);
			sprintf(tmpMarker, "mode=%s", param->mode);
			tmpUrl = wcs_String_Concat3 (url, "&", tmpMarker);
			wcs_Free (url);
			url = tmpUrl;
		}
		if (param->marker > 0)
		{
			wcs_Zero(tmpMarker);
			sprintf(tmpMarker, "marker=%ld", param->marker);
			tmpUrl = wcs_String_Concat3 (url, "&", tmpMarker);
			wcs_Free (url);
			url = tmpUrl;
		}

	}
	self->method = HTTP_METHOD_GET;
	LOG_TRACE("URL: %s", url);
	err = wcs_Client_Call (self, &root, url);
	wcs_Free (url);
	if (NULL != ret)
	{
		*ret = root;
	}

	return err;
}

wcs_Error wcs_RS_UpdateMirror(wcs_Client * self, cJSON ** ret, const char *bucketName, 
const char **fileNameList, unsigned int fileNum, const char *mgrHost)
{
	wcs_Error err;
	cJSON *root = NULL;
	unsigned long current = 0;
	char *encode = NULL;
	const char *fileName = NULL;
	char *tmpParam = NULL;
	char *param = NULL;
	char *url = NULL;

	if ((NULL == self) || (NULL == bucketName))
	{
		err.code = 9899;
		err.message = "wcs_RS_UpdateMirror the parma is invalid";
		return err;
	}

	self->method = HTTP_METHOD_POST;
	while (current < fileNum)
	{
		if ((NULL == fileNameList) || (NULL == fileNameList[current]))
		{
			break;
		}
		fileName = fileNameList[current];
		encode = wcs_String_Encode(fileName);
		if (0 == current)
		{
			tmpParam = wcs_String_Concat3(bucketName,":", encode);
		}
		else
		{
			tmpParam = wcs_String_Concat3(param, "|", encode);
		}
		wcs_Free(encode);
		if (NULL != param)
		{
			wcs_Free(param);
		}
		current++;
		param = tmpParam;
	}
	if (NULL == param)
	{
		err.code = 9899;
		err.message = "wcs_RS_UpdateMirror is null param";
		return err;
	}
	encode = wcs_String_Encode(param);
	wcs_Free(param);
	if (NULL != mgrHost)
	{
		url = wcs_String_Concat3 (mgrHost, "/prefetch/", encode);
	}
	else
	{
		url = wcs_String_Concat3 (WCS_RS_HOST, "/prefetch/", encode);
	}

	wcs_Free(encode);	
	err = wcs_Client_CallWithBuffer2 (self, &root, url, NULL, 0, "application/x-www-form-urlencoded");
	wcs_Free (url);

	if (NULL != ret)
	{
		*ret = root;
	}
	return err;
}

/*============================================================================*/
/* func wcs_RS_Delete */

wcs_Error wcs_RS_Delete (wcs_Client * self, const char *tableName, const char *key, const char *mgrHost)
{
	wcs_Error err;
	
	if ((NULL == self) || (NULL == tableName) || (NULL == key))
    	{
        	err.code = 9899;
        	err.message = "wcs_RS_Move param is NULL";
        	return err;
   	}
   	char *url = NULL;
	
	char *entryURI = wcs_String_Concat3 (tableName, ":", key);
	char *entryURIEncoded = wcs_String_Encode (entryURI);

	if (NULL != mgrHost)
	{
		url = wcs_String_Concat3 (mgrHost, "/delete/", entryURIEncoded);
	}
	else
	{
		url = wcs_String_Concat3 (WCS_RS_HOST, "/delete/", entryURIEncoded);
	}

	wcs_Free (entryURI);
	wcs_Free (entryURIEncoded);
	self->method = HTTP_METHOD_POST;
	err = wcs_Client_CallWithBuffer2 (self, NULL, url, NULL, 0, NULL);
	wcs_Free (url);

	return err;
}

/*============================================================================*/
/* func wcs_RS_Copy */

wcs_Error wcs_RS_Copy (wcs_Client * self, const char *tableNameSrc, const char *keySrc, 
	const char *tableNameDest, const char *keyDest, const char *mgrHost)
{
	wcs_Error err;
	
	if ((NULL == self) || (NULL == tableNameSrc) || (NULL == keySrc) || (NULL == tableNameDest) || (NULL == keyDest))
   	 {
        	err.code = 9899;
        	err.message = "wcs_RS_Move param is NULL";
        	return err;
    	}	

	char *url = NULL;
	char *entryURISrc = wcs_String_Concat3 (tableNameSrc, ":", keySrc);
	char *entryURISrcEncoded = wcs_String_Encode (entryURISrc);
	char *entryURIDest = wcs_String_Concat3 (tableNameDest, ":", keyDest);
	char *entryURIDestEncoded = wcs_String_Encode (entryURIDest);
	char *urlPart = wcs_String_Concat3 (entryURISrcEncoded, "/", entryURIDestEncoded);

	if (NULL != mgrHost)
	{
		url = wcs_String_Concat3 (mgrHost, "/copy/", urlPart);
	}
	else
	{
		url = wcs_String_Concat3 (WCS_RS_HOST, "/copy/", urlPart);
	}

	free (entryURISrc);
	free (entryURISrcEncoded);
	free (entryURIDest);
	free (entryURIDestEncoded);
	free (urlPart);
	self->method = HTTP_METHOD_POST;
	err = wcs_Client_CallWithBuffer2 (self, NULL, url, NULL, 0, NULL);
	free (url);

	return err;
}

/*============================================================================*/
/* func wcs_RS_Move */

wcs_Error wcs_RS_Move (wcs_Client * self, const char *tableNameSrc, const char *keySrc, 
	const char *tableNameDest, const char *keyDest, const char *mgrHost)
{
	wcs_Error err;

	if ((NULL == self) || (NULL == tableNameSrc) || (NULL == keySrc) || (NULL == tableNameDest) || (NULL == keyDest))
	{
		err.code = 9899;
		err.message = "wcs_RS_Move param is NULL";
		return err;
	}
	char *url = NULL;
	char *entryURISrc = wcs_String_Concat3 (tableNameSrc, ":", keySrc);
	char *entryURISrcEncoded = wcs_String_Encode (entryURISrc);
	char *entryURIDest = wcs_String_Concat3 (tableNameDest, ":", keyDest);
	char *entryURIDestEncoded = wcs_String_Encode (entryURIDest);
	char *urlPart = wcs_String_Concat3 (entryURISrcEncoded, "/", entryURIDestEncoded);
	if (NULL != mgrHost)
	{
		url = wcs_String_Concat3 (mgrHost, "/move/", urlPart);
	}
	else
	{
	 	url = wcs_String_Concat3 (WCS_RS_HOST, "/move/", urlPart);
	 }

	free (entryURISrc);
	free (entryURISrcEncoded);
	free (entryURIDest);
	free (entryURIDestEncoded);
	free (urlPart);
	self->method = HTTP_METHOD_POST;
	err = wcs_Client_CallWithBuffer2 (self, NULL, url, NULL, 0, NULL);
	free (url);

	return err;
}
