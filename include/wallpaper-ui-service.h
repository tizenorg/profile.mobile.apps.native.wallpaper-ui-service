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
#define MAX_MULTIPLE_SELECTION 6


#if !defined(PACKAGEID)
#define PACKAGEID "org.tizen.wallpaper-ui-service"
#endif

#define EXTRA_KEY_FILE "file_path"
#define EXTRA_KEY_POPUP_TYPE "popup_type"

typedef enum {
	WALLPAPER_POPUP_TYPE_NONE = 0,
	WALLPAPER_POPUP_TYPE_SELECTION,
	WALLPAPER_POPUP_TYPE_THEME,
} wallpaper_popup_type;

#define EXTRA_KEY_WALLPAPER_TYPE "wallpaper_type"

#define EXTRA_DATA_WP_DEFAULT "default"
#define EXTRA_DATA_WP_MULTI	"multi"

typedef enum {
	WALLPAPER_TYPE_DEFAULT = 0,
	WALLPAPER_TYPE_GALLERY,
	WALLPAPER_TYPE_MULTIPLE,
	WALLPAPER_TYPE_MAX
} wallpaper_img_type;


#define EXTRA_FROM_KEY "from"

typedef enum {
	WALLPAPER_POPUP_FROM_NONE = 0,
	WALLPAPER_POPUP_FROM_HOME_SCREEN,
	WALLPAPER_POPUP_FROM_LOCK_SCREEN,
	WALLPAPER_POPUP_FROM_SETTING,
	WALLPAPER_POPUP_FROM_GALLERY,
} wallpaper_popup_from;


#define EXTRA_KEY_SETAS "setas-type"

#define EXTRA_DATA_HOMESCREEN "Homescreen"
#define EXTRA_DATA_LOCKSCREEN	"Lockscreen"
#define EXTRA_DATA_HOME_N_LOCKSCREEN	"Home&Lockscreen"

typedef enum {
	WALLPAPER_POPUP_SETAS_NONE = 0,
	WALLPAPER_POPUP_SETAS_HOMESCREEN,
	WALLPAPER_POPUP_SETAS_LOCKSCREEN,
	WALLPAPER_POPUP_SETAS_HOME_N_LOCKSCREEN
} wallpaper_popup_menu;


#define DiSABLE_CROP_VIEW 0
#define DISABLE_MULTISELECTION 1

typedef struct _popup_colortheme_data {
	Evas_Object *popup;

	int theme_index;
	char *file_path;
} popup_colortheme_data;

typedef struct _popup_wallpaper_data {
	Evas_Object *win_main;
	Evas_Object *base;

	Evas_Object *act_pop;
	int launch_from;
	int setas_type;
} popup_wallpaper_data;

typedef struct page_data {
	Evas_Object *scroller;
	Evas_Object *index;
	Evas_Object *page[2];
	Elm_Object_Item *last_it;
	Elm_Object_Item *new_it;
	int current_page;
	Evas_Object *home_icon;
	Evas_Object *lockscreen_icon;
	Evas_Object *main_layout;
} page_data_s;

typedef struct preview_page_data {
	Evas_Object *scroller;
	Evas_Object *page[6];
	int current_page;
} preview_page_data_s;


typedef struct _wallpaper_ui_service_appdata {

	Evas_Object *navi_bar;
	Evas *evas;
	Evas_Object *win;
	Evas_Object *layout;
	int request_type;

	app_control_h service; 	// clone service to reply when terminate
	popup_wallpaper_data sel_popup_data;
	popup_colortheme_data color_popup_data;
	int popup_type;
	int lock_wallpaper_type;

	unsigned int caller_win_id;
	unsigned int wallpaper_win_id;
	page_data_s *pd;
	Evas_Object *main_layout;
	Evas_Object *thumnail_layout;
	Evas_Object *box;
	Evas_Object *popup;
	Evas_Object *preview_image;
	int preview_image_type;
	Elm_Object_Item *main_nf_it;
    Evas_Object *gengrid;

	E_DBus_Connection *dbus_conn;
	E_DBus_Signal_Handler *dbus_home_button_handler;

    char* last_preview_img_path;
    char current_preview_img_path[MAX_LENGTH_LINE];
	char saved_img_path[6][MAX_LENGTH_LINE];
} wallpaper_ui_service_appdata;

HAPI int wallpaper_ui_service_copy_wallpaper_file(const char *source, char *destination);

const char *wallpaper_ui_service_get_icon_path(const char *fileName);
const char *wallpaper_ui_service_get_edj_path(const char *fileName);
const char *wallpaper_ui_service_get_settings_wallpapers_path();
#endif


/**
* @}
*/
/* End of file */
