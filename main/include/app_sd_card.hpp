#pragma once

#include "esp_log.h"
#include "app_button.hpp"
#include "sdmmc_cmd.h"
#include "__base__.hpp"
#include "driver/sdmmc_host.h"
#include "app_camera.hpp"

extern sdmmc_card_t *card;

typedef enum
{
    SDCARD_IDLE = 0,
    SDCARD_TAKE_PHOTO = 1,
    SDCARD_START_RECORD = 2,
    SDCARD_STOP_RECORD = 3,
} sdcard_action_t;

class AppSDCard : public Observer, public Frame
{
private:
    AppButton *key;

public:
    sdcard_action_t state;
    bool switch_on;

    AppSDCard(AppButton *key,
              QueueHandle_t queue_i = nullptr,
              QueueHandle_t queue_o = nullptr,
              void (*callback)(camera_fb_t *) = esp_camera_fb_return);
    ~AppSDCard();
    void update();
    void run();
    esp_err_t write_file(const char *path, char *data);
    esp_err_t read_file(const char *path, char *data, size_t size);
};
