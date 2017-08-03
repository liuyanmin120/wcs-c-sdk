/*
 ============================================================================
 Name        : fop.c
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */

#include <curl/curl.h>

#include "fop.h"
#include "../cJSON/cJSON.h"

wcs_Error wcs_Fops_Fetch(wcs_Client * self, wcs_FOPS_Response * ret, 
	wcs_FOPS_FetchParam *ops,const char *apiHost)
{	
	wcs_Error err;
	cJSON *root = NULL;
	char *encode = NULL;
	char *url = NULL;
	char *bodys[4];
	char *finalBody = NULL;
	char *items[6];
	int itemCount = 0;
	int i = 0;

	if ((NULL == ops) || (NULL == self))
	{
		err.code = 9899;
		err.message = "wcs_fops_fetch param is invalid";
		return err;
	}

	for (i = 0; i < 4; i++)
	{
		bodys[i] = NULL;
	}
	if (ops->ops)
	{
		finalBody = ops->ops;
	}
	itemCount = 0;
	if (ops->notifyURL)
	{
		encode = wcs_String_Encode(ops->notifyURL);
		items[itemCount] = wcs_String_Concat2 ("notifyURL=", encode);
		curl_free (encode);		
		itemCount += 1;
	}

	if (ops->force == 1)
	{
		items[itemCount] = wcs_String_Dup ("force=1");
		itemCount += 1;
	}

	if (ops->separate == 1)
	{
		items[itemCount] = wcs_String_Dup ("separate=1");
		itemCount += 1;
	}
	if (itemCount > 0)
	{
		bodys[0] = wcs_String_Join("&", items, itemCount);
		bodys[1] = wcs_String_Concat3(finalBody, "&", bodys[0] );
        //wcs_Free(finalBody);
        wcs_Free(bodys[0]);
        finalBody = bodys[1];
	}
    for (i = 0; i < itemCount; i++)
    {
        wcs_Free(items[i]);
    }
	if (NULL == finalBody)
	{
		err.code = 9899;
		err.message = "wcs_Fops_Fetch param is invalid";
		return err;	
	}
	bodys[0] = wcs_String_Concat2("fops=", finalBody);
	if (itemCount > 0)
	{
		wcs_Free(finalBody);
	}
	finalBody = bodys[0];
	
	if (NULL != apiHost)
	{
		url = wcs_String_Concat2 (apiHost, "/fmgr/fetch");
	}
	else
	{
		url = wcs_String_Concat2 (WCS_Mgr_HOST, "/fmgr/fetch");
	}
	self->method = HTTP_METHOD_POST;
	
	err = wcs_Client_CallWithBuffer (self, &root, url, finalBody, strlen (finalBody), "application/x-www-form-urlencoded", NULL);
	wcs_Free (url);
	url = NULL;
	wcs_Free (finalBody);
	finalBody = NULL;	
	if (err.code == 200)
	{
		char *tmp = NULL;
		tmp= (char *)wcs_Json_GetString (root, "persistentId", 0);
		ret->persistentId = (char *)malloc(strlen(tmp) + 1);
		strcpy((char *)ret->persistentId, tmp);
	}
	if (NULL != root)
	{
		wcs_Json_Destroy(root);
		root = NULL;
	}

	return err;

}


wcs_Error wcs_Fops_Media (wcs_Client * self, wcs_FOPS_Response * ret, 
	wcs_FOPS_MediaParam *param, const char *apiHost)
{
    wcs_Error err;
    cJSON *root = NULL;
    char *encode = NULL;
    char *url = NULL;
    char *body = NULL;
    char *items[9];
    int itemCount = 0;
    int i = 0;

    if ((NULL == self) || (NULL == param))
    {
    	err.code = 9899;
    	err.message = "wcs_Fops_Media params is invalid";
    	return err;
    }
    if (param->bucket)
    {
    	encode = wcs_String_Encode(param->bucket);
    	items[itemCount] = wcs_String_Concat2 ("bucket=", encode);
    	
    	curl_free (encode);
    	itemCount += 1;
    }

    if (param->key)
    {
    	encode = wcs_String_Encode(param->key);
    	items[itemCount] = wcs_String_Concat2 ("key=", encode);
    	curl_free (encode);
    	
    	itemCount += 1;
    }

    if (param->fops)
    {
        items[itemCount] = wcs_String_Concat2 ("fops=", param->fops);
        itemCount += 1;
    }
    
    if (param->notifyURL)
    {
        encode = wcs_String_Encode(param->notifyURL);

        items[itemCount] = wcs_String_Concat2 ("notifyURL=", encode);
        curl_free (encode);		
        itemCount += 1;
    }

    if (param->force == 1)
    {
        items[itemCount] = wcs_String_Dup ("force=1");
        itemCount += 1;
    }

    if (param->separate == 1)
    {
        items[itemCount] = wcs_String_Dup ("separate=1");
        itemCount += 1;
    }
	
	if (itemCount > 0)
	{
    	body = wcs_String_Join ("&", items, itemCount);
	}


    if (NULL != apiHost)
    {
        url = wcs_String_Concat2 (apiHost, "/fops");
    }
    else
    {
        url = wcs_String_Concat2 (WCS_Mgr_HOST, "/fops");
    }
    self->method = HTTP_METHOD_POST;

    err = wcs_Client_CallWithBuffer (self, &root, url, body, strlen (body), "application/x-www-form-urlencoded", NULL);
    wcs_Free (url);
    wcs_Free (body);
    if (err.code == 200)
    {
    ret->persistentId = wcs_Json_GetString (root, "persistentId", 0);
    }

    for (i = itemCount - 1; i >= 0; i -= 1)
    {
    wcs_Free (items[i]);
    }	
    return err;
}
