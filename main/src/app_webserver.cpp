#include "app_webserver.hpp"
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#include "esp_log.h"
#include "esp_camera.h"
#include "app_camera.hpp"
#include "who_ai_utils.hpp"
#include "dl_image.hpp"
#include "app_wifi.h"
#include "app_httpd.hpp"
#include "app_mdns.h"
#include "who_camera.h"
#include "who_human_face_detection.hpp"
#include "utils.hpp"
#include "esp_wifi.h"

static const char TAG[] = "App/Webserver";
static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueHttpFrame = NULL;
volatile bool is_webserver_config = false;
static sensor_t *s = nullptr;

AppWebServer::AppWebServer(AppButton *key) : key(key), switch_on(false)
{
    
}

static void app_camera_reinit(const pixformat_t pixel_fromat,
                              const framesize_t frame_size,
                              const uint8_t fb_count)
{
#if 1
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAMERA_PIN_D0;
    config.pin_d1 = CAMERA_PIN_D1;
    config.pin_d2 = CAMERA_PIN_D2;
    config.pin_d3 = CAMERA_PIN_D3;
    config.pin_d4 = CAMERA_PIN_D4;
    config.pin_d5 = CAMERA_PIN_D5;
    config.pin_d6 = CAMERA_PIN_D6;
    config.pin_d7 = CAMERA_PIN_D7;
    config.pin_xclk = CAMERA_PIN_XCLK;
    config.pin_pclk = CAMERA_PIN_PCLK;
    config.pin_vsync = CAMERA_PIN_VSYNC;
    config.pin_href = CAMERA_PIN_HREF;
    config.pin_sscb_sda = CAMERA_PIN_SIOD;
    config.pin_sscb_scl = CAMERA_PIN_SIOC;
    config.pin_pwdn = CAMERA_PIN_PWDN;
    config.pin_reset = CAMERA_PIN_RESET;
    config.xclk_freq_hz = XCLK_FREQ_HZ;
    config.pixel_format = pixel_fromat;
    config.frame_size = frame_size;
    config.jpeg_quality = 0;
    config.fb_count = fb_count;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        
    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }
#endif

    s = esp_camera_sensor_get();

    if (s->id.PID == OV3660_PID || s->id.PID == OV2640_PID) {
        s->set_vflip(s, 1); //flip it back    
    }   
    else if (s->id.PID == GC0308_PID) {
        s->set_hmirror(s, 0);
    }
    else if (s->id.PID == GC032A_PID) {
        s->set_vflip(s, 1);
    }

    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID)
    {
        s->set_brightness(s, 1);  //up the brightness just a bit
        s->set_saturation(s, -2); //lower the saturation
    }
}


void AppWebServer::update()
{
    if (this->key->pressed > BUTTON_IDLE)
    {
        if (this->key->pressed == BUTTON_MENU)
        {
            this->switch_on = (this->key->menu == MENU_WEBSERVER) ? true : false;
            ESP_LOGI(TAG, "%s", this->switch_on ? "ON" : "OFF");

            if (this->switch_on == true)
            {

                if (xQueueAIFrame == NULL)
                    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));

                if (xQueueHttpFrame == NULL)
                    xQueueHttpFrame = xQueueCreate(2, sizeof(camera_fb_t *));

                if (is_camera_allow_run == true)
                {
                    is_camera_allow_run = false;
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    ESP_ERROR_CHECK(esp_camera_deinit());
                    register_camera(PIXFORMAT_RGB565, FRAMESIZE_240X240, 2, xQueueAIFrame);
                }
                if (is_webserver_config == false)
                {
                    app_wifi_main();
                    app_mdns_main();
                    register_human_face_detection(xQueueAIFrame, NULL, NULL, xQueueHttpFrame);
                    register_httpd(xQueueHttpFrame, NULL, true);
                    is_webserver_config = true;
                }
            }
            else 
            {
                // Stack overflow HERE........
                if (is_camera_allow_run == false)
                {
                    unregister_httpd();
                    unregister_human_face_detection();
                    app_mdns_stop();
                    app_wifi_stop();
                    unregister_camera();
                    if (xQueueAIFrame != NULL) {
                        vQueueDelete(xQueueAIFrame);
                        xQueueAIFrame = NULL;
                    }

                    if (xQueueHttpFrame != NULL) {
                        vQueueDelete(xQueueHttpFrame);
                        xQueueHttpFrame = NULL;
                    }                    
            
                    app_camera_reinit(PIXFORMAT_GRAYSCALE, FRAMESIZE_240X240, 2);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    is_camera_allow_run = true;
                    is_webserver_config = false;
                }
            }
        }   
    }
}
