/*dejan.rakijasic@gmail.com*/
#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

// Funkcija za povezivanje na WiFi (true = statička IP, false = DHCP)
esp_err_t wifi_STA_povezivanje(char *ssid, char *password, bool static_ip);


#endif