#include "app_sd_card.hpp"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"


#define MOUNT_POINT "/sdcard"

static const char *TAG = "App/SDCard";
sdmmc_card_t *card;

AppSDCard::AppSDCard(AppButton *key) : key(key), switch_on(false)
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");
    ESP_LOGI(TAG, "Using SDMMC peripheral");    

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.width = 1;    
    slot_config.clk = GPIO_NUM_39;
    slot_config.cmd = GPIO_NUM_38;
    slot_config.d0 = GPIO_NUM_40;   
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");    

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // ------------------------------
    const char *file = MOUNT_POINT"/hihi.txt";
    char data[64];
    snprintf(data, 64, "This is HIHI's Host\n");
    ret = write_file(file, data);
    if (ret != ESP_OK) 
    {
        return;
    }    
    // ------------------------------

}

esp_err_t AppSDCard::write_file(const char *path, char *data)
{
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, "%s", data);
    fclose(f);
    ESP_LOGI(TAG, "File written");
    return ESP_OK;
}

esp_err_t AppSDCard::read_file(const char *path, char *data, size_t size)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    if (fgets(data, size, f) == NULL) {
        if (feof(f)) {
            ESP_LOGI(TAG, "End of file reached");
        } else {
            ESP_LOGE(TAG, "Failed to read from file");
        }
    } else {
        ESP_LOGI(TAG, "File read");
    }
    fclose(f);
    return ESP_OK;
}


void AppSDCard::update()
{
    // Parse key
    if (this->key->pressed > BUTTON_IDLE)
    {

    }
}
