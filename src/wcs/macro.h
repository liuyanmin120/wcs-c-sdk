/*
 ============================================================================
 Name        : conf.h
 Author      : wcs.com
 Copyright   : 2017(c) Xiamen wangsu Technologies Co., Ltd
 Description : 
 ============================================================================
 */

#ifndef WCS_MACRO_H
#define WCS_MACRO_H

#if defined(USING_WCS_LIBRARY_DLL)
#define WCS_DLLAPI __declspec(dllimport)
#elif defined(COMPILING_WCS_LIBRARY_DLL)
#define WCS_DLLAPI __declspec(dllexport)
#else
#define WCS_DLLAPI
#endif

#endif
