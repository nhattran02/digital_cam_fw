#pragma once

#include "__base__.hpp"
#include "app_camera.hpp"
#include "app_button.hpp"

class AppMotion : public Observer, public Frame
{
private:
    AppButton *key;

public:
    bool switch_on;

    AppMotion(AppButton *key,
              QueueHandle_t queue_i = nullptr,
              QueueHandle_t queue_o = nullptr,
              void (*callback)(camera_fb_t *) = esp_camera_fb_return);

    void update();

    void run();
};
