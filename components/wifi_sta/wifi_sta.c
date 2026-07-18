#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_netif.h"
#include "wifi_sta.h"
#include "sdkconfig.h"

static const char *TAG = "wifi_sta";

static esp_netif_t *s_wifi_netif = NULL;
static EventGroupHandle_t s_wifi_event_group=NULL;
static wifi_netif_driver_t s_wifi_driver=NULL;

static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
 
static void wifi_start(void *esp_netif,esp_event_base_t event_base, int32_t event_id, void *event_data);

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch(event_id) {
        case WIFI_EVENT_STA_START:
        if(s_wifi_driver!=NULL) {
            wifi_start(s_wifi_driver, event_base, event_id, event_data);
        }
        case WIFI_EVENT_STA_CONNECTED:
            if(s_wifi_driver==NULL) {
                ESP_LOGE(TAG, "wifi not started");
                return;
            }
        case WIFI_EVENT_STA_STOP:
            if(s_wifi_driver!=NULL) {
                esp_netif_action_stop(s_wifi_driver,event_base,event_id,event_data);
                
            }break;
        wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *) event_data;
        ESP_LOGI(TAG, "connected to AP SSID:%s, channel:%d, authmode:%d, aid:%d",event->ssid, event->channel, event->authmode, event->aid);

        wifi_netif_driver_t driver =esp_netif_get_driver_sta(s_wifi_driver);
        if(!esp_is_if_ready_when_started(driver)) {
            esp_err_t esp_ret= esp_wifi_register_if_rxcb(driver, esp_netif_receive,s_wifi_netif);
            if(esp_ret!=ESP_OK) {
             ESP_LOGE(TAG, "Failed to wifi RX callback");
             return;
        }
        esp_netif_action_connected(s_wifi_netif,event_base,event_id,event_data);
        
        xEventGroupSetBits(s_wifi_event_group, WIFI_STA_CONNECTED_BIT);
#if CONFIG_WIFI_STA_CONNECT_IPV6 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
      esp_ret = esp_netif_create_ip6_linklocal(s_wifi_netif);
      if(esp_ret != ESP_OK) {
          ESP_LOGE(TAG, "Failed to create IPv6 link-local address");
          return;
      }
#endif 
    break;
    case WIFI_EVENT_STA_DISCONNECTED:
    if(s_wifi_netif!=NULL){
        esp_netif_action_disconnected(s_wifi_netif,event_base,event_id,event_data);

    }
    
        xEventGroupClearBits(s_wifi_event_group, WIFI_STA_CONNECTED_BIT);
#if CONFIG_WIFI_STA_AUTO_RECONNECT
        ESP_LOGI(TAG, "attempting to reconnect to the AP...");
        wifi_sta_reconnect();
#endif

        break;



    }
}
}
static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    esp_err_t esp_ret;
    switch(event_id){
#if CONFIG_WIFI_STA_CONNECT_IPV4 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
    case IP_EVENT_STA_GOT_IP:
        if(s_wifi_netif==NULL) {
            ESP_LOGE(TAG, "On obtain IPv4 addr, Interface is NULL");
            return;
        }
        esp_ret = esp_wifi_internal_get_sta_ip();
        if(esp_ret!=ESP_OK) {
            ESP_LOGE(TAG, "Failed to get IPv4 address");
            return;
        }
        xEventGroupSetBits(s_wifi_event_group, WIFI_STA_IPV4_OBTAINED_BIT);
         //print IPv4 address
         ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
         esp_netif_ip_info_t *ip_info = &event->ip_info;
         ESP_LOGI(TAG, "IPv4 address obtained");
         ESP_LOGI(TAG, " IP Address: " IPSTR, IP2STR(&ip_info->ip));
         ESP_LOGI(TAG, " Subnet Mask: " IPSTR, IP2STR(&ip_info->netmask));
         ESP_LOGI(TAG, " Gateway: " IPSTR, IP2STR(&ip_info->gw));
        break;
#endif 
 #if CONFIG_WIFI_STA_CONNECT_IPV6 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
    case IP_EVENT_GOT_IP6:
        if(s_wifi_netif==NULL) {
            ESP_LOGE(TAG, "On obtain IPv6 addr, Interface is NULL");
            return;
        }
        esp_ret = esp_wifi_internal_get_sta_ip();
        if(esp_ret!=ESP_OK) {
            ESP_LOGE(TAG, "Failed to get IPv6 address");
            return;
        }
        xEventGroupSetBits(s_wifi_event_group, WIFI_STA_IPV6_OBTAINED_BIT);
         //print IPv6 address
         ip_event_got_ip6_t *event_ipv6 = (ip_event_got_ip6_t *) event_data;
         esp_ip6_addr_t *ip6_info = &event_ipv6->ip6_info;
         ESP_LOGI(TAG, "IPv6 address obtained");
         ESP_LOGI(TAG, " IP Address: " IPV6STR, IPV6STR(ip6_info));
         ESP_LOGI(TAG," NetMask: " IPV6STR, IPV6STR(&event_ipv6->ip6_info));
         ESP_LOGI(TAG, " Gateway: " IPV6STR, IPV6STR(&event_ipv6->ip6_info));

        break;
#endif 
case IP_EVENT_STA_LOST_IP:
        
        xEventGroupClearBits(s_wifi_event_group, WIFI_STA_IPV4_OBTAINED_BIT);
        xEventGroupClearBits(s_wifi_event_group, WIFI_STA_IPV6_OBTAINED_BIT);
        ESP_LOGI(TAG, "IP address lost");
        break;
    default:
        ESP_LOGW(TAG, "Unhandled IP event: %li", event_id);
        break;
    }
}
static void wifi_start(void *esp_netif,esp_event_base_t event_base, int32_t event_id, void *event_data)
{
   uint8_t mac_addr[6]={0};
   esp_err_t esp_ret;
   wifi_netif_driver_t driver =esp_netif_get_driver_sta(esp_netif);
   if(driver==NULL) {
       ESP_LOGE(TAG, "Failed to get wifi driver");
       return;
   }    
   esp_ret=esp_wifi_get_if_mac(driver,mac_addr);
   if(esp_ret!=ESP_OK) {
       ESP_LOGE(TAG, "Failed to get MAC address");
       return;
   }
   ESP_LOGI(TAG, "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
   esp_ret=esp_wifi_internal_reg_netstack_buf_cb(esp_netif_netstack_buf_ref,esp_netif_netstack_buf_free);
    if(esp_ret!=ESP_OK) {
         ESP_LOGE(TAG, "ERROR (%d): Netstack buffer callbacks registration failed", esp_ret);
         return;
    }
    esp_ret=esp_netif_set_mac(esp_netif,mac_addr);
    if(esp_ret!=ESP_OK) {
         ESP_LOGE(TAG, "Failed to set MAC address");
         return;
    }
  esp_netif_action_start(esp_netif,event_base,event_id,event_data);
  ESP_LOGI(TAG,"Connecting to %s...",CONFIG_WIFI_STA_SSID);
  esp_ret=esp_wifi_connect();
    if(esp_ret!=ESP_OK) {
             ESP_LOGE(TAG, "Failed to connect to AP");
             return;
        }

}

