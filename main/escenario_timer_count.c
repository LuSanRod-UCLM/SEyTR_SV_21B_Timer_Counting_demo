// S21B: Timer → Semáforo Contador (ESP32-S3 / ESP-IDF v5.5.1)
// Proyecto de ejemplo para demostrar el uso de un temporizador para incrementar un contador
// Archivo: main/escenario_timer_count.c

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "escenario_timer_count.h"

static const char* TAG = "S21B_TMR_CNT";

#define DIAG_PERIOD_MS      CONFIG_S21B_DIAG_PERIOD_MS
#define TMR_PERIOD_US       CONFIG_S21B_TMR_PERIOD_US
#define MAX_TOKENS          CONFIG_S21B_MAX_TOKENS
#define TAKE_TIMEOUT_MS     CONFIG_S21B_TAKE_TIMEOUT_MS
#define AGG_PRIO            CONFIG_S21B_AGG_TASK_PRIO

static SemaphoreHandle_t s_ticks;   // counting semaphore
static gptimer_handle_t s_tmr;

static struct {
    uint32_t isr_ticks;
    uint32_t takes_ok;
    uint32_t timeouts;
    uint32_t last_wait_us;
    uint32_t max_wait_us;
} s_stats;

static inline void busy_us(uint32_t us){
    int64_t t0 = esp_timer_get_time();
    while ((int64_t)esp_timer_get_time() - t0 < (int64_t)us) { __asm__ __volatile__("nop"); }
}

static bool IRAM_ATTR on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    (void)timer; (void)edata; (void)user_ctx;
    BaseType_t hpw = pdFALSE;
    s_stats.isr_ticks++;
    xSemaphoreGiveFromISR(s_ticks, &hpw);
    return hpw; // si true, solicita cambio de contexto al salir de ISR
}

static void aggregator_task(void* arg){
    TickType_t to = (TAKE_TIMEOUT_MS==0) ? portMAX_DELAY : pdMS_TO_TICKS(TAKE_TIMEOUT_MS);
    for(;;){
        int64_t t0 = esp_timer_get_time();
        if (xSemaphoreTake(s_ticks, to) == pdTRUE){
            int64_t t1 = esp_timer_get_time();
            uint32_t waited = (uint32_t)(t1 - t0);
            s_stats.takes_ok++;
            s_stats.last_wait_us = waited;
            if (waited > s_stats.max_wait_us) s_stats.max_wait_us = waited;
            // Simula trabajo breve por tick
            busy_us(200);
        } else {
            s_stats.timeouts++;
            vTaskDelay(pdMS_TO_TICKS(2)); // backoff suave
        }
    }
}

static void diag_task(void* arg){
    TickType_t per = pdMS_TO_TICKS(DIAG_PERIOD_MS);
    for(;;){
        ESP_LOGI(TAG, "period_us=%d isr_ticks=%"PRIu32" ok=%"PRIu32" timeouts=%"PRIu32" max_wait_us=%"PRIu32,
                 TMR_PERIOD_US, s_stats.isr_ticks, s_stats.takes_ok, s_stats.timeouts, s_stats.max_wait_us);
        vTaskDelay(per);
    }
}

void escenario_timer_count_run(void){
    s_ticks = xSemaphoreCreateCounting(MAX_TOKENS, 0);
    configASSERT(s_ticks);

    // GPTimer: cuenta hacia arriba, alarma periódica
    gptimer_config_t tcfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 tick = 1 us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&tcfg, &s_tmr));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = on_alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(s_tmr, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(s_tmr));

    gptimer_alarm_config_t acfg = {
        .reload_count = 0,
        .alarm_count = TMR_PERIOD_US, // us
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(s_tmr, &acfg));
    ESP_ERROR_CHECK(gptimer_start(s_tmr));

    // Tareas
    BaseType_t r;
    r = xTaskCreate(aggregator_task, "AGG", 4096, NULL, AGG_PRIO, NULL); configASSERT(r==pdPASS);
    r = xTaskCreate(diag_task,       "DIAG", 3072, NULL, 3,       NULL); configASSERT(r==pdPASS);

    ESP_LOGI(TAG, "Timer→Contador listo. Periodo=%dus, tokens=%d", TMR_PERIOD_US, MAX_TOKENS);
}
