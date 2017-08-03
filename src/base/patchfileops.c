/**
* @file
* @brief 
* @author 
* @date 
* @version 0.2
*/
#include <stdio.h>
#include <malloc.h>
#include "inifile.h"
#include "../wcs/multipart_io.h"

#define BUF_SIZE 256
#define BATCHID_LEN 33

int getSuccBlocks(int **blocks, const char *file, const unsigned int blocksNum)
{
	if ((NULL == blocks) || (NULL == *blocks) || (NULL == file))
	{
		return -1;
	}

	char *prefix = "Block";
	char blockIdx[20];
	char *section = "SuccBlockInfo";
	int *result = *blocks;
	
	unsigned int i = 0;

	for (i = 0; i < blocksNum; i++)
	{
		memset(blockIdx, 0 ,sizeof(blockIdx));
		sprintf(blockIdx, "%s%s%d", prefix, config_array[get_index(i)], i);
		result[i] = read_profile_int(section, blockIdx, 0, file);
		LOG_TRACE("getSuccBlocks: %s=%d\n", blockIdx, result[i]);
	}	
	return 0;
}

int getSuccNum(const char *file)
{
	int succNum = 0;
	if (NULL == file)
	{
		return 0;
	}
	succNum = read_profile_int("SuccBlockInfo", "SuccNum", 0, file);
	return succNum;
}

int getVarCount(const char *file)
{
    unsigned int varCount = 0;
    if (NULL == file)
    {
    	return 0;
    }
    varCount = read_profile_int("PatchUploadInfo", "VarsCount", 0, file);
    return varCount;
}

void getXVarList(wcs_Io_PutExtraParam *param, const char *file)
{
    unsigned int i = 0;
    wcs_Io_PutExtraParam *head = NULL;
    char key[BUCKET_LEN];
    char readValue[FILENAME_LEN];
    head = param;
    while(head)
    {
        memset(readValue, 0, sizeof(readValue));
        memset(key, 0, sizeof(key));
        sprintf(key, "%d%s", i, "xVarListKey");
        if (!read_profile_string("PatchUploadInfo", key, readValue,FILENAME_LEN, "",file))
        {
            LOG_TRACE("PatchUploadInfo: %s failed", readValue);
        }
        head->key = (char *)malloc(strlen(readValue) + 1);
        strcpy(head->key, (const char *)readValue);
        memset(readValue, 0, sizeof(readValue));
        memset(key, 0, sizeof(key));
        sprintf(key, "%d%s", i, "xVarListValue");
        if (!read_profile_string("PatchUploadInfo",key, readValue, FILENAME_LEN, "",file))
        {
            LOG_TRACE("PatchUploadInfo: %s failed", readValue);
        }
        head->value = (char *)malloc(strlen(readValue) + 1);
        strcpy(head->value, (const char *)(const char *)(const char *)(const char *)(const char *)(const char *)(const char *)(const char *)(const char *)readValue);
        head = head->next;
        i++;
    }
}

int getUploadBatchInfo(const char *file, wcs_PatchInfo *patchInfo)
{
	const char *section = "PatchUploadInfo";
    const char *uploadBatchIdKey="UploadBatchId";
	char uploadBatchID[BATCHID_LEN + 1];

	memset(uploadBatchID, 0, BATCHID_LEN + 1);
	
	if ((NULL == file) || (NULL == patchInfo))
	{
		return -1;
	}
	
	if(!read_profile_string(section, uploadBatchIdKey, uploadBatchID, BATCHID_LEN,"",file))
    	{
	        LOG_ERROR("read ini file fail\n");
	        return -1;
   	 }
	strncpy((char *)patchInfo->uploadBatchId, &uploadBatchID[0], strlen(uploadBatchID));
	patchInfo->blockSize = read_profile_int(section, "BlockSize", 0, file);
	patchInfo->chunkSize = read_profile_int(section, "ChunkSize", 0, file);
	return 0;
}

int getPatchInfo(const char *file, wcs_PatchInfo *patchInfo)
{
	if ((NULL == file) || (NULL == patchInfo) || (NULL == patchInfo->uploadFile)
		|| (NULL == patchInfo->bucket))
	{
		return -1;
	}
	char fileName[FILENAME_LEN + 1];
	int *blocks = NULL;
	char *section = "PatchUploadInfo";
	memset(fileName, 0 , sizeof(char) * (FILENAME_LEN + 1));
	
    patchInfo->succNum = getSuccNum(file);
    LOG_TRACE("getPatchInfo, patchInfo->succNum = %d", patchInfo->succNum);
    if ((NULL == patchInfo->blocks) || (NULL == *patchInfo->blocks))
    {
		blocks = (int *)malloc(sizeof(int) * patchInfo->succNum);
		patchInfo->blocks = &blocks;
	}
	else
	{
		blocks = (*patchInfo->blocks);
	}
	getSuccBlocks(&blocks, file, patchInfo->succNum);
	//getUploadBatchInfo(file, patchInfo);
	if(!read_profile_string(section, "UploadBatchId", fileName, BATCHID_LEN,"",file))
    {
		printf("read ini file UploadBatchId fail\n");
    	return -1;
    }
	strncpy((char *)patchInfo->uploadBatchId, &fileName[0], strlen(fileName));
	memset(fileName, 0 ,sizeof(char) * (FILENAME_LEN + 1));
    	patchInfo->blockSize = read_profile_int(section, "BlockSize", 0, file);
    	patchInfo->chunkSize = read_profile_int(section, "ChunkSize", 0, file);
	if(!read_profile_string(section, "FileName", fileName, FILENAME_LEN, "", file))
   	{
        printf("read ini file FileName fail\n");
        return -1;
	}
	strncpy(patchInfo->uploadFile, &fileName[0], strlen(fileName));
	memset(fileName, 0 ,sizeof(char) * (FILENAME_LEN + 1));
   	
	strncpy(patchInfo->bucket, &fileName[0], strlen(fileName));
	memset(fileName, 0 ,sizeof(char) * (FILENAME_LEN + 1));
   	if (NULL != patchInfo->key)
   	{
   		read_profile_string(section, "Key", fileName, BUCKET_LEN, "" ,file);
   	}
	strncpy(patchInfo->key, &fileName[0], strlen(fileName));

	memset(fileName, 0 ,sizeof(char) * (FILENAME_LEN + 1));
   	if (NULL != patchInfo->upHost)
   	{
   		read_profile_string(section, "UpHost", fileName, BUCKET_LEN, "" ,file);
   	}
	strncpy(patchInfo->upHost, &fileName[0], strlen(fileName));
	return 0;
}

#if 0
int main()
{
	const char *file ="config/PatchUpload.ini";
	int *blocks = NULL;
	int i = 0;
	wcs_PatchInfo patchInfo;
	getPatchInfo(file, &patchInfo);
	blocks = *(patchInfo.blocks);
	printf("SuccName=%d\n",patchInfo.succNum); 
	printf("UploadBatchId=%s\n", patchInfo.uploadBatchId);
	printf("BlockSize=%d, ChunkSize=%d\n", patchInfo.blockSize, patchInfo.chunkSize);
	for (i = 0; i < patchInfo.succNum; i++)
	{
		printf("Block%d=%d\n", i, blocks[i]);
	}
	if (NULL != blocks)
	{
		free(blocks);
	}

	return 0;
}
#endif

