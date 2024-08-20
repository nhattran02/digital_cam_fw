#undef EPS
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#define EPS 192

#include "app_face.hpp"

#include <list>

#include "esp_log.h"
#include "esp_camera.h"

#include "dl_image.hpp"
#include "fb_gfx.h"

#include "who_ai_utils.hpp"

static const char TAG[] = "App/Face";
using namespace cv;


#define RGB565_MASK_RED 0xF800
#define RGB565_MASK_GREEN 0x07E0
#define RGB565_MASK_BLUE 0x001F

#define FRAME_DELAY_NUM 16

static void rgb_print(camera_fb_t *fb, uint32_t color, const char *str)
{
    fb_gfx_print(fb, (fb->width - (strlen(str) * 14)) / 2, 10, color, str);
}

static int rgb_printf(camera_fb_t *fb, uint32_t color, const char *format, ...)
{
    char loc_buf[64];
    char *temp = loc_buf;
    int len;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(copy);
    if (len >= sizeof(loc_buf))
    {
        temp = (char *)malloc(len + 1);
        if (temp == NULL)
        {
            return 0;
        }
    }
    vsnprintf(temp, len + 1, format, arg);
    va_end(arg);
    rgb_print(fb, color, temp);
    if (len > 64)
    {
        free(temp);
    }
    return len;
}

AppFace::AppFace(AppButton *key,
                 QueueHandle_t queue_i,
                 QueueHandle_t queue_o,
                 void (*callback)(camera_fb_t *)) : Frame(queue_i, queue_o, callback),
                                                    key(key),
                                                    state(FACE_IDLE),
                                                    switch_on(false)
{
    
}

AppFace::~AppFace()
{
    // Empty 
}

void AppFace::update()
{
    // Parse key
    if (this->key->pressed > BUTTON_IDLE)
    {
        if (this->key->pressed == BUTTON_MENU)
        {
            this->state = FACE_IDLE;
            this->switch_on = (this->key->menu == MENU_OPENCV) ? true : false;
            ESP_LOGI(TAG, "%s", this->switch_on ? "ON" : "OFF");
        }
        else if (this->key->pressed == BUTTON_PLAY)
        {
            this->state = FACE_THRESHOLD;
        }
        else if (this->key->pressed == BUTTON_UP)
        {
            this->state = FACE_EDGES;
        }
        else if (this->key->pressed == BUTTON_DOWN)
        {
            this->state = FACE_DELETE;
        }
    }
}


static void task(AppFace *self)
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
                    case FACE_EDGES:
                    {
                        Sobel(input_img, input_img, CV_8UC1, 1, 1, 5);
                        break;
                    }
                    case FACE_THRESHOLD:
                    {
                        threshold(input_img, input_img, 127, 255, THRESH_BINARY);
                        break;
                    }
                    case FACE_DELETE:
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




void AppFace::run()
{
    xTaskCreatePinnedToCore((TaskFunction_t)task, TAG, 6 * 1024, this, 5, NULL, 1);
}