#include "driver/gpio.h"
#include "app_button.hpp"
#include "app_camera.hpp"
#include "app_lcd.hpp"
#include "app_led.hpp"
#include "app_webserver.hpp"
#include "app_face.hpp"

extern "C" void app_main()
{
    QueueHandle_t xQueueCamFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    QueueHandle_t xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    QueueHandle_t xQueueLCDFrame = xQueueCreate(2, sizeof(camera_fb_t *));

    AppButton *key = new AppButton();
    AppCamera *camera = new AppCamera(PIXFORMAT_GRAYSCALE, FRAMESIZE_240X240, 2, xQueueCamFrame);
    AppFace *face = new AppFace(key, xQueueCamFrame, xQueueAIFrame);
    AppLCD *lcd = new AppLCD(key, xQueueAIFrame);
    AppWebServer *webserver= new AppWebServer(key, xQueueAIFrame, xQueueLCDFrame);
    AppLED *led = new AppLED(GPIO_NUM_3, key);
            
    key->attach(face);
    key->attach(webserver);
    key->attach(led);
    key->attach(lcd);
    
    lcd->run();
    // webserver->run();
    face->run();
    camera->run();
    key->run();
}
