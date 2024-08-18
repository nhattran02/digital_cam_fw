#include "app_motion.hpp"

#include "esp_log.h"
#include "esp_camera.h"

#include "dl_image.hpp"

static const char TAG[] = "App/Motion";

AppMotion::AppMotion(AppButton *key,
                     QueueHandle_t queue_i,
                     QueueHandle_t queue_o,
                     void (*callback)(camera_fb_t *)) : Frame(queue_i, queue_o, callback),
                                                        key(key),
                                                        switch_on(false) {}

void AppMotion::update()
{
    if (this->key->pressed > BUTTON_IDLE)
    {
        if (this->key->pressed == BUTTON_MENU)
        {
            this->switch_on = (this->key->menu == MENU_MOTION_DETECTION) ? true : false;
            ESP_LOGI(TAG, "%s", this->switch_on ? "ON" : "OFF");
        }
    }
}

static void task(AppMotion *self)
{
    ESP_LOGI(TAG, "Start");
    while (true)
    {
        if (self->queue_i == nullptr)
            break;

        camera_fb_t *frame1 = NULL;
        camera_fb_t *frame2 = NULL;
        if (xQueueReceive(self->queue_i, &frame1, portMAX_DELAY))
        {
            if (self->switch_on)
            {
                if (xQueueReceive(self->queue_i, &frame2, portMAX_DELAY))
                {
                    uint32_t moving_point_number = dl::image::get_moving_point_number((uint16_t *)frame1->buf, (uint16_t *)frame2->buf, frame1->height, frame1->width, 8, 15);
                    if (moving_point_number > 50)
                    {
                        ESP_LOGI(TAG, "Something moved!");
                        dl::image::draw_filled_rectangle((uint16_t *)frame1->buf, frame1->height, frame1->width, 0, 0, 20, 20);
                    }

                    self->callback(frame2);
                }
            }

            if (self->queue_o)
                xQueueSend(self->queue_o, &frame1, portMAX_DELAY);
            else
                self->callback(frame1);
        }
    }
    ESP_LOGI(TAG, "Stop");
    vTaskDelete(NULL);
}

void AppMotion::run()
{
    xTaskCreatePinnedToCore((TaskFunction_t)task, TAG, 3 * 1024, this, 5, NULL, 0);
}