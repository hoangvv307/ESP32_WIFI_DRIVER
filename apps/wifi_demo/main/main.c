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

esp_ret = esp_event_loop_create_default();
 if (esp_ret!=ESP_OK){
    ESP_LOGE(TAG," Error (%d): Failed to create default loop",esp_ret);
    abort();
 }

 esp_ret =wifi_sta_init(network_event_group);
 if (esp_ret!=ESP_OK){
    ESP_LOGE(TAG," Error (%d): Failed to initialize wifi",esp_ret);
    abort();
 }
 ESP_LOGI(TAG,"Waiting to network connect...");
 network_event_bits=xEventGroupWaitBits(network_event_group,WIFI_STA_CONNECTED_BIT,pdFALSE,pdTRUE,pdMS_TO_TICKS(connection_timeout_ms));
 if(network_event_bits&WIFI_STA_IPV4_OBTAINED_BIT){
    ESP_LOGI(TAG,"Obtained Ipv4");

 }else if (network_event_bits&WIFI_STA_IPV6_OBTAINED_BIT){
    ESP_LOGI(TAG,"Obtained IPv6");
 }else{
    ESP_LOGI(TAG,"Failed to obtain IP adress");
 }
 while(1){
    if(network_event_bits&WIFI_STA_CONNECTED_BIT){
        ESP_LOGI(TAG,"Still connected to wifi network");
    }else{
        ESP_LOGE(TAG,"Lost connected to wifi network");
    }
 }
   xTaskDelay(sleep_ms/portTICK_PERIOD_MS);
}