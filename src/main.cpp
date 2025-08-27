#include <Arduino.h>
#include <lvgl.h>
#include "pincfg.h"
#include "dispcfg.h"
#include "AXS15231B_touch.h"
#include <Arduino_GFX_Library.h>

Arduino_DataBus *bus = new Arduino_ESP32QSPI(TFT_CS, TFT_SCK, TFT_SDA0, TFT_SDA1, TFT_SDA2, TFT_SDA3);
Arduino_GFX *g = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED, 0, false, TFT_res_W, TFT_res_H);
Arduino_Canvas *gfx = new Arduino_Canvas(TFT_res_W, TFT_res_H, g, 0, 0, TFT_rot);
AXS15231B_Touch touch(Touch_SCL, Touch_SDA, Touch_INT, Touch_ADDR, TFT_rot);


// LVGL calls it when a rendered image needs to copied to the display
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);

    // Call it to tell LVGL everthing is ready
    lv_disp_flush_ready(disp);
}

// Read the touchpad
void my_touchpad_read(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    static uint16_t last_x = 0, last_y = 0;	
    if (touch.touched()) {
		uint16_t x, y;
        touch.readData(&x, &y);
        last_x = x;
        last_y = y;
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}


void init_lvgl(){
	Serial.println("Arduino_GFX LVGL ");
    String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch() + " example";
    Serial.println(LVGL_Arduino);
	

    // Display setup
    if(!gfx->begin(40000000UL)) {
        Serial.println("Failed to initialize display!");
        return;
    }
    gfx->fillScreen(BLACK);

    // Switch backlight on
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
	
    // Touch setup
    if(!touch.begin()) {
        Serial.println("Failed to initialize touch module!");
        return;
    }
    touch.enOffsetCorrection(true);
    touch.setOffsets(Touch_X_min, Touch_X_max, TFT_res_W-1, Touch_Y_min, Touch_Y_max, TFT_res_H-1);

    // Init LVGL
    lv_init();

    // Initialize the display buffer
    uint32_t screenWidth = gfx->width();
    uint32_t screenHeight = gfx->height();
    uint32_t bufSize = screenWidth * screenHeight / 10;
    lv_color_t *disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        Serial.println("LVGL failed to allocate display buffer!");
        return;
    }

    // Initialize display buffer
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, bufSize);

    // Initialize the display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Initialize the input device driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);	
};

// Event callback function must be defined at file scope, not inside setup()
static void btn_event_cb(lv_event_t * e) {
	if(e->code == LV_EVENT_RELEASED) {
		lv_obj_t * btn = lv_event_get_target(e);
		lv_obj_t * label2 = (lv_obj_t *)lv_obj_get_user_data(btn);
		lv_label_set_text(label2, "Dont do this again!11!!");
	}else if(e->code == LV_EVENT_PRESSED) {
		lv_obj_t * btn = lv_event_get_target(e);
		lv_obj_t * label2 = (lv_obj_t *)lv_obj_get_user_data(btn);
		lv_label_set_text(label2, "BOOM PENG CRUSH!!!!");
	}
    
}

void setup() {
	Serial.begin(115200);
	while(!Serial);
	init_lvgl();

	//Test Button

	lv_obj_set_style_bg_color(lv_scr_act(), lv_palette_main(LV_PALETTE_GREEN),0);
	lv_obj_t * btn_cont = lv_btn_create(lv_scr_act());
	lv_obj_set_size(btn_cont, 240, 60);
	lv_obj_align(btn_cont, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_color(btn_cont, lv_palette_main(LV_PALETTE_RED), 0);
	lv_obj_t * label = lv_label_create(btn_cont);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
	lv_label_set_text(label, "Big RED Button");
	lv_obj_center(label);	
	
    lv_obj_t * label2 = lv_label_create(lv_scr_act());
    lv_label_set_text(label2, "Dont push the button!");
	lv_obj_align(label2, LV_ALIGN_TOP_MID, 0, 40);	
	lv_obj_set_style_text_font(label2, &lv_font_montserrat_30, 0);
	lv_obj_set_style_text_color(label2, lv_palette_main(LV_PALETTE_NONE), 0);	

    // Set label2 as user data for the button
    lv_obj_set_user_data(btn_cont, label2);

    // Register the event callback function
    lv_obj_add_event_cb(btn_cont, btn_event_cb, LV_EVENT_PRESSED , 0);
	lv_obj_add_event_cb(btn_cont, btn_event_cb, LV_EVENT_RELEASED , 0);

}



void loop() {	
	lv_task_handler(); 
	gfx->flush();
	delay(5);
}


