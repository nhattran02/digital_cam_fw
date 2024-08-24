#pragma once

#include "sdkconfig.h"
#include "__base__.hpp"
#include "app_camera.hpp"
#include "app_button.hpp"

typedef enum
{
    OPENCV_IDLE = 0,
    OPENCV_EDGES = 1,
    OPENCV_THRESHOLD = 2,
    OPENCV_DELETE = 3,
} opencv_action_t;

class AppOpenCV : public Observer, public Frame
{
private:
    AppButton *key;

public:
    opencv_action_t state;
    opencv_action_t state_previous;
    
    bool switch_on;

    uint8_t frame_count;

    AppOpenCV(AppButton *key,
            QueueHandle_t queue_i = nullptr,
            QueueHandle_t queue_o = nullptr,
            void (*callback)(camera_fb_t *) = esp_camera_fb_return);
    ~AppOpenCV();

    void update();
    void run();
};
