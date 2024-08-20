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

static const char TAG[] = "App/Webserver";
static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueHttpFrame = NULL;
volatile bool is_webserver_config = false;
volatile bool is_register_httpd = false;

AppWebServer::AppWebServer(AppButton *key,
                           QueueHandle_t queue_i,
                           QueueHandle_t queue_o,
                           void (*callback)(camera_fb_t *)) : Frame(queue_i, queue_o, callback),
                                                              key(key),
                                                              switch_on(false)
{
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
                app_wifi_main();
                size_t free_mem_before = esp_get_free_heap_size();
                ESP_LOGI(TAG, "Free heap memory before init: %d bytes", free_mem_before);
                xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
                xQueueHttpFrame = xQueueCreate(2, sizeof(camera_fb_t *));

                if (is_camera_allow_run == true)
                {
                    is_camera_allow_run = false;
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    esp_camera_deinit();
                    register_camera(PIXFORMAT_RGB565, FRAMESIZE_240X240, 2, xQueueAIFrame);
                }

                app_mdns_main();
                register_human_face_detection(xQueueAIFrame, NULL, NULL, xQueueHttpFrame);
                register_httpd(xQueueHttpFrame, NULL, true);
                
                size_t free_mem_after = esp_get_free_heap_size();
                ESP_LOGI(TAG, "Free heap memory after init: %d bytes", free_mem_after);                
            }
        }
    }
}

static void task(AppWebServer *self)
{
    ESP_LOGI(TAG, "Start");
    if (xQueueAIFrame == NULL)
        xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    if (xQueueHttpFrame == NULL)
        xQueueHttpFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    
    while (true)
    {
        if (self->queue_i == nullptr)
            break;

        camera_fb_t *frame = NULL;
        if (xQueueReceive(self->queue_i, &frame, portMAX_DELAY))
        {
            if (self->switch_on)
            {
                if (is_webserver_config == false)
                {
                    app_wifi_main();
                    sensor_t* s = esp_camera_sensor_get();
                    s->set_pixformat(s, PIXFORMAT_RGB565);
                    s->set_vflip(s, 0);                    
                    app_mdns_main();
                    // register_human_face_detection(self->queue_i, NULL, NULL, xQueueHttpFrame);
                    // register_httpd(xQueueHttpFrame, NULL, true);
                    is_webserver_config = true;
                }
            }

            if (self->queue_o)
                xQueueSend(self->queue_o, &frame, portMAX_DELAY);
            else
                self->callback(frame);            
        }
    }
    ESP_LOGI(TAG, "Stop");
    vTaskDelete(NULL);
}

void AppWebServer::run()
{
    xTaskCreatePinnedToCore((TaskFunction_t)task, TAG, 5 * 1024, this, 5, NULL, 0);
}