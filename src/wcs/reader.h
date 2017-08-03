/*
 ============================================================================
 Name		: reader.h
 Author	  : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */

#ifndef WCS_READER_H
#define WCS_READER_H

#include "base.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* Declaration of Controllable Reader */

	typedef int (*wcs_Rd_FnAbort) (void *abortUserData, char *buf, size_t size);

	enum
	{
		WCS_RD_OK = 0,
		WCS_RD_ABORT_BY_CALLBACK,
		WCS_RD_ABORT_BY_READAT
	};

	typedef struct _wcs_Rd_Reader
	{
		wcs_File *file;
		wcs_Off_T offset;

		int status;

		void *abortUserData;
		wcs_Rd_FnAbort abortCallback;
	} wcs_Rd_Reader;

	WCS_DLLAPI extern wcs_Error wcs_Rd_Reader_Open (wcs_Rd_Reader * rdr, const char *localFileName);
	WCS_DLLAPI extern void wcs_Rd_Reader_Close (wcs_Rd_Reader * rdr);

	WCS_DLLAPI extern size_t wcs_Rd_Reader_Callback (char *buffer, size_t size, size_t nitems, void *rdr);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif							// WCS_READER_H
