/*
 * Copyright (c) 20014-2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @{
 */


#ifndef __WALLPAPER_UI_SERVICE_DEBUG_H__
#define __WALLPAPER_UI_SERVICE_DEBUG_H__

#include <stdio.h>

#define FONT_CLEAR          "\033[0m"
#define FONT_COLOR_RED      "\033[31m"

#define WALLPAPER_UI_SERVICE_DEBUG

#ifdef WALLPAPER_UI_SERVICE_DEBUG
#ifndef LOG_TAG
#define LOG_TAG "WALLPAPER"
#endif
#include <dlog.h>

#define HAPI __attribute__ ((visibility ("hidden")))

#define WALLPAPERUI_DBG(fmt, args...)  LOGD("["LOG_TAG"%s:%d:D] "fmt,  __func__, __LINE__, ##args)
#define WALLPAPERUI_WARN(fmt, args...) LOGW("["LOG_TAG"%s:%d:W] "fmt,  __func__, __LINE__, ##args)
#define WALLPAPERUI_ERR(fmt, args...)  LOGE("["LOG_TAG"%s:%d:E] "fmt,  __func__, __LINE__, ##args)

#else
#define WALLPAPERUI_DBG(fmt, args...) do{printf("[WALLPAPERUI_DBG][%s(%d)] "fmt " \n", __FILE__, __LINE__, ##args);}while(0);
#define WALLPAPERUI_WARN(fmt, args...) do{printf("[WALLPAPERUI_WARN][%s(%d)] "fmt " \n", __FILE__, __LINE__, ##args);}while(0);
#define WALLPAPERUI_ERR(fmt, args...) do{printf("[WALLPAPERUI_ERR][%s(%d)] "fmt " \n", __FILE__, __LINE__, ##args);}while(0);
#endif				/* LOCKD_USING_PLATFORM_DEBUG */

#define WALLPAPERUI_TRACE_BEGIN do {\
		{\
			LOGW("ENTER FUNCTION: %s.\n", __FUNCTION__);\
		}\
	}while(0);

#define WALLPAPERUI_TRACE_END do {\
		{\
			LOGW("EXIT FUNCTION: %s.\n", __FUNCTION__);\
		}\
	}while(0);

#define retvm_if(expr, val, fmt, arg...) do { \
	if(expr) { \
		WALLPAPERUI_ERR(fmt, ##arg); \
		WALLPAPERUI_ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
		return (val); \
	} \
} while (0)

#define retv_if(expr, val) do { \
	if(expr) { \
		WALLPAPERUI_ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
		return (val); \
	} \
} while (0)

#define retm_if(expr, fmt, arg...) do { \
	if(expr) { \
		WALLPAPERUI_ERR(fmt, ##arg); \
		WALLPAPERUI_ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
		return; \
	} \
} while (0)

#define ret_if(expr) do { \
	if(expr) { \
		WALLPAPERUI_ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
		return; \
	} \
} while (0)

#define __FREE(del, arg) do { \
			if(arg) { \
				del((void *)(arg)); /*cast any argument to (void*) to avoid build warring*/\
				arg = NULL; \
			} \
		} while (0);
#define FREE(arg) __FREE(free, arg)

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif				/* __WALLPAPER_UI_SERVICE_DEBUG_H__ */


/**
* @}
*/
/* End of file */
