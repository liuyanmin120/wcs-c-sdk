#include <curl/curl.h>

#include "reader.h"

WCS_DLLAPI wcs_Error wcs_Rd_Reader_Open (wcs_Rd_Reader * rdr, const char *localFileName)
{
	wcs_Error err;
	err = wcs_File_Open (&rdr->file, localFileName);
	if (err.code != 200)
	{
		return err;
	}							// if

	rdr->offset = 0;
	rdr->status = WCS_RD_OK;

	return wcs_OK;
}

WCS_DLLAPI void wcs_Rd_Reader_Close (wcs_Rd_Reader * rdr)
{
	wcs_File_Close (rdr->file);
}

WCS_DLLAPI size_t wcs_Rd_Reader_Callback (char *buffer, size_t size, size_t nitems, void *userData)
{
	ssize_t ret;
	wcs_Rd_Reader *rdr = (wcs_Rd_Reader *) userData;

	ret = wcs_File_ReadAt (rdr->file, buffer, size * nitems, rdr->offset);
	if (ret < 0)
	{
		rdr->status = WCS_RD_ABORT_BY_READAT;
		return CURL_READFUNC_ABORT;
	}							// if

	if (rdr->abortCallback && rdr->abortCallback (rdr->abortUserData, buffer, ret))
	{
		rdr->status = WCS_RD_ABORT_BY_CALLBACK;
		return CURL_READFUNC_ABORT;
	}							// if

	rdr->offset += ret;
	return ret;
}
