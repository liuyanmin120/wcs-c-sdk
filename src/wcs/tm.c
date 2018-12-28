/*
 ============================================================================
 Name        : tm.c
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description :
 ============================================================================
 */

#include "tm.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
#include <Windows.h>


WCS_DLLAPI extern wcs_Uint64 wcs_Tm_LocalTime (void)
{
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	return wtm.wSecond;
}							// wcs

#else

#include <sys/time.h>

WCS_DLLAPI extern wcs_Uint64 wcs_Tm_LocalTime (void)
{
	struct timeval tv;
	  gettimeofday (&tv, NULL);
	  return tv.tv_sec;
}							// wcs_Tm_LocalTime

#endif

#ifdef __cplusplus
}
#endif
