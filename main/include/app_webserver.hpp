#pragma once

#include "__base__.hpp"
#include "app_camera.hpp"
#include "app_button.hpp"


class AppWebServer : public Observer
{
private:
    AppButton *key;

public:
    bool switch_on;

    AppWebServer(AppButton *key);

    void update();
};
