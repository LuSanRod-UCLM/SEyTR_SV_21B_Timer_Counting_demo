# S21B: Timer → Semáforo Contador (ESP32-S3 / ESP-IDF v5.5.1)

## 1) Plataforma

* **MCU:** ESP32-S3
* **SDK:** ESP-IDF **v5.5.1**
* **RTOS:** FreeRTOS
* **Periférico:** **GPTimer** (alarma periódica)

## 2) Objetivo docente

Ilustrar cómo **contabilizar picos de evento** desde una ISR de **GPTimer** empleando un **semáforo contador** con `xSemaphoreGiveFromISR()`, y cómo una **tarea agregadora** consume esos “créditos” con `xSemaphoreTake()`.

## 3) Resumen

Un GPTimer dispara una **ISR** a un período configurable (µs). La ISR incrementa un **semáforo contador**. Una **tarea agregadora** consume los tokens, simula trabajo breve y registra métricas (takes correctos, timeouts, espera máxima).

## 4) Estructura

```
SEyTR_S21B_Timer_Counting/
├─ CMakeLists.txt
└─ main/
   ├─ CMakeLists.txt
   ├─ Kconfig.projbuild
   ├─ main.c
   ├─ scenario_timer_count.h
   └─ scenario_timer_count.c
```

## 5) Compilar, flashear y monitorizar

```bash
idf.py set-target esp32s3
idf.py menuconfig          # Ajusta periodo, tokens, timeout y prioridad
idf.py build
idf.py -p <PUERTO> flash monitor
# Salir del monitor: Ctrl+]
```

Sugerencia:

```bash
idf.py -p <PUERTO> monitor | tee "logs/s21b_$(date +%Y%m%d_%H%M%S).log"
```

## 6) Opciones de configuración (`idf.py menuconfig`)

Ruta: **SEyTR S21B Options — Timer → Contador**

* `S21B_DIAG_PERIOD_MS` (ms): periodo de métricas (def. 2000).
* `S21B_TMR_PERIOD_US` (µs): período del GPTimer (def. 2000).
* `S21B_MAX_TOKENS`: capacidad del semáforo contador (def. 32).
* `S21B_TAKE_TIMEOUT_MS` (ms): espera del agregador (def. 50; 0 = bloquea indefinido).
* `S21B_AGG_TASK_PRIO`: prioridad de la tarea agregadora (def. 6).

## 7) Descripción de funcionamiento

* **GPTimer (auto-reload):** genera una alarma periódica.
* **ISR (on_alarm):** hace `xSemaphoreGiveFromISR(ticks,&hpw)`; devuelve **`true`** si hay que ceder CPU al salir de la ISR (yield diferido).
* **Tarea agregadora:** `xSemaphoreTake(ticks, timeout)`; procesa cada token y simula trabajo breve; si no llega ningún token en `timeout`, contabiliza **timeouts**.
* **Tarea de diagnóstico:** imprime métricas con la cadencia configurada.

## 8) Interpretación de los logs

Formato típico:

```
I (.. ) S21B_TMR_CNT: period_us=2000 isr_ticks=1500 ok=1480 timeouts=2 max_wait_us=650
```

* **period_us:** período del timer configurado.
* **isr_ticks:** interrupciones generadas (tokens producidos).
* **ok (takes_ok):** tokens consumidos por la tarea.
* **timeouts:** ventanas sin consumo (no llegó token en el `timeout`).
* **max_wait_us:** espera máxima de la tarea para conseguir un token.
  **Experimentos:**
* Reducir `S21B_TMR_PERIOD_US` (más rápido) y observar si crecen **timeouts**.
* Variar `S21B_MAX_TOKENS` para amortiguar picos (capacidad del “buffer”).
* Ajustar `S21B_AGG_TASK_PRIO` para ver su efecto en **max_wait_us**.

## 9) Créditos

Material docente de **SEyTR** — Máster en Robótica y Automática.
Autoría y soporte: equipo docente SEyTR.
