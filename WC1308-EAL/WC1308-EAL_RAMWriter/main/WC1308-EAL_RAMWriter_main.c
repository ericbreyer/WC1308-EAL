#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "tcpip_adapter.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>
#include "driver/gpio.h"


//define SSID, password, TCP port, and pin numbers
#define ESP_WIFI_SSID      "EricsESP"
#define ESP_WIFI_PASS      "EricsESP"
#define MAX_STA_CONN       4
#define BLINK_GPIO         5
#define PORT               8000
const int addressGPIO[8] =  { 25,26,32,33,27,14,12,13 };
const int dTAGPIO[8] =     { 21,22,19,23,18,5,10,9 };
const int writeGPIO =       15;

//define tLine struct as a byte and the adress to write the byte too
struct tLine{
    int address;
    int data;
};

//the program is an array of tLines. However the program is not always the same length so a pointer is used.
//This means the "array" can be dynamically sized using malloc. 
struct tLine *program;

//writes a byte of data to the RAM at a specified address
void writeRAM(uint8_t address, uint8_t data) {

    //output the address and data to their respective pins using byte shifts and masks
    for (int i = 0; i < 8; ++i) {
        gpio_set_level(addressGPIO[(i)], address & 0x01);
        address = address >> 1;
        gpio_set_level(dTAGPIO[(i)], data & 0x01);
        data = data >> 1;
    }

    //send the write signal to the RAM
    gpio_set_level(writeGPIO, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(writeGPIO, 1);
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

//takes in a program and its length and writes it to the ram
void programRAM(struct tLine* program, int length) {

    //set pins to output and initalize write pin to high
    for (int i = 0; i < 8; ++i) {
        gpio_set_direction(addressGPIO[i], GPIO_MODE_OUTPUT);
        gpio_set_direction(dTAGPIO[i], GPIO_MODE_OUTPUT);
    }
    gpio_set_direction(writeGPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(writeGPIO, 1);

    //iterate through the program, and write the tLine to RAM
    for (int i = 0; i < length; ++i) {
        writeRAM(program[i].address, program[i].data);
    }

    //disable esp control of lines (inut floats the lines) and give control back to the dip switches
    for (int i = 0; i < 8; ++i) {
        gpio_set_direction(addressGPIO[i], GPIO_MODE_INPUT);
        gpio_set_direction(dTAGPIO[i], GPIO_MODE_INPUT);
    }
    gpio_set_direction(writeGPIO, GPIO_MODE_INPUT);
}

//builds the program from the TCP buffer and passes it to the program function
void makeAndProgram(uint8_t buffer[]){

    int length;

    //the length of the program will be the first byte of the sent byte array 
    length = buffer[0];

    //initalize the program with the correct length
    program = (struct tLine*)malloc(length * sizeof(struct tLine));

    //write bytes from buffer into program
    for (int i=0; i<length; ++i) {
        program[i].address = buffer[(2*i+1)];
        program[i].data = buffer[(2*i+2)];
    }

    //program the ram
    programRAM(program,length);

    //free up space used by the program pointer (prevents leaks)
    free(program);
}

//these two functions set up the ESP in softAP mode
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    static const char* TAG = "wifi softAP";

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap() {
    static const char* TAG = "wifi softAP";

    tcpip_adapter_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             ESP_WIFI_SSID, ESP_WIFI_PASS);
}




//TCP socket handling
static void tcp_server_task(void* pvParameters) {
    static const char* TAG = "tcp_server";

    //init vars, rx_buffer is the buffer we read bytes into
    uint8_t rx_buffer[1024];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {

#ifdef CONFIG_EXAMPLE_IPV4
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        struct sockaddr_in6 dest_addr;
        bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        //try to create TCP socket
        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        //try to bind to TCP socket
        int err = bind(listen_sock, (struct sockaddr*) & dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        //try to listen on TCP socket
        err = listen(listen_sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket listening");

        //try to accept client and accept it to socket
        struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr*) & source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket accepted");

        while (1) {
            //waits until bytes are received into the buffer
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

            //if error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }

            //if connection closed
            else if (len == 0) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }

            //if data received
            else {

                //get the sender's ip address as string
                if (source_addr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in*) & source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                }
                else if (source_addr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }


                rx_buffer[len] = 0; // Null-terminate whatever we received
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);

                //send the received bytes to make the program and write it
                makeAndProgram(rx_buffer);

                //if we wanted to send data back
                /*int err = send(sock, rx_buffer, len, 0);
                //if (err < 0) {
                //    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                //    break;
                }*/
            }
        }

        //if client disconects, restart socket
        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);

            shutdown(listen_sock, 0);
            close(listen_sock);

            vTaskDelay(5);
        }
    }
    vTaskDelete(NULL);
}


void app_main() {

    //config all pins for GPIO
    for(int i = 0; i<8; ++i){
        gpio_pad_select_gpio(addressGPIO[i]);
        gpio_pad_select_gpio(dTAGPIO[i]);
    }
    gpio_pad_select_gpio(writeGPIO);


    //Initialize stuff
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI("wifi softAP", "ESP_WIFI_MODE_AP");

    //start softAP and TCP server (data is written to RAM in tcp_server_task when it receives bytes)
    //tcp_server_task runs in a loop, deals with creating, binding, listening, accepting, and reciving data from sockets
    wifi_init_softap();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}
