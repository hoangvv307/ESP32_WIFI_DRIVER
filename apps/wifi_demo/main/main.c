#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

#include "wifi_sta.h"

static const uint64_t connection_timeout_ms=10000;
static const uint32_t sleep_ms=1000;

static const char *TAG="wifi_demo";

void app_main(void)
{
    esp_err_t esp_ret;
    EventGroupHandle_t network_event_group;
    EventBits_t network_event_bits;

esp_ret= nvs_flash_init();
if((esp_ret==ESP_ERR_NVS_NO_FREE_PAGES)||(esp_ret==ESP_ERR_NVS_NEW_VERSION_FOUND)){
    ESP_ERROR_CHECK(nvs_flash_erase());
    esp_ret =nvs_flash_init();

}
if(esp_ret!=ESP_OK){
    ESP_LOGE(TAG,"Error (%d): Failed to initialize NVS", esp_ret);
    abort();
}
esp_ret =esp_netif_init();
if(esp_ret!=ESP_OK){
    ESP_LOGE(TAG,"Error (%d): Failed to initialize network interface", esp_ret);
    abort();
}



}