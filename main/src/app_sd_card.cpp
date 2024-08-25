#include "app_sd_card.hpp"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"

#define MOUNT_POINT "/sdcard"
#define BMP_HEADER_SIZE 54

static const char *TAG = "App/SDCard";
sdmmc_card_t *card;

AppSDCard::AppSDCard(AppButton *key, 
                     QueueHandle_t queue_i,
                     QueueHandle_t queue_o,
                     void (*callback)(camera_fb_t *)) : Frame(queue_i, queue_o, callback),
                                                        key(key),
                                                        state(SDCARD_IDLE),
                                                        switch_on(false)
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
}

AppSDCard::~AppSDCard()
{
    // Empty
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
        if (this->key->pressed == BUTTON_MENU)
        {
            this->state = SDCARD_IDLE;
            this->switch_on = (this->key->menu == MENU_DISPLAY_ONLY) ? true : false;
            ESP_LOGI(TAG, "%s", this->switch_on ? "ON" : "OFF");
        }
        else if (this->key->pressed == BUTTON_PLAY)
        {
            this->state = SDCARD_TAKE_PHOTO;
        }
        else if (this->key->pressed == BUTTON_UP)
        {
            this->state = SDCARD_START_RECORD;
        }
        else if (this->key->pressed == BUTTON_DOWN)
        {
            this->state = SDCARD_STOP_RECORD;
        }
    }
}

void write_bmp_header(FILE *file, uint32_t width, uint32_t height)
{
    uint32_t row_padded = (width + 3) & (~3); // Row size needs to be aligned to 4 bytes
    uint32_t pixel_data_size = row_padded * height;
    uint32_t file_size = BMP_HEADER_SIZE + pixel_data_size;
    uint32_t offset = BMP_HEADER_SIZE;

    // BMP header
    uint8_t header[BMP_HEADER_SIZE] = {
        'B', 'M', // Signature
        (uint8_t)(file_size & 0xFF), (uint8_t)((file_size >> 8) & 0xFF),
        (uint8_t)((file_size >> 16) & 0xFF), (uint8_t)((file_size >> 24) & 0xFF), // File size
        0, 0,                                                                     // Reserved
        0, 0,                                                                     // Reserved
        (uint8_t)(offset & 0xFF), (uint8_t)((offset >> 8) & 0xFF),
        (uint8_t)((offset >> 16) & 0xFF), (uint8_t)((offset >> 24) & 0xFF), // Pixel data offset
        40, 0, 0, 0,                                                        // Header size
        (uint8_t)(width & 0xFF), (uint8_t)((width >> 8) & 0xFF),
        (uint8_t)((width >> 16) & 0xFF), (uint8_t)((width >> 24) & 0xFF), // Image width
        (uint8_t)(height & 0xFF), (uint8_t)((height >> 8) & 0xFF),
        (uint8_t)((height >> 16) & 0xFF), (uint8_t)((height >> 24) & 0xFF), // Image height
        1, 0,                                                               // Planes
        8, 0,                                                               // Bits per pixel (8 for grayscale)
        0, 0, 0, 0,                                                         // Compression (0 = none)
        (uint8_t)(pixel_data_size & 0xFF), (uint8_t)((pixel_data_size >> 8) & 0xFF),
        (uint8_t)((pixel_data_size >> 16) & 0xFF), (uint8_t)((pixel_data_size >> 24) & 0xFF), // Image size
        0, 0, 0, 0,                                                                           // Horizontal resolution (pixels per meter)
        0, 0, 0, 0,                                                                           // Vertical resolution (pixels per meter)
        0, 0, 0, 0,                                                                           // Colors in color palette (0 means 256 for grayscale)
        0, 0, 0, 0                                                                            // Important colors
    };

    fwrite(header, 1, BMP_HEADER_SIZE, file);

    // Write the grayscale palette (0-255)
    for (int i = 0; i < 256; i++)
    {
        uint8_t grayscale_palette[4] = {(uint8_t)i, (uint8_t)i, (uint8_t)i, 0}; // RGB + Reserved byte
        fwrite(grayscale_palette, 1, 4, file);
    }
}

static void take_photo_to_sdcard(camera_fb_t *frame)
{
    char path[64];
    sprintf(path, MOUNT_POINT "/photo_3.bmp");

    FILE *file = fopen(path, "wb");
    if (file != NULL)
    {
        write_bmp_header(file, frame->width, frame->height);

        for (int i = 0; i < frame->height; i++)
        {
            for (int j = frame->width - 1; j >= 0; j--)
            {
                fwrite(&frame->buf[i * frame->width + j], 1, 1, file);
            }

            // Padding rows to 4-byte boundary
            uint8_t padding[3] = {0, 0, 0};
            fwrite(padding, 1, (4 - (frame->width % 4)) % 4, file);
        }

        fclose(file);
        ESP_LOGI(TAG, "Photo saved to %s", path);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
    }
}

static void task(AppSDCard *self)
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
                if (self->state)
                {
                    switch (self->state)
                    {
                    case SDCARD_TAKE_PHOTO:
                    {
                        take_photo_to_sdcard(frame);
                        break;
                    }
                    case SDCARD_START_RECORD:
                    {
                        // start_record_to_sdcard(frame);
                        break;
                    }
                    case SDCARD_STOP_RECORD:
                    {
                        // stop_record_to_sdcard(frame);
                        break;
                    }
                    default:
                        break;
                    }
                }      
                self->state = SDCARD_IDLE;          
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

void AppSDCard::run()
{   
    xTaskCreatePinnedToCore((TaskFunction_t)task, TAG, 4 * 1024, this, 5, NULL, 1);
}

