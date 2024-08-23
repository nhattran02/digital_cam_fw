#pragma once

#include "esp_log.h"
#include "app_button.hpp"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

extern sdmmc_card_t *card;

class AppSDCard : public Observer
{
private:
    AppButton *key;

public:
    bool switch_on;

    AppSDCard(AppButton *key);

    void update();
    
    esp_err_t write_file(const char *path, char *data);
    esp_err_t read_file(const char *path, char *data, size_t size);
};


