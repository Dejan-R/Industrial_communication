/*dejan.rakijasic@gmail.com

primjer: ESP32 - Modbus TCP slave over Wi-Fi

cd components
git clone --recursive https://github.com/espressif/esp-modbus.git esp_modbus

idf.py reconfigure
idf.py build flash monitor

https://github.com/espressif/esp-idf/tree/v5.4/examples/protocols/modbus/tcp/mb_tcp_slave
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi.h"

#include "mbcontroller.h"  // Modbus controller API
#include "modbus_params.h"  // Uključivanje struktura registara

#define TAG "MODBUS_WIFI"
#define MB_SLAVE_TAG "MODBUS_SLAVE"

#define MB_COIL_START 0       // Početna adresa coils
#define MB_COIL_COUNT 1       // Broj coils
#define MB_REG_HOLD_START 0   // Početna adresa holding registra
#define MB_REG_HOLD_COUNT 1   // Broj holding registara

#include "driver/gpio.h"

#define LED_PIN GPIO_NUM_18

// Inicijalizacija Modbus registara
void modbus_registers_init(void) {
    mb_register_area_descriptor_t reg_area;
    reg_area.type = MB_PARAM_COIL;
    reg_area.start_offset = MB_COIL_START;
    reg_area.address = (void *)&coil_reg_params.coils_port0; 
    reg_area.size = sizeof(uint8_t); 
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));
    reg_area.type = MB_PARAM_HOLDING;
    reg_area.start_offset = MB_REG_HOLD_START;
    reg_area.address = (void *)&holding_reg_params.test_regs[0]; 
    reg_area.size = sizeof(holding_reg_params.test_regs[0]);    
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));
    ESP_LOGI(MB_SLAVE_TAG, "Modbus registri inicijalizirani!");
}



void modbus_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));


        if (coil_reg_params.coils_port0) {
            gpio_set_level(LED_PIN, 1);
        } else {
            gpio_set_level(LED_PIN, 0);
        }

        ESP_LOGI(MB_SLAVE_TAG, "Coil Status: %d", coil_reg_params.coils_port0);

        holding_reg_params.test_regs[0] = esp_random() % 100;
        ESP_LOGI(MB_SLAVE_TAG, "Holding Register Value: %u", holding_reg_params.test_regs[0]); 
    }
}


void app_main(void) {


    esp_rom_gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);


    ESP_ERROR_CHECK(nvs_flash_init());

    // Povezivanje na WiFi mrežu
  //Ako želimo dinamičku IP adresu:
esp_err_t wifi_rezultat = wifi_STA_povezivanje("mreza", "12345678", false);

//Ako želimo statičku IP adresu (192.168.1.55):
//esp_err_t wifi_rezultat = wifi_STA_povezivanje("mreza", "12345678", true);

    if (wifi_rezultat == ESP_OK) {
        ESP_LOGI(MB_SLAVE_TAG, "Uređaj je povezan na WiFi!");
    } else {
        ESP_LOGE(MB_SLAVE_TAG, "Povezivanje na WiFi nije uspjelo!");
        return;
    }

    // Postavljanje Modbus TCP slave
    void* slave_handler = NULL;
    ESP_ERROR_CHECK(mbc_slave_init_tcp(&slave_handler));

    // Inicijalizacija coils i registara
    modbus_registers_init();
    
    // Start Modbus slave
    ESP_ERROR_CHECK(mbc_slave_start());

    ESP_LOGI(MB_SLAVE_TAG, "Modbus TCP slave pokrenut!");

    // Pokreni Modbus task
    xTaskCreate(modbus_task, "modbus_task", 2048, NULL, 5, NULL);
}
