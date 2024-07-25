#include "app_lvgl.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_freertos_hooks.h"
#include "lv_examples/src/lv_demo_widgets/lv_demo_widgets.h"
#include "lv_examples/src/lv_demo_benchmark/lv_demo_benchmark.h"
#include "lv_examples/src/lv_demo_music/lv_demo_music.h"
#include "lvgl_touch/touch_driver.h"
#include "lvgl_helpers.h"
#include "gui_lvgl.h"

#define TAG "LVGL"

static lv_obj_t *_tab_view = NULL;
static lv_obj_t *_other_widget = NULL;
static lv_obj_t *_camera_widget = NULL;
static lv_obj_t *_chat_widget = NULL;
static lv_style_t _style_box;
static SemaphoreHandle_t xGuiSemaphore;

/**
 * @brief Flush callback for LVGL
*/
static void _lv_tick_task(void *arg) {
	(void)arg;
	lv_tick_inc(10);
}

/**
 * @brief gui setup
*/
static void _gui_setup(void) {
    _tab_view = lv_tabview_create(lv_scr_act(), NULL);
    _camera_widget = lv_tabview_add_tab(_tab_view, "Camera");
    _chat_widget = lv_tabview_add_tab(_tab_view, "Chat");
    _other_widget = lv_tabview_add_tab(_tab_view, "Other");

    printf("tabview height: %d \n", lv_obj_get_height(_camera_widget));

    // create label widget
    lv_style_init(&_style_box);
    lv_style_set_value_align(&_style_box, LV_STATE_DEFAULT, LV_ALIGN_OUT_TOP_LEFT);
    lv_style_set_value_ofs_y(&_style_box, LV_STATE_DEFAULT, - LV_DPX(5));
    lv_style_set_margin_top(&_style_box, LV_STATE_DEFAULT, LV_DPX(5));
    lv_style_set_text_font(&_style_box, LV_STATE_DEFAULT, &lv_font_montserrat_12);
    lv_style_set_bg_color(&_style_box, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));

    // create camera widget
    camera_widget_create(_camera_widget, &_style_box);
}

void app_lvgl_initialise(void) {
    // create gui semaphore
    xGuiSemaphore = xSemaphoreCreateMutex();

    // initialize lvgl
    lv_init();
    lvgl_driver_init();

    lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t* buf2 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

    // register display buffer
    static lv_disp_buf_t disp_buf;
    lv_disp_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);

    // register display driver
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    // register touch driver
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_driver_read;
    lv_indev_drv_register(&indev_drv);

    // register freeRTOS tick hook
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &_lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 10 * 1000));

    // setup gui
    // lv_demo_widgets();
    _gui_setup();

    while (1) {
		/* Delay 1 tick (assumes FreeRTOS tick is 10ms */
		vTaskDelay(pdMS_TO_TICKS(10));

		/* Try to take the semaphore, call lvgl related function on success */
		if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {

			lv_task_handler();
			xSemaphoreGive(xGuiSemaphore);
		}
	}
}

void app_lvgl(void) {

}