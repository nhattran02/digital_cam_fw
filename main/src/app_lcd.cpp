#include "app_lcd.hpp"
#include <string.h>
#include "esp_log.h"
#include "esp_camera.h"
#include "logo_en_240x240_lcd.h"
extern "C" {
#include "st7789.h"
#include "fontx.h"
}
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#define LCD_INVERSION   1

static const char TAG[] = "App/LCD";
static TFT_t dev;
static FontxFile fx24G[2];

static void spiffs_directory(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(__FUNCTION__,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

static void print_webserver_info(void)
{
    uint8_t ascii[30] = {0};
    strcpy((char *)ascii, "SSID:Digital-Camera");
    lcdDrawString(&dev, fx24G, 235, 135, ascii, BLACK);
    strcpy((char *)ascii, "Password:12345678");
    lcdDrawString(&dev, fx24G, 225, 105, ascii, BLACK);
    strcpy((char *)ascii, "Access:camera.local");
    lcdDrawString(&dev, fx24G, 235, 75, ascii, BLACK);
    lcdDrawFinish(&dev);
}

AppLCD::AppLCD(AppButton *key,
               QueueHandle_t queue_i,
               QueueHandle_t queue_o,
               void (*callback)(camera_fb_t *)) : Frame(queue_i, queue_o, callback),
                                                  key(key),
                                                  panel_handle(NULL),
                                                  switch_on(false)
{
	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 12,
		.format_if_mount_failed =true
	};

	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total,&used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
	}

    spiffs_directory("/spiffs/");

    // Set font file 
    InitFontx(fx24G,"/spiffs/ILGH24XB.FNT","");

    spi_master_init(&dev, BOARD_LCD_MOSI, BOARD_LCD_SCK, BOARD_LCD_CS, BOARD_LCD_DC, BOARD_LCD_RST, BOARD_LCD_BL);
	lcdInit(&dev, BOARD_LCD_V_RES, BOARD_LCD_H_RES, 0, 0);

#if CONFIG_INVERSION
    ESP_LOGI(TAG, "Enable Display Inversion");
    lcdInversionOn(&dev);
#endif

    this->draw_color(rgb565(239, 239, 239));
    lcdDrawFinish(&dev);
    vTaskDelay(pdMS_TO_TICKS(500));

    this->draw_wallpaper();
    lcdDrawFinish(&dev);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void AppLCD::draw_wallpaper()
{
    TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

    uint16_t *pixels = (uint16_t *)heap_caps_malloc((logo_en_240x240_lcd_width * logo_en_240x240_lcd_height) * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (NULL == pixels)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return;
    }
    memcpy(pixels, logo_en_240x240_lcd, (logo_en_240x240_lcd_width * logo_en_240x240_lcd_height) * sizeof(uint16_t));
        
    for (uint16_t y = 0; y < logo_en_240x240_lcd_height; y++) 
    {
        uint16_t inverted_y = logo_en_240x240_lcd_height - 1 - y;
        uint16_t *line = &pixels[inverted_y * logo_en_240x240_lcd_width];
        
        for (uint16_t x = 0; x < logo_en_240x240_lcd_width / 2; x++) 
        {
            uint16_t temp = line[x];
            line[x] = line[logo_en_240x240_lcd_width - 1 - x]   ;
            line[logo_en_240x240_lcd_width - 1 - x] = temp;
        }

        lcdDrawMultiPixels(&dev, 0, y, logo_en_240x240_lcd_width, line);
    }

    heap_caps_free(pixels);

    this->paper_drawn = true;

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%"PRIu32,diffTick*portTICK_PERIOD_MS);
}

void AppLCD::draw_color(int color)
{
    uint16_t *buffer = (uint16_t *)malloc(BOARD_LCD_H_RES * sizeof(uint16_t));
    if (NULL == buffer)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
    }
    else
    {
        for (size_t i = 0; i < BOARD_LCD_H_RES; i++)
        {
            buffer[i] = color;
        }

        for (int y = 0; y < BOARD_LCD_V_RES; y++)
        {
            lcdDrawMultiPixels(&dev, 0, y, BOARD_LCD_H_RES, buffer);
        }

        free(buffer);
    }
}

void AppLCD::update()
{
    if (this->key->pressed > BUTTON_IDLE)
    {
        if (this->key->pressed == BUTTON_MENU)
        {
            this->switch_on = (this->key->menu == MENU_STOP_WORKING || this->key->menu == MENU_WEBSERVER) ? false : true;
            if(this->key->menu == MENU_WEBSERVER)
            {
                // this->draw_wallpaper();
                this->draw_color(rgb565(239, 239, 239));
                lcdSetFontDirection(&dev, 2);                
                print_webserver_info();
            }
            ESP_LOGI(TAG, "%s", this->switch_on ? "ON" : "OFF");
        }
    }

    if (this->switch_on == false)
    {
        this->paper_drawn = false;
    }
}

static void task(AppLCD *self)
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
                for (int y = 0; y < frame->height; y++)
                {
                    lcdDrawMultiPixelsGrayScale(&dev, 0, y, frame->width, (uint8_t *)frame->buf + y * frame->width);
                }
            }
            else if (self->paper_drawn == false)
            {
                self->draw_wallpaper();
            }

            if (self->queue_o)
                xQueueSend(self->queue_o, &frame, portMAX_DELAY);
            else
                self->callback(frame);
        }
    }
    ESP_LOGI(TAG, "Stop");
    self->draw_wallpaper();
    vTaskDelete(NULL);
}

void AppLCD::run()
{
    xTaskCreatePinnedToCore((TaskFunction_t)task, TAG, 3 * 1024, this, 5, NULL, 1);
}