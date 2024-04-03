#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <nvs_flash.h>
#include "ds18x20.h"
#include "ultrasonic.h"

#define TRIGGER_GPIO 16
#define ECHO_GPIO 17
#define ONE_WIRE_GPIO 4

#define DS18B20_ADDRESS 0xA300000C0844B528

#define GATE_MAC {0xE8, 0xDB, 0x84, 0x03, 0xE6, 0x8C}
const uint8_t gate_mac[ESP_NOW_ETH_ALEN] = GATE_MAC;

typedef struct sensor_data
{
    float temperature;
    uint8_t distance;
} sensor_data_t;

static void app_wifi_init()
{
    esp_now_peer_info_t peer_info = {
        .channel = 0,
        .ifidx = ESP_IF_WIFI_STA,
        .encrypt = false,
        .peer_addr = GATE_MAC
    };

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));
}

void measure_and_send_task(void *pvParameters)
{
    TickType_t last_wake_time = xTaskGetTickCount();

    ultrasonic_sensor_t ultrasonic_sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
    };
    ultrasonic_init(&ultrasonic_sensor);

    while (true)
    {
        uint32_t distance;
        sensor_data_t sensor_data;
        ds18x20_measure_and_read(ONE_WIRE_GPIO, DS18B20_ADDRESS, &sensor_data.temperature);
        ultrasonic_measure_cm(&ultrasonic_sensor, UINT8_MAX, &distance);
        sensor_data.distance = (uint8_t)distance;

        uint8_t *data = malloc(sizeof(sensor_data_t));
        memcpy(data, &sensor_data, sizeof(sensor_data_t));
        esp_now_send(gate_mac, data, sizeof(sensor_data_t));

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(10000));
    }
}

void app_main()
{
    app_wifi_init();

    xTaskCreate(measure_and_send_task, "measure_and_send_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}
