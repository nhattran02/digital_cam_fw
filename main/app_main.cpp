#include "driver/gpio.h"
#include "app_button.hpp"
#include "app_camera.hpp"
#include "app_lcd.hpp"
#include "app_led.hpp"
#include "app_webserver.hpp"
#include "app_opencv.hpp"
#include "app_usb_msc.hpp"
#include "app_sd_card.hpp"
#include "utils.hpp"

extern "C" void app_main()
{
    QueueHandle_t xQueueFrame1 = xQueueCreate(2, sizeof(camera_fb_t *));
    QueueHandle_t xQueueFrame2 = xQueueCreate(2, sizeof(camera_fb_t *));
    QueueHandle_t xQueueFrame3 = xQueueCreate(2, sizeof(camera_fb_t *));

    AppButton *key = new AppButton();

    AppCamera *camera = new AppCamera(PIXFORMAT_GRAYSCALE, FRAMESIZE_240X240, 2, xQueueFrame1);
    AppOpenCV *opencv = new AppOpenCV(key, xQueueFrame1, xQueueFrame2);
    AppSDCard *sd_card = new AppSDCard(key, xQueueFrame2, xQueueFrame3);
    AppLCD *lcd = new AppLCD(key, xQueueFrame3);

    AppWebServer *webserver = new AppWebServer(key);
    AppLED *led = new AppLED(GPIO_NUM_3, key);
    AppUSBMSC *usb_msc = new AppUSBMSC();

    key->attach(opencv);
    key->attach(webserver);
    key->attach(led);
    key->attach(lcd);
    key->attach(sd_card);

    lcd->run();
    opencv->run();
    camera->run();
    key->run();
    sd_card->run();
}
