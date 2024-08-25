#undef EPS
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#define EPS 192

#include "app_opencv.hpp"

#include <list>

#include "esp_log.h"
#include "esp_camera.h"

#include "dl_image.hpp"
#include "fb_gfx.h"

#include "who_ai_utils.hpp"

static const char TAG[] = "App/OpenCV";
using namespace cv;

#define RGB565_MASK_RED 0xF800
#define RGB565_MASK_GREEN 0x07E0
#define RGB565_MASK_BLUE 0x001F

#define FRAME_DELAY_NUM 16

AppOpenCV::AppOpenCV(AppButton *key,
                     QueueHandle_t queue_i,
                     QueueHandle_t queue_o,
                     void (*callback)(camera_fb_t *)) : Frame(queue_i, queue_o, callback),
                                                        key(key),
                                                        state(OPENCV_IDLE),
                                                        switch_on(false)
{
    // Empty
}

AppOpenCV::~AppOpenCV()
{
    // Empty
}

void AppOpenCV::update()
{
    // Parse key
    if (this->key->pressed > BUTTON_IDLE)
    {
        if (this->key->pressed == BUTTON_MENU)
        {
            this->state = OPENCV_IDLE;
            this->switch_on = (this->key->menu == MENU_OPENCV) ? true : false;
            ESP_LOGI(TAG, "%s", this->switch_on ? "ON" : "OFF");
        }
        else if (this->key->pressed == BUTTON_PLAY)
        {
            this->state = OPENCV_THRESHOLD;
        }
        else if (this->key->pressed == BUTTON_UP)
        {
            this->state = OPENCV_EDGES;
        }
        else if (this->key->pressed == BUTTON_DOWN)
        {
            this->state = OPENCV_DELETE;
        }
    }
}

static void task(AppOpenCV *self)
{
    ESP_LOGI(TAG, "Start");
    camera_fb_t *frame = nullptr;

    while (true)
    {
        if (self->queue_i == nullptr)
            break;

        if (xQueueReceive(self->queue_i, &frame, portMAX_DELAY))
        {
            if (self->switch_on)
            {

                Mat input_img = Mat(frame->height, frame->width, CV_8UC1, frame->buf);

                if (self->state)
                {
                    switch (self->state)
                    {
                    case OPENCV_EDGES:
                    {
                        Sobel(input_img, input_img, CV_8UC1, 1, 1, 5);
                        break;
                    }
                    case OPENCV_THRESHOLD:
                    {
                        threshold(input_img, input_img, 127, 255, THRESH_BINARY);
                        break;
                    }
                    case OPENCV_DELETE:
                    {
                        break;
                    }
                    default:
                        break;
                    }
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

void AppOpenCV::run()
{
    xTaskCreatePinnedToCore((TaskFunction_t)task, TAG, 6 * 1024, this, 5, NULL, 1);
}