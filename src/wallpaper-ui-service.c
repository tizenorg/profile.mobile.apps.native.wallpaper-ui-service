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


#include <app.h>
#include <Elementary.h>
#include <Evas.h>
#include <vconf.h>
#include <system_settings.h>
#include <app_alarm.h>
#include <media_content.h>
#include <fcntl.h>
#include <tzplatform_config.h>
//#include <dbus/dbus.h>
#include <app_control_internal.h>

#include "wallpaper-ui-service.h"
#include "wallpaper-ui-service-main.h"

#define WALLPAPER_FILE_PATH_HOME_N_LOCK tzplatform_mkpath(TZ_USER_SHARE, "lockscreen/wallpaper_list/home_n_lock.jpg")

static bool flag_view_exist = false;

#define DBUS_HOME_BUS_NAME "org.tizen.coreapps.home"
#define DBUS_HOME_RAISE_PATH "/Org/Tizen/Coreapps/home/raise"
#define DBUS_HOME_RAISE_INTERFACE DBUS_HOME_BUS_NAME".raise"
#define DBUS_HOME_RAISE_MEMBER "homeraise"

static bool _g_is_system_init = false;

/**
* The event process when win object is destroyed
*/

static char *_str_error_db(int error)
{
	WALLPAPERUI_TRACE_BEGIN;

	switch (error) {
	case MEDIA_CONTENT_ERROR_INVALID_PARAMETER:
		return "Invalid parameter";
	case MEDIA_CONTENT_ERROR_OUT_OF_MEMORY:
		return "Out of memory";
	case MEDIA_CONTENT_ERROR_DB_FAILED:
		return "DB operation failed";
	default:
		{
			static char buf[40];
			snprintf(buf, sizeof(buf), "Error Code=%d", error);
			return buf;
		}
	}
    WALLPAPERUI_TRACE_END;
}
static void _essential_system_db_init(void)
{
	WALLPAPERUI_TRACE_BEGIN;

	if (_g_is_system_init == true) {
		WALLPAPERUI_ERR("_g_is_system_init == true");
		return;
	}
	_g_is_system_init = true;

	WALLPAPERUI_DBG("media_content_connect");
	if (media_content_connect() != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("media_content_connect is FAILED .....");
	}
    WALLPAPERUI_TRACE_END;
}

static void _essential_system_db_destroy(void)
{
	WALLPAPERUI_TRACE_BEGIN;

	if (_g_is_system_init == false) {
		WALLPAPERUI_ERR("_g_is_system_init == false");
		return;
	}
	_g_is_system_init = false;

	WALLPAPERUI_DBG("media_content_disconnect");
	media_content_disconnect();
    WALLPAPERUI_TRACE_END;
}

static bool _wallpaper_db_create(void)
{
	WALLPAPERUI_TRACE_BEGIN;

	int ret = MEDIA_CONTENT_ERROR_NONE;

	ret = media_content_connect();
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("media_content_connect is failed, err:%s", _str_error_db(ret));
		return false;
	}
    WALLPAPERUI_TRACE_END;
	return true;
}

static bool _wallpaper_db_destroy(void)
{
	WALLPAPERUI_TRACE_BEGIN;

	int ret = MEDIA_CONTENT_ERROR_NONE;

	ret = media_content_disconnect();
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("media_content_disconnect is failed, err:%s", _str_error_db(ret));
		return false;
	}
    WALLPAPERUI_TRACE_END;
	return true;
}

static void _reply_to_sender(void *data, int result)
{
	WALLPAPERUI_TRACE_BEGIN;
	ret_if(data == NULL);

	wallpaper_ui_service_appdata *ad = (wallpaper_ui_service_appdata *)data;

	app_control_h svc;

	if (app_control_create(&svc) == 0) {
		WALLPAPERUI_DBG("reply to caller :: app_control_reply_to_launch_request(%d)", result);
		app_control_reply_to_launch_request(svc, ad->service, result);
		app_control_destroy(svc);
	}
    WALLPAPERUI_TRACE_END;
}

static void _del_win(void *data, Evas_Object *obj, void *event)
{
	WALLPAPERUI_TRACE_BEGIN;

	ui_app_exit();
}

static void _win_rot_changed_cb(void *data, Evas_Object *obj, void *event)
{
	WALLPAPERUI_TRACE_BEGIN;
	int changed_ang = elm_win_rotation_get(obj);

	switch (changed_ang) {
		case APP_DEVICE_ORIENTATION_0:
			break;
		case APP_DEVICE_ORIENTATION_90:
			break;
		case APP_DEVICE_ORIENTATION_180:
			break;
		case APP_DEVICE_ORIENTATION_270:
			break;
	}
	WALLPAPERUI_TRACE_END;
}

static Evas_Object *_create_win(const char *name, bool transparent)
{
	WALLPAPERUI_TRACE_BEGIN;
	Evas_Object *eo;
	int w, h;

	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);

	if (eo) {
		elm_win_title_set(eo, name);
		elm_win_conformant_set(eo, EINA_TRUE);

		if (transparent) {
			elm_win_alpha_set(eo, EINA_TRUE);

/*			unsigned int opaqueVal = 1; */
/*			Ecore_X_Atom opaqueAtom = ecore_x_atom_get("_E_ILLUME_WINDOW_REGION_OPAQUE"); */
/*			Ecore_X_Window xwin = elm_win_xwindow_get(eo); */
/*			ecore_x_window_prop_card32_set(xwin, opaqueAtom, &opaqueVal, 1); */
		}

		evas_object_smart_callback_add(eo, "delete,request", _del_win, NULL);

		elm_win_screen_size_get(eo, NULL, NULL, &w, &h);

		evas_object_resize(eo, w, h);

		if (transparent) {
			_disable_effect(eo);
			elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_HIDE);
		} else {
			elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_SHOW);
			elm_win_indicator_opacity_set(eo, ELM_WIN_INDICATOR_TRANSPARENT);
		}

		evas_object_show(eo);
		elm_win_activate(eo);
	}

	return eo;
}

static void _create_conformant(Evas_Object *win, Evas_Object *layout)
{
	WALLPAPERUI_TRACE_BEGIN;
	ret_if(!win);

	Evas_Object *conform = elm_conformant_add(win);
	evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(conform, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_win_resize_object_add(win, conform);
	elm_object_content_set(conform, layout);
	evas_object_show(conform);
	elm_win_conformant_set(win, EINA_TRUE);

	/*indicator bg */
	Evas_Object *indicator_bg = elm_bg_add(conform);
	elm_object_style_set(indicator_bg, "indicator/headerbg");
	elm_object_part_content_set(conform, "elm.swallow.indicator_bg", indicator_bg);
	evas_object_show(indicator_bg);

	WALLPAPERUI_TRACE_END;
}

static Evas_Object *_create_main_layout(Evas_Object *win, const char *edj_path, const char *group)
{
	WALLPAPERUI_TRACE_BEGIN;
	retv_if(!win, NULL);

	Evas_Object *layout = NULL;

	layout = elm_layout_add(win);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	if (edj_path) {
		elm_layout_file_set(layout, edj_path, group);
	} else {
		elm_layout_theme_set(layout, "layout", "application", "default");
	}

	_create_conformant(win, layout);

	WALLPAPERUI_TRACE_END;
	return layout;
}

HAPI int wallpaper_ui_service_copy_wallpaper_file(char *source, char *destination)
{
	WALLPAPERUI_TRACE_BEGIN;

	if (source == NULL || destination == NULL) {
		WALLPAPERUI_ERR("file error");
		return 0;
	}

	if (ecore_file_exists(source) == EINA_FALSE) {
		WALLPAPERUI_ERR("source error %s", source);
		return 0;
	}

	if (strcmp(source, destination) != 0) {
		ecore_file_remove(destination);
		if (EINA_FALSE == ecore_file_cp(source, destination)) {
			WALLPAPERUI_ERR("ecore_file_cp fail");
			return 0;
		}
	}

	WALLPAPERUI_TRACE_END;
	return 1;
}

static int _wallpaper_txt_write(char *filename, char *path_array)
{
	WALLPAPERUI_TRACE_BEGIN;

	FILE *fp = fopen(filename, "w");
	if (!fp) {
		WALLPAPERUI_DBG("fopen wallpaper txt file failed.");
		return -1;
	}

	if (path_array) {
		WALLPAPERUI_DBG("path=%s", path_array);
		fprintf(fp, "%s\n", path_array);
		WALLPAPERUI_DBG("path=%s", path_array);
	}

	fclose(fp);

	WALLPAPERUI_TRACE_END;
	return 0;
}

static void _wallpaper_dbus_destroy(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	wallpaper_ui_service_appdata *ad = (wallpaper_ui_service_appdata *)data;
	ret_if(ad == NULL);

	if (ad->dbus_conn) {
		if (ad->dbus_home_button_handler) {
			e_dbus_signal_handler_del(ad->dbus_conn, ad->dbus_home_button_handler);
			ad->dbus_home_button_handler = NULL;
		}
		e_dbus_connection_close(ad->dbus_conn);
		e_dbus_shutdown();

		ad->dbus_conn = NULL;
	}
	WALLPAPERUI_TRACE_END;
}

static void _wallpaper_dbus_init(void)
{
	WALLPAPERUI_TRACE_BEGIN;

	e_dbus_init();

	WALLPAPERUI_TRACE_END;
}

static void _wallpaper_set_dbus_handler(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	wallpaper_ui_service_appdata *ad = (wallpaper_ui_service_appdata *)data;
	ret_if(ad == NULL);


	E_DBus_Connection *conn = NULL;

	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (conn == NULL) {
		WALLPAPERUI_ERR("e_dbus_bus_get() failed");
		return;
	}

	ad->dbus_conn = conn;
	WALLPAPERUI_TRACE_END;
}

static void _home_button_clicked_cb(void *data, DBusMessage *msg)
{
	WALLPAPERUI_TRACE_BEGIN;

	extern int scale_resize_state;
	wallpaper_ui_service_appdata *ad = (wallpaper_ui_service_appdata *)data;
	ret_if(ad == NULL);

	if (ad->sel_popup_data.launch_from == WALLPAPER_POPUP_FROM_HOME_SCREEN
		&& ad->popup_type == WALLPAPER_POPUP_TYPE_SELECTION) {
		if (scale_resize_state == 1) {
			WALLPAPERUI_ERR("Destroy wallpaper cancel");
			return;
		}
		WALLPAPERUI_ERR("Destroy wallpaper");
		elm_exit();
	}

	WALLPAPERUI_TRACE_END;
}

static void _wallpaper_register_home_button_cb(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	wallpaper_ui_service_appdata *ad = (wallpaper_ui_service_appdata *)data;
	ret_if(ad == NULL);
	ret_if(ad->dbus_conn == NULL);

	E_DBus_Signal_Handler *handler = NULL;

	handler = e_dbus_signal_handler_add(ad->dbus_conn, NULL, "/Org/Tizen/Coreapps/home/raise",
					"org.tizen.coreapps.home.raise", "homeraise",
					_home_button_clicked_cb, ad);
	if (handler == NULL) {
		WALLPAPERUI_ERR("e_dbus_signal_handler_add() failed");
		_wallpaper_dbus_destroy(ad);
		return;
	}
	ad->dbus_home_button_handler = handler;

	WALLPAPERUI_TRACE_END;
}

void *_register_view(app_control_h service, void *data)
{
	WALLPAPERUI_TRACE_BEGIN;
	retv_if(!data, NULL);
	wallpaper_ui_service_appdata *ad = (wallpaper_ui_service_appdata *)data;
	retv_if(!ad, NULL);

	char *from = NULL;
	char *popup_type = NULL;
	char *setas = NULL;
	app_control_get_extra_data(service, EXTRA_KEY_POPUP_TYPE, &popup_type);

	if (popup_type != NULL) {
		WALLPAPERUI_ERR("popup_type %s", popup_type);
	}

	if (popup_type != NULL) {
		ad->popup_type = WALLPAPER_POPUP_TYPE_SELECTION;

		free(popup_type);
	} else {
		ad->popup_type = WALLPAPER_POPUP_TYPE_SELECTION;
	}
	app_control_get_extra_data(service, EXTRA_FROM_KEY, &from);
	WALLPAPERUI_ERR("from %s", from);

	if (from != NULL) {
		if (strcmp(from, "Homescreen") == 0) {
			ad->sel_popup_data.launch_from = WALLPAPER_POPUP_FROM_HOME_SCREEN;
		} else if (strcmp(from, "Setting") == 0) {
			ad->sel_popup_data.launch_from = WALLPAPER_POPUP_FROM_SETTING;
		} else {
			ad->sel_popup_data.launch_from = WALLPAPER_POPUP_FROM_LOCK_SCREEN;
		}
		free(from);
	} else {
		ad->sel_popup_data.launch_from = WALLPAPER_POPUP_FROM_GALLERY;
	}

	app_control_get_extra_data(service, EXTRA_KEY_SETAS, &setas);

	if (setas == NULL) {
		WALLPAPERUI_ERR("setas (%s) failed", setas);
	}

	if (setas != NULL) {
		WALLPAPERUI_DBG("setas is (%s)", setas);
		if (strcmp(setas, EXTRA_DATA_HOMESCREEN) == 0) {
			ad->sel_popup_data.setas_type = WALLPAPER_POPUP_SETAS_HOMESCREEN;
		} else if (strcmp(setas, EXTRA_DATA_LOCKSCREEN) == 0) {
			char *wallpaper_type = NULL;
			ad->sel_popup_data.setas_type = WALLPAPER_POPUP_SETAS_LOCKSCREEN;

			app_control_get_extra_data(service, EXTRA_KEY_WALLPAPER_TYPE, &wallpaper_type);

			if (wallpaper_type != NULL) {
				WALLPAPERUI_ERR("wallpaper_type (%s) failed", wallpaper_type);
				if (strcmp(wallpaper_type, EXTRA_DATA_WP_DEFAULT) == 0) {
					ad->lock_wallpaper_type = WALLPAPER_TYPE_DEFAULT;
				} else if (strcmp(wallpaper_type, EXTRA_DATA_WP_MULTI) == 0) {
					ad->lock_wallpaper_type = WALLPAPER_TYPE_MULTIPLE;
				} else {
					ad->lock_wallpaper_type = WALLPAPER_TYPE_GALLERY;
				}
				free(wallpaper_type);
			} else {
				ad->lock_wallpaper_type = WALLPAPER_TYPE_GALLERY;
			}
		} else {
			ad->sel_popup_data.setas_type = WALLPAPER_POPUP_SETAS_HOME_N_LOCKSCREEN;
		}
		free(setas);
	} else {
		WALLPAPERUI_ERR("app_control_get_extra_data(%s) failed", EXTRA_KEY_SETAS);
		ad->sel_popup_data.setas_type = WALLPAPER_POPUP_SETAS_LOCKSCREEN;
	}


	if (ad->popup_type == WALLPAPER_POPUP_TYPE_SELECTION) {
		ad->sel_popup_data.win_main = ad->win;

		/*popup_wallpaper_main_create_view(data); */
		wallpaper_main_create_view(data);
	} else if (ad->popup_type == WALLPAPER_POPUP_TYPE_THEME) {
/*		WALLPAPERUI_ERR("EXTRA_FROM_KEY(%s) failed", from); */

		char *file_name = NULL;
		app_control_get_extra_data(service, EXTRA_KEY_FILE, &file_name);
		if (file_name == NULL) {
			WALLPAPERUI_ERR("app_control_get_extra_data(%s) failed", EXTRA_KEY_FILE);
			if (ad->sel_popup_data.setas_type == WALLPAPER_POPUP_SETAS_LOCKSCREEN) {
				if (system_settings_get_value_string(SYSTEM_SETTINGS_KEY_WALLPAPER_LOCK_SCREEN, &file_name) != SYSTEM_SETTINGS_ERROR_NONE) {
					WALLPAPERUI_ERR("system_settings_get_value_string() failed");
				}
			} else {
				if (system_settings_get_value_string(SYSTEM_SETTINGS_KEY_WALLPAPER_HOME_SCREEN, &file_name) != SYSTEM_SETTINGS_ERROR_NONE) {
					WALLPAPERUI_ERR("system_settings_get_value_string() failed");
				}
			}

			if (file_name == NULL) {
				WALLPAPERUI_ERR("CIRITICAL ERROR : wallpaper file is NULL");
				elm_exit();
				return NULL;
			}
		}
		ad->color_popup_data.file_path = strdup(file_name);
		free(file_name);
		if (ad->color_popup_data.file_path == NULL) {
			WALLPAPERUI_ERR("CIRITICAL ERROR : strdup() failed");
/*			elm_exit(); */
		}

	}

	WALLPAPERUI_TRACE_END;
	return NULL;
}

/**
* The function is called when Setting is terminated
*/
static void _app_terminate(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;
	ret_if(!data);

	wallpaper_ui_service_appdata *ad = data;

	_wallpaper_dbus_destroy(data);
	_essential_system_db_destroy();

	WALLPAPERUI_DBG("fingerprint_manager_terminate!");
	_wallpaper_db_destroy();

	flag_view_exist = false;

	if (ad->pd) {
		free(ad->pd);
		ad->pd = NULL;
	}

	if (ad->win) {
		evas_object_del(ad->win);
		ad->win = NULL;
	}
	feedback_deinitialize();
	elm_exit();

	WALLPAPERUI_TRACE_END;
}

/**
* The function is called to create Setting view widgets
*/
static bool _app_create(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	elm_config_preferred_engine_set("opengl_x11");
	elm_app_base_scale_set(2.4);

	bindtextdomain(PKGNAME, "/usr/apps/org.tizen.wallpaper-ui-service/res/locale");

	_essential_system_db_init();

	_wallpaper_db_create();

	WALLPAPERUI_TRACE_END;
	return TRUE;
}

/**
* The function is called when Setting begins run in background from forground
*/
static void _app_pause(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;
	char *value = NULL;

	if (system_settings_get_value_string(SYSTEM_SETTINGS_KEY_WALLPAPER_LOCK_SCREEN, &value) != SYSTEM_SETTINGS_ERROR_NONE) {
		WALLPAPERUI_ERR("system_settings_get_value_string() failed");
	}
	WALLPAPERUI_DBG("value = %s", value);
	/* terminate */
}

/**
* The function is called when Setting begins run in forground from background
*/
static void _app_resume(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;
	wallpaper_ui_service_appdata *ad = (wallpaper_ui_service_appdata *)data;
	char *value = NULL;
	Evas_Coord w;
	Evas_Coord h;

	if (system_settings_get_value_string(SYSTEM_SETTINGS_KEY_WALLPAPER_LOCK_SCREEN, &value) != SYSTEM_SETTINGS_ERROR_NONE) {
		WALLPAPERUI_ERR("system_settings_get_value_string() failed");
	}
	WALLPAPERUI_DBG("value = %s", value);

	evas_object_geometry_get(ad->main_layout, NULL, NULL, &w, &h);

	WALLPAPERUI_DBG("main_layout W = %d, H = %d", w, h);

	WALLPAPERUI_TRACE_END;
}

static void _app_reset(app_control_h service, void *data)
{
	WALLPAPERUI_TRACE_BEGIN;
	ret_if(!data);
	int bTransparent = 0;
	wallpaper_ui_service_appdata *ad = data;
	char *popup_type = NULL;
	app_control_get_extra_data(service, EXTRA_KEY_POPUP_TYPE, &popup_type);

	feedback_initialize();

	/* clone service */
	app_control_clone(&(ad->service), service);

	ad->popup_type = WALLPAPER_POPUP_TYPE_SELECTION;

	if (ad->win != NULL && ad->popup_type == WALLPAPER_POPUP_TYPE_SELECTION) {
		WALLPAPERUI_DBG("ALREADY EXIST");
		return;
/*		evas_object_del(ad->win); */
/*		ad->win = NULL; */
	}

	if (ad->popup_type == WALLPAPER_POPUP_TYPE_THEME) {
		bTransparent = 1;
	}

    /* create window */
    ad->win = _create_win("org.tizen.setting.wallpaper-ui-service", bTransparent);
    if (ad->win == NULL) {
        WALLPAPERUI_DBG("Can't create window");
        return FALSE;
    }

    flag_view_exist = false;

    ad->evas = evas_object_evas_get(ad->win);
    ad->layout = _create_main_layout(ad->win, NULL, NULL);

	elm_theme_extension_add(NULL, EDJDIR"/button_customized_theme.edj");

	if (ad->win && flag_view_exist && popup_type && ad->popup_type == WALLPAPER_POPUP_TYPE_SELECTION) {
		elm_win_activate(ad->win);
	} else {
		_register_view(service, ad);
		flag_view_exist = true;
	}

	_wallpaper_dbus_init();
	_wallpaper_set_dbus_handler(ad);
	_wallpaper_register_home_button_cb(ad);

	if (popup_type) {
		free(popup_type);
	}

	WALLPAPERUI_TRACE_END;
}

static void update_text(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	wallpaper_ui_service_appdata *ad = (wallpaper_ui_service_appdata *) data;
	ret_if(ad == NULL);

	Evas_Object *cancel_button;
	Evas_Object *done_button;
	Evas_Object *obj;

	if (ad->main_nf_it) {
		elm_object_item_text_set(ad->main_nf_it, APP_STRING("IDS_LCKSCN_MBODY_WALLPAPERS"));
		Evas_Object *cancel_button = elm_object_item_part_content_get(ad->main_nf_it, "title_left_text_btn");
		if (cancel_button) {
			elm_object_text_set(cancel_button, APP_STRING("IDS_TPLATFORM_ACBUTTON_CANCEL_ABB"));
		}
		Evas_Object *done_button = elm_object_item_part_content_get(ad->main_nf_it, "title_right_text_btn");
		if (done_button) {
			elm_object_text_set(done_button, APP_STRING("IDS_TPLATFORM_ACBUTTON_DONE_ABB"));
		}
	}

	WALLPAPERUI_TRACE_END;
}

static void _app_lang_changed(app_event_info_h event_info, void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	wallpaper_ui_service_appdata *ad = (wallpaper_ui_service_appdata *) data;
	ret_if(ad == NULL);

	char *lang = NULL;

	lang = vconf_get_str(VCONFKEY_LANGSET);
	if (lang)	{
		elm_language_set((const char *)lang);
		FREE(lang);
	}

	update_text(data);

	WALLPAPERUI_TRACE_END;
}


/* End : Support to enable fingerprint lock */
HAPI int main(int argc, char *argv[])
{
	WALLPAPERUI_TRACE_BEGIN;
	int r = 0;
	wallpaper_ui_service_appdata ad;
	app_event_handler_h handlers[5] = {NULL,};

	ui_app_lifecycle_callback_s ops = {
		.create = _app_create,
		.terminate = _app_terminate,
		.pause = _app_pause,
		.resume = _app_resume,
		.app_control = _app_reset,
	};

	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, _app_lang_changed, NULL);

	memset(&ad, 0x00, sizeof(wallpaper_ui_service_appdata));

	r = ui_app_main(argc, argv, &ops, &ad);
	WALLPAPERUI_DBG("r = %d", r);

	if (r == -1) {
		WALLPAPERUI_ERR("ui_app_main() returns -1");
		return -1;
	}

	WALLPAPERUI_TRACE_END;
	return 0;
}

/**
* @}
*/
/* End of file */

