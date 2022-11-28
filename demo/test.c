
#include "../src/wcs/io.h"
#include "../src/wcs/rs.h"
#include "../src/wcs/base.h"
#include "../src/wcs/multipart_io.h"
#include "../src/wcs/fop.h"
#include "../src/base/log.h"
#include "../src/base/inifile.h"

#include <stdio.h>
#include <stdlib.h>

#define copyArgv(argc, num, pointer) { if (argc > num) { if (strcmp(argv[num], "NULL") == 0) { pointer = NULL;} \
	else { pointer = argv[num];} } }
#define copyArgvInt(argc, num, pointer) { if (argc > num) { pointer = atoi(argv[num]);} }
#define myFree(pointer) {if (NULL != pointer) { free(pointer); pointer = NULL; }}
#define freeJson(ret) {if (NULL != ret) {wcs_Json_Destroy(ret); ret = NULL;}}

/*
*	@param AK SK bucketName keyName mgrHost
*/
wcs_Error wcs_RS_Delete_Test(int argc, char **argv)
{
	wcs_Error err = {200, "OK"};
	wcs_Client client;
	wcs_Mac mac;
	char *keyName = NULL;
	char *bucketName = NULL;
	char *mgrHost = NULL;

	wcs_Zero (client);
	wcs_Zero (mac);
	wcs_Servend_Init (0);

	copyArgv(argc, 2, mac.accessKey);
   	copyArgv(argc, 3, mac.secretKey);
   	copyArgv(argc, 4, bucketName);
   	copyArgv(argc, 5, keyName);
   	copyArgv(argc, 6, mgrHost);
	
	wcs_Client_InitMacAuth (&client, 4096, &mac);

	err = wcs_RS_Delete (&client, bucketName, keyName, mgrHost);

	wcs_Client_Cleanup (&client);
	wcs_Global_Cleanup();
	return err;
}

/*
*	@param accessKey secretKey srcBucketName srcKey destBucketName destKey mgrHost
*/
wcs_Error wcs_RS_Move_Test(int argc, char **argv)
{
	wcs_Error err = {200, "OK"};
	wcs_Client client;
	wcs_Mac mac;
	char *srcBucketName = NULL;
	char *destBucketName = NULL;
	char *srcKey = NULL;
	char *destKey = NULL;
	char *mgrHost = NULL;

	wcs_Zero (client);
	wcs_Zero (mac);
	wcs_Servend_Init (0);

	copyArgv(argc, 2, mac.accessKey);
   	copyArgv(argc, 3, mac.secretKey);
   	copyArgv(argc, 4, srcBucketName);
   	copyArgv(argc, 5, srcKey);
   	copyArgv(argc, 6, destBucketName);
   	copyArgv(argc, 7, destKey);
   	copyArgv(argc, 8, mgrHost);
	
	wcs_Client_InitMacAuth (&client, 4096, &mac);

	err = wcs_RS_Move (&client, srcBucketName, srcKey, destBucketName, destKey, mgrHost);

	wcs_Client_Cleanup (&client);
	wcs_Global_Cleanup();
	return err;
}

/*
*	@param accessKey secretKey srcBucketName srcKey destBucketName destKey mgrHost
*/
wcs_Error wcs_RS_Copy_Test(int argc, char **argv)
{
	wcs_Error err = {200, "OK"};
	wcs_Client client;
	wcs_Mac mac;
	char *srcBucketName = NULL;
	char *destBucketName = NULL;
	char *srcKey = NULL;
	char *destKey = NULL;
	char *mgrHost = NULL;

	wcs_Zero (client);
	wcs_Zero (mac);
	wcs_Servend_Init (0);

	copyArgv(argc, 2, mac.accessKey);
   	copyArgv(argc, 3, mac.secretKey);
   	copyArgv(argc, 4, srcBucketName);
   	copyArgv(argc, 5, srcKey);
   	copyArgv(argc, 6, destBucketName);
   	copyArgv(argc, 7, destKey);
   	copyArgv(argc, 8, mgrHost);
	
	wcs_Client_InitMacAuth (&client, 4096, &mac);

	err = wcs_RS_Copy (&client, srcBucketName, srcKey, destBucketName, destKey, mgrHost);

	wcs_Client_Cleanup (&client);
	wcs_Global_Cleanup();
	return err;
}

/*
*	@param accessKey secretKey bucketName keyName mgrHost
*/
wcs_Error wcs_RS_Stat_Test(int argc, char **argv)
{
	wcs_Error err = {200, "OK"};
	wcs_Client client;
	wcs_Mac mac;
	char *bucketName = NULL;
	char *keyName = NULL;
	char *mgrHost = NULL;
	wcs_RS_StatRet statRet;
	cJSON *root = NULL;
	

	wcs_Zero (client);
	wcs_Zero (mac);
	wcs_Zero(statRet);
	wcs_Servend_Init (0);

	copyArgv(argc, 2, mac.accessKey);
   	copyArgv(argc, 3, mac.secretKey);
   	copyArgv(argc, 4, bucketName);
   	copyArgv(argc, 5, keyName);
   	copyArgv(argc, 6, mgrHost);

	
	wcs_Client_InitMacAuth (&client, 4096, &mac);

	err = wcs_RS_Stat (&client, &root, bucketName, keyName, mgrHost);

	if (NULL != root)
	{
		statRet.hash = wcs_Json_GetString (root, "hash", NULL);
		statRet.mimeType = wcs_Json_GetString (root, "mimeType", NULL);
		statRet.fsize = wcs_Json_GetString (root, "fsize", 0);
		statRet.putTime = wcs_Json_GetInt64 (root, "putTime", 0);
		statRet.code = wcs_Json_GetInt64 (root, "code", 0);
		statRet.fileName = wcs_Json_GetString(root, "name", NULL);
		statRet.message = wcs_Json_GetString(root, "message", NULL);
		statRet.expirationDate = wcs_Json_GetString(root, "expirationDate", NULL);
		statRet.result = wcs_Json_GetString(root, "result", NULL);
	}
	printf("wcs_RS_Stat, result = %d\n", statRet.result);
	printf("wcs_RS_Stat, code = %d\n", statRet.code);
	if (NULL != statRet.message)
	{
		printf("wcs_RS_Stat, message = %s \n", statRet.message);
	}
	if (NULL != statRet.fsize)
	{
	    printf("wcs_RS_Stat, fsize = %s\n", statRet.fsize);
	}
	if (NULL != statRet.hash)
	{
		printf("wcs_RS_Stat, hash = %s \n", statRet.hash);
		
	}
	if (NULL != statRet.mimeType)
	{
		printf("wcs_RS_Stat, mimeType = %s\n", statRet.mimeType);
	}
	printf("wcs_RS_Stat, putTime = %ld\n", statRet.putTime);
	if (NULL != statRet.expirationDate)
	{
		printf("wcs_RS_Stat, expirationDate = %s\n", statRet.expirationDate);
	}

	wcs_Client_Cleanup (&client);
	wcs_Global_Cleanup();
	return err;
}

/*
	@param accessKey secretKey bucketName prefix limit mode marker mgrHost
*/
wcs_Error wcs_RS_List_Test(int argc, char **argv)
{
	wcs_Error err = {200, "OK"};
	wcs_Client client;
	wcs_Mac mac;
	char *mgrHost = NULL;
	char *bucketName = NULL;
	wcs_Common_Param param;
	cJSON *ret = NULL;
	

	wcs_Zero (client);
	wcs_Zero (mac);
	wcs_Zero(param);
	wcs_Servend_Init (0);

	copyArgv(argc, 2, mac.accessKey);
   	copyArgv(argc, 3, mac.secretKey);
   	copyArgv(argc, 4, bucketName);
   	copyArgv(argc, 5, mgrHost);
   	copyArgv(argc, 6,param.prefix);
   	copyArgvInt(argc, 7, param.limit);
   	copyArgv(argc, 8, param.mode);
   	copyArgvInt(argc, 9, param.marker);
	
	wcs_Client_InitMacAuth (&client, 4096, &mac);

	err = wcs_RS_List (&client, &ret, bucketName, &param, mgrHost);
	char *resultStr = NULL;
	cJSON *object = NULL;
	cJSON *items = NULL;
	long int resultInt = 0;
	int i = 0;
	int size = 0;
	
	if (NULL != ret)
	{
		resultStr = wcs_Json_GetString (ret, "marker", 0);
		if (NULL != resultStr)
		{
			printf("wcs_RS_List marker = %s\n", resultStr);
		}
		items = cJSON_GetObjectItem(ret, "commonPrefixes");
		if (NULL != items)
		{
			size = cJSON_GetArraySize(items);
			for (i = 0; i < size; i++)
			{
				object = cJSON_GetArrayItem(items, i);
				if (NULL != object)
				{
					resultStr = object->valuestring;
					if (NULL != resultStr)
					{
						printf("resultStr = %s\n", resultStr);
					}
				}
			}
		}
		items = cJSON_GetObjectItem(ret, "items");
		if (NULL != items)
		{
			size = cJSON_GetArraySize(items);
			for ( i = 0; i < size; i++)
			{
				printf("itemIndex = %d\n", i);
				object=cJSON_GetArrayItem(items,i);
				resultStr = wcs_Json_GetString(object, "key", 0);
				if (NULL != resultStr)
				{
					printf("key  = %s \n", resultStr);
				}
				resultInt = wcs_Json_GetInt64(object, "putTime", 0);
				printf("putTime = %ld\n", resultInt);
				resultStr = wcs_Json_GetString(object, "hash", 0);
				if (NULL != resultStr)
				{
					printf("hash  = %s \n", resultStr);
				}
				resultStr = wcs_Json_GetString(object, "fsize", 0);
				if (NULL != resultStr)
				{
					printf("fsize = %d\n", resultStr);
				}
				
				resultStr = wcs_Json_GetString(object, "mimeType", 0);
				if (NULL != resultStr)
				{
					printf("mimeType  = %s \n", resultStr);
				}
				resultStr = wcs_Json_GetString(object, "expirationDate", 0);
				if (NULL != resultStr)
				{
					printf("expirationDate  = %s \n", resultStr);
				}
			}
		}
	}
	
	wcs_Client_Cleanup (&client);
	wcs_Global_Cleanup();
	return err;
}

/*
*	@param accessKey secretKey bucketName mgrHost fileNum fileName1 fileName2 ....fileNameN
	the N is equal fileNum
*/
wcs_Error wcs_RS_UpdateMirror_Test(int argc, char **argv)
{
	wcs_Error err = {200, "OK"};
	wcs_Client client;
	wcs_Mac mac;
	char *mgrHost = NULL;
	char *bucketName = NULL;
	unsigned int fileNum = 0;
	char **fileNameList = NULL;
	char *resultStr = NULL;
	int resultInt = 0;
	int size = 0;
	cJSON *ret = NULL;
	cJSON *items = NULL;
	cJSON *object = NULL;
	unsigned int i = 0;
	unsigned int tmpNum = 0;
	

	wcs_Zero (client);
	wcs_Zero (mac);
	wcs_Global_Init (0);
	wcs_Servend_Init (0);
	wcs_MacAuth_Init ();

	copyArgv(argc, 2, mac.accessKey);
   	copyArgv(argc, 3, mac.secretKey);
	wcs_Client_InitMacAuth (&client, 4096, &mac);
   	copyArgv(argc, 4, bucketName);
   	copyArgv(argc, 5, mgrHost);
   	copyArgvInt(argc, 6, fileNum);
   	
	fileNameList = (char **)malloc(sizeof(char) * fileNum * 10);
   	for (i = 0; i < fileNum; i++)
   	{
   		tmpNum = 7+i;
   		copyArgv(argc, tmpNum, fileNameList[i]);
   	}

	err = wcs_RS_UpdateMirror(&client, &ret, bucketName, (const char ** )fileNameList, fileNum, mgrHost);
	
	if (NULL != ret)
	{
		bucketName = wcs_Json_GetString (ret, "bucket", NULL);
		items = cJSON_GetObjectItem(ret, "items");
		if (NULL != items)
		{
			size = cJSON_GetArraySize(items);
			for ( i = 0; i < size; i++)
			{
				object=cJSON_GetArrayItem(items,i);
				resultStr = wcs_Json_GetString(object, "key", NULL);
				if (NULL != resultStr)
				{
					printf("wcs_RS_UpdateMirror key = %s \n", resultStr);
				}
				resultStr = wcs_Json_GetString(object, "mirrorAddress", NULL);
				if (NULL != resultStr)
				{
					printf("wcs_RS_UpdateMirror mirrorAddress = %s \n", resultStr);
				}
				resultInt = wcs_Json_GetInt(object, "code", 0);
				printf("wcs_RS_UpdateMirror code = %d\n", resultInt);
				
				resultStr = wcs_Json_GetString(object, "message", NULL);
				if (NULL != resultStr)
				{
					printf("wcs_RS_UpdateMirror message = %s \n", resultStr);
				}
			}
		}
	}	
	
	freeJson(ret);
	free(fileNameList);
	fileNameList = NULL;
	
	wcs_Client_Cleanup (&client);
	wcs_Global_Cleanup();
	return err;
}

/*
*	@param accessKey secretKey bucketName key localFile upHost
*/

wcs_Error wcs_putFile_Test(int argc, char **argv)
{
	
	wcs_Error err;
	wcs_Client cli;
	wcs_Mac mac;
	wcs_Io_PutExtra extra;
	cJSON *ret = NULL;
	wcs_RS_PutPolicy pp;
	const char *uptoken = NULL;
	const char *key = NULL;
	const char *localFile = NULL;
	wcs_Io_PutExtraParam *head = NULL;
	wcs_Io_PutExtraParam *node = NULL;
	unsigned int paramNum = 0;
	unsigned int tmpIndex = 0;
	unsigned int i = 0;

	wcs_Zero(extra);
	wcs_Zero(pp);
	wcs_Zero(mac);
	wcs_Zero(cli);
	wcs_Servend_Init (0);

   	copyArgv(argc, 2, uptoken);
   	copyArgv(argc, 3, key);
   	copyArgv(argc, 4, localFile);
   	copyArgv(argc, 5, extra.upHost);
   	tmpIndex = 6;
   	copyArgvInt(argc, tmpIndex, paramNum);
   	tmpIndex++;
        
        for (i = 0; i < paramNum; i++)
        {
            node = (wcs_Io_PutExtraParam *)malloc(sizeof(wcs_Io_PutExtraParam));
            memset((void *)node, 0 , sizeof(wcs_Io_PutExtraParam));
            copyArgv(argc, tmpIndex, node->key);
            tmpIndex++;
            copyArgv(argc, tmpIndex, node->value);
            tmpIndex++;
            if (NULL == head)
            {
                head = node;
                extra.params = head;
            }
            else
            {
                head->next = node;
                head = node;
            }
        }
        
	wcs_Client_InitNoAuth (&cli, 8192);
	err = wcs_Io_PutFile (&cli, &ret, uptoken, key, localFile, &extra);

	LOG_TRACE ("code=%d msg=[%s]\n", err.code, err.message);
	if (cli.b.buf)
	{
	    LOG_TRACE("client.resph : %s", cli.b.buf);
	}
	
       head =  extra.params;
	for (i = 0; i < paramNum; i++)
	{
	    if (NULL != head)
	    {
	        node = head->next;
	        wcs_Free(head);
	        head = node;
	    }
	}

	wcs_Client_Cleanup (&cli);
	wcs_Global_Cleanup ();
	return err;
}

 /*
 *	 @param accessKey secretKey  key localFile upHost blockSize chunkSize
 */
 wcs_Error wcs_Multipart_Test(int  argc, char **argv)
{
	wcs_Error err = {200, "OK"};
	wcs_Client client;
	wcs_Multipart_PutExtra extra;
	wcs_RS_PutPolicy pp;
	wcs_Multipart_PutRet ret;
	wcs_Io_PutExtraParam *head = NULL;
	wcs_Io_PutExtraParam *node = NULL;
	wcs_Mac mac;
	char *key = NULL;
	const char *uptoken = NULL;
	const char *localFile = NULL;
	int index = 0;
	unsigned int i = 0;
	unsigned int xVarsCount = 0;

	wcs_Zero(ret);
	wcs_Zero(client);
	wcs_Zero(extra);
	wcs_Zero(pp);
	wcs_Zero(mac);
	client.patchInfoFile = "G:/acode/work/wangsu/wcs-c-sdk/build32/demo/Debug/PatchUpload.ini";
	index = 2;
	copyArgv(argc, index, uptoken);
	index++;
   	copyArgv(argc, index, key);
	index++;
   	copyArgv(argc, index, localFile);
	index++;
   	copyArgv(argc, index, extra.upHost);
	index++;
   	copyArgvInt(argc, index, extra.blockSize);
	index++;
   	copyArgvInt(argc, index, extra.chunkSize);
        index++;
        copyArgvInt(argc, index, xVarsCount);
        index++;
        for (i = 0; i < xVarsCount; i++)
        {
            node = (wcs_Io_PutExtraParam *)malloc(sizeof(wcs_Io_PutExtraParam));
            memset((void *)node, 0 , sizeof(wcs_Io_PutExtraParam));
            copyArgv(argc, index, node->key);
            index++;
            copyArgv(argc, index, node->value);
            index++;
            if (NULL == head)
            {
                head = node;
                extra.params = head;
            }
            else
            {
                head->next = node;
                head = node;
            }
        }
	copyArgv(argc, index, client.patchInfoFile);

	wcs_Global_Init (0);
	wcs_MacAuth_Init ();

	wcs_Client_InitNoAuth (&client, 8192);
	extra.key = key;

	err = wcs_Multipart_PutFile (&client, &ret, uptoken, key, localFile, &extra);
	uptoken = NULL;
       head = extra.params;
	while (head)
	{
	   node = head;
	   head = head->next;
	   wcs_Free(node);
	}
	wcs_Multipart_putRetCleanUp(&ret);
	wcs_Client_Cleanup (&client);
	wcs_Global_Cleanup ();
	return err;
}

/*
	@param
*/
wcs_Error wcs_Patchupload_Test(int argc, char **argv)
{
	wcs_Client client;
	wcs_Multipart_PutRet ret;
	char *configFile = NULL;
	wcs_Error err = {200, "OK"};

	wcs_Zero(ret);
	wcs_Zero(client);

	wcs_Global_Init (0);
	wcs_MacAuth_Init ();
	wcs_Client_InitNoAuth (&client, 4096);
	copyArgv(argc, 2, configFile);

	err = wcs_Multipart_UploadCheck(configFile, &client, &ret);

	wcs_Client_Cleanup (&client);
	wcs_Free(ret.persistentId);
	//wcs_Free(ret.hash);
	//wcs_Free(ret.key);
	wcs_MacAuth_Cleanup();
	wcs_Global_Cleanup ();	
	return err;
}

/*
*	@param 
               AK:
		SK:
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
*/
wcs_Error wcs_RS_PutPolicy_Token_Test(int argc, char **argv)
{
	wcs_Error err = {200, "OK"};
	wcs_Mac mac;
	wcs_RS_PutPolicy putPolicy;
	char *uptoken = NULL;

	wcs_Zero(mac);
	wcs_Zero(putPolicy);

	copyArgv(argc, 2, mac.accessKey);
	copyArgv(argc, 3, mac.secretKey);
	copyArgv(argc, 4, putPolicy.scope);
	copyArgv(argc, 5, putPolicy.saveKey);
	copyArgv(argc, 6, putPolicy.returnUrl);
	copyArgv(argc, 7, putPolicy.returnBody);
	copyArgv(argc, 8, putPolicy.callbackUrl);
	copyArgv(argc, 9, putPolicy.callbackBody);
	copyArgv(argc, 10, putPolicy.persistentOps);
	copyArgv(argc, 11, putPolicy.contentDetect);
	copyArgv(argc, 12, putPolicy.persistentNotifyUrl);
	copyArgv(argc, 13, putPolicy.detectNotifyURL);
	copyArgv(argc, 14, putPolicy.detectNotifyRule);
	copyArgvInt(argc, 15, putPolicy.overwrite);
	copyArgvInt(argc, 16, putPolicy.separate);
	copyArgvInt(argc, 17, putPolicy.fsizeLimit);
	copyArgvInt(argc, 18, putPolicy.expires);
	
	uptoken = wcs_RS_PutPolicy_Token (&putPolicy, &mac);
	LOG_TRACE("wcs_RS_PutPolicy_Token , uptoken = %s \n", uptoken);
	if (NULL == uptoken)
	{
		err.code = 9899;
		err.message = "the uptoken is NULL";
	}
	return err;
}

/*
*	@parm
	AK SK apiHost opsNum 
	
	char* fetchURL;
	char *bucket;
	char *key;
	char* prefix;
	char* md5;
	char* decompression;
	
	char *notifyURL;
	int force;
	wcs_Bool separate;
*/
wcs_Error wcs_Fops_Fetch_Test (int argc, char **argv)
{
	unsigned int i = 0;
	unsigned int tmpNum = 0;
	wcs_Error err;
	wcs_Mac mac;
	wcs_Client client;
	char *apiHost = NULL;
	wcs_FOPS_Response ret;
	wcs_FOPS_FetchParam *fetchOps = NULL;
	unsigned int opsNum = 0;

	wcs_Zero(err);
	wcs_Zero(mac);
	wcs_Zero(client);
	wcs_Zero(ret);
	
	wcs_Servend_Init (0);
	copyArgv(argc, 2, mac.accessKey);
   	copyArgv(argc, 3, mac.secretKey);
   	wcs_Client_InitMacAuth (&client, 2048, &mac);

   	copyArgv(argc, 4, apiHost);
	
	fetchOps = (wcs_FOPS_FetchParam *)malloc(sizeof(wcs_FOPS_FetchParam));
	memset(fetchOps, 0, sizeof(wcs_FOPS_FetchParam));
	tmpNum = 5;
	copyArgv(argc, tmpNum, fetchOps->ops);
	tmpNum++;
	copyArgv(argc, tmpNum, fetchOps->notifyURL);
	tmpNum++;
	copyArgvInt(argc, tmpNum, fetchOps->force);
	tmpNum++;
	copyArgvInt(argc, tmpNum, fetchOps->separate);
	tmpNum++;
	
	err = wcs_Fops_Fetch(&client, &ret, fetchOps, apiHost);
	myFree(fetchOps);
	myFree(ret.persistentId);
	wcs_Client_Cleanup (&client);
	wcs_Servend_Cleanup ();
	return err;
}


/*
*	@param
	AK SK mgrHost
	char *bucket;
	char *key;
	char *notifyURL;
	int force;
	wcs_Bool separate;
	unsigned int fopsNum;
	char **fops;
	wcs_Bool saveas;
	char *saveasBucket;
	char *saveasKey;
*/
wcs_Error wcs_Fops_Media_Test(int argc, char **argv)

{
	wcs_Error err;
	wcs_Mac mac;
	wcs_Client client;
	unsigned int i = 0;
	char *apiHost = NULL;
	wcs_FOPS_Response ret;
	unsigned int tmpNum = 0;
	wcs_FOPS_MediaParam mediaOps;

	wcs_Zero(mediaOps);
	wcs_Zero(client);
	wcs_Zero(mac);
	wcs_Zero(ret);
	wcs_Servend_Init (0);
	copyArgv(argc, 2, mac.accessKey);
   	copyArgv(argc, 3, mac.secretKey);
   	wcs_Client_InitMacAuth (&client, 1024, &mac);
   	copyArgv(argc, 4, apiHost);
   	
	tmpNum = 5;
	copyArgv(argc, tmpNum, mediaOps.bucket);
	tmpNum++;
	copyArgv(argc, tmpNum, mediaOps.key);
	tmpNum++;
	copyArgv(argc, tmpNum, mediaOps.fops);
	tmpNum++;
	copyArgv(argc, tmpNum, mediaOps.notifyURL);
	tmpNum++;
	copyArgvInt(argc, tmpNum, mediaOps.force);
	tmpNum++;
	copyArgvInt(argc, tmpNum, mediaOps.separate);
	
	
	err = wcs_Fops_Media(&client, &ret, &mediaOps, apiHost);
	
 	wcs_Client_Cleanup (&client);
 	wcs_Servend_Cleanup ();
 	return err;
}


wcs_Error wcs_Encode_Base64(int argc, char **argv)
{
    wcs_Error err = {0, ""};
    unsigned int tmpIndex = 2;
    char *toBeEncode = NULL;
    char *upToken = NULL;
    copyArgv(argc, tmpIndex, toBeEncode);
    upToken = wcs_String_Encode(toBeEncode);
    LOG_TRACE("ToBeEncodeString: %s", toBeEncode);
    if (NULL != upToken)
    {
        err.code = 200;
        err.message = "OK";
        LOG_TRACE("Uptoken: %s", upToken);
    }
    return err;
}

int main (int argc, char **argv)
{
	char interface_ = 0;
	wcs_Error err = {0, "OK"};
	copyArgvInt(argc, 1, interface_);
#if 1
	char *configFile = "G:/acode/work/wangsu/wcs-c-sdk/build32/demo/Debug/config.ini";
	int logLevel = LOG_TRACE;
	int writeLogFile = 1;
	FILE *file = NULL;
	logLevel = read_profile_int("TestLogConfig","LOG_LEVEL", logLevel, configFile);
	log_set_level(logLevel);
	writeLogFile =read_profile_int("TestLogConfig","WRITE_FILE", writeLogFile, configFile);
	if (writeLogFile)
	{
	    file = fopen("G:/acode/work/wangsu/wcs-c-sdk/build32/demo/Debug/sdktest.log", "a"); 
	    log_set_fp(file);
	}
#else
    FILE *file = NULL;
    wcs_Log_Init("G:/acode/work/wangsu/wcs-c-sdk/build32/demo/Debug/LogConfig.ini", file);
#endif

	switch (interface_)
	{
		case 1:
			err = wcs_RS_PutPolicy_Token_Test(argc, argv);
			break;
		case 2:
			err = wcs_putFile_Test(argc, argv);
			break;
		case 3:
			err = wcs_Multipart_Test(argc, argv);
			break;
		case 4:
			err = wcs_Patchupload_Test(argc, argv);
			break;
		case 5:
			err = wcs_RS_Delete_Test(argc, argv);
			break;
		case 6:
			err = wcs_RS_Stat_Test(argc, argv);
			break;
		case 7:
			err = wcs_RS_List_Test(argc, argv);
			break;
		case 8:
			err = wcs_RS_Move_Test(argc, argv);
			break;
		case 9:
			err = wcs_RS_UpdateMirror_Test(argc, argv);
			break;
		case 10:
			err = wcs_Fops_Media_Test(argc, argv);
			break;
		case 11:
			err = wcs_Fops_Fetch_Test(argc, argv);
			break;
		case 12:
			err = wcs_RS_Copy_Test(argc, argv);
			break;
		case 13:
		        err = wcs_Encode_Base64(argc, argv);
		default:
			break;
	}
	LOG_TRACE("Functin = %d, code = %d \n", interface_, err.code);
	wcs_close_Logfile(file);
	return 0;
}

// 
// -----------------------------------------------------------------------------
