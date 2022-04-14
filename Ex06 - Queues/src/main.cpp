#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define LED_RED_PIN		(2)
#define LED_RED_TIME	(100)
#define LED_GREEN_PIN	(4)
#define LED_GREEN_TIME	(500)

typedef struct blink_led_t
{
	uint8_t pin;
	uint8_t state;
	uint32_t timeBlink;
	TaskHandle_t taskHandle;
} blink_led_t;

blink_led_t ledRed = {LED_RED_PIN, 0, LED_RED_TIME, NULL};
blink_led_t ledGreen = {LED_GREEN_PIN, 0, LED_GREEN_TIME, NULL};

void vTaskLedBlink (void *pvParameters);
void vTaskCountPrint (void *pvParameters);

QueueHandle_t queueLedSuspend = NULL;

void setup()
{
	// put your setup code here, to run once:
	Serial.begin(9600);

	xTaskCreate(vTaskLedBlink, "LED Blink - Red", configMINIMAL_STACK_SIZE + 5000, &ledRed, 1, &ledRed.taskHandle);
	xTaskCreate(vTaskLedBlink, "LED Blink - Green", configMINIMAL_STACK_SIZE + 5000, &ledGreen, 1, &ledGreen.taskHandle);
	xTaskCreate(vTaskCountPrint, "Count", 2048 + 5000, NULL, 2, NULL);

	queueLedSuspend = xQueueCreate(10, sizeof(blink_led_t*));
}

void loop()
{
	// put your main code here, to run repeatedly:
	vTaskDelay(pdMS_TO_TICKS(5000));
}

void vTaskLedBlink (void *pvParameters)
{
	blink_led_t *led = (blink_led_t *)pvParameters;
	uint32_t count = 0;

	pinMode(led->pin, OUTPUT);
	digitalWrite(led->pin, led->state);

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(led->timeBlink));
		led->state = !led->state;
		digitalWrite(led->pin, led->state);
		
		++count;
		if (count == 10)
		{
			if (uxQueueSpacesAvailable(queueLedSuspend) > 0)
				xQueueSend(queueLedSuspend, &led, portMAX_DELAY);

			count = 0;
		}
	}
}

void vTaskCountPrint (void *pvParameters)
{
	(void)pvParameters;

	int count = 0;

	while (1)
	{
		++count;
		Serial.printf("Count: %d\n", count);
		
		if (count == 10)
		{
			count = 0;
			vTaskResume(ledRed.taskHandle);
			vTaskResume(ledGreen.taskHandle);
		}

		if (uxQueueMessagesWaiting(queueLedSuspend) > 0)
		{
			blink_led_t *led = NULL;
			
			xQueueReceive(queueLedSuspend, &led, 0);
			
			if (led != NULL)
				vTaskSuspend(led->taskHandle);
		}

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
