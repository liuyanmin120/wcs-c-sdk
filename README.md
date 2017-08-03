# wcs-c-sdk
C SDK for wcs


### 概要:

本SDK使用符合C89标准的C语言实现。
-  SDK主要包含以下几方面内容：
	1. 公共部分：src/wcs/base.c, src/wcs/conf.c, src/wcs/http.c, src/base/log.c
	2. 客户端原子上传小文件：src/wcs/base_io.c io.c
	3. 客户端分片上传断点续传： src/base/threadpoo.c, src/base/inifile.c, 				   src/base/patchfileops.c, src/wcs/ multipart_io.c
	4. 数据处理（视频转码，截图等）: src/wcs/fop.c


### 安装：
	C-SDK使用了cURL, OpenSSL ,使用时需要自行安装前述依赖库。
	GCC编译选项：-lcurl -lssl -lcrypto -lm 
	如果项目编译时出现连接等错误，需要先检查依赖库和编译选项是否正确。

### WCS存储相关信息：
调用接口需要用到ACCESS_KEY ，SECRET_KEY ，上传域名和管理域名，这些可通过网宿官方获取。

### LOG模块: 
    当出现问题需要分析时可以打开Log, 分为6个等级,可以选择是否写入文件(为了性能,平常不建议配置写入文件),
	配置文件(LogConfig.ini): [SDKLogConfig]
				LOG_LEVEL=5 //0 打印的最多，建议平常设置为5
				WRITE_FILE=0 //0 不写入文件，1 写入文件
				LOG_FILE=./SDKTest.log //写入log的文件名 
	注意：如果没有LogConfig.ini, 默认是LOG_LEVEL=5且不写入文件
 	void wcs_Log_Init(char *logConfigFile, FILE *file); // logConfigFile 为上述配置文件的路径和名字，file只传入定义即可，不要打开文件，主要用与文件关闭
	void wcs_close_Logfile(FILE *file);

### 接口调用
    初始化和反初始化：
	wcs_Global_Init (0);
	wcs_MacAuth_Init ();
	wcs_Client_InitNoAuth (&client, 8192);
	。。。。。
	wcs_Client_Cleanup (&client);	
	wcs_Global_Cleanup ();
	接口在省略号部分调用，该初始话要在主线程中调用一次，最好不要在多线程中多次调用，否则会出现不可预料的错误，该问题是由cURL中的一些接口不是线程安全的引起。
	
### 详细实例参考：c_sdk/demo/test.c



### API 概述(注：各接口传入的ret参数需要初始化)：

接口功能 | 接口函数名称 | 备注
---|---|---
生成上传凭证   |	char *wcs_RS_PutPolicy_Token (wcs_RS_PutPolicy * auth, wcs_Mac * mac)    |
普通文件上传   |	wcs_Error wcs_Io_PutFile (wcs_Client * self, wcs_Io_PutRet * ret, const char *uptoken, const char *key, const char *localFile, wcs_Io_PutExtra * extra)|
分片上传   |	wcs_Error wcs_Multipart_PutFile (wcs_Client * self, wcs_Multipart_PutRet * ret, const char *uptoken	, const char *key, const char *localFile, wcs_Multipart_PutExtra * extra) | 注：该接口通过多线程分片上传，可以更改 最大线程数：wcs_Multipart_MaxThreadNum|
断点续传 |	wcs_Error wcs_Multipart_UploadCheck(const char *configFile, wcs_Client * self, 	wcs_Multipart_PutRet * ret)|
删除文件 |	wcs_Error wcs_RS_Delete (wcs_Client * self, const char *tableName, const char *key, const char *mgrHost)|
获取文件信息 |	wcs_Error wcs_RS_Stat (wcs_Client * self, wcs_RS_StatRet * ret, const char *tableName, const char *key, const char *mgrHost)|
列取资源 |	wcs_Error wcs_RS_List (wcs_Client * self, wcs_RS_ListRet * ret, const char *bucketName, wcs_Common_Param * param, const char *mgrHost)|
移动资源	| wcs_Error wcs_RS_Move (wcs_Client * self, const char *tableNameSrc, const char *keySrc, const char *tableNameDest, const char *keyDest, const char *mgrHost)|
更新镜像资源   |	wcs_Error wcs_RS_UpdateMirror(wcs_Client * self, wcs_RS_StatRet * ret, const char *bucketName, const char **fileNameList, unsigned int fileNum, const char *mgrHost)|
音视频操作   |	wcs_Error wcs_Fops_Media (wcs_Client * self, wcs_FOPS_Response * ret, wcs_FOPS_MediaParam *param, const char *mgrHost)|
抓取资源   |	wcs_Error wcs_Fops_Fetch(wcs_Client * self, wcs_FOPS_Response * ret,  wcs_FOPS_FetchParam *ops[], unsigned int opsNum, const char *mgrHost )|
复制资源   |	wcs_Error wcs_RS_Copy (wcs_Client * self, const char *tableNameSrc, const char *keySrc, const char *tableNameDest, const char *keyDest, const char *mgrHost)|
Base64编码    |	wcs_Error wcs_Encode_Base64(int argc, char **argv)|



