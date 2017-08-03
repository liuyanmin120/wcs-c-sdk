/*
 ============================================================================
 Name        : base.c
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description :
 ============================================================================
 */

#include "base.h"
#include "../b64/urlsafe_b64.h"
#include "../base/inifile.h"
#include "../base/log.h"
#include <assert.h>
#include <time.h>
#include <errno.h>

/*============================================================================*/
/* type wcs_Free */

void wcs_Free (void *addr)
{
        if (NULL != addr)
        {
	    free (addr);
	    addr = NULL;
	 }
}

/*============================================================================*/
/* type wcs_Count */

#if defined(_WIN32)

#include <windows.h>

wcs_Count wcs_Count_Inc (wcs_Count * self)
{
	return InterlockedIncrement (self);
}

wcs_Count wcs_Count_Dec (wcs_Count * self)
{
	return InterlockedDecrement (self);
}

#else

wcs_Count wcs_Count_Inc (wcs_Count * self)
{
	return __sync_add_and_fetch (self, 1);
}

wcs_Count wcs_Count_Dec (wcs_Count * self)
{
	return __sync_sub_and_fetch (self, 1);
}

#endif

/*============================================================================*/
/* func wcs_Seconds */

wcs_Int64 wcs_Seconds ()
{
	return (wcs_Int64) time (NULL);
}

/*============================================================================*/
/* func wcs_QueryEscape */

typedef enum
{
	encodePath,
	encodeUserPassword,
	encodeQueryComponent,
	encodeFragment,
} escapeMode;

// Return true if the specified character should be escaped when
// appearing in a URL string, according to RFC 3986.
static int wcs_shouldEscape (int c, escapeMode mode)
{
	if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9'))
	{
		return 0;
	}

	switch (c)
	{
	case '-':
	case '_':
	case '.':
	case '~':					// §2.3 Unreserved characters (mark)
		return 0;
	case '$':
	case '&':
	case '+':
	case ',':
	case '/':
	case ':':
	case ';':
	case '=':
	case '?':
	case '@':					// §2.2 Reserved characters (reserved)
		switch (mode)
		{
		case encodePath:		// §3.3
			return c == '?';
		case encodeUserPassword:	// §3.2.2
			return c == '@' || c == '/' || c == ':';
		case encodeQueryComponent:	// §3.4
			return 1;
		case encodeFragment:	// §4.1
			return 0;
		}
	}

	return 1;
}

static const char wcs_hexTable[] = "0123456789ABCDEF";

static char *wcs_escape (const char *s, escapeMode mode, wcs_Bool * fesc)
{
	int spaceCount = 0;
	int hexCount = 0;
	int i, j, len = strlen (s);
	int c;
	char *t;

	for (i = 0; i < len; i++)
	{
		// prevent c from sign extension
		c = ((int) s[i]) & 0xFF;
		if (wcs_shouldEscape (c, mode))
		{
			if (c == ' ' && mode == encodeQueryComponent)
			{
				spaceCount++;
			}
			else
			{
				hexCount++;
			}
		}
	}

	if (spaceCount == 0 && hexCount == 0)
	{
		*fesc = wcs_False;
		return (char *) s;
	}

	t = (char *) malloc (len + 2 * hexCount + 1);
	j = 0;
	for (i = 0; i < len; i++)
	{
		// prevent c from sign extension
		c = ((int) s[i]) & 0xFF;
		if (wcs_shouldEscape (c, mode))
		{
			if (c == ' ' && mode == encodeQueryComponent)
			{
				t[j] = '+';
				j++;
			}
			else
			{
				t[j] = '%';
				t[j + 1] = wcs_hexTable[c >> 4];
				t[j + 2] = wcs_hexTable[c & 15];
				j += 3;
			}
		}
		else
		{
			t[j] = s[i];
			j++;
		}
	}
	t[j] = '\0';
	*fesc = wcs_True;
	return t;
}

char *wcs_PathEscape (const char *s, wcs_Bool * fesc)
{
	return wcs_escape (s, encodePath, fesc);
}

char *wcs_QueryEscape (const char *s, wcs_Bool * fesc)
{
	return wcs_escape (s, encodeQueryComponent, fesc);
}

/*============================================================================*/
/* func wcs_String_Concat */

char *wcs_String_Concat2 (const char *s1, const char *s2)
{
        if ((NULL == s1) || (NULL == s2))
        {
            return NULL;
        }
	size_t len1 = strlen (s1);
	size_t len2 = strlen (s2);
	char *p = (char *) malloc (len1 + len2 + 1);
	memcpy (p, s1, len1);
	memcpy (p + len1, s2, len2);
	p[len1 + len2] = '\0';
	return p;
}

char *wcs_String_Concat3 (const char *s1, const char *s2, const char *s3)
{
        if ((NULL == s1) || (NULL == s2) || (NULL == s3))
        {
            return NULL;
        }
	size_t len1 = strlen (s1);
	size_t len2 = strlen (s2);
	size_t len3 = strlen (s3);
	char *p = (char *) malloc (len1 + len2 + len3 + 1);
	memcpy (p, s1, len1);
	memcpy (p + len1, s2, len2);
	memcpy (p + len1 + len2, s3, len3);
	p[len1 + len2 + len3] = '\0';
	return p;
}

char *wcs_String_Concat (const char *s1, ...)
{
	va_list ap;
	char *p;
	const char *s;
	size_t len, slen, len1 = strlen (s1);

	va_start (ap, s1);
	len = len1;
	for (;;)
	{
		s = va_arg (ap, const char *);
		if (s == NULL)
		{
			break;
		}
		len += strlen (s);
	}

	p = (char *) malloc (len + 1);

	va_start (ap, s1);
	memcpy (p, s1, len1);
	len = len1;
	for (;;)
	{
		s = va_arg (ap, const char *);
		if (s == NULL)
		{
			break;
		}
		slen = strlen (s);
		memcpy (p + len, s, slen);
		len += slen;
	}
	p[len] = '\0';
	return p;
}

char *wcs_String_Join (const char *deli, char *strs[], int strCount)
{
	int i = 0;
	char *ret = NULL;
	char *pos = NULL;
	size_t totalLen = 0;
	size_t copyLen = 0;
	size_t deliLen = 0;

	if (strCount == 1)
	{
		return strdup (strs[0]);
	}

	for (i = 0; i < strCount; i += 1)
	{
		totalLen += strlen (strs[i]);
	}							// for

	deliLen = strlen (deli);
	totalLen += deliLen * (strCount - 1);
	ret = (char *) malloc (totalLen + 1);
	if (ret == NULL)
	{
		return NULL;
	}

	pos = ret;
	copyLen = strlen (strs[0]);
	memcpy (pos, strs[0], copyLen);
	pos += copyLen;

	for (i = 1; i < strCount; i += 1)
	{
		memcpy (pos, deli, deliLen);
		pos += deliLen;

		copyLen = strlen (strs[i]);
		memcpy (pos, strs[i], copyLen);
		pos += copyLen;
	}							// for

	ret[totalLen] = '\0';
	return ret;
}								// wcs_String_Join

char *wcs_String_Dup (const char *src)
{
	return strdup (src);
}								// wcs_String_Dup

/*============================================================================*/
/* func wcs_String_Encode */

char *wcs_String_Encode (const char *buf)
{
	const size_t cb = strlen (buf);
	const size_t cbDest = urlsafe_b64_encode (buf, cb, NULL, 0);
	char *dest = (char *) malloc (cbDest + 1);
	const size_t cbReal = urlsafe_b64_encode (buf, cb, dest, cbDest);
	dest[cbReal] = '\0';
	return dest;
}

char *wcs_Memory_Encode (const char *buf, const size_t cb)
{
	const size_t cbDest = urlsafe_b64_encode (buf, cb, NULL, 0);
	char *dest = (char *) malloc (cbDest + 1);
	const size_t cbReal = urlsafe_b64_encode (buf, cb, dest, cbDest);
	dest[cbReal] = '\0';
	return dest;
}

char *wcs_String_Decode (const char *buf)
{
	const size_t cb = strlen (buf);
	const size_t cbDest = urlsafe_b64_decode (buf, cb, NULL, 0);
	char *dest = (char *) malloc (cbDest + 1);
	const size_t cbReal = urlsafe_b64_decode (buf, cb, dest, cbDest);
	dest[cbReal] = '\0';
	return dest;
}

/*============================================================================*/
/* type wcs_Buffer */

static void wcs_Buffer_expand (wcs_Buffer * self, size_t expandSize)
{
	size_t oldSize = self->curr - self->buf;
	size_t newSize = (self->bufEnd - self->buf) << 1;
	expandSize += oldSize;
	while (newSize < expandSize)
	{
		newSize <<= 1;
	}
	self->buf = realloc (self->buf, newSize);
	self->curr = self->buf + oldSize;
	self->bufEnd = self->buf + newSize;
}

void wcs_Buffer_Init (wcs_Buffer * self, size_t initSize)
{
	self->buf = self->curr = (char *) malloc (initSize);
	self->bufEnd = self->buf + initSize;
	if (NULL != self->buf)
	{
		memset(self->buf, 0 ,initSize);
	}
}

void wcs_Buffer_Reset (wcs_Buffer * self)
{
	self->curr = self->buf;
}

void wcs_Buffer_Cleanup (wcs_Buffer * self)
{
	if (self->buf != NULL)
	{
		free (self->buf);
		self->buf = NULL;
	}
}

size_t wcs_Buffer_Len (wcs_Buffer * self)
{
	return self->curr - self->buf;
}

const char *wcs_Buffer_CStr (wcs_Buffer * self)
{
	if (self->curr >= self->bufEnd)
	{
		wcs_Buffer_expand (self, 1);
	}
	*self->curr = '\0';
	return self->buf;
}

void wcs_Buffer_PutChar (wcs_Buffer * self, char ch)
{
	if (self->curr >= self->bufEnd)
	{
		wcs_Buffer_expand (self, 1);
	}
	*self->curr++ = ch;
}

size_t wcs_Buffer_Write (wcs_Buffer * self, const void *buf, size_t n)
{
	if (self->curr + n > self->bufEnd)
	{
		wcs_Buffer_expand (self, n);
	}
	memcpy (self->curr, buf, n);
	self->curr += n;
	return n;
}

size_t wcs_Buffer_Fwrite (const void *buf, size_t size, size_t nmemb, void *self)
{
	assert (size == 1);
	return wcs_Buffer_Write ((wcs_Buffer *) self, buf, nmemb);
}

wcs_Writer wcs_BufWriter (wcs_Buffer * self)
{
	wcs_Writer writer = { self, wcs_Buffer_Fwrite };
	return writer;
}

/*============================================================================*/
/* wcs Format Functions */

char *wcs_Buffer_Expand (wcs_Buffer * self, size_t n)
{
	if (self->curr + n > self->bufEnd)
	{
		wcs_Buffer_expand (self, n);
	}
	return self->curr;
}

void wcs_Buffer_Commit (wcs_Buffer * self, char *p)
{
	assert (p >= self->curr);
	assert (p <= self->bufEnd);
	self->curr = p;
}

void wcs_Buffer_AppendUint (wcs_Buffer * self, wcs_Uint64 v)
{
	char buf[32];
	char *p = buf + 32;
	for (;;)
	{
		*--p = '0' + (char) (v % 10);
		v /= 10;
		if (v == 0)
		{
			break;
		}
	}
	wcs_Buffer_Write (self, p, buf + 32 - p);
}

void wcs_Buffer_AppendInt (wcs_Buffer * self, wcs_Int64 v)
{
	if (v < 0)
	{
		v = -v;
		wcs_Buffer_PutChar (self, '-');
	}
	wcs_Buffer_AppendUint (self, v);
}

void wcs_Buffer_AppendError (wcs_Buffer * self, wcs_Error v)
{
	wcs_Buffer_PutChar (self, 'E');
	wcs_Buffer_AppendInt (self, v.code);
	if (v.message)
	{
		wcs_Buffer_PutChar (self, ' ');
		wcs_Buffer_Write (self, v.message, strlen (v.message));
	}
}

void wcs_Buffer_AppendEncodedBinary (wcs_Buffer * self, const char *buf, size_t cb)
{
	const size_t cbDest = urlsafe_b64_encode (buf, cb, NULL, 0);
	char *dest = wcs_Buffer_Expand (self, cbDest);
	const size_t cbReal = urlsafe_b64_encode (buf, cb, dest, cbDest);
	wcs_Buffer_Commit (self, dest + cbReal);
}

void wcs_Buffer_appendUint (wcs_Buffer * self, wcs_Valist * ap)
{
	unsigned v = va_arg (ap->items, unsigned);
	wcs_Buffer_AppendUint (self, v);
}

void wcs_Buffer_appendInt (wcs_Buffer * self, wcs_Valist * ap)
{
	int v = va_arg (ap->items, int);
	wcs_Buffer_AppendInt (self, v);
}

void wcs_Buffer_appendUint64 (wcs_Buffer * self, wcs_Valist * ap)
{
	wcs_Uint64 v = va_arg (ap->items, wcs_Uint64);
	wcs_Buffer_AppendUint (self, v);
}

void wcs_Buffer_appendInt64 (wcs_Buffer * self, wcs_Valist * ap)
{
	wcs_Int64 v = va_arg (ap->items, wcs_Int64);
	wcs_Buffer_AppendInt (self, v);
}

void wcs_Buffer_appendString (wcs_Buffer * self, wcs_Valist * ap)
{
	const char *v = va_arg (ap->items, const char *);
	if (v == NULL)
	{
		v = "(null)";
	}
	wcs_Buffer_Write (self, v, strlen (v));
}

void wcs_Buffer_appendEncodedString (wcs_Buffer * self, wcs_Valist * ap)
{
	const char *v = va_arg (ap->items, const char *);
	size_t n = strlen (v);
	wcs_Buffer_AppendEncodedBinary (self, v, n);
}

void wcs_Buffer_appendError (wcs_Buffer * self, wcs_Valist * ap)
{
	wcs_Error v = va_arg (ap->items, wcs_Error);
	wcs_Buffer_AppendError (self, v);
}

void wcs_Buffer_appendPercent (wcs_Buffer * self, wcs_Valist * ap)
{
	wcs_Buffer_PutChar (self, '%');
}

/*============================================================================*/
/* wcs Format */

typedef struct _wcs_formatProc
{
	wcs_FnAppender Append;
	char esc;
} wcs_formatProc;

static wcs_formatProc wcs_formatProcs[] = {
	{wcs_Buffer_appendInt, 'd'},
	{wcs_Buffer_appendUint, 'u'},
	{wcs_Buffer_appendInt64, 'D'},
	{wcs_Buffer_appendUint64, 'U'},
	{wcs_Buffer_appendString, 's'},
	{wcs_Buffer_appendEncodedString, 'S'},
	{wcs_Buffer_appendError, 'E'},
	{wcs_Buffer_appendPercent, '%'},
};

static wcs_FnAppender wcs_Appenders[128] = { 0 };

void wcs_Format_Register (char esc, wcs_FnAppender appender)
{
	if ((unsigned) esc < 128)
	{
		wcs_Appenders[esc] = appender;
	}
}

void wcs_Buffer_formatInit ()
{
	wcs_formatProc *p;
	wcs_formatProc *pEnd = (wcs_formatProc *) ((char *) wcs_formatProcs + sizeof (wcs_formatProcs));
	for (p = wcs_formatProcs; p < pEnd; p++)
	{
		wcs_Appenders[p->esc] = p->Append;
	}
}

void wcs_Buffer_AppendFormatV (wcs_Buffer * self, const char *fmt, wcs_Valist * args)
{
	unsigned char ch;
	const char *p;
	wcs_FnAppender appender;

	for (;;)
	{
		p = strchr (fmt, '%');
		if (p == NULL)
		{
			break;
		}
		if (p > fmt)
		{
			wcs_Buffer_Write (self, fmt, p - fmt);
		}
		p++;
		ch = *p++;
		fmt = p;
		if (ch < 128)
		{
			appender = wcs_Appenders[ch];
			if (appender != NULL)
			{
				appender (self, args);
				continue;
			}
		}
		wcs_Buffer_PutChar (self, '%');
		wcs_Buffer_PutChar (self, ch);
	}
	if (*fmt)
	{
		wcs_Buffer_Write (self, fmt, strlen (fmt));
	}
}

void wcs_Buffer_AppendFormat (wcs_Buffer * self, const char *fmt, ...)
{
	wcs_Valist args;
	va_start (args.items, fmt);
	wcs_Buffer_AppendFormatV (self, fmt, &args);
}

const char *wcs_Buffer_Format (wcs_Buffer * self, const char *fmt, ...)
{
	wcs_Valist args;
	va_start (args.items, fmt);
	wcs_Buffer_Reset (self);
	wcs_Buffer_AppendFormatV (self, fmt, &args);
	return wcs_Buffer_CStr (self);
}

char *wcs_String_Format (size_t initSize, const char *fmt, ...)
{
	wcs_Valist args;
	wcs_Buffer buf;
	va_start (args.items, fmt);
	wcs_Buffer_Init (&buf, initSize);
	wcs_Buffer_AppendFormatV (&buf, fmt, &args);
	return (char *) wcs_Buffer_CStr (&buf);
}

/*============================================================================*/
/* func wcs_FILE_Reader */

wcs_Reader wcs_FILE_Reader (FILE * fp)
{
	wcs_Reader reader = { fp, (wcs_FnRead) fread };
	return reader;
}

wcs_Writer wcs_FILE_Writer (FILE * fp)
{
	wcs_Writer writer = { fp, (wcs_FnWrite) fwrite };
	return writer;
}

/*============================================================================*/
/* func wcs_Copy */

wcs_Error wcs_OK = {
	200, "OK"
};

wcs_Error wcs_Copy (wcs_Writer w, wcs_Reader r, void *buf, size_t n, wcs_Int64 * ret)
{
	wcs_Int64 fsize = 0;
	size_t n1, n2;
	char *p = (char *) buf;
	if (buf == NULL)
	{
		p = (char *) malloc (n);
	}
	for (;;)
	{
		n1 = r.Read (p, 1, n, r.self);
		if (n1 > 0)
		{
			n2 = w.Write (p, 1, n1, w.self);
			fsize += n2;
		}
		else
		{
			n2 = 0;
		}
		if (n2 != n)
		{
			break;
		}
	}
	if (buf == NULL)
	{
		free (p);
	}
	if (ret)
	{
		*ret = fsize;
	}
	return wcs_OK;
}

/*============================================================================*/
/* func wcs_Null_Fwrite */

size_t wcs_Null_Fwrite (const void *buf, size_t size, size_t nmemb, void *self)
{
	return nmemb;
}

wcs_Writer wcs_Discard = {
	NULL, wcs_Null_Fwrite
};

/*============================================================================*/
/* func wcs_Null_Log */

void wcs_Null_Log (const char *fmt, ...)
{
}

/*============================================================================*/
/* func wcs_Stderr_Info/Warn */

static const char *wcs_Levels[] = {
	"[DEBUG]",
	"[INFO]",
	"[WARN]",
	"[ERROR]",
	"[PANIC]",
	"[FATAL]"
};

void wcs_Logv (wcs_Writer w, int ilvl, const char *fmt, wcs_Valist * args)
{
	const char *level = wcs_Levels[ilvl];
	wcs_Buffer log;
	wcs_Buffer_Init (&log, 512);
	wcs_Buffer_Write (&log, level, strlen (level));
	wcs_Buffer_PutChar (&log, ' ');
	wcs_Buffer_AppendFormatV (&log, fmt, args);
	wcs_Buffer_PutChar (&log, '\n');
	w.Write (log.buf, 1, log.curr - log.buf, w.self);
	wcs_Buffer_Cleanup (&log);
}

void wcs_Stderr_Info (const char *fmt, ...)
{
	wcs_Valist args;
	va_start (args.items, fmt);
	wcs_Logv (wcs_Stderr, wcs_Linfo, fmt, &args);
}

void wcs_Stderr_Warn (const char *fmt, ...)
{
	wcs_Valist args;
	va_start (args.items, fmt);
	wcs_Logv (wcs_Stderr, wcs_Lwarn, fmt, &args);
}

void wcs_Log_Init(char *logConfigFile, FILE *file)
{
	unsigned int logLevel = 5;
	unsigned int writeLogFile = 0;
	char logFileName[LOG_FILE_LEN];
	memset(logFileName, 0, sizeof(logFileName));
	if (logConfigFile)
	{
		logLevel = read_profile_int("SDKLogConfig","LOG_LEVEL", 5, logConfigFile);
		writeLogFile =read_profile_int("SDKLogConfig","WRITE_FILE", 0, logConfigFile);
		read_profile_string("SDKLogConfig", "LOG_FILE", logFileName, LOG_FILE_LEN, "./sdktest.log", logConfigFile);
	}
	log_set_level(logLevel);
	if (writeLogFile)
	{
	    file = fopen(logFileName, "a"); 
	    log_set_fp(file);
	}
}
void wcs_close_Logfile(FILE *file)
{
	if (file)
	{
		fclose(file);
		file = NULL;
	}
}



/*============================================================================*/
