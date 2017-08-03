/*
 ============================================================================
 Name        : fop.h
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */

#ifndef WCS_FOP_H
#define WCS_FOP_H

#include "http.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct _wcs_FOPS_FetchParam
{
    //wcs_FOPS_FetchCustom *custom;
    char *ops;
    char *notifyURL;
    int force;
    wcs_Bool separate;
} wcs_FOPS_FetchParam;


typedef struct _wcs_FOPS_MediaParam
{
	char *bucket;
	char *key;
	char *notifyURL;
	int force;
	wcs_Bool separate;
	char *fops;
	
}wcs_FOPS_MediaParam;

typedef struct _wcs_FOPS_Response
{
	const char *persistentId;
} wcs_FOPS_Response;

WCS_DLLAPI extern wcs_Error wcs_Fops_Fetch(wcs_Client * self, wcs_FOPS_Response * ret, 
	wcs_FOPS_FetchParam *ops, const char *apiHost);

WCS_DLLAPI extern wcs_Error wcs_Fops_Media (wcs_Client * self, wcs_FOPS_Response * ret, 
	wcs_FOPS_MediaParam *param, const char *apiHost);

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif							/* WCS_FOP_H */
