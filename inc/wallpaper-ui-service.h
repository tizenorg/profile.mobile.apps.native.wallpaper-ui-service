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

#ifndef __WALLPAPER_UI_SERVICE_H__
#define __WALLPAPER_UI_SERVICE_H__

#include <app.h>
#include <Elementary.h>
#include <Eina.h>
#include <Evas.h>
#include "wallpaper-ui-service-debug.h"
#include <E_DBus.h>

#define PKGNAME "wallpaper-ui-service"

//#define _X(x) (x*elm_config_scale_get())
#define _EDJ(o)					elm_layout_edje_get(o)
#define SYS_STRING(str)			dgettext("sys_string", str)
#define APP_STRING(str)			dgettext(PKGNAME, str)
#define NOT_LOCALIZED(str)		(str)

#define _NOT_LOCALIZED(str) (str)

#define GENGRID_ITEM_SIZE (126)
#define MAX_LENGTH_LINE 2001
#define MAX_LENGTH_STRING 4096


#if !defined(PACKAGEID)
#define PACKAGEID "org.tizen.wallpaper-ui-service"
#endif

typedef enum {
	WALLPAPER_TYPE_DEFAULT = 0,
	WALLPAPER_TYPE_GALLERY,
	WALLPAPER_TYPE_MAX
} wallpaper_img_type;

typedef struct _wallpaper_ui_service_appdata {

	Evas_Object *navi_bar;
	Evas *evas;
	Evas_Object *win;
	Evas_Object *layout;
	Evas_Object *main_layout;
	Evas_Object *preview_image;
	int preview_image_type;
    Evas_Object *gengrid;

    // DBus doesn't work any more in this case. Replace this in scope of TizenRefApp-6828
	E_DBus_Connection *dbus_conn;
	E_DBus_Signal_Handler *dbus_home_button_handler;

    char* last_preview_img_path;
    char current_preview_img_path[MAX_LENGTH_LINE];
	char saved_img_path[MAX_LENGTH_LINE];
} wallpaper_ui_service_appdata;

HAPI int wallpaper_ui_service_copy_wallpaper_file(const char *source, char *destination);

char *wallpaper_ui_service_get_icon_path(const char *fileName);
char *wallpaper_ui_service_get_edj_path(const char *fileName);
const char *wallpaper_ui_service_get_settings_wallpapers_path();
const char *get_working_dir();
int get_max_prescale_img_size(wallpaper_ui_service_appdata *app);
#endif


/**
* @}
*/
/* End of file */
