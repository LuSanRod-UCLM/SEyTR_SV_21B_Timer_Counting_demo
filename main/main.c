// S21B: Timer → Semáforo Contador (ESP32-S3 / ESP-IDF v5.5.1)
// Proyecto de ejemplo para demostrar el uso de un temporizador para incrementar un contador
// Archivo: main/main.c

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "escenario_timer_count.h"

static const char* TAG = "APP_S21B";

void app_main(void){
    ESP_LOGI(TAG, "SEyTR S21B — Timer→Contador (ESP32-S3 / ESP-IDF 5.5.1)");
    escenario_timer_count_run();
}
