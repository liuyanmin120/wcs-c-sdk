/*
 ============================================================================
 Name        : multipart_io.c
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description :
 ============================================================================
 */

#include "multipart_io.h"
#include "../base/inifile.h"
#include "rs.h"
#include "conf.h"
#include <curl/curl.h>
#include <sys/stat.h>

#define	blockBits			22
#define blockMask			((1 << blockBits) - 1)

#define defaultTryTimes		3
#define defaultWorkers		4
#define defaultChunkSize	(256 * 1024)	// 256k
#define defaultBlockSize (4 * 1024 * 1024)

/*============================================================================*/
/* type wcs_Multipart_ST - SingleThread */

#if defined(_WIN32)

#include <windows.h>

typedef struct _wcs_Multipart_MTWG_Data
{
	wcs_Count addedCount;
	wcs_Count doneCount;
	HANDLE event;
} wcs_Multipart_MTWG_Data;

static void wcs_Multipart_MTWG_Add (void *self, int n)
{
	wcs_Count_Inc (&((wcs_Multipart_MTWG_Data *) self)->addedCount);
}								// wcs_Multipart_MTWG_Add

static void wcs_Multipart_MTWG_Done (void *self)
{
	wcs_Count_Inc (&((wcs_Multipart_MTWG_Data *) self)->doneCount);
	SetEvent (((wcs_Multipart_MTWG_Data *) self)->event);
}								// wcs_Multipart_MTWG_Done

static void wcs_Multipart_MTWG_Wait (void *self)
{
	wcs_Multipart_MTWG_Data *data = (wcs_Multipart_MTWG_Data *) self;
	wcs_Count lastDoneCount = data->doneCount;
	DWORD ret = 0;

	while (lastDoneCount != data->addedCount)
	{
		ret = WaitForSingleObject (((wcs_Multipart_MTWG_Data *) self)->event, INFINITE);
		if (ret == WAIT_OBJECT_0)
		{
			lastDoneCount = data->doneCount;
		}
	}							// while
}								// wcs_Multipart_MTWG_Wait

static void wcs_Multipart_MTWG_Release (void *self)
{
	CloseHandle (((wcs_Multipart_MTWG_Data *) self)->event);
	free (self);
}								// wcs_Multipart_MTWG_Release

static wcs_Multipart_WaitGroup_Itbl wcs_Multipart_MTWG_Itbl = {
	&wcs_Multipart_MTWG_Add,
	&wcs_Multipart_MTWG_Done,
	&wcs_Multipart_MTWG_Wait,
	&wcs_Multipart_MTWG_Release,
};

wcs_Multipart_WaitGroup wcs_Multipart_MTWG_Create (void)
{
	wcs_Multipart_WaitGroup wg;
	wcs_Multipart_MTWG_Data *newData = NULL;

	newData = (wcs_Multipart_MTWG_Data *) malloc (sizeof (*newData));
	newData->addedCount = 0;
	newData->doneCount = 0;
	newData->event = CreateEvent (NULL, FALSE, FALSE, NULL);

	wg.itbl = &wcs_Multipart_MTWG_Itbl;
	wg.self = newData;
	return wg;
}								// wcs_Multipart_MTWG_Create

#endif

static void wcs_Multipart_STWG_Add (void *self, int n)
{
}

static void wcs_Multipart_STWG_Done (void *self)
{
	//printf("wcs_Multipart_STWG_Done");
	if (NULL != self)
	{
		wcs_Multipart_task *task = (wcs_Multipart_task *)self;
		wcs_Client *client = NULL;	
		client = task->mc;
		if ((NULL != client) && (NULL != client->threadInfo))
		{
			wcs_Mutex_Lock(&(client->threadInfo->doneBlkNumMutex));
			client->threadInfo->doneBlkNum++;
			wcs_Mutex_Unlock(&client->threadInfo->doneBlkNumMutex);
		}

		if (NULL != task->multiCurl)
		{
			//curl_easy_clean(task->multiCurl);
			wcs_Curl_cleanup(task->multiCurl);
			task->multiCurl = NULL;
		}
		if (client->threadInfo->doneBlkNum >= client->threadInfo->totalBlkNum)
		{
			wcs_Mutex_Lock(&client->threadInfo->totalDoneMutex);
			pthread_cond_signal(&client->threadInfo->notify);
			wcs_Mutex_Unlock(&client->threadInfo->totalDoneMutex);
		}
	}
}

static void wcs_Multipart_STWG_Wait (void *self)
{
	//printf("wcs_Multipart_STWG_Wait");
	if (NULL != self)
	{
		wcs_Client *client = (wcs_Client *)self;
		
		if ((NULL != client) && (NULL != client->threadInfo))
		{
			
			wcs_Mutex_Lock(&client->threadInfo->totalDoneMutex);
			pthread_cond_wait(&client->threadInfo->notify, &client->threadInfo->totalDoneMutex);
			wcs_Mutex_Unlock(&client->threadInfo->totalDoneMutex);
		}
		if (NULL != client->threadPool)
		{
			threadpool_destroy((threadpool_t *)(client->threadPool), 0);
		}
	}
}

static void wcs_Multipart_STWG_Release (void *self)
{
}

static wcs_Multipart_WaitGroup_Itbl wcs_Multipart_STWG_Itbl = {
	wcs_Multipart_STWG_Add,
	wcs_Multipart_STWG_Done,
	wcs_Multipart_STWG_Wait,
	wcs_Multipart_STWG_Release
};

static wcs_Multipart_WaitGroup wcs_Multipart_STWG = {
	NULL, &wcs_Multipart_STWG_Itbl
};

static wcs_Multipart_WaitGroup wcs_Multipart_ST_WaitGroup (void *self)
{
	return wcs_Multipart_STWG;
}

static wcs_Client *wcs_Multipart_ST_ClientTls (void *self, wcs_Client * mc)
{
	return mc;
}

static int wcs_multiThread_info_init(wcs_multiThread_info *info)
{
	int ret = 0;
	if (NULL == info)
	{
		ret = -1;
		return ret;
	}
	wcs_Mutex_Init(&info->totalDoneMutex);
	wcs_Mutex_Init(&info->doneBlkNumMutex);
	
	wcs_Mutex_Lock(&info->totalDoneMutex);
	info->totalBlkNum = 0;
	wcs_Mutex_Unlock(&info->totalDoneMutex);
	wcs_Mutex_Lock(&info->totalDoneMutex);
	info->doneBlkNum= 0;
	wcs_Mutex_Unlock(&info->totalDoneMutex);
	pthread_cond_init(&info->notify, NULL);
	return 0;
}

static int wcs_Multipart_ST_RunTask (void *self, void (*task) (void *params), void *params)
{
	wcs_Multipart_task *taskInfo = NULL;
	if ((NULL == self) || (NULL == task) || (NULL == params))
	{
		return WCS_RIO_NOTIFY_EXIT;
	}
	taskInfo =  (wcs_Multipart_task *)params;
	wcs_Client *client = taskInfo->mc;
	if ((NULL != client) && (client->multiTask == wcs_True))
	{
		//add task to threadpool
		threadpool_add(taskInfo->threadPool, task, params, 0);
	}
	else
	{
		task (params);
		LOG_WARN("wcs_Multipart_ST_RunTask error");
	}
	return WCS_RIO_NOTIFY_OK;
}

static wcs_Multipart_ThreadModel_Itbl wcs_Multipart_ST_Itbl = {
	wcs_Multipart_ST_WaitGroup,
	wcs_Multipart_ST_ClientTls,
	wcs_Multipart_ST_RunTask
};

wcs_Multipart_ThreadModel wcs_Multipart_ST = {
	NULL, &wcs_Multipart_ST_Itbl
};

/*============================================================================*/
/* type wcs_Multipart_Settings */

static wcs_Multipart_Settings settings = {
	defaultWorkers * 4,
	defaultWorkers,
	defaultChunkSize,
	defaultTryTimes,
	{NULL, &wcs_Multipart_ST_Itbl}
};

void wcs_Multipart_SetSettings (wcs_Multipart_Settings * v)
{
	settings = *v;
	if (settings.workers == 0)
	{
		settings.workers = defaultWorkers;
	}
	if (settings.taskQsize == 0)
	{
		settings.taskQsize = settings.workers * 4;
	}
	if (settings.chunkSize == 0)
	{
		settings.chunkSize = defaultChunkSize;
	}
	if (settings.tryTimes == 0)
	{
		settings.tryTimes = defaultTryTimes;
	}
	if (settings.threadModel.itbl == NULL)
	{
		settings.threadModel = wcs_Multipart_ST;
	}
}

/*============================================================================*/
/* func wcs_UptokenAuth */

static wcs_Error wcs_UptokenAuth_Auth (void *self, wcs_Header ** header, const char *url, const char *addition, size_t addlen)
{
	wcs_Error err;

	*header = curl_slist_append (*header, self);

	err.code = 200;
	err.message = "OK";
	return err;
}

static void wcs_UptokenAuth_Release (void *self)
{
	free (self);
}

static wcs_Auth_Itbl wcs_UptokenAuth_Itbl = {
	wcs_UptokenAuth_Auth,
	wcs_UptokenAuth_Release
};

static wcs_Auth wcs_UptokenAuth (const char *uptoken)
{
	char *self = wcs_String_Concat2 ("Authorization: UpToken ", uptoken);
	wcs_Auth auth = { self, &wcs_UptokenAuth_Itbl };
	return auth;
}

/*============================================================================*/
/* type wcs_Multipart_BlkputRet */

static void wcs_Multipart_BlkputRet_Cleanup (wcs_Multipart_BlkputRet * self)
{
	wcs_Buffer *resph;
	wcs_Buffer *resp;
	if (self->ctx != NULL)
	{
		free ((void *) self->ctx);
		resph = self->resph;
		resp = self->resp;
		memset (self, 0, sizeof (*self));
		self->resph = resph;
		self->resp = resp;
	}
}

static void wcs_Multipart_BlkputRet_Assign (wcs_Multipart_BlkputRet * self, wcs_Multipart_BlkputRet * ret)
{
	char *p;
	size_t n1 = 0, n2 = 0, n3 = 0;
	if ((NULL == self) || (NULL == ret))
	{
		return;
	}
	wcs_Multipart_BlkputRet_Cleanup (self);

	*self = *ret;
	if (ret->ctx == NULL)
	{
		return;
	}

	n1 = strlen (ret->ctx) + 1;
	if (ret->checksum)
	{
		n2 = strlen (ret->checksum) + 1;
	}

	p = (char *) malloc (n1 + n2 + n3);

	memcpy (p, ret->ctx, n1);
	self->ctx = p;

	if (n2)
	{
		memcpy (p + n1 + n3, ret->checksum, n2);
		self->checksum = p + n1 + n3;
	}
}

static void wcs_Multipart_putRetAssign(wcs_Multipart_PutRet *self, wcs_Multipart_PutRet *ret)
{
	char *p;
	size_t n1 = 0, n2 = 0, n3 = 0;
	if ((NULL == self) || (NULL == ret))
	{
		return;
	}
	if (NULL != self->hash)
	{
		free((void *)self->hash);
		self->hash = NULL;
	}
	if (NULL != self->key)
	{
		free((void *)self->key);
		self->key = NULL;
	}
	if (NULL != ret->hash)
	{
		n1 = strlen(ret->hash) + 1;
	}
	if (NULL != ret->key)
	{
		n2 = strlen(ret->key) + 1;
	}
	if (NULL != ret->persistentId)
	{
		n3 = strlen(ret->persistentId) + 1;
	}

	p = (char *) malloc (n1 + n2 + n3);

	if (n1)
	{
		memcpy(p, ret->hash, n1);
		self->hash = p;
	}
	if (n2)
	{
		memcpy(p+n1, ret->key, n2);
		self->key = p+n1;
	}

	if (n3)
	{
		memcpy(p, ret->persistentId, n3);
		self->persistentId = p + n1 + n2;
	}
	
}

void wcs_Multipart_putRetCleanUp(wcs_Multipart_PutRet *ret)
{
	if (NULL != ret)
	{
		if (NULL != ret->hash)
		{
			free((void *)ret->hash);
			ret->hash = NULL;
			return;
		}	
		if (NULL != ret->key)
		{
			free((void *)ret->key);
			ret->key = NULL;
			return;
		}
		if (NULL != ret->persistentId)
		{
			free((void *)ret->persistentId);
			ret->persistentId = NULL;
			return;
		}
	}
}

/*============================================================================*/
/* type wcs_Multipart_PutExtra */

static int notifyNil (void *self, int blkIdx, int blkSize, wcs_Multipart_BlkputRet * ret)
{
	return WCS_RIO_NOTIFY_OK;
}

static int notifyErrNil (void *self, int blkIdx, int blkSize, wcs_Error err)
{
	return WCS_RIO_NOTIFY_OK;
}

static wcs_Error ErrInvalidPutProgress = {
	wcs_Multipart_InvalidPutProgress, "invalid put progress"
};

static wcs_Error wcs_Multipart_PutExtra_Init (wcs_Multipart_PutExtra * self, wcs_Int64 fsize, wcs_Multipart_PutExtra * extra)
{
	size_t cbprog;
	int i, blockCnt =0;
	int blockSize = 0;
	int fprog = (extra != NULL) && (extra->progresses != NULL);

	if (NULL != extra)
	{
		blockSize = extra->blockSize;
	}
	blockCnt = wcs_Multipart_BlockCount (fsize, blockSize);
	if (fprog && extra->blockCnt != (size_t) blockCnt)
	{
		return ErrInvalidPutProgress;
	}

	if (extra)
	{
		*self = *extra;
	}
	else
	{
		memset (self, 0, sizeof (wcs_Multipart_PutExtra));
	}

	cbprog = sizeof (wcs_Multipart_BlkputRet) * blockCnt;
	LOG_TRACE("wcs_Multipart_PutExtra_Init: blockCnt = %d", blockCnt);
	self->progresses = (wcs_Multipart_BlkputRet *) malloc (cbprog);
	self->blockCnt = blockCnt;
	memset (self->progresses, 0, cbprog);
	if (fprog)
	{
		for (i = 0; i < blockCnt; i++)
		{
			wcs_Multipart_BlkputRet_Assign (&self->progresses[i], &extra->progresses[i]);
		}
	}
	
	if (self->chunkSize == 0)
	{
		self->chunkSize = settings.chunkSize;
	}
	if (self->tryTimes == 0)
	{
		self->tryTimes = settings.tryTimes;
	}
	if (self->notify == NULL)
	{
		self->notify = notifyNil;
	}
	if (self->notifyErr == NULL)
	{
		self->notifyErr = notifyErrNil;
	}
	if (self->threadModel.itbl == NULL)
	{
		self->threadModel = settings.threadModel;
	}
	if (self->blockSize == 0)
	{
		self->blockSize  = defaultBlockSize;
	}
	if (NULL == self->upHost)
	{
		self->upHost = WCS_UP_HOST;
	}
	//Generate UUID

	unsigned char *uuid = NULL;
	const unsigned int uuid_len = 32;
	uuid = (unsigned char *) malloc (sizeof (char) * uuid_len + 1);
	wcs_GenerateUUID (uuid, uuid_len + 1);
	if (self->uploadBatch == NULL)
	{
		self->uploadBatch = uuid;
	}

	return wcs_OK;
}

static void wcs_Multipart_PutExtra_Cleanup (wcs_Multipart_PutExtra * self)
{
	size_t i;
	wcs_Io_PutExtraParam *head = NULL;
	wcs_Io_PutExtraParam *node = NULL;
	for (i = 0; i < self->blockCnt; i++)
	{
		wcs_Multipart_BlkputRet_Cleanup (&self->progresses[i]);
	}
	free (self->progresses);
	self->progresses = NULL;
	self->blockCnt = 0;
	if (self->uploadBatch != NULL)
	{
		free ((char *)self->uploadBatch);
		self->uploadBatch = NULL;
	}
	wcs_Free(self->key);
	
}

static wcs_Int64 wcs_Multipart_PutExtra_ChunkSize (wcs_Multipart_PutExtra * self)
{
	if (self)
	{
		return self->chunkSize;
	}
	return settings.chunkSize;
}

static void wcs_Io_PutExtra_initFrom (wcs_Io_PutExtra * self, wcs_Multipart_PutExtra * extra)
{
	wcs_Io_PutExtraParam *head = NULL;
	wcs_Io_PutExtraParam *node = NULL;
	wcs_Io_PutExtraParam *headExtra = NULL;
	if (extra)
	{
		self->mimeType = extra->mimeType;
		self->localFileName = extra->localFileName;
		self->upHost = wcs_String_Concat2(extra->upHost, "/file/upload");
		headExtra = extra->params;
		while(headExtra)
		{
			node = (wcs_Io_PutExtraParam *)malloc(sizeof(wcs_Io_PutExtraParam));
			memset(node, 0, sizeof(wcs_Io_PutExtraParam));
			node->key = (char *)malloc(sizeof(char) *strlen(headExtra->key) + 1);
			strcpy(node->key, headExtra->key);
			node->value = (char *)malloc(sizeof(char) *strlen(headExtra->value) + 1);
			strcpy(node->value, headExtra->value);
			if (NULL == head)
			{
				head = node;
				self->params = head;
			}
			else
			{
				head->next = node;
				head = node;
			}
			headExtra = headExtra->next;
		}
	}
	else
	{
		memset (self, 0, sizeof (*self));
	}
}

/*
*	外部申请空间，大小为32
*/
void wcs_GenerateUUID (unsigned char *result, unsigned int len)
{
	if (NULL == result)
	{
		return;
	}
	unsigned char t[36] = "abcdefghijklmnopqrstuvwxyz1234567890";
	int i;
	srand (time (NULL));		// 种子
	for (i = 0; i < 32; i++)
		sprintf ((char *)&result[i], "%c", t[rand () % 36]);
	return;
}

static wcs_Bool wcs_isUploadedBlock(wcs_PatchInfo *patchInfo, const int *blkIdx)
{
	wcs_Bool ret = wcs_False;
	unsigned int i = 0;
	int *blocks = NULL;
	if ((NULL == patchInfo) || (NULL == blkIdx) || (NULL == patchInfo->blocks))
	{
		return ret;
	}
	blocks = *patchInfo->blocks;
	for (i = 0; i < patchInfo->succNum; i++)
	{
		if (*blkIdx == blocks[i])
		{
			ret = wcs_True;
			break;
		}
	}
	return ret;
}


/*============================================================================*/

static wcs_Error wcs_Multipart_bput (wcs_Client * self, wcs_Multipart_BlkputRet * ret, wcs_Reader body, 
	int bodyLength, const char *url, wcs_Multipart_PutExtra * extra, CURL *curl)
{
	wcs_Multipart_BlkputRet retFromResp;
	wcs_Json *root = NULL;
	wcs_Common_Header common_header;

	wcs_Zero(common_header);
	wcs_Zero(retFromResp);
	if (extra != NULL)
	{
		common_header.uploadBatch = extra->uploadBatch;
		common_header.key = extra->key;
		common_header.curl = curl;
		common_header.resp = ret->resp;
		common_header.resph = ret->resph;
	}
	
	wcs_Error err = wcs_Client_CallWithBinary (self, &root, url, body, bodyLength, 
		NULL, &common_header);
	if (err.code == 200)
	{
		retFromResp.ctx = wcs_Json_GetString (root, "ctx", NULL);
		retFromResp.checksum = wcs_Json_GetString (root, "checksum", NULL);
		retFromResp.crc32 = (wcs_Uint32) wcs_Json_GetInt64 (root, "crc32", 0);
		retFromResp.offset = (wcs_Uint32) wcs_Json_GetInt64 (root, "offset", 0);

		if (retFromResp.ctx == NULL)
		{
			err.code = 9998;
			err.message = "unexcepted response: invalid ctx, host or offset";
			
			if (NULL != root)
			{
				wcs_Json_Destroy(root);
				root = NULL;
			}
			return err;
		}
		wcs_Multipart_BlkputRet_Assign (ret, &retFromResp);
	}
	
	if (NULL != root)
	{
		wcs_Json_Destroy(root);
		root = NULL;
	}
	

	return err;
}

static wcs_Error wcs_Multipart_Mkblock (wcs_Client * self, wcs_Multipart_BlkputRet * ret, 
	int blkSize, int blkIdx,
	wcs_Reader body, int bodyLength, wcs_Multipart_PutExtra * extra, CURL *curl)
{
	wcs_Error err;
	const char *upHost = NULL;
	char *url = NULL;

	if ((upHost = extra->upHost) == NULL)
	{
		err.code = 9988;
		err.message = "No proper upload host name";
		return err;
	}

	url = wcs_String_Format (256, "%s/mkblk/%d/%d", upHost, blkSize, blkIdx);
	err = wcs_Multipart_bput (self, ret, body, bodyLength, url, extra, curl);
	wcs_Free (url);

	return err;
}

static wcs_Error wcs_Multipart_Blockput (wcs_Client * self, wcs_Multipart_BlkputRet * ret, wcs_Reader body, int bodyLength, 
	wcs_Multipart_PutExtra * extra, CURL *curl)
{
	wcs_Error err = { 200, NULL };

	if ((NULL == extra) || (NULL == extra->upHost))
	{
		err.code = 9988;
		return err;
	}

	char *url = wcs_String_Format (1024, "%s/bput/%s/%d", extra->upHost, ret->ctx, (int) ret->offset);
	err = wcs_Multipart_bput (self, ret, body, bodyLength, url, extra, curl);
	wcs_Free (url);
	return err;
}

/*============================================================================*/

static wcs_Error ErrUnmatchedChecksum = {
	wcs_Multipart_UnmatchedChecksum, "unmatched checksum"
};

static int wcs_TemporaryError (int code)
{
	return code != 401;
}

static wcs_Error wcs_Multipart_ResumableBlockput (wcs_Client * c, wcs_Multipart_BlkputRet * ret, wcs_ReaderAt f, int blkIdx, int blkSize, 
	wcs_Multipart_PutExtra * extra, CURL *curl)
{
	wcs_Error err = { 200, NULL };
	wcs_Tee tee;
	wcs_Section section;
	wcs_Reader body, body1;

	wcs_Crc32 crc32;
	wcs_Writer h = wcs_Crc32Writer (&crc32, 0);
	wcs_Int64 offbase = 0 ; //(wcs_Int64) (blkIdx) << blockBits;
	wcs_Buffer *resp = NULL;
	wcs_Buffer *resph = NULL;

	int chunkSize = extra->chunkSize;
	int bodyLength;
	int tryTimes;
	int notifyRet = 0;


	offbase = (wcs_Int64) (blkIdx) * (extra->blockSize);
	resph = ret->resph;
	resp = ret->resp;
	if (ret->ctx == NULL)
	{

		if (chunkSize < blkSize)
		{
			bodyLength = chunkSize;
		}
		else
		{
			bodyLength = blkSize;
		}

		body1 = wcs_SectionReader (&section, f, (wcs_Off_T) offbase, bodyLength);
		body = wcs_TeeReader (&tee, body1, h);
		//extra->blkIdx = blkIdx;
		if (err.code != 200)
		{
			return err;
		}
		LOG_TRACE("wcs_Multipart_Mkblock before: blkIdx = %d", blkIdx);
		err = wcs_Multipart_Mkblock (c, ret, blkSize, blkIdx, body, bodyLength, extra, curl);
		if (err.code != 200)
		{
			LOG_ERROR("wcs_Multipart_Mkblock return code = %d", err.code);
			return err;
		}
		if (ret->crc32 != crc32.val || (int) (ret->offset) != bodyLength)
		{
			LOG_ERROR("unmatched checksum");
			return ErrUnmatchedChecksum;
		}
		notifyRet = extra->notify (extra->notifyRecvr, blkIdx, blkSize, ret);
		if (notifyRet == WCS_RIO_NOTIFY_EXIT)
		{
			// Terminate the upload process if  the caller requests
			err.code = wcs_Multipart_PutInterrupted;
			err.message = "Interrupted by the caller";
			LOG_ERROR("Interrupted by the caller");
			return err;
		}
	}
	while ((int) (ret->offset) < blkSize)
	{

		if (chunkSize < blkSize - (int) (ret->offset))
		{
			bodyLength = chunkSize;
		}
		else
		{
			bodyLength = blkSize - (int) (ret->offset);
		}

		tryTimes = extra->tryTimes;

	  lzRetry:
		crc32.val = 0;
		ret->resp = resp;
		ret->resph = resph;
		if (((int )(ret->offset) >= blkSize))
		{
			LOG_TRACE("blkSize = %d, offbase = %d, ret->offset = %d ,blkIdx = %d, bodyLength = %d",
					blkSize, offbase, ret->offset, blkIdx, bodyLength);
			
			break;
		}
		body1 = wcs_SectionReader (&section, f, (wcs_Off_T) (offbase + (ret->offset)), bodyLength);
		body = wcs_TeeReader (&tee, body1, h);

		err = wcs_Multipart_Blockput (c, ret, body, bodyLength, extra, curl);
		if (err.code == 200)
		{
			if (ret->crc32 == crc32.val)
			{
				notifyRet = extra->notify (extra->notifyRecvr, blkIdx, blkSize, ret);
				if (notifyRet == WCS_RIO_NOTIFY_EXIT)
				{
					// Terminate the upload process if the caller requests
					err.code = wcs_Multipart_PutInterrupted;
					err.message = "Interrupted by the caller";
					return err;
				}
				continue;
			}
			LOG_ERROR ("ResumableBlockput: invalid checksum, retry, blkIdx = %d, crc32 = %lu, val = %lu", 
				blkIdx, ret->crc32, crc32.val);
			err = ErrUnmatchedChecksum;
		}
		else
		{
			if (err.code == wcs_Multipart_InvalidCtx)
			{
				wcs_Multipart_BlkputRet_Cleanup (ret);	// reset
				LOG_ERROR ("ResumableBlockput: invalid ctx, please retry");
				return err;
			}
			LOG_WARN ("ResumableBlockput %d off:%d failed - %E", blkIdx, (int) ret->offset, err);
		}
		if (tryTimes > 1 && wcs_TemporaryError (err.code))
		{
			tryTimes--;
			LOG_WARN ("ResumableBlockput %E, retrying ...", err);
			goto lzRetry;
		}
		break;
	}
	return err;
}

/*============================================================================*/

static wcs_Error wcs_Multipart_Mkfile2 (wcs_Client * c, wcs_Multipart_PutRet * ret, const char *key, wcs_Int64 fsize, wcs_Multipart_PutExtra * extra)
{
	size_t i, blkCount = 0; //extra->blockCnt;
	wcs_Json *root = NULL;
	wcs_Error err;
	const char *upHost = NULL;
	wcs_Multipart_BlkputRet *prog = NULL;
	wcs_Buffer url, body;
	wcs_Common_Header extraHeader;
	wcs_Multipart_PutRet respRet;
	wcs_Io_PutExtraParam *head = NULL;
	int bodyLen = 0;

	wcs_Buffer_Init (&url, 2048);
	wcs_Zero (extraHeader);
	wcs_Zero(respRet);
	wcs_Zero(err);
	memset(url.buf, 0, 2048);
	if ((NULL == c) || (NULL == extra))
	{
		err.code = 9899;
		err.message = "wcs_Multipart_Mkfile2 param is invalid";
		return err;
	}

	if ((upHost = extra->upHost) == NULL)
	{
		if (upHost == NULL)
		{
			err.code = 9988;
			err.message = "No proper upload host name";
			return err;
		}
	}
	
	blkCount = extra->blockCnt;
	wcs_Buffer_AppendFormat (&url, "%s/mkfile/%D", upHost, fsize);

	if (key != NULL)
	{
		// Allow using empty key
		wcs_Buffer_AppendFormat (&url, "/key/%S", key);
	}
	
	head = extra->params;
	while(head)
	{
	    wcs_Buffer_AppendFormat (&url, "/%s/%S", head->key,  head->value);
	    head = head->next;
	}

	wcs_Buffer_Init (&body, 176 * blkCount);
	
	for (i = 0; i < blkCount; i++)
	{
		prog = &extra->progresses[i];
		if (NULL != prog->ctx)
		{
			wcs_Buffer_Write (&body, prog->ctx, strlen (prog->ctx));
			wcs_Buffer_PutChar (&body, ',');
		}
	}
	if (blkCount > 0)
	{
		body.curr--;
	}
	if (extra->uploadBatch != NULL)
	{
		extraHeader.uploadBatch = extra->uploadBatch;
	}
	if (extra->key != NULL)
	{
		extraHeader.key = extra->key;
	}
	
	bodyLen =  body.curr - body.buf;
	if (bodyLen < 0)
	{
		bodyLen = 0;
	}
	err = wcs_Client_CallWithBuffer (c, &root, wcs_Buffer_CStr (&url), body.buf, bodyLen, "text/plain", &extraHeader);

	wcs_Buffer_Cleanup (&url);
	wcs_Buffer_Cleanup (&body);

	if (err.code == 200)
	{
		respRet.hash = wcs_Json_GetString (root, "hash", NULL);
		respRet.key = wcs_Json_GetString (root, "key", NULL);

		wcs_Multipart_putRetAssign(ret, &respRet);
		
	}

	if (NULL != root)
	{
    	    wcs_Json_Destroy(root);
	}
	
	return err;
}

/*============================================================================*/

int wcs_Multipart_BlockCount (wcs_Int64 fsize, wcs_Int64 blockSize)
{
	//return (int) ((fsize + blockMask) >> blockBits);
	if (0 == blockSize) return (int) ((fsize + blockMask) >> blockBits);
	
	// modify by liutl20180813
	// return (int) ((fsize /blockSize) + 1);
	int count = 0;
	//count = ceil((double)(fsize / blockSize));
	count = fsize / blockSize + ((fsize % blockSize) == 0 ? 0 : 1);
	return count;
}

static void wcs_Multipart_doTask (void *params)
{
	wcs_Error err;
	wcs_Multipart_BlkputRet ret;
	wcs_Multipart_task *task = (wcs_Multipart_task *) params;
	wcs_Multipart_WaitGroup wg = task->wg;
	wcs_Multipart_PutExtra *extra = task->extra;
	wcs_Multipart_ThreadModel tm = extra->threadModel;
	wcs_Client *c = tm.itbl->ClientTls (tm.self, task->mc);
	char ctx[32];
	CURL *curl = NULL;
	int blkIdx = task->blkIdx;
	int tryTimes = extra->tryTimes;

	wcs_Zero(ret);

	if ((*task->ninterrupts) > 0)
	{
		wcs_Mutex_Lock(&task->mc->ninterruptsMutex);
		wcs_Count_Inc (task->ninterrupts);
		wcs_Mutex_Unlock(&task->mc->ninterruptsMutex);
		//wg.itbl->Done (wg.self);
		
		wg.itbl->Done(task);
		LOG_WARN ("task->ninterrupts = %d", *task->ninterrupts);
		wcs_Buffer_Cleanup(&task->resp);
        	wcs_Buffer_Cleanup(&task->resph);
		free (task);
		task = NULL;
		return;
	}

	memset (&ret, 0, sizeof (ret));
	curl = task->multiCurl;
	if (wcs_True == task->mc->isPatchUpload)
	{
		if (wcs_True == wcs_isUploadedBlock(extra->patchInfo, &blkIdx))
		{
		        LOG_TRACE("wcs_isUploadedBlock: blkIdx = %d", blkIdx);
			wg.itbl->Done(task);
			wcs_Buffer_Cleanup(&task->resp);
        		wcs_Buffer_Cleanup(&task->resph);
			free (task);
			task = NULL;
			return;
		}
	}

	
	
  lzRetry:
	wcs_Multipart_BlkputRet_Assign (&ret, &extra->progresses[blkIdx]);
	ret.resp = &task->resp;
    	ret.resph = &task->resph;
	err = wcs_Multipart_ResumableBlockput (c, &ret, task->f, blkIdx, task->blkSize1, extra, curl);
	if (err.code != 200)
	{
		if (err.code == wcs_Multipart_PutInterrupted)
		{
			// Terminate the upload process if the caller requests
			wcs_Multipart_BlkputRet_Cleanup (&ret);
			wcs_Mutex_Lock(&task->mc->ninterruptsMutex);
			wcs_Count_Inc (task->ninterrupts);
			wcs_Mutex_Unlock(&task->mc->ninterruptsMutex);
			//wg.itbl->Done (wg.self);
			wg.itbl->Done(task);
			wcs_Buffer_Cleanup(&task->resp);
			wcs_Buffer_Cleanup(&task->resph);
			free (task);
			task = NULL;
			return;
		}

		if (tryTimes > 1 && wcs_TemporaryError (err.code))
		{
			tryTimes--;
			LOG_WARN ("multipart.Put %E, retrying ...", err);
			goto lzRetry;
		}
		LOG_WARN ("multipart.Put %d failed: %E", blkIdx, err);
		extra->notifyErr (extra->notifyRecvr, task->blkIdx, task->blkSize1, err);
		wcs_Mutex_Lock(&task->mc->nfailsMutex);
		(*task->nfails)++;
		wcs_Mutex_Unlock(&task->mc->nfailsMutex);
	}
	else
	{
		wcs_Multipart_BlkputRet_Assign (&extra->progresses[blkIdx], &ret);
	}
	wcs_Multipart_BlkputRet_Cleanup (&ret);
	//wg.itbl->Done (wg.self);
	if ((NULL != task->mc) && (wcs_True == task->mc->multiTask) &&
		(NULL != task->mc->patchInfoFile) && (200 == err.code))
	{
		wcs_Mutex_Lock(&task->mc->writeFileMutex);	
		char prefix[32];
		wcs_Zero(ctx);
		wcs_Zero(prefix);
		sprintf(prefix, "%s%s%d", "Block", config_array[get_index(*task->succNum)], *task->succNum);
		sprintf(ctx, "%s%s%d", "Ctx", config_array[get_index(task->blkIdx)], task->blkIdx);
		LOG_TRACE("succNum = %d, prefix=%s, Ctx=%s", *task->succNum, prefix, ctx);

		if (NULL != extra->progresses[blkIdx].ctx)
		{
			(*task->succNum)++;
			int tmp = 0;
			char tmpValueStr[256];
	               tmp = write_profile_int("SuccBlockInfo", "SuccNum", (int *)task->succNum, task->mc->patchInfoFile);
			LOG_TRACE("SuccBlockInfo return value : %d, task->succNum = %d", tmp, *task->succNum);
        	        tmp = write_profile_int("SuccBlockInfo", prefix, &task->blkIdx, task->mc->patchInfoFile);
			LOG_TRACE("SuccBlockInfo return value : %d, prefix = %s, task->blkIdx = %d", tmp, prefix, task->blkIdx);
			tmp = write_profile_string("SuccBlockInfo", ctx, extra->progresses[blkIdx].ctx, task->mc->patchInfoFile);
			LOG_TRACE("SuccBlockInfo return value : %d,CTX = %s, ctx = %s, ", tmp, ctx, extra->progresses[blkIdx].ctx);
			memset(tmpValueStr, 0, sizeof(tmpValueStr));
		}
		wcs_Mutex_Unlock(&task->mc->writeFileMutex);
	}
	wg.itbl->Done(task);
	wcs_Buffer_Cleanup(&task->resp);
    	wcs_Buffer_Cleanup(&task->resph);
	free (task);
	task = NULL;
}

/*============================================================================*/
/* func wcs_Multipart_PutXXX */

static wcs_Error ErrPutFailed = {
	wcs_Multipart_PutFailed, "multipart put failed"
};

static wcs_Error ErrPutInterrupted = {
	wcs_Multipart_PutInterrupted, "multipart put interrupted"
};

wcs_Error wcs_Multipart_UploadCheck(const char *configFile, wcs_Client * self, wcs_Multipart_PutRet* ret, wcs_Multipart_PutExtra* extra)
{
	wcs_Error err;
	char *bucket = NULL;
	char *key = NULL;
	char *uptoken = NULL;
	const char *localFile = NULL;
	const char *upHost = NULL;
	char *patchConfigFile = NULL;
	int *blocks = NULL;
	unsigned int i = 0;
	wcs_PatchInfo uploadPatchInfo;
	wcs_Io_PutExtraParam *head = NULL;
	wcs_Io_PutExtraParam *node = NULL;

	if ((NULL == self))
	{
		err.code = 9899;
		err.message = "wcs_Multipart_UploadCheck params  is invalid";
		return err;
	}

	wcs_Zero(uploadPatchInfo);

	if (NULL != configFile)
	{
		patchConfigFile = (char *)configFile;
	}
	else
	{
		patchConfigFile = Default_ConfigFile;
	}
	
	if (!is_file_exist(patchConfigFile))
	{
		err.code = 9899;
		err.message = "config is not exist";
		return err;
	}
	uploadPatchInfo.succNum = getSuccNum(patchConfigFile);
	LOG_TRACE("uploadPatchInfo.succNum = %d", uploadPatchInfo.succNum);
	uploadPatchInfo.varCount = getVarCount(patchConfigFile);
	self->patchInfoFile = patchConfigFile;
	bucket = malloc(sizeof(char) * BUCKET_LEN);
	key = malloc(sizeof(char) * KEY_LEN);
	blocks = (int *)malloc(sizeof(int) * uploadPatchInfo.succNum);
	localFile = malloc(sizeof(char) * FILENAME_LEN);
	upHost = malloc(sizeof(char) * FILENAME_LEN);
	uploadPatchInfo.uploadBatchId = (unsigned char *)malloc(UPLOADBATCHID_LEN +1); //其它地方释放
       uptoken = (char *)malloc(TOKEN_LEN);

       for( i = 0; i < uploadPatchInfo.varCount; i++)
       {
            node = (wcs_Io_PutExtraParam *)malloc(sizeof(wcs_Io_PutExtraParam));
            memset((void *)node, 0, sizeof(wcs_Io_PutExtraParam));
            if (NULL == head)
            {
                head = node;
                uploadPatchInfo.param = head;
            }
            else
            {
                head->next = node;
                head = node;
            }
       }
       
	memset(key, 0, KEY_LEN);
	memset(bucket, 0, BUCKET_LEN);
	memset(blocks, 0, sizeof(int) * uploadPatchInfo.succNum);
	memset((void *)localFile, 0, FILENAME_LEN);
	memset((void *)upHost, 0, FILENAME_LEN);
	memset((void *)uploadPatchInfo.uploadBatchId , 0, UPLOADBATCHID_LEN+1);
	memset((void *)uptoken, 0, TOKEN_LEN);
	uploadPatchInfo.blocks = &blocks;
	uploadPatchInfo.bucket = bucket;
	uploadPatchInfo.key = key;
	uploadPatchInfo.uploadFile = (char *)localFile;
	uploadPatchInfo.upHost = (char *)upHost;
	self->isPatchUpload = wcs_True;
	if (getPatchInfo(patchConfigFile, &uploadPatchInfo) < 0)
	{
		err.code = 9899;
		err.message = "The config file content is invalid";
		return err;
	}

	getXVarList(uploadPatchInfo.param, patchConfigFile);
	
	if (!read_profile_string("PatchUploadInfo","UpToken", uptoken ,TOKEN_LEN, "",patchConfigFile))
	{
	    err.code = 9899;
	    err.message = "Get upToken error";
	    return err;
	}
	extra->patchInfo = &uploadPatchInfo;
	extra->localFileName = localFile;
	extra->upHost = upHost;
	extra->blockSize = uploadPatchInfo.blockSize;
	extra->chunkSize = uploadPatchInfo.chunkSize;
	//extra->uploadBatch = uploadPatchInfo.uploadBatchId;
	extra->params = uploadPatchInfo.param;
			
	err = wcs_Multipart_PutFile (self, ret, uptoken, key, localFile, extra);
	free(bucket);
	free(key);
	free(blocks);
	free((char *)localFile);
	free((char *)upHost);
	upHost = NULL;
	bucket = NULL;
	key = NULL;
	blocks = NULL;
	extra->localFileName = NULL;
	extra->patchInfo = NULL;
	if (NULL != uptoken)
	{
		free((void *)uptoken);
		uptoken = NULL;
	}
	head = uploadPatchInfo.param;
	while(head)
	{
	    node = head;
	    head = head->next;
		//wcs_Free(node->key);
	    //wcs_Free(node->value);
	    wcs_Free(node);
	}
	
	return err;
}
void wcs_Load_CtxFromFile(wcs_Multipart_PutExtra *extra, char *patchConfigFile)
{
    unsigned int i = 0;
    char ctx[32];
    char ctxValue[KEY_LEN];
    wcs_Multipart_BlkputRet *prog = NULL;
    unsigned int blkCount = 0;
    
    if ((NULL == extra) || (NULL == patchConfigFile))
    {
        return;
    }

    blkCount = extra->blockCnt;
    LOG_TRACE("wcs_Load_CtxFromFile Enter blkCount = %d", blkCount);
    for(i = 0; i < blkCount; i++)
    {
        wcs_Zero(ctx);
        sprintf(ctx, "Ctx%s%d", config_array[get_index(i)], i);
        prog = &extra->progresses[i];

        if ((NULL != prog) && (NULL == prog->ctx))
        {
           memset(ctxValue, 0, KEY_LEN);
           if (!read_profile_string("SuccBlockInfo", ctx, ctxValue , KEY_LEN, "", patchConfigFile))
           {
                LOG_TRACE("SuccBlockInfo, %s failed in wcs_Load_CtxFromFile", ctx);
           }
           else
           {
               prog->ctx = (char *)malloc(strlen(ctxValue) + 1);
               strcpy((char *)prog->ctx, ctxValue);
               LOG_TRACE("wcs_Load_CtxFromFile %s = %s", ctx, ctxValue);
           }
        }
        
    }
    LOG_TRACE("wcs_Load_CtxFromFile End");
}

void wcs_SaveCommon4Patch(wcs_Client * self, wcs_Multipart_PutExtra *extra)
{
    wcs_Io_PutExtraParam *head = NULL;
    int count = 0;
    char *decode = NULL;
    char varListkey[BUCKET_LEN];
    char varListValue[BUCKET_LEN];
    wcs_Mutex_Lock(&self->writeFileMutex);
    //check wether the patchInfoFile exist
    if (NULL != self->patchInfoFile)
    {
    	remove(self->patchInfoFile);
    }
    if (NULL != extra->localFileName)
    {
    	write_profile_string("PatchUploadInfo" ,"FileName", extra->localFileName, self->patchInfoFile);
    }
    
    if (NULL != extra->key)
    {
    	write_profile_string("PatchUploadInfo" ,"Key", extra->key, self->patchInfoFile);
    }
    if (NULL != extra->uploadBatch)
    {
    	write_profile_string("PatchUploadInfo" ,"UploadBatchId", (const char*)extra->uploadBatch, self->patchInfoFile);
    }
    if (NULL != extra->upHost)
    {
    	write_profile_string("PatchUploadInfo" ,"UpHost", extra->upHost, self->patchInfoFile);
    }
    if (NULL != extra->uptoken)
    {
        write_profile_string("PatchUploadInfo" ,"UpToken", extra->uptoken, self->patchInfoFile);
    }
    write_profile_int("PatchUploadInfo" ,"BlockSize", &extra->blockSize, self->patchInfoFile);
    write_profile_int("PatchUploadInfo" ,"ChunkSize", &extra->chunkSize, self->patchInfoFile);

    //Save UserParam
    head = extra->params;
    while(head)
    {
        memset(varListkey, 0, sizeof(varListkey));
        memset(varListValue, 0, sizeof(varListValue));
        sprintf(varListkey, "%d%s", count, "xVarListKey");
        sprintf(varListValue, "%d%s", count, "xVarListValue");
        if (!write_profile_string("PatchUploadInfo", varListkey, head->key, self->patchInfoFile))
        {
            LOG_TRACE("PatchUploadInfo: %s, %s", varListkey, head->key);
        }
        if (!write_profile_string("PatchUploadInfo", varListValue, head->value, self->patchInfoFile))
        {
            LOG_TRACE("PatchUploadInfo: %s, %s", varListValue, head->value);
        }
        wcs_Free(decode);
        count++;
        head = head->next;
    }
    if (!write_profile_int("PatchUploadInfo", "VarsCount", &count, self->patchInfoFile))
    {
        LOG_TRACE("PatchUploadInfo: VarsCount, %d", count);
    }
    //Save count to PatchUploadInfo, VarListCount
    wcs_Mutex_Unlock(&self->writeFileMutex);
}
wcs_Error wcs_Multipart_Put (wcs_Client * self, wcs_Multipart_PutRet * ret, const char *uptoken, const char *key, 
	wcs_ReaderAt f, wcs_Int64 fsize, wcs_Multipart_PutExtra * extra1)
{
	wcs_Int64 offbase = 0;
	wcs_Multipart_task *task = NULL;
	wcs_Multipart_WaitGroup wg;
	wcs_Multipart_PutExtra extra;
	wcs_Multipart_ThreadModel tm;
	wcs_Auth auth, auth1 = self->auth;
	int i, last, blkSize;
	int nfails = 0;
	int retCode = 0;
	unsigned int succNum = 0;
	wcs_Count ninterrupts;
	threadpool_t *threadPool = NULL;
	wcs_multiThread_info threadInfo;
	wcs_Error err = {200, "OK"};
	char *tmpKey = NULL;

	
	wcs_Zero(extra);
	wcs_Zero(wg);
	wcs_Zero(tm);
	wcs_Zero(threadInfo);
	i = last = blkSize = 0;
	err = wcs_Multipart_PutExtra_Init (&extra, fsize, extra1);
	if (err.code != 200)
	{
		return err;
	}

	if (NULL != ret)
	{
		memset(ret, 0, sizeof(wcs_Multipart_PutRet));
	}

	wcs_Mutex_Init(&self->nfailsMutex);
	wcs_Mutex_Init(&self->ninterruptsMutex);
	wcs_Mutex_Init(&self->writeFileMutex);
	self->multiTask = wcs_True;
	tm = extra.threadModel;
	wg = tm.itbl->WaitGroup (tm.self);

	last = extra.blockCnt - 1;
	if (wcs_True != self->isPatchUpload)
	{
		blkSize = extra.blockSize;
	}
	else
	{
		if (NULL != extra1->patchInfo)
		{
			blkSize = extra1->patchInfo->blockSize;
			extra.chunkSize = extra1->patchInfo->chunkSize;
			extra.bucket = extra1->patchInfo->bucket;
			extra.key = extra1->patchInfo->key;
			if (NULL != extra.uploadBatch)
			{
				wcs_Free(extra.uploadBatch);
			}
			extra.uploadBatch = extra1->patchInfo->uploadBatchId;
		}
	}
	extra.uptoken = (char*)uptoken;

	nfails = 0;
	ninterrupts = 0;

	self->auth = auth = wcs_UptokenAuth (uptoken);
	wcs_multiThread_info_init(&threadInfo);
	self->threadInfo = &threadInfo;
	if (wcs_True == self->multiTask)
	{
		threadPool = threadpool_create(wcs_Multipart_MaxThreadNum, extra.blockCnt, 0);
		if (NULL == threadPool)
		{
			err.code = 9899;
			err.message = "threadpool_create faile";
			return err;
		}
		self->threadPool = threadPool;
		threadInfo.totalBlkNum = extra.blockCnt;
		if (NULL == self->patchInfoFile)
		{
			self->patchInfoFile = Default_ConfigFile;
		}
	}
	
	/* Write chunkSize, blockSize , uploadBatchto file*/
	if ((wcs_True == self->multiTask) && (wcs_False == self->isPatchUpload))
	{
		wcs_SaveCommon4Patch(self, &extra);
	}
	
	if (NULL != extra.key)
       {
            tmpKey = wcs_String_Encode(extra.key);
            extra.key = tmpKey;
       }
	
	if (wcs_True == self->isPatchUpload)
	{
		succNum = extra.patchInfo->succNum;
		LOG_TRACE("PatchUpload SuccNum = %d", succNum);
	}
        //check whether the blkSize is times of 4M
        if ((blkSize %(4 * 1024 * 1024) != 0) || (blkSize <= 0))
        {
            err.code = 9899;
            err.message = "The blkSize is not the times of 4M or blkSize is zero";
            LOG_ERROR("The blkSize is not the times of 4M, blkSize = %s", blkSize);
            return err;
        }
	
	for (i = 0; i < (int) extra.blockCnt; i++)
	{
		task = (wcs_Multipart_task *) malloc (sizeof (wcs_Multipart_task));
		task->f = f;
		task->extra = &extra;
		task->mc = self;
		task->wg = wg;
		task->nfails = &nfails;
		task->ninterrupts = &ninterrupts;
		task->succNum = &succNum;
		task->blkIdx = i;
		task->blkSize1 = blkSize;
		task->threadPool = threadPool;
		task->multiCurl = curl_easy_init();
		wcs_Buffer_Init(&task->resp, 1024);
		wcs_Buffer_Init(&task->resph, 1024);
		if (i == last)
		{
			//offbase = (wcs_Int64) (i) << blockBits;
			offbase = (wcs_Int64) (i) * blkSize;
			task->blkSize1 = (int) (fsize - offbase);
			//wcs_Log_Warn("The last blkSize = %d", task->blkSize1);
		}

		wg.itbl->Add (wg.self, 1);
		retCode = tm.itbl->RunTask (self, wcs_Multipart_doTask, task);
		if (wcs_False == task->mc->multiTask)
		{
			if (retCode == WCS_RIO_NOTIFY_EXIT)
			{
				//wg.itbl->Done (wg.self);
				wg.itbl->Done(task->mc);
				wcs_Count_Inc (&ninterrupts);
				wcs_Buffer_Cleanup(&task->resp);
        		wcs_Buffer_Cleanup(&task->resph);
				free (task);
				task = NULL;
			}

			if (ninterrupts > 0)
			{
				break;
			}
		}
	}
	
	//wg.itbl->Wait (wg.self);
	wg.itbl->Wait (self);

	if (nfails != 0)
	{
		err = ErrPutFailed;
	}
	else if (ninterrupts != 0)
	{
		err = ErrPutInterrupted;
	}
	else
	{
	        if (wcs_True == self->isPatchUpload)
	        {
	            wcs_Load_CtxFromFile(&extra,  self->patchInfoFile);
	        }
		err = wcs_Multipart_Mkfile2 (self, ret, key, fsize, &extra);
		if ((200 == err.code) )
		{
			//delete file
			if (remove(self->patchInfoFile))
			{
				LOG_WARN("remove file fail");
			}
		}
	}

	wcs_Multipart_PutExtra_Cleanup (&extra);

	wg.itbl->Release (wg.self);
	auth.itbl->Release (auth.self);
	self->auth = auth1;

	return err;
}
void wcs_Io_PutExtra_Cleanup(wcs_Io_PutExtra *extra)
{
	wcs_Io_PutExtraParam *head = NULL;
	wcs_Io_PutExtraParam *node = NULL;
	if (NULL == extra)
	{
		return ;
	}
	head = extra->params;
	while(head)
	{
		wcs_Free(head->key);
		wcs_Free(head->value);
		node = head;
		head = head->next;
		wcs_Free(node);
	}
}


wcs_Error wcs_Multipart_PutFile (wcs_Client * self, wcs_Multipart_PutRet * ret, const char *uptoken
	, const char *key, const char *localFile, wcs_Multipart_PutExtra * extra)
{
	wcs_Io_PutExtra extra1;
	wcs_Int64 fsize;
	wcs_FileInfo fi;
	wcs_File *f;
	wcs_Error err;
	cJSON *normalRet = NULL;

	if ((NULL == self) || (NULL == uptoken) || (NULL == localFile) ||
		(NULL == extra))
	{
		err.code = 9899;
		err.message = "wcs_Multipart_PutFile param is invalid";
		return err;
	}

	if ( ((extra->blockSize % defaultBlockSize) != 0) || (extra->chunkSize > extra->blockSize))
	{
		err.code = 9898;
		err.message = "wcs_Multipart_PutFile blockSize must be  4M's times";
		return err;
	}
	
	err = wcs_File_Open (&f, localFile);
	if (err.code != 200)
	{
		return err;
	}
	err = wcs_File_Stat (f, &fi);
	if (err.code == 200)
	{
		fsize = wcs_FileInfo_Fsize (fi);
		if (fsize <= wcs_Multipart_PutExtra_ChunkSize (extra))
		{						// file is too small, don't need multipart-io
			wcs_File_Close (f);

			wcs_Zero (extra1);
			wcs_Io_PutExtra_initFrom (&extra1, extra);

			err = wcs_Io_PutFile (self, &normalRet, uptoken, key, localFile, &extra1);
			wcs_Io_PutExtra_Cleanup(&extra1);
			return err;
		}
		if (NULL == extra->localFileName)
		{
			extra->localFileName = localFile;
		}
		err = wcs_Multipart_Put (self, ret, uptoken, key, wcs_FileReaderAt (f), fsize, extra);
	}
	wcs_File_Close (f);
	return err;
}
