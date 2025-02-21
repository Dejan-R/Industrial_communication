/*dejan.rakijasic@gmail.com*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <netdb.h>

static const char *TAG = "WiFi";

/* FreeRTOS event group */
static EventGroupHandle_t wifi_status_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static esp_netif_t *esp_netif;

void event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "WiFi pokrenut, pokušavam se povezati...");
        esp_wifi_connect();
        break;

    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "WiFi uspješno povezan!");
        break;

    case IP_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Dobivena IP adresa!");
        xEventGroupSetBits(wifi_status_event_group, WIFI_CONNECTED_BIT);
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
    {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "WiFi nije povezan, razlog: %d. Pokušavam se ponovno povezati...", event->reason);
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_wifi_connect();
        xEventGroupSetBits(wifi_status_event_group, WIFI_FAIL_BIT);
        break;
    }
    
    default:
        break;
    }
}

esp_err_t wifi_STA_povezivanje(char *ssid, char *password, bool static_ip)
{   
    // provjera je li event group već kreiran
    if (wifi_status_event_group == NULL) {
        wifi_status_event_group = xEventGroupCreate();
    }

    // Inicijalizacija WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t konfiguracija = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&konfiguracija));

    // Registracija event handlera
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    esp_netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Konfiguracija WiFi postavki
    wifi_config_t wifi_STA_podaci = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg.capable = true,
            .pmf_cfg.required = false,
        },
    };
    strlcpy((char *)wifi_STA_podaci.sta.ssid, ssid, sizeof(wifi_STA_podaci.sta.ssid));
    strlcpy((char *)wifi_STA_podaci.sta.password, password, sizeof(wifi_STA_podaci.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_STA_podaci));

    // Ako koristimo statičku IP adresu, postavljamo ručno IP, gateway i netmask
    if (static_ip) {
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(esp_netif)); // Isključujemo DHCP
        esp_netif_ip_info_t ip_info;
        ip_info.ip.addr = ipaddr_addr("192.168.1.55");
        ip_info.gw.addr = ipaddr_addr("192.168.1.1");
        ip_info.netmask.addr = ipaddr_addr("255.255.255.0");
        ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif, &ip_info));
        ESP_LOGI(TAG, "Postavljena statička IP adresa: 192.168.1.55");
    } else {
        ESP_LOGI(TAG, "Koristi se DHCP za IP adresu.");
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    // Čekanje na status povezivanja
    EventBits_t wifi_status = xEventGroupWaitBits(
        wifi_status_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(10000)
    );

    if (wifi_status & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi uspješno povezan!");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "WiFi povezivanje nije uspjelo!");
        return ESP_FAIL;
    }
}
