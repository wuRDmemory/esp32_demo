#include "gui_lvgl.h"
#include "esp_log.h"
#include "stdio.h"

const char * TAG = "GUI";
lv_obj_t *image_page = NULL;
lv_obj_t *open_btn = NULL;

void camera_widget_create(lv_obj_t *parent, lv_style_t *style) {
    // set parent property
    lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY_TOP);

    lv_disp_size_t disp_size = lv_disp_get_size_category(NULL);
    printf("disp size: %d\n", disp_size);
    lv_coord_t grid_w = lv_page_get_width_grid(parent, disp_size <= LV_DISP_SIZE_SMALL ? 1 : 2, 1);

    // add image page in container
    lv_obj_t *h = lv_cont_create(parent, NULL);
    lv_cont_set_layout(h, LV_LAYOUT_PRETTY_BOTTOM);
    lv_obj_add_style(h, LV_CONT_PART_MAIN, style);
    lv_obj_set_drag_parent(h, true);
    
    lv_cont_set_fit2(h, LV_FIT_NONE, LV_FIT_TIGHT);
    lv_obj_set_width(h, grid_w);

    // image create
    image_page = lv_img_create(h, NULL);
    lv_btn_set_fit2(image_page, LV_FIT_NONE, LV_FIT_TIGHT);
    lv_obj_set_width(image_page, lv_obj_get_width_grid(h, disp_size <= LV_DISP_SIZE_SMALL ? 1 : 2, 1));
    
    static lv_style_t style_img;
	lv_style_init(&style_img);
	lv_style_set_image_recolor(&style_img, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_image_recolor_opa(&style_img, LV_STATE_DEFAULT, 0);
	lv_style_set_image_opa(&style_img, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(image_page, LV_IMG_PART_MAIN, &style_img);
	lv_obj_set_pos(image_page, 0, 0);
	lv_obj_set_size(image_page, (int)(320*0.5), (int)(240*0.5));

    // open button
}