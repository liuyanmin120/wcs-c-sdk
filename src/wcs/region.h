/*
 ============================================================================
 Name		: region.h
 Author	  : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */

#ifndef WCS_REGION_H
#define WCS_REGION_H

#include "http.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

	WCS_DLLAPI extern void wcs_Rgn_Enable (void);
	WCS_DLLAPI extern void wcs_Rgn_Disable (void);
	WCS_DLLAPI extern wcs_Uint32 wcs_Rgn_IsEnabled (void);

	enum
	{
		WCS_RGN_HTTP_HOST = 0x0001,
		WCS_RGN_HTTPS_HOST = 0x0002,
		WCS_RGN_CDN_HOST = 0x0004,
		WCS_RGN_DOWNLOAD_HOST = 0x0008
	};

	typedef struct _wcs_Rgn_HostInfo
	{
		const char *host;
		wcs_Uint32 flags;
		wcs_Uint32 voteCount;
	} wcs_Rgn_HostInfo;

	typedef struct _wcs_Rgn_RegionInfo
	{
		wcs_Uint64 nextTimestampToUpdate;

		const char *bucket;

		wcs_Int64 ttl;
		wcs_Int64 global;

		wcs_Uint32 upHostCount;
		wcs_Rgn_HostInfo **upHosts;

		wcs_Uint32 ioHostCount;
		wcs_Rgn_HostInfo **ioHosts;
	} wcs_Rgn_RegionInfo;

	WCS_DLLAPI extern wcs_Error wcs_Rgn_Info_Fetch (wcs_Client * cli, wcs_Rgn_RegionInfo ** rgnInfo, const char *bucket, const char *accessKey);
	WCS_DLLAPI extern wcs_Error wcs_Rgn_Info_FetchByUptoken (wcs_Client * cli, wcs_Rgn_RegionInfo ** rgnInfo, const char *uptoken);
	WCS_DLLAPI extern void wcs_Rgn_Info_Destroy (wcs_Rgn_RegionInfo * rgnInfo);
	WCS_DLLAPI extern wcs_Uint32 wcs_Rgn_Info_HasExpirated (wcs_Rgn_RegionInfo * rgnInfo);
	WCS_DLLAPI extern wcs_Uint32 wcs_Rgn_Info_CountUpHost (wcs_Rgn_RegionInfo * rgnInfo);
	WCS_DLLAPI extern wcs_Uint32 wcs_Rgn_Info_CountIoHost (wcs_Rgn_RegionInfo * rgnInfo);
	WCS_DLLAPI extern const char *wcs_Rgn_Info_GetHost (wcs_Rgn_RegionInfo * rgnInfo, wcs_Uint32 n, wcs_Uint32 hostFlags);
	WCS_DLLAPI extern const char *wcs_Rgn_Info_GetIoHost (wcs_Rgn_RegionInfo * rgnInfo, wcs_Uint32 n, wcs_Uint32 hostFlags);

	typedef struct _wcs_Rgn_RegionTable
	{
		wcs_Uint32 rgnCount;
		wcs_Rgn_RegionInfo **regions;
	} wcs_Rgn_RegionTable;

	typedef struct _wcs_Rgn_HostVote
	{
		wcs_Rgn_RegionInfo *rgnInfo;
		wcs_Rgn_HostInfo **host;
		wcs_Rgn_HostInfo **hosts;
		wcs_Uint32 hostCount;
		wcs_Uint32 hostFlags;
	} wcs_Rgn_HostVote;

	WCS_DLLAPI extern wcs_Rgn_RegionTable *wcs_Rgn_Table_Create (void);
	WCS_DLLAPI extern void wcs_Rgn_Table_Destroy (wcs_Rgn_RegionTable * rgnTable);

	WCS_DLLAPI extern wcs_Error wcs_Rgn_Table_FetchAndUpdate (wcs_Rgn_RegionTable * rgnTable, wcs_Client * cli, const char *bucket, const char *access_key);
	WCS_DLLAPI extern wcs_Error wcs_Rgn_Table_FetchAndUpdateByUptoken (wcs_Rgn_RegionTable * rgnTable, wcs_Client * cli, const char *uptoken);
	WCS_DLLAPI extern wcs_Error wcs_Rgn_Table_SetRegionInfo (wcs_Rgn_RegionTable * rgnTable, wcs_Rgn_RegionInfo * rgnInfo);
	WCS_DLLAPI extern wcs_Rgn_RegionInfo *wcs_Rgn_Table_GetRegionInfo (wcs_Rgn_RegionTable * rgnTable, const char *bucket);
	WCS_DLLAPI extern wcs_Error wcs_Rgn_Table_GetHost (wcs_Rgn_RegionTable * rgnTable, wcs_Client * cli, const char *bucket, const char *accessKey, wcs_Uint32 hostFlags, const char **upHost,
		wcs_Rgn_HostVote * vote);
	WCS_DLLAPI extern wcs_Error wcs_Rgn_Table_GetHostByUptoken (wcs_Rgn_RegionTable * rgnTable, wcs_Client * cli, const char *uptoken, wcs_Uint32 hostFlags, const char **upHost,
		wcs_Rgn_HostVote * vote);
	WCS_DLLAPI extern void wcs_Rgn_Table_VoteHost (wcs_Rgn_RegionTable * rgnTable, wcs_Rgn_HostVote * vote, wcs_Error err);

#ifdef __cplusplus
}
#endif

#pragma pack()

#endif							// WCS_REGION_H
