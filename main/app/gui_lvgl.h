#ifndef __APP_GUI_LVGL_H
#define __APP_GUI_LVGL_H

#include "lvgl/lvgl.h"

// image page
extern lv_obj_t *image_page;

/**
 * @brief Create the camera widget
 * @param parent The parent object
*/
void camera_widget_create(lv_obj_t *parent, lv_style_t *style);



#endif
