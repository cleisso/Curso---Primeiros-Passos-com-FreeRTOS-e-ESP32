#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define BUTTON_PIN		23

#define LED_RED_PIN		2
#define LED_RED_STATE	0
#define LED_RED_TIME	5000

#define LED_GREEN_PIN	4
#define LED_GREEN_STATE	0
#define LED_GREEN_TIME	500

typedef struct blink_led_t
{
	uint8_t pin;
	uint8_t state;
	uint32_t timeBlink;

	TaskHandle_t taskHandle;
	TimerHandle_t timerHandle;
} blink_led_t;

typedef struct button_t
{
	uint8_t pin;
	TaskHandle_t taskHandle;
} button_t;

blink_led_t ledRed = {LED_RED_PIN, LED_RED_STATE, LED_RED_TIME};
blink_led_t ledGreen = {LED_GREEN_PIN, LED_GREEN_STATE, LED_GREEN_TIME};

button_t button = {BUTTON_PIN};

void vTask1 (void *pvParameters);
void vTimerCallbackLEDRed (TimerHandle_t timer);
void vTimerCallbackBlinkLEDGreen (TimerHandle_t timer);

void setup ()
{
	Serial.begin(9600);

	ledRed.timerHandle = xTimerCreate("Flash LED Red", ledRed.timeBlink, pdFALSE, NULL, vTimerCallbackLEDRed);
	xTaskCreate(vTask1, "Button Task", configMINIMAL_STACK_SIZE + 1000, &button, 1, &button.taskHandle);
	
	pinMode(ledGreen.pin, OUTPUT);
	digitalWrite(ledGreen.pin, ledGreen.state);
	ledGreen.timerHandle = xTimerCreate("Blink LED Green", ledGreen.timeBlink, pdTRUE, NULL, vTimerCallbackBlinkLEDGreen);
	if (xTimerStart(ledGreen.timerHandle, 0) == pdTRUE)
		Serial.printf("vTimerCallbackBlinkLEDGreen inicializado\r\n");
	
}

void loop ()
{
	vTaskDelay(pdMS_TO_TICKS(portMAX_DELAY));
}

void vTask1 (void *pvParameters)
{
	button_t *button = (button_t *)pvParameters;

	pinMode(button->pin, INPUT_PULLUP);
	pinMode(ledRed.pin, OUTPUT);
	digitalWrite(ledRed.pin, ledRed.state);

	while (1)
	{
		if (digitalRead(button->pin) == 0)
		{
			if (xTimerIsTimerActive(ledRed.timerHandle) == pdFALSE)
			{
				xTimerStart(ledRed.timerHandle, 0);
				xTimerStop(ledGreen.timerHandle, 0);
				digitalWrite(ledRed.pin, 1);
				digitalWrite(ledGreen.pin, 0);
				Serial.printf("vTimerCallbackLEDRed inicializado\r\n");
			}
		}

		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

void vTimerCallbackLEDRed (TimerHandle_t timer)
{
	digitalWrite(ledRed.pin, 0);
	xTimerStart(ledGreen.timerHandle, 0);
}

void vTimerCallbackBlinkLEDGreen (TimerHandle_t timer)
{
	ledGreen.state = !ledGreen.state;
	digitalWrite(ledGreen.pin, ledGreen.state);
}