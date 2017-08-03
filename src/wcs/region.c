/*
 ============================================================================
 Name        : region.c
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description :
 ============================================================================
 */

#include <ctype.h>
#include <curl/curl.h>

#include "conf.h"
#include "tm.h"
#include "region.h"

#ifdef __cplusplus
extern "C"
{
#endif

	static wcs_Uint32 wcs_Rgn_enabling = 0;

	WCS_DLLAPI extern void wcs_Rgn_Enable (void)
	{
		wcs_Rgn_enabling = 1;
	}							// wcs_Rgn_Enable

	WCS_DLLAPI extern void wcs_Rgn_Disable (void)
	{
		wcs_Rgn_enabling = 0;
	}							// wcs_Rgn_Disable

	WCS_DLLAPI extern wcs_Uint32 wcs_Rgn_IsEnabled (void)
	{
		return (wcs_Rgn_enabling != 0);
	}							// wcs_Rgn_IsEnabled

	static inline int wcs_Rgn_Info_isValidHost (const char *str)
	{
		return (strstr (str, "http") == str) || (strstr (str, "HTTP") == str);
	}							// wcs_Rgn_Info_isValidHost

	static void wcs_Rgn_Info_measureHosts (wcs_Json * root, wcs_Uint32 * bufLen, wcs_Uint32 * upHostCount, wcs_Uint32 * ioHostCount)
	{
		int i = 0;
		wcs_Json *arr = NULL;
		const char *str = NULL;

		arr = wcs_Json_GetObjectItem (root, "up", NULL);
		if (arr)
		{
			while ((str = wcs_Json_GetStringAt (arr, i++, NULL)))
			{
				if (!wcs_Rgn_Info_isValidHost (str))
				{
					// Skip URLs which contains incorrect schema.
					continue;
				}				// if
				*bufLen += strlen (str) + 1;
				*upHostCount += 1;
			}					// while
		}						// if

		i = 0;
		arr = wcs_Json_GetObjectItem (root, "io", NULL);
		if (arr)
		{
			while ((str = wcs_Json_GetStringAt (arr, i++, NULL)))
			{
				if (!wcs_Rgn_Info_isValidHost (str))
				{
					// Skip URLs which contains incorrect schema.
					continue;
				}				// if
				*bufLen += strlen (str) + 1;
				*ioHostCount += 1;
			}					// while
		}						// if
	}							// wcs_Rgn_Info_measureHosts

	static void wcs_Rgn_Info_duplicateHosts (wcs_Json * root, wcs_Rgn_HostInfo *** upHosts, wcs_Rgn_HostInfo *** ioHosts, char **strPos, wcs_Uint32 hostFlags)
	{
		int i = 0;
		wcs_Uint32 len = 0;
		wcs_Json *arr = NULL;
		const char *str = NULL;

		arr = wcs_Json_GetObjectItem (root, "up", NULL);
		if (arr)
		{
			while ((str = wcs_Json_GetStringAt (arr, i++, NULL)))
			{
				if (!wcs_Rgn_Info_isValidHost (str))
				{
					// Skip URLs which contains incorrect schema.
					continue;
				}				// if
				(**upHosts)->flags = hostFlags;
				(**upHosts)->host = *strPos;
				len = strlen (str);
				memcpy ((void *) (**upHosts)->host, str, len);
				(*strPos) += len + 1;
				*upHosts += 1;
			}					// while
		}						// if

		i = 0;
		arr = wcs_Json_GetObjectItem (root, "io", NULL);
		if (arr)
		{
			while ((str = wcs_Json_GetStringAt (arr, i++, NULL)))
			{
				if (!wcs_Rgn_Info_isValidHost (str))
				{
					// Skip URLs which contains incorrect schema.
					continue;
				}				// if
				(**ioHosts)->flags = hostFlags | WCS_RGN_DOWNLOAD_HOST;
				(**ioHosts)->host = *strPos;
				len = strlen (str);
				memcpy ((void *) (**ioHosts)->host, str, len);
				(*strPos) += len + 1;
				*ioHosts += 1;
			}					// while
		}						// if
	}							// wcs_Rgn_Info_duplicateHosts

	WCS_DLLAPI extern wcs_Error wcs_Rgn_Info_Fetch (wcs_Client * cli, wcs_Rgn_RegionInfo ** rgnInfo, const char *bucket, const char *accessKey)
	{
		wcs_Error err;
		wcs_Json *root = NULL;
		wcs_Json *http = NULL;
		wcs_Json *https = NULL;
		wcs_Uint32 i = 0;
		wcs_Uint32 upHostCount = 0;
		wcs_Uint32 ioHostCount = 0;
		wcs_Rgn_HostInfo **upHosts = NULL;
		wcs_Rgn_HostInfo **ioHosts = NULL;
		wcs_Rgn_RegionInfo *newRgnInfo = NULL;
		char *buf = NULL;
		char *pos = NULL;
		char *url = NULL;
		wcs_Uint32 bufLen = 0;

		url = wcs_String_Format (256, "%s/v1/query?ak=%s&bucket=%s", WCS_UC_HOST, accessKey, bucket);
		err = wcs_Client_Call (cli, &root, url);
		free (url);
		if (err.code != 200)
		{
			return err;
		}						// if

		bufLen += sizeof (wcs_Rgn_RegionInfo) + strlen (bucket) + 1;

		http = wcs_Json_GetObjectItem (root, "http", NULL);
		if (http)
		{
			wcs_Rgn_Info_measureHosts (http, &bufLen, &upHostCount, &ioHostCount);
		}						// if
		https = wcs_Json_GetObjectItem (root, "https", NULL);
		if (https)
		{
			wcs_Rgn_Info_measureHosts (https, &bufLen, &upHostCount, &ioHostCount);
		}						// if
		bufLen += (sizeof (wcs_Rgn_HostInfo *) + sizeof (wcs_Rgn_HostInfo)) * (upHostCount + ioHostCount);

		buf = calloc (1, bufLen);
		if (!buf)
		{
			err.code = 499;
			err.message = "No enough memory";
			return err;
		}						// buf

		pos = buf;

		newRgnInfo = (wcs_Rgn_RegionInfo *) pos;
		pos += sizeof (wcs_Rgn_RegionInfo);

		newRgnInfo->upHosts = upHosts = (wcs_Rgn_HostInfo **) pos;
		pos += sizeof (wcs_Rgn_HostInfo *) * upHostCount;

		for (i = 0; i < upHostCount; i += 1)
		{
			newRgnInfo->upHosts[i] = (wcs_Rgn_HostInfo *) (pos);
			pos += sizeof (wcs_Rgn_HostInfo);
		}						// for

		newRgnInfo->ioHosts = ioHosts = (wcs_Rgn_HostInfo **) pos;
		pos += sizeof (wcs_Rgn_HostInfo *) * ioHostCount;

		for (i = 0; i < ioHostCount; i += 1)
		{
			newRgnInfo->ioHosts[i] = (wcs_Rgn_HostInfo *) (pos);
			pos += sizeof (wcs_Rgn_HostInfo);
		}						// for

		if (http)
		{
			wcs_Rgn_Info_duplicateHosts (http, &upHosts, &ioHosts, &pos, WCS_RGN_HTTP_HOST);
		}						// if
		if (https)
		{
			wcs_Rgn_Info_duplicateHosts (https, &upHosts, &ioHosts, &pos, WCS_RGN_HTTPS_HOST);
		}						// if

		newRgnInfo->upHostCount = upHostCount;
		newRgnInfo->ioHostCount = ioHostCount;
		newRgnInfo->global = wcs_Json_GetBoolean (root, "global", 0);
		newRgnInfo->ttl = wcs_Json_GetInt64 (root, "ttl", 86400);
		newRgnInfo->nextTimestampToUpdate = newRgnInfo->ttl + wcs_Tm_LocalTime ();

		newRgnInfo->bucket = pos;
		memcpy ((void *) newRgnInfo->bucket, bucket, strlen (bucket));

		*rgnInfo = newRgnInfo;
		return wcs_OK;
	}							// wcs_Rgn_Info_Fetch

	static wcs_Error wcs_Rgn_parseQueryArguments (const char *uptoken, char **bucket, char **accessKey)
	{
		wcs_Error err;
		const char *begin = uptoken;
		const char *end = uptoken;
		char *putPolicy = NULL;

		end = strchr (begin, ':');
		if (!end)
		{
			err.code = 9989;
			err.message = "Invalid uptoken";
			return err;
		}						// if

		*accessKey = calloc (1, end - begin + 1);
		if (!*accessKey)
		{
			err.code = 499;
			err.message = "No enough memory";
			return err;
		}						// if
		memcpy (*accessKey, begin, end - begin);

		begin = strchr (end + 1, ':');
		if (!begin)
		{
			free (*accessKey);
			*accessKey = NULL;
			err.code = 9989;
			err.message = "Invalid uptoken";
			return err;
		}						// if

		putPolicy = wcs_String_Decode (begin);
		begin = strstr (putPolicy, "\"scope\"");
		if (!begin)
		{
			free (*accessKey);
			*accessKey = NULL;
			err.code = 9989;
			err.message = "Invalid uptoken";
			return err;
		}						// if

		begin += strlen ("\"scope\"");
		while (isspace (*begin) || *begin == ':')
		{
			begin += 1;
		}						// while
		if (*begin != '"')
		{
			free (putPolicy);
			free (*accessKey);
			*accessKey = NULL;

			err.code = 9989;
			err.message = "Invalid uptoken";
			return err;
		}						// if

		begin += 1;
		end = begin;
		while (1)
		{
			if (*end == ':' || (*end == '"' && *(end - 1) != '\\'))
			{
				break;
			}					// if
			end += 1;
		}						// while

		*bucket = calloc (1, end - begin + 1);
		if (!*bucket)
		{
			free (putPolicy);
			free (*accessKey);
			*accessKey = NULL;

			err.code = 499;
			err.message = "No enough memory";
			return err;
		}						// if
		memcpy (*bucket, begin, end - begin);
		return wcs_OK;
	}							// wcs_Rgn_parseQueryArguments

	WCS_DLLAPI extern wcs_Error wcs_Rgn_Info_FetchByUptoken (wcs_Client * cli, wcs_Rgn_RegionInfo ** rgnInfo, const char *uptoken)
	{
		wcs_Error err;
		char *bucket = NULL;
		char *accessKey = NULL;

		err = wcs_Rgn_parseQueryArguments (uptoken, &bucket, &accessKey);
		if (err.code != 200)
		{
			return err;
		}						// if

		err = wcs_Rgn_Info_Fetch (cli, rgnInfo, bucket, accessKey);
		free (bucket);
		free (accessKey);
		return err;
	}							// wcs_Rgn_Info_FetchByUptoken

	WCS_DLLAPI extern void wcs_Rgn_Info_Destroy (wcs_Rgn_RegionInfo * rgnInfo)
	{
		free (rgnInfo);
	}							// wcs_Rgn_Info_Destroy

	WCS_DLLAPI extern wcs_Uint32 wcs_Rgn_Info_HasExpirated (wcs_Rgn_RegionInfo * rgnInfo)
	{
		return (rgnInfo->nextTimestampToUpdate <= wcs_Tm_LocalTime ());
	}							// wcs_Rgn_Info_HasExpirated

	WCS_DLLAPI extern wcs_Uint32 wcs_Rgn_Info_CountUpHost (wcs_Rgn_RegionInfo * rgnInfo)
	{
		return rgnInfo->upHostCount;
	}							// wcs_Rgn_Info_CountUpHost

	WCS_DLLAPI extern wcs_Uint32 wcs_Rgn_Info_CountIoHost (wcs_Rgn_RegionInfo * rgnInfo)
	{
		return rgnInfo->ioHostCount;
	}							// wcs_Rgn_Info_CountIoHost

	WCS_DLLAPI extern const char *wcs_Rgn_Info_GetHost (wcs_Rgn_RegionInfo * rgnInfo, wcs_Uint32 n, wcs_Uint32 hostFlags)
	{
		if ((hostFlags & WCS_RGN_HTTPS_HOST) == 0)
		{
			hostFlags |= WCS_RGN_HTTP_HOST;
		}						// if
		if (hostFlags & WCS_RGN_DOWNLOAD_HOST)
		{
			if (n < rgnInfo->ioHostCount && (rgnInfo->ioHosts[n]->flags & hostFlags) == hostFlags)
			{
				return rgnInfo->ioHosts[n]->host;
			}					// if
		}
		else
		{
			if (n < rgnInfo->upHostCount && (rgnInfo->upHosts[n]->flags & hostFlags) == hostFlags)
			{
				return rgnInfo->upHosts[n]->host;
			}					// if
		}						// if
		return NULL;
	}							// wcs_Rgn_Info_GetHost

	WCS_DLLAPI extern wcs_Rgn_RegionTable *wcs_Rgn_Table_Create (void)
	{
		wcs_Rgn_RegionTable *new_tbl = NULL;

		new_tbl = calloc (1, sizeof (wcs_Rgn_RegionTable));
		if (!new_tbl)
		{
			return NULL;
		}						// if

		return new_tbl;
	}							// wcs_Rgn_Create

	WCS_DLLAPI extern void wcs_Rgn_Table_Destroy (wcs_Rgn_RegionTable * rgnTable)
	{
		wcs_Uint32 i = 0;
		if (rgnTable)
		{
			for (i = 0; i < rgnTable->rgnCount; i += 1)
			{
				wcs_Rgn_Info_Destroy (rgnTable->regions[i]);
			}					// for
			free (rgnTable);
		}						// if
	}							// wcs_Rgn_Destroy

	WCS_DLLAPI extern wcs_Error wcs_Rgn_Table_FetchAndUpdate (wcs_Rgn_RegionTable * rgnTable, wcs_Client * cli, const char *bucket, const char *accessKey)
	{
		wcs_Error err;
		wcs_Rgn_RegionInfo *newRgnInfo = NULL;

		err = wcs_Rgn_Info_Fetch (cli, &newRgnInfo, bucket, accessKey);
		if (err.code != 200)
		{
			return err;
		}						// if

		return wcs_Rgn_Table_SetRegionInfo (rgnTable, newRgnInfo);
	}							// wcs_Rgn_Table_FetchAndUpdate

	WCS_DLLAPI extern wcs_Error wcs_Rgn_Table_FetchAndUpdateByUptoken (wcs_Rgn_RegionTable * rgnTable, wcs_Client * cli, const char *uptoken)
	{
		wcs_Error err;
		wcs_Rgn_RegionInfo *newRgnInfo = NULL;

		err = wcs_Rgn_Info_FetchByUptoken (cli, &newRgnInfo, uptoken);
		if (err.code != 200)
		{
			return err;
		}						// if

		return wcs_Rgn_Table_SetRegionInfo (rgnTable, newRgnInfo);
	}							// wcs_Rgn_Table_FetchAndUpdateByUptoken

	WCS_DLLAPI extern wcs_Error wcs_Rgn_Table_SetRegionInfo (wcs_Rgn_RegionTable * rgnTable, wcs_Rgn_RegionInfo * rgnInfo)
	{
		wcs_Error err;
		wcs_Uint32 i = 0;
		wcs_Rgn_RegionInfo **newRegions = NULL;

		for (i = 0; i < rgnTable->rgnCount; i += 1)
		{
			if (strcmp (rgnTable->regions[i]->bucket, rgnInfo->bucket) == 0)
			{
				wcs_Rgn_Info_Destroy (rgnTable->regions[i]);
				rgnTable->regions[i] = rgnInfo;
				return wcs_OK;
			}					// if
		}						// for

		newRegions = calloc (1, sizeof (wcs_Rgn_RegionInfo *) * rgnTable->rgnCount + 1);
		if (!newRegions)
		{
			err.code = 499;
			err.message = "No enough memory";
			return err;
		}						// if

		if (rgnTable->rgnCount > 0)
		{
			memcpy (newRegions, rgnTable->regions, sizeof (wcs_Rgn_RegionInfo *) * rgnTable->rgnCount);
		}						// if
		newRegions[rgnTable->rgnCount] = rgnInfo;

		free (rgnTable->regions);
		rgnTable->regions = newRegions;
		rgnTable->rgnCount += 1;

		return wcs_OK;
	}							// wcs_Rgn_Table_SetRegionInfo

	WCS_DLLAPI extern wcs_Rgn_RegionInfo *wcs_Rgn_Table_GetRegionInfo (wcs_Rgn_RegionTable * rgnTable, const char *bucket)
	{
		wcs_Uint32 i = 0;
		for (i = 0; i < rgnTable->rgnCount; i += 1)
		{
			if (strcmp (rgnTable->regions[i]->bucket, bucket) == 0)
			{
				return rgnTable->regions[i];
			}					// if
		}						// for
		return NULL;
	}							// wcs_Rgn_Table_GetRegionInfo

	WCS_DLLAPI extern wcs_Error wcs_Rgn_Table_GetHost (wcs_Rgn_RegionTable * rgnTable,
		wcs_Client * cli, const char *bucket, const char *accessKey, wcs_Uint32 hostFlags, const char **upHost, wcs_Rgn_HostVote * vote)
	{
		wcs_Error err;
		wcs_Rgn_RegionInfo *rgnInfo = NULL;
		wcs_Rgn_RegionInfo *newRgnInfo = NULL;

		memset (vote, 0, sizeof (wcs_Rgn_HostVote));
		rgnInfo = wcs_Rgn_Table_GetRegionInfo (rgnTable, bucket);
		if (!rgnInfo || wcs_Rgn_Info_HasExpirated (rgnInfo))
		{
			err = wcs_Rgn_Info_Fetch (cli, &newRgnInfo, bucket, accessKey);
			if (err.code != 200)
			{
				return err;
			}					// if

			err = wcs_Rgn_Table_SetRegionInfo (rgnTable, newRgnInfo);
			if (err.code != 200)
			{
				return err;
			}					// if

			rgnInfo = newRgnInfo;
		}						// if

		if ((hostFlags & WCS_RGN_HTTPS_HOST) == 0)
		{
			hostFlags |= WCS_RGN_HTTP_HOST;
		}						// if
		*upHost = wcs_Rgn_Info_GetHost (rgnInfo, 0, hostFlags);

		if (vote)
		{
			vote->rgnInfo = rgnInfo;
			vote->host = &rgnInfo->upHosts[0];
			vote->hostFlags = hostFlags;
			vote->hosts = rgnInfo->upHosts;
			vote->hostCount = rgnInfo->upHostCount;
		}						// if
		return wcs_OK;
	}							// wcs_Rgn_Table_GetHost

	WCS_DLLAPI extern wcs_Error wcs_Rgn_Table_GetHostByUptoken (wcs_Rgn_RegionTable * rgnTable,
		wcs_Client * cli, const char *uptoken, wcs_Uint32 hostFlags, const char **upHost, wcs_Rgn_HostVote * vote)
	{
		wcs_Error err;
		char *bucket = NULL;
		char *accessKey = NULL;

		err = wcs_Rgn_parseQueryArguments (uptoken, &bucket, &accessKey);
		if (err.code != 200)
		{
			return err;
		}						// if

		err = wcs_Rgn_Table_GetHost (rgnTable, cli, bucket, accessKey, hostFlags, upHost, vote);
		free (bucket);
		free (accessKey);
		return wcs_OK;
	}							// wcs_Rgn_Table_GetHostByUptoken

	static void wcs_Rgn_Vote_downgradeHost (wcs_Rgn_HostVote * vote)
	{
		wcs_Rgn_HostInfo **next = vote->host + 1;
		wcs_Rgn_HostInfo *t = NULL;

		while (next < (vote->hosts + vote->hostCount) && ((*next)->flags & vote->hostFlags) == vote->hostFlags)
		{
			if ((*next)->voteCount >= (*vote->host)->voteCount)
			{
				t = *next;
				*next = *(vote->host);
				*(vote->host) = t;
			}					// if
			vote->host = next;
			next += 1;
		}						// while
	}							// wcs_Rgn_Vote_downgradeHost

	WCS_DLLAPI extern void wcs_Rgn_Table_VoteHost (wcs_Rgn_RegionTable * rgnTable, wcs_Rgn_HostVote * vote, wcs_Error err)
	{
		if (!vote->rgnInfo)
		{
			return;
		}						// if
		if (err.code >= 100)
		{
			(*vote->host)->voteCount += 1;
			return;
		}						// if
		switch (err.code)
		{
		case CURLE_UNSUPPORTED_PROTOCOL:
		case CURLE_COULDNT_RESOLVE_PROXY:
		case CURLE_COULDNT_RESOLVE_HOST:
		case CURLE_COULDNT_CONNECT:
			(*vote->host)->voteCount /= 4;
			wcs_Rgn_Vote_downgradeHost (vote);
			break;

		default:
			break;
		}						// switch
	}							// wcs_Rgn_VoteHost
