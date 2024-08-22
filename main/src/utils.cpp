#include "utils.hpp"
#include "freertos/FreeRTOS.h"

void print_mem_info(const char *TAG) 
{
   ESP_LOGW(TAG,
        "\n  Free Heap: %u bytes\n"
        "  MALLOC_CAP_8BIT      %7zu bytes\n"
        "  MALLOC_CAP_DMA       %7zu bytes\n"
        "  MALLOC_CAP_SPIRAM    %7zu bytes\n"
        "  MALLOC_CAP_INTERNAL  %7zu bytes\n"
        "  MALLOC_CAP_DEFAULT   %7zu bytes\n"
        "  MALLOC_CAP_IRAM_8BIT %7zu bytes\n"
        "  MALLOC_CAP_RETENTION %7zu bytes\n",
        xPortGetFreeHeapSize(),
        heap_caps_get_free_size(MALLOC_CAP_8BIT),
        heap_caps_get_free_size(MALLOC_CAP_DMA),
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
        heap_caps_get_free_size(MALLOC_CAP_IRAM_8BIT),
        heap_caps_get_free_size(MALLOC_CAP_RETENTION)
    );
}