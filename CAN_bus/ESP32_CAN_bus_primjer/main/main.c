/*
dejan.rakijasic@gmail.com

Primjer: komunikacija CAN protokolom

CAN libraray: 
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html

*/


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/twai.h"
#include <inttypes.h> //PRIx32 makro - formatiranje 32-bitnog broja u heksadecimalnom obliku

#define TX_GPIO GPIO_NUM_21 
#define RX_GPIO GPIO_NUM_22
#define BAUD_RATE TWAI_TIMING_CONFIG_500KBITS() // brzina prijenosa podataka 500 kbps

#define LED_GPIO GPIO_NUM_19

// Globalna konfiguracija TWAI drajvera
twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO, RX_GPIO, TWAI_MODE_NORMAL);
twai_timing_config_t timing_config = BAUD_RATE;
twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); // Prihvati sve poruke

/**************MASKIRANJE I FILTIRANJE*********************
// Primjer konfiguracije filtra: Prihvati samo ID 0x100
// Maska: Provjeri sve bitove ID-a (11-bitni standardni ID)

twai_filter_config_t filter_config = {
    .acceptance_code = 0x100 << 21, // ID 0x100 pomaknut u standardni položaj
    .acceptance_mask = 0x7FF << 21  // Maskira sve bitove standardnog ID-a
};
*********************************************************/

// Funkcija za task slanja podataka
void tx_task(void *arg) {
    while (1) {
        // Kreiranje poruke za slanje
        twai_message_t tx_msg = {
            .identifier = 0x101, // ID poruke
            .data_length_code = 8, // Duljina podataka u bajtovima
            .data = {0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE5, 0xE7, 0xE8} // Podaci za slanje
        };

        // Pokušaj slanja poruke
        if (twai_transmit(&tx_msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
            // Ako je uspješno poslano, ispiši ID i podatke
            printf("Poruka poslana: ID=0x%" PRIx32 ", Podaci=0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
                tx_msg.identifier, 
                tx_msg.data[0], 
                tx_msg.data[1], 
                tx_msg.data[2], 
                tx_msg.data[3], 
                tx_msg.data[4], 
                tx_msg.data[5], 
                tx_msg.data[6], 
                tx_msg.data[7]);
        } else {
            // Ako nije uspješno poslano, ispiši poruku o grešci
            printf("Neuspješno slanje poruke\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Pauza od 1 sekunde
    }
}

// Funkcija za task primanja podataka
void rx_task(void *arg) {
    while (1) {
        twai_message_t rx_msg; // Varijabla za primanje poruke

        // Pokušaj primanja poruke
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
            // Ako je poruka primljena, ispiši ID i podatke
            printf("Poruka primljena: ID=0x%" PRIx32 ", Podaci=", rx_msg.identifier);
            for (int i = 0; i < rx_msg.data_length_code; i++) {
                printf("0x%x ", rx_msg.data[i]);
            }
            printf("\n");

                  if (rx_msg.data[0] == 0x00) {
                gpio_set_level(LED_GPIO, 0); // LED OFF
                printf("LED OFF\n");
            } else if (rx_msg.data[0] == 0x01) {
                gpio_set_level(LED_GPIO, 1); // LED ON
                printf("LED ON\n");
            }
        } else {
            // Ako nije primljena poruka unutar timeouta, ispiši poruku
            printf("Nema primljene poruke unutar timeouta\n");
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Mala pauza između pokušaja primanja
    }
}

void app_main(void) {

  esp_rom_gpio_pad_select_gpio(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);


    // Inicijalizacija i instalacija TWAI drivera
    if (twai_driver_install(&general_config, &timing_config, &filter_config) == ESP_OK) {
        printf("TWAI driver instaliran.\n");
    } else {
        // Ako instalacija nije uspjela, ispiši poruku i prekini
        printf("Greška pri instalaciji TWAI drivera.\n");
        return;
    }

    // Pokretanje TWAI drivera
    if (twai_start() == ESP_OK) {
        printf("TWAI driver pokrenut.\n");
    } else {
        // Ako pokretanje nije uspjelo, ispiši poruku i prekini
        printf("Greška pri pokretanju TWAI drivera.\n");
        return;
    }

    // Kreiranje taskova za slanje i primanje podataka
    xTaskCreate(tx_task, "TX Task", 2048, NULL, 5, NULL);
    xTaskCreate(rx_task, "RX Task", 2048, NULL, 5, NULL);
}
