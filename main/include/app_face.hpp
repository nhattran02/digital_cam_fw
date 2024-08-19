#pragma once

#include "sdkconfig.h"
#include "__base__.hpp"
#include "app_camera.hpp"
#include "app_button.hpp"

typedef enum
{
    FACE_IDLE = 0,
    FACE_EDGES = 1,
    FACE_THRESHOLD = 2,
    FACE_DELETE = 3,
} face_action_t;

class AppFace : public Observer, public Frame
{
private:
    AppButton *key;

public:
    face_action_t state;
    face_action_t state_previous;

    bool switch_on;

    uint8_t frame_count;

    AppFace(AppButton *key,
            QueueHandle_t queue_i = nullptr,
            QueueHandle_t queue_o = nullptr,
            void (*callback)(camera_fb_t *) = esp_camera_fb_return);
    ~AppFace();

    void update();
    void run();
};
