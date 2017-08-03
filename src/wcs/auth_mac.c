/*
 ============================================================================
 Name        : mac_auth.c
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */

#include "http.h"
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/engine.h>

#if defined(_WIN32)
#pragma comment(lib, "libeay32.lib")
#endif

/*============================================================================*/
/* Global */

void wcs_MacAuth_Init ()
{
	ENGINE_load_builtin_engines ();
	ENGINE_register_all_complete ();
}

void wcs_MacAuth_Cleanup ()
{
}

void wcs_Servend_Init (long flags)
{
	wcs_Global_Init (flags);
	wcs_MacAuth_Init ();
}

void wcs_Servend_Cleanup ()
{
	wcs_Global_Cleanup ();
}

/*============================================================================*/
/* type wcs_Mac */

static wcs_Error wcs_Mac_Auth (void *self, wcs_Header ** header, const char *url, const char *addition, size_t addlen)
{
	wcs_Error err;
	char *auth;
	char *enc_digest;
	unsigned char digest[EVP_MAX_MD_SIZE + 1];
	unsigned int dgtlen = sizeof (digest);
	HMAC_CTX ctx;
	wcs_Mac mac;

	char const *path = strstr (url, "://");
	if (path != NULL)
	{
		path = strchr (path + 3, '/');
	}
	if (path == NULL)
	{
		err.code = 400;
		err.message = "invalid url";
		return err;
	}

	if (self)
	{
		mac = *(wcs_Mac *) self;
	}
	else
	{
		mac.accessKey = WCS_ACCESS_KEY;
		mac.secretKey = WCS_SECRET_KEY;
	}

	HMAC_CTX_init (&ctx);
	HMAC_Init_ex (&ctx, mac.secretKey, strlen (mac.secretKey), EVP_sha1 (), NULL);
	HMAC_Update (&ctx, path, strlen (path));
	HMAC_Update (&ctx, "\n", 1);

	if (addlen > 0)
	{
		HMAC_Update (&ctx, addition, addlen);
	}

	HMAC_Final (&ctx, digest, &dgtlen);
	HMAC_CTX_cleanup (&ctx);

	//handle digest
	unsigned char *result;
	result = (unsigned char *) malloc (sizeof (char) * dgtlen * 2 + 1);
	int i = 0;
	for (i = 0; i < dgtlen; i++)
	{
		sprintf (&result[i * 2], "%02x", (unsigned int) digest[i]);
	}
	result[dgtlen * 2] = '\0';

	enc_digest = wcs_Memory_Encode (result, dgtlen * 2);

	auth = wcs_String_Concat ("Authorization: ", mac.accessKey, ":", enc_digest, NULL);
	wcs_Free (enc_digest);
	free((void *)result);
	result = NULL;

	*header = curl_slist_append (*header, auth);
	wcs_Free (auth);

	return wcs_OK;
}

static void wcs_Mac_Release (void *self)
{
	if (self)
	{
		free (self);
	}
}

static wcs_Mac *wcs_Mac_Clone (wcs_Mac * mac)
{
	wcs_Mac *p;
	char *accessKey;
	size_t n1, n2;
	if ((NULL != mac) && (NULL != mac->accessKey) && (NULL != mac->secretKey))
	{
		n1 = strlen (mac->accessKey) + 1;
		n2 = strlen (mac->secretKey) + 1;
		p = (wcs_Mac *) malloc (sizeof (wcs_Mac) + n1 + n2);
		accessKey = (char *) (p + 1);
		memcpy (accessKey, mac->accessKey, n1);
		memcpy (accessKey + n1, mac->secretKey, n2);
		p->accessKey = accessKey;
		p->secretKey = accessKey + n1;
		return p;
	}
	return NULL;
}

static wcs_Auth_Itbl wcs_MacAuth_Itbl = {
	wcs_Mac_Auth,
	wcs_Mac_Release
};

wcs_Auth wcs_MacAuth (wcs_Mac * mac)
{
	wcs_Auth auth = { wcs_Mac_Clone (mac), &wcs_MacAuth_Itbl };
	return auth;
};

void wcs_Client_InitMacAuth (wcs_Client * self, size_t bufSize, wcs_Mac * mac)
{
	wcs_Auth auth = { wcs_Mac_Clone (mac), &wcs_MacAuth_Itbl };
	wcs_Client_InitEx (self, auth, bufSize);
}

/*============================================================================*/
/* func wcs_Mac_Sign*/

char *wcs_Mac_Sign (wcs_Mac * self, char *data)
{
	char *sign;
	char *encoded_digest;
	unsigned char digest[EVP_MAX_MD_SIZE + 1];
	unsigned int dgtlen = sizeof (digest);
	digest[EVP_MAX_MD_SIZE] = '\0';
	HMAC_CTX ctx;
	wcs_Mac mac;

	if (self)
	{
		mac = *self;
	}
	else
	{
		mac.accessKey = WCS_ACCESS_KEY;
		mac.secretKey = WCS_SECRET_KEY;
	}

	HMAC_CTX_init (&ctx);
	HMAC_Init_ex (&ctx, mac.secretKey, strlen (mac.secretKey), EVP_sha1 (), NULL);
	HMAC_Update (&ctx, (unsigned char *) data, strlen (data));
	HMAC_Final (&ctx, (unsigned char *) digest, &dgtlen);
	HMAC_CTX_cleanup (&ctx);

	//handle digest
	unsigned char *result;
	result = (unsigned char *) malloc (sizeof (char) * dgtlen * 2 + 1);
	int i = 0;
	for (i = 0; i < dgtlen; i++)
	{
		sprintf (&result[i * 2], "%02x", (unsigned int) digest[i]);
	}
	result[dgtlen * 2] = '\0';
	encoded_digest = wcs_Memory_Encode (result, dgtlen * 2);
	sign = wcs_String_Concat3 (mac.accessKey, ":", encoded_digest);
	wcs_Free (encoded_digest);
	free (result);
	result = NULL;

	return sign;
}

/*============================================================================*/
/* func wcs_Mac_SignToken */

char *wcs_Mac_SignToken (wcs_Mac * self, char *policy_str)
{
	char *data;
	char *sign;
	char *token;

	data = wcs_String_Encode (policy_str);
	sign = wcs_Mac_Sign (self, data);
	token = wcs_String_Concat3 (sign, ":", data);

	wcs_Free (sign);
	wcs_Free (data);

	return token;
}

/*============================================================================*/
