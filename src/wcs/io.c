/*
 ============================================================================
 Name        : io.c
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description :
 ============================================================================
 */

#include "io.h"
#include "reader.h"
#include "http.h"
#include <curl/curl.h>

/*============================================================================*/
/* func wcs_Io_form */

typedef struct _wcs_Io_form
{
	struct curl_httppost *formpost;
	struct curl_httppost *lastptr;
} wcs_Io_form;

static wcs_Io_PutExtra wcs_defaultExtra = { NULL, NULL, 0, 0, NULL };

static void wcs_Io_form_init (wcs_Io_form * self, const char *uptoken, const char *key, wcs_Io_PutExtra ** extra)
{
	wcs_Io_PutExtraParam *param;
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;

	curl_formadd (&formpost, &lastptr, CURLFORM_COPYNAME, "token", CURLFORM_COPYCONTENTS, uptoken, CURLFORM_END);

	if (*extra == NULL)
	{
		*extra = &wcs_defaultExtra;
	}
	if (key != NULL)
	{
		curl_formadd (&formpost, &lastptr, CURLFORM_COPYNAME, "key", CURLFORM_COPYCONTENTS, key, CURLFORM_END);
	}
	for (param = (*extra)->params; param != NULL; param = param->next)
	{
		curl_formadd (&formpost, &lastptr, CURLFORM_COPYNAME, param->key, CURLFORM_COPYCONTENTS, param->value, CURLFORM_END);
	}

	self->formpost = formpost;
	self->lastptr = lastptr;
}

/*============================================================================*/
/* func wcs_Io_PutXXX */

//CURL *wcs_Client_reset (wcs_Client * self);
//wcs_Error wcs_callex (CURL * curl, wcs_Buffer * resp, wcs_Json ** ret, wcs_Bool simpleError, wcs_Buffer * resph);

static wcs_Error wcs_Io_call (wcs_Client * self, wcs_Json ** ret, struct curl_httppost *formpost, wcs_Io_PutExtra * extra)
{
	int retCode = 0;
	wcs_Error err;
	struct curl_slist *headers = NULL;
	const char *upHost = NULL;

	CURL *curl = wcs_Client_reset (self, NULL);
    // solve return curl truncation bug
    curl = (CURL *) self->curl;

	// Bind the NIC for sending packets.
	if (self->boundNic != NULL)
	{
		retCode = curl_easy_setopt (curl, CURLOPT_INTERFACE, self->boundNic);
		if (retCode == CURLE_INTERFACE_FAILED)
		{
			err.code = 9994;
			err.message = "Can not bind the given NIC";
			return err;
		}
	}

	// Specify the low speed limit and time
	if (self->lowSpeedLimit > 0 && self->lowSpeedTime > 0)
	{
		retCode = curl_easy_setopt (curl, CURLOPT_LOW_SPEED_LIMIT, self->lowSpeedLimit);
		if (retCode == CURLE_INTERFACE_FAILED)
		{
			err.code = 9994;
			err.message = "Can not specify the low speed limit";
			return err;
		}
		retCode = curl_easy_setopt (curl, CURLOPT_LOW_SPEED_TIME, self->lowSpeedTime);
		if (retCode == CURLE_INTERFACE_FAILED)
		{
			err.code = 9994;
			err.message = "Can not specify the low speed time";
			return err;
		}
	}

	headers = curl_slist_append (NULL, "Expect:");

	if ((upHost = extra->upHost) == NULL)
	{
		//upHost = WCS_UP_HOST;
		upHost = wcs_String_Concat2(WCS_UP_HOST, "/file/upload");

		if (upHost == NULL)
		{
			err.code = 9988;
			err.message = "No proper upload host name";
			return err;
		}
	}
	
	curl_easy_setopt (curl, CURLOPT_URL, upHost);
	curl_easy_setopt (curl, CURLOPT_HTTPPOST, formpost);
	headers = curl_slist_append (headers, userAgent);
	curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);

	//// For aborting uploading file.
	if (extra->upAbortCallback)
	{
		curl_easy_setopt (curl, CURLOPT_READFUNCTION, wcs_Rd_Reader_Callback);
	}
	err = wcs_callex (curl, &self->b, &self->root, wcs_False, &self->respHeader);
	if (err.code == 200 && ret != NULL)
	{
		if (extra->callbackRetParser != NULL)
		{
			err = (*extra->callbackRetParser) (extra->callbackRet, self->root);
		}
		else
		{
			*ret = self->root;
		}
	}

	curl_formfree (formpost);
	curl_slist_free_all (headers);
	return err;
}


wcs_Error wcs_Io_PutFile (wcs_Client * self, cJSON ** ret, const char *uptoken, const char *key, const char *localFile, wcs_Io_PutExtra * extra)
{
	wcs_Error err;
	wcs_FileInfo fi;
	wcs_Rd_Reader rdr;
	wcs_Io_form form;
	size_t fileSize;
	const char *localFileName;
	wcs_Io_form_init (&form, uptoken, key, &extra);

	// BugFix : If the filename attribute of the file form-data section is not assigned or holds an empty string,
	//          and the real file size is larger than 10MB, then the Go server will return an error like
	//          "multipart: message too large".
	//          Assign an arbitary non-empty string to this attribute will force the Go server to write all the data
	//          into a temporary file and then every thing goes right.
	localFileName = (extra->localFileName) ? extra->localFileName : "WCS-C-SDK-UP-FILE";

	//// For aborting uploading file.
	if (extra->upAbortCallback)
	{
		wcs_Zero (rdr);

		rdr.abortCallback = extra->upAbortCallback;
		rdr.abortUserData = extra->upAbortUserData;

		err = wcs_Rd_Reader_Open (&rdr, localFile);
		if (err.code != 200)
		{
			return err;
		}

		if (extra->upFileSize == 0)
		{
			wcs_Zero (fi);
			err = wcs_File_Stat (rdr.file, &fi);
			if (err.code != 200)
			{
				return err;
			}

			fileSize = fi.st_size;
		}
		else
		{
			fileSize = extra->upFileSize;
		}

		curl_formadd (&form.formpost, &form.lastptr, CURLFORM_COPYNAME, "file", CURLFORM_STREAM, &rdr, CURLFORM_CONTENTSLENGTH, (long) fileSize, CURLFORM_FILENAME, localFileName, CURLFORM_END);
	}
	else
	{
		curl_formadd (&form.formpost, &form.lastptr, CURLFORM_COPYNAME, "file", CURLFORM_FILE, localFile, CURLFORM_FILENAME, localFileName, CURLFORM_END);
	}
#if 0
	//for multi-region stroage
	if (wcs_Rgn_IsEnabled ())
	{
		if (!extra->uptoken)
		{
			extra->uptoken = uptoken;
		}
	}
	
#endif
	err = wcs_Io_call (self, ret, form.formpost, extra);

	// For aborting uploading file.
	if (extra->upAbortCallback)
	{
		wcs_Rd_Reader_Close (&rdr);
		if (err.code == CURLE_ABORTED_BY_CALLBACK)
		{
			if (rdr.status == WCS_RD_ABORT_BY_CALLBACK)
			{
				err.code = 9987;
				err.message = "Upload progress has been aborted by caller";
			}
			else if (rdr.status == WCS_RD_ABORT_BY_READAT)
			{
				err.code = 9986;
				err.message = "Upload progress has been aborted by wcs_File_ReadAt()";
			}
		}
	}
	return err;
}

wcs_Error wcs_Io_PutBuffer (wcs_Client * self, cJSON ** ret, const char *uptoken, const char *key, const char *buf, size_t fsize, wcs_Io_PutExtra * extra)
{
	wcs_Io_form form;
	wcs_Error err = {200, "OK"};
	cJSON *root = NULL;
	wcs_Io_form_init (&form, uptoken, key, &extra);

	if (key == NULL)
	{
		key = "";
	}
	if ((NULL != extra) && (NULL != extra->upHost))
	{
		LOG_TRACE("wcs_Io_PutBuffer: upHost= %s", extra->upHost);
	}
	else
	{
		LOG_TRACE("wcs_Io_PutBuffer: extra/uphost is NULL");
	}
	curl_formadd (&form.formpost, &form.lastptr, CURLFORM_COPYNAME, "file", CURLFORM_BUFFER, key, CURLFORM_BUFFERPTR, buf, CURLFORM_BUFFERLENGTH, fsize, CURLFORM_END);

	err = wcs_Io_call (self, &root, form.formpost, extra);
	if ((200 == err.code) && (NULL != ret))
	{
	    if (wcs_Buffer_Len (&self->respHeader) != 0)
	    {
                root = cJSON_Parse (wcs_Buffer_CStr (&self->respHeader));
                *ret = root;
            }
	}
	return err;
}

// This function  will be called by 'wcs_Io_PutStream'
// In this function, readFunc(read-stream-data) will be set
static wcs_Error wcs_Io_call_with_callback (wcs_Client * self, wcs_Io_PutRet * ret, struct curl_httppost *formpost, rdFunc rdr, wcs_Io_PutExtra * extra)
{
	int retCode = 0;
	wcs_Error err;
	struct curl_slist *headers = NULL;

	CURL *curl = NULL;

	curl = wcs_Client_reset (self, NULL);
    // solve return curl truncation bug
    curl = (CURL *) self->curl;

	// Bind the NIC for sending packets.
	if (self->boundNic != NULL)
	{
		retCode = curl_easy_setopt (curl, CURLOPT_INTERFACE, self->boundNic);
		if (retCode == CURLE_INTERFACE_FAILED)
		{
			err.code = 9994;
			err.message = "Can not bind the given NIC";
			return err;
		}
	}

	headers = curl_slist_append (NULL, "Expect:");

	curl_easy_setopt (curl, CURLOPT_URL, WCS_UP_HOST);
	curl_easy_setopt (curl, CURLOPT_HTTPPOST, formpost);
	headers = curl_slist_append (headers, userAgent);
	curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt (curl, CURLOPT_READFUNCTION, rdr);

	err = wcs_callex (curl, &self->b, &self->root, wcs_False, &self->respHeader);
	if (err.code == 200 && ret != NULL)
	{
		if (extra->callbackRetParser != NULL)
		{
			err = (*extra->callbackRetParser) (extra->callbackRet, self->root);
		}
		else
		{
			ret->hash = wcs_Json_GetString (self->root, "hash", NULL);
			ret->key = wcs_Json_GetString (self->root, "key", NULL);
		}
	}

	curl_formfree (formpost);
	curl_slist_free_all (headers);
	return err;
}

wcs_Error wcs_Io_PutStream (wcs_Client * self, wcs_Io_PutRet * ret, const char *uptoken, const char *key, void *ctx, size_t fsize, rdFunc rdr, wcs_Io_PutExtra * extra)
{
	wcs_Io_form form;
	wcs_Io_form_init (&form, uptoken, key, &extra);

	if (key == NULL)
	{
		// Use an empty string instead of the NULL pointer to prevent the curl lib from crashing
		// when read it.
		// **NOTICE**: The magic variable $(filename) will be set as empty string.
		key = "";
	}

	// Add 'filename' property to make it like a file upload one
	// Otherwise it may report: CURL_ERROR(18) or "multipart/message too large"
	// See https://curl.haxx.se/libcurl/c/curl_formadd.html#CURLFORMSTREAM
	curl_formadd (&form.formpost, &form.lastptr, CURLFORM_COPYNAME, "file", CURLFORM_FILENAME, "filename", CURLFORM_STREAM, ctx, CURLFORM_CONTENTSLENGTH, fsize, CURLFORM_END);

	return wcs_Io_call_with_callback (self, ret, form.formpost, rdr, extra);
}
