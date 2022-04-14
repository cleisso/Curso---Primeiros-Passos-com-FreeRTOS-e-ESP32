#include <Arduino.h>

#include "stdio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//#define	DELETE_TASK
#ifndef DELETE_TASK
	#define SUSPEND_TASK
#endif

#define LED_RED_PIN		(2)
#define LED_RED_TIME	(100)
#define LED_GREEN_PIN	(4)
#define LED_GREEN_TIME	(500)

typedef struct blink_led_t
{
	uint8_t pin;
	uint8_t state;
	uint32_t timeBlink;
} blink_led_t;

blink_led_t ledRed = {LED_RED_PIN, 0, LED_RED_TIME};
blink_led_t ledGreen = {LED_GREEN_PIN, 0, LED_GREEN_TIME};

void vTaskLedBlink (void *pvParameters);
void vTaskCountPrint (void *pvParameters);

TaskHandle_t ledRedHandle = NULL;

void setup()
{
	// put your setup code here, to run once:
	xTaskCreate(vTaskLedBlink, "LED Blink - Red", configMINIMAL_STACK_SIZE, &ledRed, 1, &ledRedHandle);
	xTaskCreate(vTaskLedBlink, "LED Blink - Green", configMINIMAL_STACK_SIZE, &ledGreen, 1, NULL);
	xTaskCreate(vTaskCountPrint, "Count", 2048, ledRedHandle, 2, NULL);
}

void loop()
{
	// put your main code here, to run repeatedly:
	vTaskDelay(pdMS_TO_TICKS(5000));
}

void vTaskLedBlink (void *pvParameters)
{
	blink_led_t *led = (blink_led_t *)pvParameters;

	pinMode(led->pin, OUTPUT);
	digitalWrite(led->pin, led->state);

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(led->timeBlink));
		led->state = !led->state;
		digitalWrite(led->pin, led->state);
	}
}

void vTaskCountPrint (void *pvParameters)
{
	TaskHandle_t taskHandle = (TaskHandle_t)pvParameters;
	int count = 0;

	Serial.begin(9600);

	while (1)
	{
		Serial.printf("Count: %d\n", count);
		
		++count;

		#if defined(DELETE_TASK)
		if (taskHandle != NULL && count == 10)
		{
			vTaskDelete(taskHandle);
			taskHandle = NULL;
		}
		#elif defined(SUSPEND_TASK)
		if (count == 10)
			vTaskSuspend(taskHandle);

		if (count == 20)
			vTaskResume(taskHandle);
		#endif

		if (count == 50)
			count = 0;

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}