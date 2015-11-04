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



#ifndef __WALLPAPER_UI_SERVICE_MAIN_H__
#define __WALLPAPER_UI_SERVICE_MAIN_H__

#include "wallpaper-ui-service.h"

#include <app.h>
#include <Elementary.h>
#include <Eina.h>
#include <Evas.h>

#include "wallpaper-ui-service-debug.h"

typedef void *filter_handle;
typedef void *media_handle;

typedef struct {
	Evas_Object *content;
	Elm_Object_Item *item;
	bool bSelected;
	char *title;
	char *path;
	int index;
	int type;
} Thumbnail;

#ifndef VCONFKEY_LOCKSCREEN_WALLPAPER_TYPE
#define VCONFKEY_LOCKSCREEN_WALLPAPER_TYPE "db/lockscreen/wallpaper_type"
#endif

#ifndef VCONFKEY_LOCKSCREEN_WALLPAPER_COUNT
#define VCONFKEY_LOCKSCREEN_WALLPAPER_COUNT "db/lockscreen/wallpaper_count"
#endif

#ifndef WALLPAPER_TXT_FILE
#define WALLPAPER_TXT_FILE "/opt/usr/share/lockscreen/wallpaper_list/lockscreen_selected_images.txt"
#endif


HAPI void wallpaper_main_create_view(void *data);


#endif

/**
* @}
*/
/* End of file */
