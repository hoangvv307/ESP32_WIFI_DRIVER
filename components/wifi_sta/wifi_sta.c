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

// Public function to initialize Wi-Fi station mode
esp_err_t wifi_sta_init(EventGroupHandle_t event_group){
    esp_err_t esp_ret;
    ESP_LOGI(TAG,"Starting Wi-Fi station mode");
    if(event_group!=NULL){
        s_wifi_event_group=event_group;
    } else {
        ESP_LOGE(TAG, "Event group is NULL");
        return ESP_FAIL;
    }
    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_STA();
    s_wifi_netif = esp_netif_new(&netif_config);
    if(s_wifi_netif==NULL) {
        ESP_LOGE(TAG, "Failed to create netif");
        return ESP_FAIL;
    }   
    s_wifi_driver=esp_wifi_create_if_driver(WIFI_IF_STA);
    if(s_wifi_driver==NULL) {
        ESP_LOGE(TAG, "Failed to create wifi driver");
        return ESP_FAIL;
    }

    esp_ret =esp_netif_attach(s_wifi_netif,s_wifi_driver);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to attach netif to wifi driver");
        return ESP_FAIL;
    }
    //
    esp_ret=esp_event_handler_register(WIFI_EVENT,WIFI_EVENT_STA_START, &on_wifi_event, NULL);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to register Wi-Fi event handler");
        return ESP_FAIL;
    }
    //
     esp_ret=esp_event_handler_register(WIFI_EVENT,WIFI_EVENT_STA_STOP, &on_wifi_event, NULL);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to register Wi-Fi stop event handler");
        return ESP_FAIL;
    }
    //
     esp_ret=esp_event_handler_register(WIFI_EVENT,WIFI_EVENT_STA_CONNECTED, &on_wifi_event, NULL);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to register Wi-Fi connected event handler");
        return ESP_FAIL;
    }
    //
     esp_ret=esp_event_handler_register(WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED, &on_wifi_event, NULL);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to register Wi-Fi disconnected event handler");
        return ESP_FAIL;
    }
#if CONFIG_WIFI_STA_CONNECT_IPV4 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
    esp_ret=esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP, &on_ip_event, NULL);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler");
        return ESP_FAIL;
    }
#endif

#if CONFIG_WIFI_STA_CONNECT_IPV6 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
    esp_ret=esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP6, &on_ip_event, NULL);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IPv6 event handler");
        return ESP_FAIL;
    }
#endif
esp_ret =esp_event_handler_register(IP_EVENT,IP_EVENT_STA_LOST_IP, &on_ip_event, NULL);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP lost event handler");
        return ESP_FAIL;
    }
   
    esp_ret=esp_register_shutdown_handler((shutdown_handler_t)esp_wifi_stop);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to register shutdown handler");
        return ESP_FAIL;
    }   

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_ret=esp_wifi_init(&cfg);    
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Wi-Fi");
        return ESP_FAIL;
    }

    esp_ret=esp_wifi_set_mode(WIFI_MODE_STA);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Wi-Fi mode");
        return ESP_FAIL;
    }

#if CONFIG_WIFI_STA_AUTH_OPEN
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_OPEN;
#elif CONFIG_WIFI_STA_AUTH_WEP
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WEP;
#elif CONFIG_WIFI_STA_AUTH_WPA_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA_PSK;
#elif CONFIG_WIFI_STA_AUTH_WPA2_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK;
#elif CONFIG_WIFI_STA_AUTH_WPA_WPA2_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA_WPA2_PSK;
#elif CONFIG_WIFI_STA_AUTH_WPA3_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA3_PSK;
#elif CONFIG_WIFI_STA_AUTH_WPA2_WPA3_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_WPA3_PSK;
#elif CONFIG_WIFI_STA_AUTH_WAPI_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WAPI_PSK;
#else
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_OPEN;
#endif

#if CONFIG_WIFI_STA_WPA3_SAE_PWE_HUNT_AND_PECK
  const wifi_sae_pwe_method_t sae_pwe_method = WPA3_SAE_PWE_HUNT_AND_PECK;
#elif CONFIG_WIFI_STA_WPA3_SAE_PWE_H2E
    const wifi_sae_pwe_method_t sae_pwe_method = WPA3_SAE_PWE_HASH_TO_ELEMENT;
#elif CONFIG_WIFI_STA_WPA3_SAE_PWE_BOTH
    const wifi_sae_pwe_method_t sae_pwe_method = WPA3_SAE_PWE_BOTH;
#else 

    const wifi_sae_pwe_method_t sae_pwe_method = WPA3_SAE_PWE_UNSPECIFIED;
#endif
 wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_STA_SSID,
            .password = CONFIG_WIFI_STA_PASSWORD,
            .threshold.authmode = auth_mode,
            .sae_pwe_h2e = sae_pwe_method,
        },
    };
    esp_ret=esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Wi-Fi configuration");
        return ESP_FAIL;
    }
    esp_ret=esp_wifi_start();
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi");
        return ESP_FAIL;
    }
    return ESP_OK;

}

esp_err_t wifi_sta_stop(void){
    esp_err_t esp_ret;
    ESP_LOGI(TAG,"Stopping Wi-Fi station mode");
    esp_ret=esp_event_handler_unregister(WIFI_EVENT,WIFI_EVENT_STA_START, &on_wifi_event);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister Wi-Fi event handler");
        return ESP_FAIL;
    }
    

    esp_ret=esp_event_handler_unregister(WIFI_EVENT,WIFI_EVENT_STA_STOP, &on_wifi_event);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister Wi-Fi stop event handler");
        return ESP_FAIL;
    }

    esp_ret=esp_event_handler_unregister(WIFI_EVENT,WIFI_EVENT_STA_CONNECTED, &on_wifi_event);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister Wi-Fi connected event handler");
        return ESP_FAIL;
    }
    

    esp_ret=esp_event_handler_unregister(WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED, &on_wifi_event);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister Wi-Fi disconnected event handler");
        return ESP_FAIL;
    }

#if CONFIG_WIFI_STA_CONNECT_IPV4 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
    esp_ret=esp_event_handler_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP, &on_ip_event);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister IPv4 event handler");
        return ESP_FAIL;
    }
#endif

#if CONFIG_WIFI_STA_CONNECT_IPV6 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
    esp_ret=esp_event_handler_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP6, &on_ip_event);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister IPv6 event handler");
        return ESP_FAIL;
    }
#endif 
esp_ret=esp_event_handler_unregister(IP_EVENT,IP_EVENT_STA_LOST_IP, &on_ip_event);
    if(esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister IP lost event handler");
        return ESP_FAIL;
    }
    esp_ret=esp_unregister_shutdown_handler((shutdown_handler_t)esp_wifi_stop);
    if(esp_ret==ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Shutdown handler not registered");
    
    }else if (esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister shutdown handler");
        return ESP_FAIL;
    }
    esp_ret=esp_wifi_disconnect();
    if (esp_ret ==ESP_ERR_WIFI_NOT_INIT||esp_ret==ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGI(TAG, "wifi already disconnected or not started");
    } else if (esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Error (%d): Failed to disconnect Wi-Fi", esp_ret);
        return ESP_FAIL;
    }
    
    esp_ret=esp_wifi_stop();
    if (esp_ret ==ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGI(TAG, "wifi already stopped or not started");
    } else if (esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Error (%d): Failed to stop Wi-Fi", esp_ret);
        return ESP_FAIL;
    }
    
    esp_ret=esp_wifi_deinit();
    if (esp_ret ==ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGI(TAG, "wifi already deinitialized or not started");
    } else if (esp_ret!=ESP_OK) {
        ESP_LOGE(TAG, "Error (%d): Failed to deinitialize Wi-Fi", esp_ret);
        return ESP_FAIL;
    }

    if(s_wifi_driver!=NULL) {
        esp_wifi_destroy_if_driver(s_wifi_driver);
        s_wifi_driver=NULL;

    }
    if(s_wifi_netif!=NULL){
        esp_netif_destroy(s_wifi_netif);
    }
    if(s_wifi_event_group!=NULL){
        xEventGroupClearBits(s_wifi_event_group,WIFI_STA_CONNECTED_BIT);
        xEventGroupClearBits(s_wifi_event_group,WIFI_STA_IPV4_OBTAINED_BIT);
        xEventGroupClearBits(s_wifi_event_group,WIFI_STA_IPV6_OBTAINED_BIT);
    }

    s_wifi_netif=NULL;

    ESP_LOGI(TAG,"wifi stopped");

    return ESP_OK;

}

esp_err_t wifi_sta_reconnect(void){
    esp_err_t esp_ret;
    esp_ret=wifi_sta_stop();
    if(esp_ret!=ESP_OK){
        ESP_LOGE(TAG,"Failed to stop wifi during reconnect");
        return esp_ret;
    }
    esp_ret=wifi_sta_init(NULL);
    if(esp_ret!=ESP_OK){
        ESP_LOGE(TAG,"Failed to initialize wifi during reconnect");
        return esp_ret;
    }
    return ESP_OK;
}



