#include <Arduino.h>

#include "FreeRTOS.h"
#include "freertos\task.h"

#define LED (2)

void TaskBlinkLED (void *pvParameters);
void TaskCount (void *pvParameters);

void setup()
{
	// put your setup code here, to run once:
	TaskHandle_t handleTaskBlinkLED;

	xTaskCreate(TaskBlinkLED, "TaskBlinkLED", configMINIMAL_STACK_SIZE, NULL, 1, &handleTaskBlinkLED);
	xTaskCreate(TaskCount, "TaskCount", configMINIMAL_STACK_SIZE + 1000, handleTaskBlinkLED, 2, NULL);
}

void loop()
{
	// put your main code here, to run repeatedly:
	vTaskDelay(pdMS_TO_TICKS(portMAX_DELAY));
}

void TaskBlinkLED (void *pvParameters)
{
	(void)pvParameters;

	pinMode(LED, OUTPUT);

	while (1)
	{
		digitalWrite(LED, HIGH);
		vTaskDelay(pdMS_TO_TICKS(100));
		digitalWrite(LED, LOW);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void TaskCount (void *pvParameters)
{
	uint8_t count = 0;
	TaskHandle_t handleTask = (TaskHandle_t)pvParameters;

	Serial.begin(9600);

	while (1)
	{

		Serial.printf("Count :%d\r\n", count);
		++count;
		vTaskDelay(pdMS_TO_TICKS(1000));

		if (count == 10)
		{
			Serial.printf("Deletando a TaskBlinkLED...\r\n");
			vTaskDelete(handleTask);
			digitalWrite(LED, LOW);
		}

		if (count == 15)
		{
			Serial.printf("Deletando a TaskCount...\r\n");
			vTaskDelete(NULL);
		}
	}
}
