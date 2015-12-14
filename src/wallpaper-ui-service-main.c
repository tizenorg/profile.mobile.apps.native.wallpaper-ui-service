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
#include <efl_extension.h>
#include <system_settings.h>
#include <media_content.h>
#include <feedback.h>
#include <app_control_internal.h>
#include <tzplatform_config.h>

#include "wallpaper-ui-service.h"
#include "wallpaper-ui-service-main.h"

#define DEFAULT_IMAGE_DIR tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.setting/shared/res/settings/Wallpapers")
#define DEFAULT_IMAGE_DIR_TEMPLATE tzplatform_mkpath(TZ_SYS_RO_APP, "org.tizen.setting/shared/res/settings/Wallpapers/%s")

static Elm_Gengrid_Item_Class *gic_for_main = NULL;
static wallpaper_ui_service_appdata *ad = NULL;
static Elm_Object_Item *nf_it = NULL;

static struct _wallpaper_ui_service_state_data {
    Eina_Bool flag_changed;
    Eina_Bool flag_edit_click;
    Eina_Bool flag_image_from_gallery;
	char *from[MAX_LENGTH_LINE];
	char *to[MAX_LENGTH_LINE];
} state_data = {
    .flag_changed = EINA_FALSE,
    .flag_edit_click = EINA_FALSE,
    .flag_image_from_gallery = EINA_FALSE,
	.from = { NULL, },
	.to = { NULL, },
};


int scale_resize_state = 0;

enum {
	SCALE_JOB_INIT = -1,
	SCALE_JOB_0_BUFFER_MAKING = 0,
	SCALE_JOB_1_RESIZE_START,
	SCALE_JOB_2_RENDERING,
	SCALE_JOB_3_BUFFER_GET,
	SCALE_JOB_4_RESIZE_IMAGE,
	SCALE_JOB_5_SAVE_IMAGE,
	SCALE_JOB_END,

	SCALE_JOB_ERR,
};

struct scaledata {
	int img_idx;
	int to_w;
	int to_h;

	int curr_job;
	int next_job;

	int img_w;
	int img_h;

	Ecore_Evas *ee;
	Evas_Object *img;

	Evas *canvas;
	void *pixels;
	Evas_Object *o;

	Ecore_Idler *idler_handler;
};

static void _wallpaper_destroy(void *data);
static Evas_Object *main_gengrid_add(Evas_Object *parent, void *data);
static int _lockscreen_gallery_scale_job_maker(int to_w, int to_h, int idx);
static void _lockscreen_gallery_destroy_func();
static void _wallpaper_show_focus_highlight(int selected_index) ;
static void _done_to_set_wallpaper();
static void _wallpaper_back_key_cb(void *data, Evas_Object *obj, void *event_info);
static void _preview_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _wallpaper_preview_main();
HAPI int wallpaper_ui_service_copy_wallpaper_file(const char *source, char *destination);

static int _lockscreen_gallery_file_cb(const char *src, const char *dst)
{
	WALLPAPERUI_TRACE_BEGIN;

	FILE *f1, *f2;
	char buf[16384];
	size_t num;
	int ret = 1;

	f1 = fopen(src, "rb");
	if (!f1) {
		return 0;
	}

	f2 = fopen(dst, "wb");
	if (!f2) {
		fclose(f1);
		return 0;
	}

	while ((num = fread(buf, 1, sizeof(buf), f1)) > 0) {
		if (fwrite(buf, 1, num, f2) != num) {
			ret = 0;

		}
	}

	fclose(f1);
	fclose(f2);

	WALLPAPERUI_TRACE_END;
	return ret;
}

static void _rotate_right(Evas_Object *image)
{
	WALLPAPERUI_TRACE_BEGIN;
	ret_if(image == NULL);

	unsigned int *data1 = NULL;
	unsigned int *data2 = NULL;
	unsigned int *to = NULL;
	unsigned int *from = NULL;
	int x = 0;
	int y = 0;
	int w = 0;
	int hw = 0;
	int iw = 0;
	int ih = 0;
	int size = 0;

	evas_object_image_size_get(image, &iw, &ih);
	WALLPAPERUI_DBG("iw = %d,ih = %d", iw, ih);
	size = iw * ih * sizeof(unsigned int);

	/* EINA_FALSE for reading */
	data1 = (unsigned int *)evas_object_image_data_get(image, EINA_FALSE);
	if (data1 == NULL) {
		WALLPAPERUI_DBG("evas_object_image_data_get : data1 failed");
		return;
	}

	/* memcpy */
	data2 = (unsigned int *)calloc(1, size);
	if (data2 == NULL) {
		WALLPAPERUI_DBG("calloc data2 failed");
		return;
	}

	memcpy(data2, data1, size);

	/* set width, height */
	w = ih;
	ih = iw;
	iw = w;
	hw = w * ih;

	/* set width, height to image obj */
	evas_object_image_size_set(image, iw, ih);
	data1 = (unsigned int *)evas_object_image_data_get(image, EINA_TRUE);
	if (data1 == NULL) {
		WALLPAPERUI_DBG("evas_object_image_data_get : data1 failed2");
		free(data2);
		return;
	}
	to = data1 + w - 1;
	hw = -hw - 1;
	from = data2;

	for (x = iw; --x >= 0;) {
		for (y = ih; --y >= 0;) {
			*to = *from;
			from++;
			to += w;
		}
		to += hw;
	}

	if (data2) {
		free(data2);
	}
	WALLPAPERUI_DBG("final iw = %d,ih = %d", iw, ih);
	evas_object_image_data_set(image, data1);
	evas_object_image_data_update_add(image, 0, 0, iw, ih);

	WALLPAPERUI_TRACE_BEGIN;
}

static void _rotate_left(Evas_Object *image)
{
	WALLPAPERUI_TRACE_BEGIN;
	ret_if(image == NULL);

	unsigned int *data1 = NULL;
	unsigned int *data2 = NULL;
	unsigned int *to = NULL;
	unsigned int *from = NULL;
	int x = 0;
	int y = 0;
	int w = 0;
	int hw = 0;
	int iw = 0;
	int ih = 0;
	int size = 0;

	WALLPAPERUI_DBG("iw = %d,ih = %d", iw, ih);
	evas_object_image_size_get(image, &iw, &ih);
	size = iw * ih * sizeof(unsigned int);

	/* EINA_FALSE for reading */
	data1 = (unsigned int *)evas_object_image_data_get(image, EINA_FALSE);
	if (data1 == NULL) {
		WALLPAPERUI_DBG("evas_object_image_data_get : data1 failed");
		return;
	}
	data2 = (unsigned int *)calloc(1, size);
	if (data2 == NULL) {
		WALLPAPERUI_DBG("calloc data2 failed");
		return;
	}

	memcpy(data2, data1, size);

	/* set width, height */
	w = ih;
	ih = iw;
	iw = w;
	hw = w * ih;

	/* set width, height to image obj */
	evas_object_image_size_set(image, iw, ih);
	data1 = (unsigned int *)evas_object_image_data_get(image, EINA_TRUE);
	if (data1 == NULL) {
		WALLPAPERUI_DBG("evas_object_image_data_get : data1 failed2");
		free(data2);
		return;
	}

	to = data1 + hw - w;
	w = -w;
	hw = hw + 1;
	from = data2;

	for (x = iw; --x >= 0;) {
		for (y = ih; --y >= 0;) {
			*to = *from;
			from++;
			to += w;
		}

		to += hw;
	}

	if (data2) {
		free(data2);
	}
	WALLPAPERUI_DBG("final iw = %d,ih = %d", iw, ih);
	evas_object_image_data_set(image, data1);
	evas_object_image_data_update_add(image, 0, 0, iw, ih);

	WALLPAPERUI_TRACE_END;
}

static void _rotate_verteical(Evas_Object *image)
{
	WALLPAPERUI_TRACE_BEGIN;

	_rotate_left(image);
	_rotate_left(image);

	WALLPAPERUI_TRACE_END;
}

static bool _get_media_info_cb(media_info_h pItem, void *pUserData)
{
	WALLPAPERUI_TRACE_BEGIN;
	media_info_h *pAssignFolderItem = (media_info_h *)pUserData;

	if (pItem != NULL) {
		media_info_clone(pAssignFolderItem, pItem);
	}

	WALLPAPERUI_TRACE_END;
	return FALSE;
}

static media_content_orientation_e _lockscreen_gallery_get_orientation_by_path(const char *path)
{
	WALLPAPERUI_TRACE_BEGIN;

	if (!path) {
		WALLPAPERUI_ERR("No exist video path.");
		return -1;
	}

	media_info_h pItem = NULL;
	filter_h m_FilterHandle = NULL;
	char szTmpStr[MAX_LENGTH_STRING] = {0,};
	media_content_orientation_e orientation;
	image_meta_h image_handle = NULL;

	if (media_filter_create(&m_FilterHandle) != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("Fail to create media filter handle.");
		return -1;
	}

	memset(szTmpStr, 0, MAX_LENGTH_STRING);
	snprintf(szTmpStr, MAX_LENGTH_STRING, "MEDIA_PATH = \"%s\"", path);
	WALLPAPERUI_DBG("path = %s", path);
	WALLPAPERUI_DBG("szTmpStr = %s", szTmpStr);
	if (media_filter_set_condition(m_FilterHandle, szTmpStr, MEDIA_CONTENT_COLLATE_DEFAULT) != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("Fail to set filter condition.");
		media_filter_destroy(m_FilterHandle);
		return -1;
	}

	if (media_info_foreach_media_from_db(m_FilterHandle, _get_media_info_cb, &pItem) != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_DBG("Fail to media_info_foreach_media_from_db");
		media_filter_destroy(m_FilterHandle);
		return -1;
	}

	if (media_filter_destroy(m_FilterHandle) != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("Fail to destroy media filter handle.");
		return -1;
	}

	if (pItem) {
		if (media_info_get_image(pItem, &image_handle) != MEDIA_CONTENT_ERROR_NONE) {
			WALLPAPERUI_ERR("media_info_get_image Failed");
			media_info_destroy(pItem);
			return -1;
		}
		media_info_destroy(pItem);
	}


	if (image_handle) {
		if (image_meta_get_orientation(image_handle, &orientation) == MEDIA_CONTENT_ERROR_NONE) {
			image_meta_destroy(image_handle);
			return orientation;
		}
		image_meta_destroy(image_handle);
	}

	WALLPAPERUI_TRACE_END;
	return -1;
}

static void _rotate_image(Evas_Object *image, const char *path)
{
	WALLPAPERUI_TRACE_BEGIN;

	ret_if(path == NULL || image == NULL);

	media_content_orientation_e orientation = _lockscreen_gallery_get_orientation_by_path(path);
	WALLPAPERUI_DBG("orientation == %d", orientation);

	ret_if(orientation == -1);

	switch (orientation) {
		case MEDIA_CONTENT_ORIENTATION_ROT_180:
			_rotate_verteical(image);
			break;
		case MEDIA_CONTENT_ORIENTATION_ROT_90:
			_rotate_right(image);
			break;
		case MEDIA_CONTENT_ORIENTATION_ROT_270:
			_rotate_left(image);
			break;
		case MEDIA_CONTENT_ORIENTATION_NORMAL:
			break;
		default:
			break;
	}
	WALLPAPERUI_TRACE_END;
}

static Eina_Bool _lockscreen_gallery_scale_job_0(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	static int job_idx = SCALE_JOB_0_BUFFER_MAKING;
	Evas_Load_Error err = EVAS_LOAD_ERROR_NONE;

	WALLPAPERUI_DBG("DBG 0 : buffer make and set");
	struct scaledata *sd = data;

	sd->ee = ecore_evas_buffer_new(sd->to_w, sd->to_h);
	if (!sd->ee) {
		WALLPAPERUI_ERR("ecore_evas_buffer_new() failed");
		sd->next_job = SCALE_JOB_ERR;
		return ECORE_CALLBACK_CANCEL;
	}

	sd->canvas = ecore_evas_get(sd->ee);
	sd->img = evas_object_image_filled_add(sd->canvas);
	if (!sd->img) {
		WALLPAPERUI_ERR("evas_object_image_filled_add() failed");
		sd->next_job = SCALE_JOB_ERR;
		return ECORE_CALLBACK_CANCEL;
	}

	if (state_data.from[sd->img_idx]) {
		WALLPAPERUI_DBG("from[%s]", state_data.from[sd->img_idx]);
	}
	evas_object_image_file_set(sd->img, state_data.from[sd->img_idx], NULL);
	err = evas_object_image_load_error_get(sd->img);
	if (err != EVAS_LOAD_ERROR_NONE) {
		WALLPAPERUI_ERR("evas_object_image_file_set() failed");
		WALLPAPERUI_ERR("file(%s) err(%s)", state_data.from[sd->img_idx], evas_load_error_str(err));
		sd->next_job = SCALE_JOB_ERR;
		return ECORE_CALLBACK_CANCEL;
	}

	WALLPAPERUI_DBG("from[i]:%s", state_data.from[sd->img_idx]);
	if (!strstr(state_data.from[sd->img_idx], "wallpaper_list")) {
		WALLPAPERUI_DBG("need rotateRight:%s", state_data.from[sd->img_idx]);
		_rotate_image(sd->img, state_data.from[sd->img_idx]);
	}

	evas_object_image_alpha_set(sd->img, EINA_FALSE);
	evas_object_image_size_get(sd->img, &sd->img_w, &sd->img_h);

	if ((sd->img_w == sd->to_w && sd->img_h >= sd->to_h) ||
		(sd->img_h == sd->to_h && sd->img_w >= sd->to_w)) {
		WALLPAPERUI_DBG("No need to be scaled. cp(%s, %s)", state_data.from[sd->img_idx], state_data.to[sd->img_idx]);
		_lockscreen_gallery_file_cb(state_data.from[sd->img_idx], state_data.to[sd->img_idx]);
		WALLPAPERUI_DBG("cp end");
		sd->next_job = SCALE_JOB_END;
		return ECORE_CALLBACK_CANCEL;
	}

	evas_object_move(sd->img, 0, 0);
	evas_object_image_fill_set(sd->img, 0, 0, sd->img_w, sd->img_h);

	sd->next_job = job_idx+1;

	WALLPAPERUI_TRACE_END;
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _lockscreen_gallery_scale_job_1(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	static int job_idx = SCALE_JOB_1_RESIZE_START;
	struct scaledata *sd = data;
	WALLPAPERUI_DBG("DBG 1 : resize");

	sd->to_w = (float)sd->img_w / sd->img_h * sd->to_h;

	WALLPAPERUI_DBG("idx(%d) img_w(%d) img_h(%d)", sd->img_idx, sd->img_w, sd->img_h);
	WALLPAPERUI_DBG(" -> to_w(%d) to_h(%d)", sd->to_w, sd->to_h);
	ecore_evas_resize(sd->ee, sd->to_w, sd->to_h);
	evas_object_resize(sd->img, sd->to_w, sd->to_h);
	evas_object_show(sd->img);

	sd->next_job = job_idx+1;

	WALLPAPERUI_TRACE_END;
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _lockscreen_gallery_scale_job_2(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	static int job_idx = SCALE_JOB_2_RENDERING;
	struct scaledata *sd = data;
	WALLPAPERUI_DBG("DBG 2 : rendering");

	ecore_evas_manual_render(sd->ee);

	sd->next_job = job_idx+1;

	WALLPAPERUI_TRACE_END;
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _lockscreen_gallery_scale_job_3(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	static int job_idx = SCALE_JOB_3_BUFFER_GET;
	struct scaledata *sd = data;
	WALLPAPERUI_DBG("DBG 3 : buffer get");

	sd->pixels = (void *)ecore_evas_buffer_pixels_get(sd->ee);

	sd->next_job = job_idx+1;

	WALLPAPERUI_TRACE_END;
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _lockscreen_gallery_scale_job_4(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	static int job_idx = SCALE_JOB_4_RESIZE_IMAGE;
	struct scaledata *sd = data;
	WALLPAPERUI_DBG("DBG 4 : resized image");

	sd->o = evas_object_image_add(sd->canvas);
	evas_object_image_size_set(sd->o, sd->to_w, sd->to_h);

	evas_object_image_data_set(sd->o, sd->pixels);
	evas_object_image_alpha_set(sd->o, EINA_FALSE);

	evas_object_image_colorspace_set(sd->o, EVAS_COLORSPACE_ARGB8888);

	sd->next_job = job_idx+1;

	WALLPAPERUI_TRACE_END;
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _lockscreen_gallery_scale_job_5(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	static int job_idx = SCALE_JOB_5_SAVE_IMAGE;
	struct scaledata *sd = data;
	WALLPAPERUI_DBG("DBG 5 : save image");

	Eina_Bool b = evas_object_image_save(sd->o, state_data.to[sd->img_idx], NULL, "quality=100 compress=4");
	if (b == EINA_FALSE) {
		WALLPAPERUI_DBG("evas_object_image_save to %s fail!", state_data.to[sd->img_idx]);
	}

	sd->next_job = job_idx+1;

	WALLPAPERUI_TRACE_END;
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _lockscreen_gallery_scale_job_end(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	struct scaledata *sd = data;
	int idx = 0;

	if (sd->ee) {
		ecore_evas_free(sd->ee);
	}

	if (state_data.from[sd->img_idx+1] == NULL) {
		WALLPAPERUI_ERR("There is no image to process : ALL job end");
		free(sd);
		_lockscreen_gallery_destroy_func();
		return ECORE_CALLBACK_CANCEL;
	}

	idx = sd->img_idx;

	free(sd);

	if (_lockscreen_gallery_scale_job_maker(480, 800, idx+1) != 0) {
		WALLPAPERUI_DBG("All job end");
		_lockscreen_gallery_destroy_func();
	}

	WALLPAPERUI_TRACE_END;
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _lockscreen_gallery_scale_job_handler(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	struct scaledata *sd = data;

	WALLPAPERUI_DBG("idx(%d) curr(%d) next(%d)", sd->img_idx, sd->curr_job, sd->next_job);
	sd->curr_job = sd->next_job;

	switch (sd->curr_job) {
		case SCALE_JOB_INIT:
			WALLPAPERUI_ERR("wrong process! : goto job 0");
			_lockscreen_gallery_scale_job_0(sd);
			break;
		case SCALE_JOB_0_BUFFER_MAKING:
			_lockscreen_gallery_scale_job_0(sd);
			break;
		case SCALE_JOB_1_RESIZE_START:
			_lockscreen_gallery_scale_job_1(sd);
			break;
		case SCALE_JOB_2_RENDERING:
			_lockscreen_gallery_scale_job_2(sd);
			break;
		case SCALE_JOB_3_BUFFER_GET:
			_lockscreen_gallery_scale_job_3(sd);
			break;
		case SCALE_JOB_4_RESIZE_IMAGE:
			_lockscreen_gallery_scale_job_4(sd);
			break;
		case SCALE_JOB_5_SAVE_IMAGE:
			_lockscreen_gallery_scale_job_5(sd);
			break;
		case SCALE_JOB_ERR:
			WALLPAPERUI_ERR("ERROR");
		case SCALE_JOB_END:
			if (sd->idler_handler) {
				ecore_idler_del(sd->idler_handler);
				sd->idler_handler = NULL;
			}
			_lockscreen_gallery_scale_job_end(sd);
			return ECORE_CALLBACK_CANCEL;
			break;
	}

	WALLPAPERUI_DBG("job(%d) finished : next job(%d)", sd->curr_job, sd->next_job);

	WALLPAPERUI_TRACE_END;
	return ECORE_CALLBACK_RENEW;
}

static int _lockscreen_gallery_scale_job_maker(int to_w, int to_h, int idx)
{
	WALLPAPERUI_TRACE_BEGIN;

	static int job_idx = SCALE_JOB_INIT;
	struct scaledata *sd = calloc(1, sizeof(struct scaledata));
	scale_resize_state = 1;

	WALLPAPERUI_DBG("make scale log(%d)", idx);
	if (sd != NULL) {
		sd->img_idx = idx;
		sd->to_w = to_w;
		sd->to_h = to_h;
		sd->curr_job = job_idx;
		sd->next_job = job_idx+1;
		sd->idler_handler = ecore_idler_add(_lockscreen_gallery_scale_job_handler, sd);
	}

	WALLPAPERUI_TRACE_END;
	return 0;
}

static void _main_done_button_cb(void *data, Evas_Object *obj, void *event_info)
{
	WALLPAPERUI_TRACE_BEGIN;

    if (state_data.flag_changed) {
		Elm_Object_Item *object_item = elm_gengrid_first_item_get(ad->gengrid);
		Thumbnail *s_item = NULL;

		while (object_item) {
			s_item = (Thumbnail *)elm_object_item_data_get(object_item);
			if (s_item && s_item->path && s_item->bSelected) {
				WALLPAPERUI_DBG("Done button Selected path=%s %d", s_item->path, s_item->index);
				strncpy(ad->saved_img_path[0], s_item->path, MAX_LENGTH_LINE - 1);
				_done_to_set_wallpaper();
			}
			object_item = elm_gengrid_item_next_get(object_item);
		}
	}

	elm_naviframe_item_pop(ad->navi_bar);

	_wallpaper_destroy(ad);

	WALLPAPERUI_TRACE_END;
}

static void _get_filename_extension_in_uppercase(const char *inputString, char *buf)
{
	WALLPAPERUI_TRACE_BEGIN;
	char *pointChar = strrchr(inputString, '.');
    memset(buf, 0, sizeof(buf));
	while(*pointChar != '\0')
	{
		*buf = toupper(*pointChar);
		++pointChar;
		++buf;
	}
	WALLPAPERUI_TRACE_END;
}

static void _service_gallery_ug_result_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	char **path_array = NULL;
	int array_length = 0;
	Elm_Object_Item *object_item = NULL;
	int i = 0;
	Thumbnail *item = NULL;

	if (app_control_get_extra_data_array(reply, APP_CONTROL_DATA_SELECTED, &path_array, &array_length) == APP_CONTROL_ERROR_NONE) {
		WALLPAPERUI_DBG("array_length = %d", array_length);

		if (array_length < 1) {
			WALLPAPERUI_DBG("array_length < 1, do not get result from gallery");
			return;
		} else {
			char file_ext[4095] = {0,}; //4095 symbols = kernel limitation
			for (; i < array_length; i++) {
				_get_filename_extension_in_uppercase(path_array[i],file_ext);
				WALLPAPERUI_DBG("file_ext = %s", file_ext);
				if (!strcmp(file_ext, ".PNG")
					&& !strcmp(file_ext, ".BMP")
					&& !strcmp(file_ext, ".WBMP")
					&& !strcmp(file_ext, ".PCX")
					&& !strcmp(file_ext, ".TIFF")
					&& !strcmp(file_ext, ".JPEG")
					&& !strcmp(file_ext, ".TGA")
					&& !strcmp(file_ext, ".EXIF")
					&& !strcmp(file_ext, ".FPX")
					&& !strcmp(file_ext, ".SVG")
					&& !strcmp(file_ext, ".PSD")
					&& !strcmp(file_ext, ".CDR")
					&& !strcmp(file_ext, ".PCD")
					&& !strcmp(file_ext, ".DXF")
					&& !strcmp(file_ext, ".UFO")
					&& !strcmp(file_ext, ".EPS")
					&& !strcmp(file_ext, ".JPG")
					&& !strcmp(file_ext, ".GIF")) {
					WALLPAPERUI_DBG("invalid image path: path_array[%d] = %d", i, path_array[i]);
					return;
				} else {
					WALLPAPERUI_DBG("path_array[%d] = %s", i, path_array[i]);
				}
			}
		}
		memset(ad->saved_img_path, 0, sizeof(ad->saved_img_path));
		strncpy(ad->saved_img_path[0], path_array[0], MAX_LENGTH_LINE - 1);
		WALLPAPERUI_DBG("saved_img_path is %s", ad->saved_img_path[0]);

		elm_image_file_set(ad->preview_image, ad->saved_img_path[0], NULL);
		evas_object_show(ad->preview_image);

		object_item = elm_gengrid_first_item_get(ad->gengrid);

		item = (Thumbnail *)elm_object_item_data_get(object_item);
		item->bSelected = EINA_TRUE;
		item->path = ad->saved_img_path[0];
		item->type = WALLPAPER_TYPE_GALLERY;
		ad->preview_image_type = WALLPAPER_TYPE_GALLERY;

		elm_object_item_data_set(object_item, item);
		elm_gengrid_item_update(object_item);
		elm_object_signal_emit(ad->main_layout, "show_preview", "preview_image");

		_wallpaper_show_focus_highlight(item->index);

		state_data.flag_edit_click = EINA_FALSE;
		state_data.flag_image_from_gallery = EINA_TRUE;
		state_data.flag_changed = EINA_TRUE;

       }


	WALLPAPERUI_TRACE_END;
}

static void _gallery_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	WALLPAPERUI_TRACE_BEGIN;

	Thumbnail *item = (Thumbnail *)data;
	elm_gengrid_item_selected_set(item->item,  EINA_FALSE);
	app_control_h svc_handle = NULL;

	if (obj) {
		elm_object_signal_emit(obj, "unpressed", "elm");
	}

	feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TAP);

	if (!app_control_create(&svc_handle)) {
		app_control_set_operation(svc_handle, APP_CONTROL_OPERATION_PICK);
        app_control_set_launch_mode(svc_handle, APP_CONTROL_LAUNCH_MODE_GROUP);
		app_control_set_app_id(svc_handle,  "ug-gallery-efl");
		app_control_set_mime(svc_handle, "image/*");

		if (!DISABLE_MULTISELECTION) {
			app_control_add_extra_data(svc_handle, "max-count", "6");
			app_control_add_extra_data(svc_handle, "launch-type", "select-multiple");
		} else {
		   app_control_add_extra_data(svc_handle, "launch-type", "select-setas");
		   app_control_add_extra_data(svc_handle, "setas-type", "crop");
		   app_control_add_extra_data(svc_handle, "View Mode", "SETAS");
		   app_control_add_extra_data(svc_handle, "Setas type", "Crop");
		   app_control_add_extra_data(svc_handle, "Fixed ratio", "TRUE");
		   /*app_control_add_extra_data(svc_handle, "Area Size", "100"); */
		   app_control_add_extra_data(svc_handle, "Resolution", "480x800");
		}
		app_control_add_extra_data(svc_handle, "file-type", "image");
		app_control_add_extra_data(svc_handle, "hide-personal", "true");
		app_control_send_launch_request(svc_handle, _service_gallery_ug_result_cb, data);
		app_control_destroy(svc_handle);
	}
	WALLPAPERUI_TRACE_END;
}

static bool _wallpaper_db_create_filter(filter_handle *filter)
{
	WALLPAPERUI_TRACE_BEGIN;

	filter_h *media_filter = (filter_h *)filter;
	int ret = MEDIA_CONTENT_ERROR_NONE;

	ret = media_filter_create(media_filter);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("media_filter_create is failed, err");
		return false;
	}

	WALLPAPERUI_TRACE_END;
	return true;
}

static bool _wallpaper_db_destroy_filter(filter_handle filter)
{
	WALLPAPERUI_TRACE_BEGIN;

	filter_h media_filter = (filter_h)filter;

	int ret = MEDIA_CONTENT_ERROR_NONE;

	ret = media_filter_destroy(media_filter);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("media_filter_destroy is failed, err");
		return false;
	}

	WALLPAPERUI_TRACE_END;
	return true;
}

static bool _wallpaper_db_set_filter_condition(filter_handle media_filter, const char *condition)
{
	WALLPAPERUI_TRACE_BEGIN;

	retv_if(condition == NULL, false);
	WALLPAPERUI_DBG("Set DB Filter : %s", condition);

	int ret = media_filter_set_condition(media_filter, condition, MEDIA_CONTENT_COLLATE_DEFAULT);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("media_filter_set_condition is failed, err");
		return false;
	}

	ret = media_filter_set_order(media_filter, MEDIA_CONTENT_ORDER_DESC, "MEDIA_MODIFIED_TIME", MEDIA_CONTENT_COLLATE_DEFAULT);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("media_filter_set_condition is failed, err");
		return false;
	}

	WALLPAPERUI_TRACE_END;
	return true;
}

static bool _media_item_cb(media_info_h item, void *user_data)
{
	WALLPAPERUI_TRACE_BEGIN;

	media_handle *m_handle = (media_handle *)user_data;

	media_info_clone((media_info_h *)m_handle, item);

	WALLPAPERUI_TRACE_END;
	return false;/*only 1 item */
}

static char *_get_last_image_from_db()
{
	WALLPAPERUI_TRACE_BEGIN;

	int ret = MEDIA_CONTENT_ERROR_NONE;
	filter_handle media_filter = NULL;
	char buf[1024] = {0,};
	media_handle m_handle = NULL;
	int media_count = 0;
	char *thumbnailPath = NULL;

	snprintf(buf, sizeof(buf), "MEDIA_TYPE=0");

	if (_wallpaper_db_create_filter(&media_filter) == false) {
		return NULL;
	}

	if (_wallpaper_db_set_filter_condition(media_filter, buf) == false) {
		_wallpaper_db_destroy_filter(media_filter);
		return NULL;
	}

	media_info_get_media_count_from_db(media_filter, &media_count);
	if (media_count == 0) {
		_wallpaper_db_destroy_filter(media_filter);
		return NULL;
	}

	ret = media_info_foreach_media_from_db(media_filter, _media_item_cb, &m_handle);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_ERR("media_info_foreach_media_from_db is failed, err");
	}

	if (m_handle) {
		if (media_info_get_thumbnail_path(m_handle, &thumbnailPath) != MEDIA_CONTENT_ERROR_NONE) {
			WALLPAPERUI_ERR("Get media type failed!");
		}
		WALLPAPERUI_DBG("thumbnailPath is %s", thumbnailPath);
		free(m_handle);
	}

	_wallpaper_db_destroy_filter(media_filter);

	WALLPAPERUI_TRACE_END;
	return thumbnailPath;
}

static void _main_cancel_button_cb(void *data, Evas_Object *obj, void *event_info)
{
	WALLPAPERUI_TRACE_BEGIN;

	_wallpaper_destroy(ad);
	WALLPAPERUI_DBG("cancel_button_cb end");

	WALLPAPERUI_TRACE_END;
}

static void _set_wallpaper(char *path)
{
	WALLPAPERUI_TRACE_BEGIN;

	WALLPAPERUI_ERR("set wallpaper : %s", path);
	if (ecore_file_exists(path) != EINA_TRUE) {
		WALLPAPERUI_ERR("%s does not exist", path);
		return;
	}

	vconf_set_int(VCONFKEY_LOCKSCREEN_WALLPAPER_TYPE, ad->preview_image_type);
	WALLPAPERUI_DBG("Set VCONFKEY_LOCKSCREEN_WALLPAPER_TYPE %d", ad->preview_image_type);

	elm_image_file_set(ad->preview_image, path, NULL);
	evas_object_show(ad->preview_image);

	/* lockscreen */
	if (system_settings_set_value_string(SYSTEM_SETTINGS_KEY_WALLPAPER_LOCK_SCREEN, path) != SYSTEM_SETTINGS_ERROR_NONE) {
		WALLPAPERUI_ERR("Lockscreen set Error : %s", path);
		elm_exit();
		return;
	}

	/* homescreen */
	if (system_settings_set_value_string(SYSTEM_SETTINGS_KEY_WALLPAPER_HOME_SCREEN, path) != SYSTEM_SETTINGS_ERROR_NONE) {
		WALLPAPERUI_ERR("Homescreen set Error : %s", path);
		elm_exit();
		return;
	}

	WALLPAPERUI_TRACE_END;
}

static void _done_to_set_wallpaper()
{
	WALLPAPERUI_TRACE_BEGIN;

	char *p = NULL;
	char filepath[MAX_LENGTH_LINE] = {0};
	char filename[MAX_LENGTH_LINE] = {0};
	char *q = NULL;
	int i = 0;
	int index = 0;
	char *temp_path[6] = {NULL};

	/*copy lock wallpaper */
	while (i < MAX_MULTIPLE_SELECTION) {
		if (strlen(ad->saved_img_path[i]) > 1) {
			WALLPAPERUI_DBG("saved_img_path[%d] = %s", i, ad->saved_img_path[i]);
			p = strrchr(ad->saved_img_path[i], '/');
			if (p) {
				q = strrchr(p, '.');
				if (q && ((strcmp(q, ".gif") == 0) || (strcmp(q, ".wbmp") == 0) || (strcmp(q, ".bmp") == 0))) {
					WALLPAPERUI_DBG(".gif||.wbmp||.bmp image");
					strncpy(filename, p, MAX_LENGTH_LINE-1);
					q = strrchr(filename, '.');
					if (q) {
						*q = '\0';
					}
				}
				WALLPAPERUI_DBG("filepath = %s", filepath);
				if (ad->preview_image_type == WALLPAPER_TYPE_GALLERY) {
					wallpaper_ui_service_copy_wallpaper_file(ad->saved_img_path[i], filepath);
				}
				if (state_data.from[index] != NULL) {
					free(state_data.from[index]);
					state_data.from[index] = NULL;
				}
				state_data.from[index] = strdup(ad->saved_img_path[i]);

				if (state_data.to[index] != NULL) {
					free(state_data.to[index]);
					state_data.to[index] = NULL;
				}
				state_data.to[index] = strdup(filepath);
				index++;

				if (ad->preview_image_type == WALLPAPER_TYPE_GALLERY) {
					WALLPAPERUI_DBG("Gallery image");
					temp_path[i] = strdup(filepath);
				} else {
					WALLPAPERUI_DBG("Default image");
					temp_path[i] = strdup(ad->saved_img_path[i]);
				}
				_set_wallpaper(temp_path[i]);

				strncpy(ad->current_preview_img_path, temp_path[0], MAX_LENGTH_LINE-1);
				WALLPAPERUI_DBG("current_preview_img_path %s", ad->current_preview_img_path);
			}
		}
		i++;
	}

	i--;
	while (i >= 0 && temp_path[i]) {
		free(temp_path[i]);
		i--;
	}

	vconf_set_int(VCONFKEY_LOCKSCREEN_WALLPAPER_COUNT, index);

	if (state_data.flag_image_from_gallery) {
		vconf_set_int(VCONFKEY_LOCKSCREEN_WALLPAPER_TYPE, WALLPAPER_TYPE_GALLERY);
		WALLPAPERUI_DBG("Set VCONFKEY_LOCKSCREEN_WALLPAPER_TYPE = WALLPAPER_TYPE_GALLERY");
	} else {
		vconf_set_int(VCONFKEY_LOCKSCREEN_WALLPAPER_TYPE, WALLPAPER_TYPE_DEFAULT);
		WALLPAPERUI_DBG("Set VCONFKEY_LOCKSCREEN_WALLPAPER_TYPE = WALLPAPER_TYPE_DEFAULT");
	}

	WALLPAPERUI_DBG("from[%d] : %s", 0, state_data.from[0]);

	if (ad->preview_image_type == WALLPAPER_TYPE_GALLERY) {
		WALLPAPERUI_DBG("SCALE start!");

		_lockscreen_gallery_scale_job_maker(480, 800, 0);
	}

	state_data.flag_changed = EINA_FALSE;

	sync();
	WALLPAPERUI_DBG("done_to_set_wallpaper end");

	WALLPAPERUI_TRACE_END;
}

static void _lockscreen_gallery_destroy_func()
{
	WALLPAPERUI_TRACE_BEGIN;

	/*delete unused files */
	char path[6][MAX_LENGTH_LINE] = {{0 } };
	char *value = NULL;

	memset(path, 0, sizeof(path));

	if (system_settings_get_value_string(SYSTEM_SETTINGS_KEY_WALLPAPER_HOME_SCREEN, &value) != SYSTEM_SETTINGS_ERROR_NONE) {
		WALLPAPERUI_ERR("system_settings_get_value_string() failed");
	}
	WALLPAPERUI_DBG("value = %s", value);

	/*_wallpaper_destroy(ad); */

	sync();
	scale_resize_state = 0;
	/*wallpaper_back_key_cb(ad, NULL, NULL); */
	WALLPAPERUI_TRACE_END;
}

static void _wallpaper_destroy(void *data)
{
	WALLPAPERUI_TRACE_BEGIN

	ret_if(!data);

	wallpaper_ui_service_appdata *ad = data;

	if (ad->last_preview_img_path != NULL) {
		WALLPAPERUI_ERR("free last_preview_img_path");
		free(ad->last_preview_img_path);
	}
	if (ad->win) {
		evas_object_del(ad->win);
		ad->win = NULL;
	}

	elm_theme_extension_del(NULL, EDJDIR"/button_customized_theme.edj");

	elm_exit();

	WALLPAPERUI_TRACE_END
}

static void _wallpaper_back_key_cb(void *data, Evas_Object *obj, void *event_info)
{
	WALLPAPERUI_TRACE_BEGIN;
	ret_if(!data);

	wallpaper_ui_service_appdata *ad = data;
	Elm_Object_Item *item = NULL;

	if (scale_resize_state == 1) {
		WALLPAPERUI_ERR("image_resizeing ignore");
		return;
	}

	item = elm_naviframe_top_item_get(ad->navi_bar);
	if (item && (item == nf_it)) {
		_wallpaper_destroy(ad);
	} else if (item == NULL) {
		WALLPAPERUI_ERR("item == NULL : destroy UG");
		_wallpaper_destroy(ad);
	} else {
		WALLPAPERUI_ERR("item != nf_it");
		elm_naviframe_item_pop(ad->navi_bar);
	}
	WALLPAPERUI_TRACE_END;
}

static void _wallpaper_db_update_cb(media_content_error_e error, int pid,
			      media_content_db_update_item_type_e update_item,
			      media_content_db_update_type_e update_type,
			      media_content_type_e media_type, char *uuid,
			      char *path, char *mime_type, void *data)
{
	WALLPAPERUI_TRACE_BEGIN;
	WALLPAPERUI_DBG("update_item[%d], update_type[%d], media_type[%d]", update_item, update_type, media_type);

	wallpaper_ui_service_appdata *ad = (wallpaper_ui_service_appdata *)data;
	char *last_image = NULL;

	if (media_type != MEDIA_CONTENT_TYPE_IMAGE) {
		WALLPAPERUI_DBG("Not Image update");
		/*return; */
	}

	if (MEDIA_CONTENT_ERROR_NONE != error) {
		WALLPAPERUI_ERR("Update db error[%d]!", error);
		return;
	}

	last_image = _get_last_image_from_db();
	WALLPAPERUI_DBG("last_image == %s", last_image);

	if (last_image != NULL) {
		elm_image_file_set(ad->preview_image, last_image, NULL);
		free(last_image);
		last_image = NULL;
	} else {
		elm_image_file_set(ad->preview_image, ICONDIR"/no_gallery_bg.png", NULL);
	}
	evas_object_show(ad->preview_image);

	WALLPAPERUI_TRACE_END;
}

static void _main_preview_image_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	WALLPAPERUI_TRACE_BEGIN;

	Evas_Object *navi_bar = (Evas_Object *)data;
	if (navi_bar == NULL) {
		WALLPAPERUI_DBG("navi_bar == NULL");
		return;
	}

	feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TAP);
	_wallpaper_preview_main();

	WALLPAPERUI_TRACE_END;
}

static void _wallpaper_preview_main()
{
	WALLPAPERUI_TRACE_BEGIN;

	Evas_Object *preveiw_main_layout = NULL;
    Evas_Object *preview_image = NULL;

    preveiw_main_layout = elm_layout_add(ad->navi_bar);
    elm_layout_file_set(preveiw_main_layout, EDJDIR"/popup-wallpaper.edj", "wallpaper.preview");
    evas_object_size_hint_weight_set(preveiw_main_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    preview_image = elm_image_add(preveiw_main_layout);

	WALLPAPERUI_DBG("current_preview_img_path = %s", ad->current_preview_img_path);

    elm_image_file_set(preview_image, ad->current_preview_img_path, NULL);
    elm_image_aspect_fixed_set(preview_image, EINA_TRUE);
    elm_image_fill_outside_set(preview_image, EINA_TRUE);
    elm_image_preload_disabled_set(preview_image, EINA_TRUE);
    elm_object_part_content_set(preveiw_main_layout, "preview", preview_image);
    edje_object_signal_callback_add(_EDJ(preveiw_main_layout), "preview_clicked", "edj", _preview_clicked_cb, ad->navi_bar);
    elm_object_part_content_unset(preveiw_main_layout, "thumblist");
    evas_object_show(preveiw_main_layout);

	Elm_Object_Item *navi_item = elm_naviframe_item_push(ad->navi_bar, NULL, NULL, NULL, preveiw_main_layout, NULL);
	elm_naviframe_item_title_enabled_set (navi_item, EINA_FALSE, EINA_FALSE);
	WALLPAPERUI_TRACE_END;
}

HAPI void wallpaper_main_create_view(void *data)
{
	WALLPAPERUI_TRACE_BEGIN;
	char *value = NULL;

	state_data.flag_edit_click = EINA_FALSE;
	state_data.flag_changed = EINA_FALSE;

	ad = (wallpaper_ui_service_appdata *)data;
	if (ad == NULL) {
		WALLPAPERUI_DBG("wallpaper_ui_service_appdata is NULL");
		return;
	}

	if (ad->win == NULL) {
		WALLPAPERUI_DBG("ad->win is NULL");
		return;
	}

	if (media_content_set_db_updated_cb(_wallpaper_db_update_cb, data) != MEDIA_CONTENT_ERROR_NONE) {
		WALLPAPERUI_DBG("Set db updated cb failed!");
	}

	/* Naviframe */
	Evas_Object *navi_bar = elm_naviframe_add(ad->layout);
	elm_object_part_content_set(ad->layout, "elm.swallow.content", navi_bar);
	evas_object_show(navi_bar);
	ad->navi_bar = navi_bar;

	/* layout */
	Evas_Object *preveiw_main_layout = elm_layout_add(ad->navi_bar);
	elm_layout_file_set(preveiw_main_layout, EDJDIR"/popup-wallpaper.edj", "main_page_layout");
	evas_object_size_hint_weight_set(preveiw_main_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(preveiw_main_layout);
	ad->main_layout = preveiw_main_layout;

	if (system_settings_get_value_string(SYSTEM_SETTINGS_KEY_WALLPAPER_LOCK_SCREEN, &value) != SYSTEM_SETTINGS_ERROR_NONE) {
		WALLPAPERUI_ERR("system_settings_get_value_string() failed");
	}
	WALLPAPERUI_DBG("value = %s", value);

	/* preview image */
	Evas_Object *image = elm_image_add(preveiw_main_layout);
	elm_image_file_set(image, value, NULL);
	elm_image_aspect_fixed_set(image,	EINA_TRUE);
	elm_image_fill_outside_set(image, EINA_TRUE);
	elm_image_preload_disabled_set(image, EINA_FALSE);
	elm_object_part_content_set(preveiw_main_layout, "preview_image", image);
    evas_object_smart_callback_add(image, "clicked", _main_preview_image_clicked_cb, ad->navi_bar);

	ad->preview_image = image;

	/* preview text */
	elm_object_translatable_part_text_set(preveiw_main_layout, "text_tap", APP_STRING("IDS_LCKSCN_NPBODY_TAP_TO_PREVIEW"));
/*	elm_object_signal_emit(preveiw_main_layout, "hide_preview", "preview_image"); */

	/* gallery last image */
	ad->last_preview_img_path = _get_last_image_from_db();
	if (ad->last_preview_img_path != NULL) {
		WALLPAPERUI_DBG("last_preview_img_path == %s", ad->last_preview_img_path);
	}

	/* thumnail icon */
	ad->gengrid = main_gengrid_add(preveiw_main_layout, NULL);
	elm_object_part_content_set(preveiw_main_layout, "thumblist", ad->gengrid);
	evas_object_show(ad->gengrid);

	eext_object_event_callback_add(navi_bar, EEXT_CALLBACK_BACK, _wallpaper_back_key_cb, (void *)ad);
	nf_it = elm_naviframe_item_push(navi_bar, APP_STRING("IDS_LCKSCN_MBODY_WALLPAPERS"), NULL , NULL, preveiw_main_layout, NULL);

	elm_object_translatable_part_text_set(preveiw_main_layout, "elm.text.title", APP_STRING("IDS_LCKSCN_MBODY_WALLPAPERS"));
	elm_object_signal_emit(navi_bar, "elm,state,title,hide", "elm");

	/* Title Cancel Button */
	Evas_Object *cancel_btn = elm_button_add(preveiw_main_layout);
	elm_object_style_set(cancel_btn, "naviframe/back_btn/custom");
	evas_object_smart_callback_add(cancel_btn, "clicked", _main_cancel_button_cb, NULL);
	elm_object_part_content_set(preveiw_main_layout, "title_left_btn", cancel_btn);
	evas_object_show(cancel_btn);
	elm_object_signal_emit(preveiw_main_layout, "elm,state,title_left_btn,show", "elm");

	/* Title Done Button */
	Evas_Object *done_btn = elm_button_add(preveiw_main_layout);
	elm_object_style_set(done_btn, "naviframe/title_right_custom");
	evas_object_smart_callback_add(done_btn, "clicked", _main_done_button_cb, NULL);
	elm_object_translatable_part_text_set(done_btn, "elm.text", APP_STRING("IDS_TPLATFORM_ACBUTTON_DONE_ABB"));
	elm_object_part_content_set(preveiw_main_layout, "title_right_btn", done_btn);
	evas_object_show(done_btn);
	elm_object_signal_emit(preveiw_main_layout, "elm,state,title_right_btn,show", "elm");

	WALLPAPERUI_TRACE_END;
}

static Evas_Object *_preview_create_edje_content(Evas_Object *parent, const char *path, Thumbnail *thm)
{
	WALLPAPERUI_TRACE_BEGIN;
	WALLPAPERUI_DBG("path=%s", path);
	Evas_Object *layout;

	layout = elm_layout_add(parent);
	if (elm_layout_file_set(layout, EDJDIR"/popup-wallpaper.edj", "preview_gengrid.item") == EINA_FALSE) {
		WALLPAPERUI_DBG("Cannot load mutiple-wallpaper edj");
		return NULL;
	}

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	Evas_Object *image = elm_image_add(layout);
	elm_object_part_content_set(layout, "item", image);
	elm_image_file_set(image, path, NULL);

	elm_image_aspect_fixed_set(image, EINA_TRUE);
	elm_image_fill_outside_set(image, EINA_TRUE);
	elm_image_preload_disabled_set(image, EINA_FALSE);
	evas_object_show(image);

	if (thm->title) {
		elm_object_translatable_part_text_set(layout, "text", thm->title);
	}

	WALLPAPERUI_TRACE_END;
	return layout;
}

static Evas_Object *_preview_grid_content_get(void *data, Evas_Object *obj, const char *part)
{
	WALLPAPERUI_TRACE_BEGIN;
	Thumbnail *thm = (Thumbnail *)data;
	WALLPAPERUI_DBG("preview_grid_content_get Content get : %s", part);

	if (!strcmp(part, "elm.swallow.icon")) {
		Evas_Object *contents = _preview_create_edje_content(obj, (thm->path), thm);

		thm->content = contents;

		WALLPAPERUI_DBG(">>path=%s", thm->path);

		if (thm->bSelected) {
			WALLPAPERUI_DBG("Selected");

			edje_object_signal_emit(elm_layout_edje_get(thm->content), "elm,focus_highlight,show", "app");
            memset(ad->current_preview_img_path, 0, sizeof(ad->current_preview_img_path));
            strncpy(ad->current_preview_img_path, thm->path, MAX_LENGTH_LINE-1);
            WALLPAPERUI_DBG("current_preview_img_path %s", ad->current_preview_img_path);
		} else {
			WALLPAPERUI_DBG("UnSelected");
			elm_gengrid_item_selected_set(thm->item, EINA_FALSE);
			edje_object_signal_emit(elm_layout_edje_get(thm->content), "elm,state,unselected", "app");
		}

		return contents;
	}

	WALLPAPERUI_TRACE_END;
	return NULL;
}

static void _preview_grid_content_del(void *data, Evas_Object *obj)
{
	WALLPAPERUI_TRACE_BEGIN;
	Thumbnail *thm = (Thumbnail *)data;

	if (thm) {
		free(thm->path);
		free(thm->title);
		free(thm);
	}

	WALLPAPERUI_TRACE_END;
}

static void _wallpaper_show_focus_highlight(int selected_index)
{
	WALLPAPERUI_TRACE_BEGIN;

	Thumbnail *item = NULL;

	Elm_Object_Item *object_item = elm_gengrid_first_item_get(ad->gengrid);

	while (object_item) {
		item = (Thumbnail *)elm_object_item_data_get(object_item);

		if (item && item->path && item->bSelected) {
			if (item->index != selected_index) {
				WALLPAPERUI_DBG("path=%s", item->path);
				elm_gengrid_item_selected_set(item->item,  EINA_FALSE);
				edje_object_signal_emit(elm_layout_edje_get(item->content), "elm,state,unselected", "app");
				item->bSelected = EINA_FALSE;
			} else {
				edje_object_signal_emit(elm_layout_edje_get(item->content), "elm,focus_highlight,show", "app");
			}
		}
		object_item = elm_gengrid_item_next_get(object_item);
	}
	WALLPAPERUI_TRACE_END;
}

static void _wallpaper_on_item_selected(void *data, Evas_Object *obj, void *event_info)
{
	WALLPAPERUI_TRACE_BEGIN;
	Thumbnail *item = (Thumbnail *)data;

	if (item == NULL) {
		WALLPAPERUI_DBG("item == NULL");
		return;
	}

	if (item->type != WALLPAPER_TYPE_GALLERY) {
/*		elm_object_signal_emit(ad->main_layout, "hide_preview", "preview_image"); */
		elm_image_file_set(ad->preview_image, item->path, NULL);
        memset(ad->current_preview_img_path, 0, sizeof(ad->current_preview_img_path));
        strncpy(ad->current_preview_img_path, item->path, MAX_LENGTH_LINE-1);
        WALLPAPERUI_DBG("current_preview_img_path %s", ad->current_preview_img_path);
	}

	elm_gengrid_item_selected_set(item->item, EINA_FALSE);
	ad->preview_image_type = item->type;

	item->bSelected = EINA_TRUE;

	_wallpaper_show_focus_highlight(item->index);

    state_data.flag_changed = EINA_TRUE;

	WALLPAPERUI_TRACE_END;
}


static Evas_Object *main_gengrid_add(Evas_Object *parent, void *data)
{
	WALLPAPERUI_TRACE_BEGIN;

	Thumbnail *s_item = NULL;
	int index = 0;
	char *setting_value = NULL;
	char string[MAX_LENGTH_LINE] = {0};
	int setting_type = 0;
	Eina_List *file_list = NULL;
	char *temp = NULL;
	int count = 0;
	Thumbnail *temp_item = NULL;

	if (system_settings_get_value_string(SYSTEM_SETTINGS_KEY_WALLPAPER_LOCK_SCREEN, &setting_value) != SYSTEM_SETTINGS_ERROR_NONE) {
		WALLPAPERUI_ERR("system_settings_get_value_string() failed");
	}

	vconf_get_int(VCONFKEY_LOCKSCREEN_WALLPAPER_TYPE, &setting_type);
	WALLPAPERUI_DBG("VCONFKEY_LOCKSCREEN_WALLPAPER_TYPE %d", setting_type);


	ad->gengrid = elm_gengrid_add(parent);
	elm_scroller_policy_set(ad->gengrid, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	evas_object_size_hint_weight_set(ad->gengrid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->gengrid, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_gengrid_align_set(ad->gengrid, 0.0, 1.0);
	elm_gengrid_horizontal_set(ad->gengrid, EINA_TRUE);
	elm_scroller_bounce_set(ad->gengrid, EINA_FALSE, EINA_FALSE);
	elm_gengrid_multi_select_set(ad->gengrid, EINA_FALSE);
	elm_object_style_set(ad->gengrid, "no_effect");
	elm_gengrid_select_mode_set(ad->gengrid, ELM_OBJECT_SELECT_MODE_ALWAYS);

	/*int iw, ih;
	iw = 96 * elm_config_scale_get();
	ih = 136 * elm_config_scale_get();*/
	elm_gengrid_item_size_set(ad->gengrid, ELM_SCALE_SIZE(195), ELM_SCALE_SIZE(195));

	gic_for_main = elm_gengrid_item_class_new();
	gic_for_main->item_style = "default";
	gic_for_main->func.text_get = NULL;
	gic_for_main->func.content_get = _preview_grid_content_get;
	gic_for_main->func.state_get = NULL;
	gic_for_main->func.del = _preview_grid_content_del;

	int i = 0;

	/* Gallery icon */
	s_item = (Thumbnail *)calloc(1, sizeof(Thumbnail));
	if (s_item != NULL) {
		if (setting_type == WALLPAPER_TYPE_DEFAULT) {
			if (ad->last_preview_img_path != NULL) {
				s_item->path = strdup(ad->last_preview_img_path);
			} else {
				s_item->path = strdup(ICONDIR"/no_gallery_bg.png");
			}
		} else {
			s_item->path = setting_value;
			s_item->bSelected = EINA_TRUE;

			edje_object_signal_emit(elm_layout_edje_get(s_item->content), "elm,focus_highlight,show", "app");
			/*elm_object_signal_emit(ad->main_layout, "show_preview", "preview_image"); */
		}

		s_item->type = WALLPAPER_TYPE_GALLERY;
		s_item->index = index++;
		s_item->item = elm_gengrid_item_append(ad->gengrid, gic_for_main, s_item, _gallery_clicked_cb, s_item);
		s_item->title = strdup(APP_STRING("IDS_LCKSCN_BODY_GALLERY"));
	}
	file_list = ecore_file_ls(DEFAULT_IMAGE_DIR);
	count = eina_list_count(file_list);
	WALLPAPERUI_DBG("count = %d", count);

	/* default directory */
	if (count > 0) {
		for (i = 0; i < count; i++) {
			temp = (char *)eina_list_nth(file_list, i);
			WALLPAPERUI_DBG("temp = %s", temp);
			snprintf(string, sizeof(string), DEFAULT_IMAGE_DIR_TEMPLATE, temp);

			s_item = (Thumbnail *)calloc(1, sizeof(Thumbnail));
			if (s_item) {
				s_item->path = strdup(string);
				s_item->type = WALLPAPER_TYPE_DEFAULT;
				s_item->title = strdup(temp);

				s_item->index = index++;
	/*			s_item->data = data; */

				if (s_item->path && strcmp(s_item->path, setting_value) == 0) {
					s_item->bSelected = EINA_TRUE;
					WALLPAPERUI_DBG("selected item = %d", s_item->index);
				} else {
					s_item->bSelected = EINA_FALSE;
				}
				/*				s_item->item = elm_gengrid_item_append(gengrid, gic, s_item, _gallery_clicked_cb, s_item); */
				s_item->item = elm_gengrid_item_append(ad->gengrid, gic_for_main, s_item, _wallpaper_on_item_selected, s_item);
			}

		}

	}

	if (file_list != NULL) {
		eina_list_free(file_list);
	}

	/* Shows the portion of a gengrid internal grid containing a given item immediately */

	Elm_Object_Item *object_item = elm_gengrid_first_item_get(ad->gengrid);

	while (object_item) {
		temp_item = (Thumbnail *)elm_object_item_data_get(object_item);

		if (temp_item && temp_item->path && temp_item->bSelected) {
			elm_gengrid_item_show(object_item, ELM_GENGRID_ITEM_SCROLLTO_IN);
			edje_object_signal_emit(elm_layout_edje_get(temp_item->content), "elm,focus_highlight,show", "app");
			ad->preview_image_type = temp_item->type;
		}
		object_item = elm_gengrid_item_next_get(object_item);
	}

	WALLPAPERUI_TRACE_END;
	return ad->gengrid;
}

static void _preview_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	WALLPAPERUI_TRACE_BEGIN;
	Evas_Object *navi_bar = (Evas_Object *)data;

	if (state_data.flag_edit_click) {
		WALLPAPERUI_DBG("flag_edit_click=%d", state_data.flag_edit_click);
		state_data.flag_edit_click = EINA_FALSE;
		return;
	}

	feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TAP);

	elm_naviframe_item_pop(navi_bar);

	WALLPAPERUI_TRACE_END;
}


/**
* @}
*/
/* End of file */
