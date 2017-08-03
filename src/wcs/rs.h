/*
 ============================================================================
 Name        : rs.h
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */

#ifndef WCS_RS_H
#define WCS_RS_H

#include "http.h"
#include "../cJSON/cJSON.h"
#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* type PutPolicy, GetPolicy */
/*
scope	是	上传的目标空间和文件名
deadline	是	上传请求授权截止时间戳
saveKey	否	自定义资源名。仅支持普通上传方式
fsizeLimit	否	限定上传文件的大小
overwrite	否	是否覆盖已存在文件。0-不覆盖 1-覆盖
returnUrl	否	Web文件长传成功后跳转地址 
returnBody	否	Web文件上传成功后返回数据 
callbackUrl	否	上传成功后回调地址
callbackBody	否	上传成功后回调内容
persistentNotifyUrl	否	预处理结果通知地址
persistentOps	否	文件上传成功后预处理指令列表
contentDetect	否	文件上传成功后，进行内容鉴定
detectNotifyURL	否	接收鉴定结果通知地址。需测试值
detectNotifyRule	否	鉴定结果通知规则
*/
/* @gist put-policy */

	typedef struct _wcs_RS_PutPolicy
	{
		const char *scope;
		const char *saveKey;
		const char *returnUrl;
		const char *returnBody;
		const char *callbackUrl;
		const char *callbackBody;
		const char *persistentOps;
		const char *contentDetect;
		const char *persistentNotifyUrl;
		const char *detectNotifyURL;
		const char *detectNotifyRule;
		wcs_Bool overwrite;
		wcs_Bool separate;
		wcs_Uint64 fsizeLimit;
		wcs_Uint32 expires;
	} wcs_RS_PutPolicy;

/* @endgist */

	typedef struct _wcs_RS_GetPolicy
	{
		wcs_Uint32 expires;
	} wcs_RS_GetPolicy;

	WCS_DLLAPI extern char *wcs_RS_PutPolicy_Token (wcs_RS_PutPolicy * policy, wcs_Mac * mac);
	WCS_DLLAPI extern char *wcs_RS_GetPolicy_MakeRequest (wcs_RS_GetPolicy * policy, const char *baseUrl, wcs_Mac * mac);
	WCS_DLLAPI extern char *wcs_RS_MakeBaseUrl (const char *domain, const char *key);

/*============================================================================*/
/* func wcs_RS_Stat */

	typedef struct _wcs_RS_StatRet
	{
		wcs_Bool result;
		long code;
		const char *fileName;
		const char *message;
		const char *hash;
		const char *mimeType;
		const char *expirationDate;
		wcs_Int64 fsize;
		wcs_Int64 putTime;
	} wcs_RS_StatRet;

	typedef struct _wcs_RS_ListRet
	{
		char *marker;
		char *commonPrefix;
	} wcs_RS_ListRet;


	WCS_DLLAPI extern wcs_Error wcs_RS_Stat (wcs_Client * self, cJSON ** ret, 
		const char *bucket, const char *key, const char *mgrHost);

/*============================================================================*/
/* func wcs_RS_Delete */

	WCS_DLLAPI extern wcs_Error wcs_RS_Delete (wcs_Client * self, const char *bucket, const char *key, const char *mgrHost);

/*
	list resource
*/
	WCS_DLLAPI extern wcs_Error wcs_RS_List (wcs_Client * self, cJSON ** ret, const char *bucketName, 
		wcs_Common_Param * param, const char *mgrHost);


WCS_DLLAPI extern wcs_Error wcs_RS_UpdateMirror(wcs_Client * self, cJSON ** ret, const char *bucketName, 
	const char **fileNameList, unsigned int fileNum, const char *mgrHost);

/*============================================================================*/
/* func wcs_RS_Copy */

	WCS_DLLAPI extern wcs_Error wcs_RS_Copy (wcs_Client * self, const char *tableNameSrc, 
		const char *keySrc, const char *tableNameDest, const char *keyDest, const char *mgrHost);

/*============================================================================*/
/* func wcs_RS_Move */

	WCS_DLLAPI extern wcs_Error wcs_RS_Move (wcs_Client * self, const char *tableNameSrc, 
		const char *keySrc, const char *tableNameDest, const char *keyDest, const char *mgrHost);

/*============================================================================*/
/* func wcs_RS_BatchStat */

/* @gist entrypath */

	typedef struct _wcs_RS_EntryPath
	{
		const char *bucket;
		const char *key;
	} wcs_RS_EntryPath;



	typedef int wcs_ItemCount;

	typedef struct _wcs_RS_EntryPathPair
	{
		wcs_RS_EntryPath src;
		wcs_RS_EntryPath dest;
	} wcs_RS_EntryPathPair;

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif							/* WCS_RS_H */
