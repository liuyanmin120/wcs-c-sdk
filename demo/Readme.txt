测试说明（以下参数没有指明类型的全部为字符串,对于字符串参数，如果不设置，需要传递 NULL， 测试只关注/* */之间的参数即可。函数名供产品写文档使用）：
参数设置： 指定测试接口 指定接口需要的参数
测试第一个参数为需要测试的接口，对应关系如下：
1 生成上传Token
2 上传小文件
3 分片上传（文件大小需大于4MB,）
4 断点续传
5 删除文件
6 查询文件信息
7 列取资源
8 移动资源
9 更新镜像
10 视频操作
11 抓取资源
12 复制资源
13 Base64编码

wcs_Init_Log()


接口参数说明：

1. 生成Token
char *wcs_RS_PutPolicy_Token (wcs_RS_PutPolicy * auth, wcs_Mac * mac)；
/*
*	@param 
		AK:
		SK:
		const char *scope;
		const char *saveKey;
		const char *returnUrl;
		const char *returnBody;
		const char *callbackUrl;
		const char *callbackBody;(带魔法变量时需要转义$,请参考下面例子)
		const char *persistentOps;
		const char *contentDetect;
		const char *persistentNotifyUrl;
		const char *detectNotifyURL
		const char *detectNotifyRule;
		wcs_Bool overwrite;
		wcs_Bool separate;
		wcs_Uint64 fsizeLimit;
		wcs_Uint32 expires;
	例如： ./wcsTest 1 yourAk yourSk yourBucketname NULL NULL "TestParam=\$(x:MyTest)&Parma1=\$(x:MyParam)&Param2=\$(x:MyParam2)" NULL  NULL NULL NULL NULL NULL  NULL 1 1	
*/

2. 上传文件
wcs_Error wcs_Io_PutFile (wcs_Client * self, cJSON ** ret, const char *uptoken, const char *key, const char *localFile, wcs_Io_PutExtra * extra)；
/*
*	@param uptoken key localFile upHost numXParam key value
	例如：（numXParam = 3则后面跟3组 key value）
	2 youruploadtoken fop.c /home/haley/fop.c http://test.up0.v1.wcsapi.com/file/upload 3 x:MyTest MyTestValue x:MyParam ParamValue x:MyParam2 Param2Value
*/

3. 分片上传(blockSize, chunkSize 都是Byte)
wcs_Error wcs_Multipart_PutFile (wcs_Client * self, wcs_Multipart_PutRet * ret, const char *uptoken
	, const char *key, const char *localFile, wcs_Multipart_PutExtra * extra)；
 /*
 *	 @param uptoken key localFile upHost blockSize chunkSize
 */
例如： ./wcsTest 3 youruploadtoken 文00.tar.gz  /root/api_test/test_tool/csdk/c-sdk-refactor-build.tar.gz http://test.up0.v1.wcsapi.com NULL NULL 1 x:MyTest MyTestValue


4. 断点续传
wcs_Error wcs_Multipart_UploadCheck(const char *configFile, wcs_Client * self, 
	wcs_Multipart_PutRet * ret)
/*
	@param 
*/

5. 删除文件
wcs_Error wcs_RS_Delete (wcs_Client * self, const char *tableName, const char *key, const char *mgrHost)
/*
*	@param AK SK bucketName keyName mgrHost
*/

6. 获取文件状态
wcs_Error wcs_RS_Stat (wcs_Client * self, cJSON ** ret, const char *tableName, const char *key, const char *mgrHost)
/*
*	@param accessKey secretKey bucketName keyName mgrHost
*/

7. 列取资源
wcs_Error wcs_RS_List (wcs_Client * self, cJSON ** ret, const char *bucketName, wcs_Common_Param * param, const char *mgrHost)
/*
	@param accessKey secretKey bucketName mgrHost prefix limit mode marker 
*/

8. 
wcs_Error wcs_RS_Move (wcs_Client * self, const char *tableNameSrc, const char *keySrc, 
	const char *tableNameDest, const char *keyDest, const char *mgrHost)；
/*
@param accessKey secretKey srcBucketName srcKey destBucketName destKey mgrHost
*/

9. 更新镜像
wcs_Error wcs_RS_UpdateMirror(wcs_Client * self, cJSON ** ret, const char *bucketName, 
const char **fileNameList, unsigned int fileNum, const char *mgrHost)；
/*
*	@param accessKey secretKey bucketName mgrHost fileNum fileName1 fileName2 ....fileNameN
	the N is equal fileNum
*/

10. 视频处理
wcs_Error wcs_Fops_Media (wcs_Client * self, wcs_FOPS_Response * ret, 
	wcs_FOPS_MediaParam *param, const char *apiHost)
/*
*	@param
	AK SK mgrHost
	char *bucket;
	char *key;	
	//fops的拼接和base64编码由客户自己完成，然后通过字符串输入
	char *fops;

	char *notifyURL;
	int force;
	wcs_Bool separate;
*/

11. 抓取资源
wcs_Error wcs_Fops_Fetch(wcs_Client * self, wcs_FOPS_Response * ret, 
	wcs_FOPS_FetchParam *ops,const char *apiHost)
/*
*	@parm
	AK SK apiHost
	char *ops;  //拼接和编码都由客户自己完成

	//公共部分
	char *notifyURL;
	int force;
	wcs_Bool separate;
*/

12. 复制资源
wcs_Error wcs_RS_Copy (wcs_Client * self, const char *tableNameSrc, const char *keySrc, 
	const char *tableNameDest, const char *keyDest, const char *mgrHost)
/*
*	@param accessKey secretKey srcBucketName srcKey destBucketName destKey mgrHost
*/

13. base64 编码
char *wcs_String_Encode (const char *buf)
/*
	string
*/


各接口返回值代表意义请参考：http://www.cnblogs.com/wainiwann/p/3492939.html

