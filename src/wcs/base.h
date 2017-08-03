/*
 ============================================================================
 Name        : base.h
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */

#ifndef WCS_BASE_H
#define WCS_BASE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "macro.h"

/*============================================================================*/
/* func type ssize_t */

#ifdef _WIN32

#include <sys/types.h>

#ifndef _W64
#define _W64
#endif

typedef _W64 int ssize_t;

#endif

#if defined(_MSC_VER)

typedef _int64 wcs_Off_T;

#else

typedef off_t wcs_Off_T;

#endif

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

#define LOG_FILE_LEN 512

/*============================================================================*/
/* func wcs_Zero */

#define wcs_Zero(v)		memset(&v, 0, sizeof(v))

/*============================================================================*/
/* func wcs_snprintf */

#if defined(_MSC_VER)
#define wcs_snprintf		_snprintf
#else
#define wcs_snprintf		snprintf
#endif

/*============================================================================*/
/* type wcs_Int64, wcs_Uint32 */

#if defined(_MSC_VER)
	typedef _int64 wcs_Int64;
	typedef unsigned _int64 wcs_Uint64;
#else
	typedef long long wcs_Int64;
	typedef unsigned long long wcs_Uint64;
#endif

	typedef unsigned int wcs_Uint32;
	typedef unsigned short wcs_Uint16;

/*============================================================================*/
/* type wcs_Bool */

	typedef int wcs_Bool;

	enum
	{
		wcs_False = 0,
		wcs_True = 1
	};

/*============================================================================*/
/* type wcs_Error */

/* @gist error */

	typedef struct _wcs_Error
	{
		int code;
		const char *message;
	} wcs_Error;

/* @endgist */

	WCS_DLLAPI extern wcs_Error wcs_OK;

/*============================================================================*/
/* type wcs_Free */

	void wcs_Free (void *addr);

/*============================================================================*/
/* type wcs_Count */

	typedef long wcs_Count;

	WCS_DLLAPI extern wcs_Count wcs_Count_Inc (wcs_Count * self);
	WCS_DLLAPI extern wcs_Count wcs_Count_Dec (wcs_Count * self);

/*============================================================================*/
/* func wcs_String_Concat */

	WCS_DLLAPI extern char *wcs_String_Concat2 (const char *s1, const char *s2);
	WCS_DLLAPI extern char *wcs_String_Concat3 (const char *s1, const char *s2, const char *s3);
	WCS_DLLAPI extern char *wcs_String_Concat (const char *s1, ...);

	WCS_DLLAPI extern char *wcs_String_Format (size_t initSize, const char *fmt, ...);

	WCS_DLLAPI extern char *wcs_String_Join (const char *deli, char *strs[], int strCount);
	WCS_DLLAPI extern char *wcs_String_Dup (const char *src);

/*============================================================================*/
/* func wcs_String_Encode */

	WCS_DLLAPI extern char *wcs_Memory_Encode (const char *buf, const size_t cb);
	WCS_DLLAPI extern char *wcs_String_Encode (const char *s);
	WCS_DLLAPI extern char *wcs_String_Decode (const char *s);

/*============================================================================*/
/* func wcs_QueryEscape */

	char *wcs_PathEscape (const char *s, wcs_Bool * fesc);
	char *wcs_QueryEscape (const char *s, wcs_Bool * fesc);

/*============================================================================*/
/* func wcs_Seconds */

	wcs_Int64 wcs_Seconds ();

/*============================================================================*/
/* type wcs_Reader */

	typedef size_t (*wcs_FnRead) (void *buf, size_t, size_t n, void *self);

	typedef struct _wcs_Reader
	{
		void *self;
		wcs_FnRead Read;
	} wcs_Reader;

	WCS_DLLAPI extern wcs_Reader wcs_FILE_Reader (FILE * fp);

/*============================================================================*/
/* type wcs_Writer */

	typedef size_t (*wcs_FnWrite) (const void *buf, size_t, size_t n, void *self);

	typedef struct _wcs_Writer
	{
		void *self;
		wcs_FnWrite Write;
	} wcs_Writer;

	WCS_DLLAPI extern wcs_Writer wcs_FILE_Writer (FILE * fp);
	WCS_DLLAPI extern wcs_Error wcs_Copy (wcs_Writer w, wcs_Reader r, void *buf, size_t n, wcs_Int64 * ret);

#define wcs_Stderr wcs_FILE_Writer(stderr)

/*============================================================================*/
/* type wcs_ReaderAt */

	typedef ssize_t (*wcs_FnReadAt) (void *self, void *buf, size_t bytes, wcs_Off_T offset);

	typedef struct _wcs_ReaderAt
	{
		void *self;
		wcs_FnReadAt ReadAt;
	} wcs_ReaderAt;

/*============================================================================*/
/* type wcs_Buffer */

	typedef struct _wcs_Valist
	{
		va_list items;
	} wcs_Valist;

	typedef struct _wcs_Buffer
	{
		char *buf;
		char *curr;
		char *bufEnd;
	} wcs_Buffer;

	WCS_DLLAPI extern void wcs_Buffer_Init (wcs_Buffer * self, size_t initSize);
	WCS_DLLAPI extern void wcs_Buffer_Reset (wcs_Buffer * self);
	WCS_DLLAPI extern void wcs_Buffer_AppendInt (wcs_Buffer * self, wcs_Int64 v);
	WCS_DLLAPI extern void wcs_Buffer_AppendUint (wcs_Buffer * self, wcs_Uint64 v);
	WCS_DLLAPI extern void wcs_Buffer_AppendError (wcs_Buffer * self, wcs_Error v);
	WCS_DLLAPI extern void wcs_Buffer_AppendEncodedBinary (wcs_Buffer * self, const char *buf, size_t cb);
	WCS_DLLAPI extern void wcs_Buffer_AppendFormat (wcs_Buffer * self, const char *fmt, ...);
	WCS_DLLAPI extern void wcs_Buffer_AppendFormatV (wcs_Buffer * self, const char *fmt, wcs_Valist * args);
	WCS_DLLAPI extern void wcs_Buffer_Cleanup (wcs_Buffer * self);

	WCS_DLLAPI extern const char *wcs_Buffer_CStr (wcs_Buffer * self);
	WCS_DLLAPI extern const char *wcs_Buffer_Format (wcs_Buffer * self, const char *fmt, ...);

	WCS_DLLAPI extern void wcs_Buffer_PutChar (wcs_Buffer * self, char ch);

	WCS_DLLAPI extern size_t wcs_Buffer_Len (wcs_Buffer * self);
	WCS_DLLAPI extern size_t wcs_Buffer_Write (wcs_Buffer * self, const void *buf, size_t n);
	WCS_DLLAPI extern size_t wcs_Buffer_Fwrite (const void *buf, size_t, size_t n, void *self);

	WCS_DLLAPI extern wcs_Writer wcs_BufWriter (wcs_Buffer * self);

	WCS_DLLAPI extern char *wcs_Buffer_Expand (wcs_Buffer * self, size_t n);
	WCS_DLLAPI extern void wcs_Buffer_Commit (wcs_Buffer * self, char *p);

	typedef void (*wcs_FnAppender) (wcs_Buffer * self, wcs_Valist * ap);

	WCS_DLLAPI extern void wcs_Format_Register (char esc, wcs_FnAppender appender);

/*============================================================================*/
/* func wcs_Null_Fwrite */

	WCS_DLLAPI extern size_t wcs_Null_Fwrite (const void *buf, size_t, size_t n, void *self);

	WCS_DLLAPI extern wcs_Writer wcs_Discard;

/*============================================================================*/
/* type wcs_ReadBuf */

	typedef struct _wcs_ReadBuf
	{
		const char *buf;
		wcs_Off_T off;
		wcs_Off_T limit;
	} wcs_ReadBuf;

	WCS_DLLAPI extern wcs_Reader wcs_BufReader (wcs_ReadBuf * self, const char *buf, size_t bytes);
	WCS_DLLAPI extern wcs_ReaderAt wcs_BufReaderAt (wcs_ReadBuf * self, const char *buf, size_t bytes);

/*============================================================================*/
/* type wcs_Tee */

	typedef struct _wcs_Tee
	{
		wcs_Reader r;
		wcs_Writer w;
	} wcs_Tee;

	WCS_DLLAPI extern wcs_Reader wcs_TeeReader (wcs_Tee * self, wcs_Reader r, wcs_Writer w);

/*============================================================================*/
/* type wcs_Section */

	typedef struct _wcs_Section
	{
		wcs_ReaderAt r;
		wcs_Off_T off;
		wcs_Off_T limit;
	} wcs_Section;

	WCS_DLLAPI extern wcs_Reader wcs_SectionReader (wcs_Section * self, wcs_ReaderAt r, wcs_Off_T off, size_t n);

/*============================================================================*/
/* type wcs_Crc32 */

	WCS_DLLAPI extern unsigned long wcs_Crc32_Update (unsigned long inCrc32, const void *buf, size_t bufLen);

	typedef struct _wcs_Crc32
	{
		unsigned long val;
		//haleytest
		//wcs_Uint32 val;
	} wcs_Crc32;

	WCS_DLLAPI extern wcs_Writer wcs_Crc32Writer (wcs_Crc32 * self, unsigned long inCrc32);

/*============================================================================*/
/* type wcs_File */

	typedef struct _wcs_File wcs_File;

#if defined(_MSC_VER)
	typedef struct _wcs_FileInfo
	{
		wcs_Off_T st_size;		/* total size, in bytes */
		time_t st_atime;		/* time of last access */
		time_t st_mtime;		/* time of last modification */
		time_t st_ctime;		/* time of last status change */
	} wcs_FileInfo;
#else

#include <sys/stat.h>

	typedef struct stat wcs_FileInfo;

#endif

	WCS_DLLAPI extern wcs_Error wcs_File_Open (wcs_File ** pp, const char *file);
	WCS_DLLAPI extern wcs_Error wcs_File_Stat (wcs_File * self, wcs_FileInfo * fi);

#define wcs_FileInfo_Fsize(fi) ((fi).st_size)

	WCS_DLLAPI extern void wcs_File_Close (void *self);

	WCS_DLLAPI extern ssize_t wcs_File_ReadAt (void *self, void *buf, size_t bytes, wcs_Off_T offset);

	WCS_DLLAPI extern wcs_ReaderAt wcs_FileReaderAt (wcs_File * self);

/*============================================================================*/
/* type wcs_Log */

#define wcs_Ldebug	0
#define wcs_Linfo		1
#define wcs_Lwarn		2
#define wcs_Lerror	3
#define wcs_Lpanic	4
#define wcs_Lfatal	5

	WCS_DLLAPI extern void wcs_Logv (wcs_Writer w, int level, const char *fmt, wcs_Valist * args);

	WCS_DLLAPI extern void wcs_Stderr_Info (const char *fmt, ...);
	WCS_DLLAPI extern void wcs_Stderr_Warn (const char *fmt, ...);

	WCS_DLLAPI extern void wcs_Null_Log (const char *fmt, ...);
	WCS_DLLAPI extern void wcs_Log_Init(char *logConfigFile, FILE *file);
	WCS_DLLAPI void wcs_close_Logfile(FILE *file);

#ifndef wcs_Log_Info

#ifdef WCS_DISABLE_LOG

#define wcs_Log_Info	wcs_Null_Log
#define wcs_Log_Warn	wcs_Null_Log

#else

#define wcs_Log_Info	wcs_Stderr_Info
#define wcs_Log_Warn	wcs_Stderr_Warn

#endif

#endif

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif							/* WCS_BASE_H */
