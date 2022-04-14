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

	xTaskCreatePinnedToCore(TaskBlinkLED, "TaskBlinkLED", configMINIMAL_STACK_SIZE, NULL, 1, &handleTaskBlinkLED, PRO_CPU_NUM);
	xTaskCreatePinnedToCore(TaskCount, "TaskCount", configMINIMAL_STACK_SIZE + 1000, handleTaskBlinkLED, 2, NULL, APP_CPU_NUM);
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
	Serial.printf("Verificando nivel de stack do LED: %d\n", uxTaskGetStackHighWaterMark(handleTask));
	Serial.printf("Verificando nivel de stack do Count: %d\n", uxTaskGetStackHighWaterMark(NULL));

	while (1)
	{
		Serial.printf("Count: %d\r\n", count);
		Serial.printf("Verificando nivel de stack do LED: %d\n", uxTaskGetStackHighWaterMark(handleTask));
		Serial.printf("Verificando nivel de stack do Count: %d\n", uxTaskGetStackHighWaterMark(NULL));
		++count;
		vTaskDelay(pdMS_TO_TICKS(1000));

		if (count == 10)
		{
			Serial.printf("Suspendendo a TaskBlinkLED...\r\n");
			Serial.printf("Verificando nivel de stack do LED: %d\n", uxTaskGetStackHighWaterMark(handleTask));
			Serial.printf("Verificando nivel de stack do Count: %d\n", uxTaskGetStackHighWaterMark(NULL));
			vTaskSuspend(handleTask);
			digitalWrite(LED, LOW);
		}

		if (count == 20)
		{
			Serial.printf("Retomando a TaskBlinkLED...\r\n");
			Serial.printf("Verificando nivel de stack do LED: %d\n", uxTaskGetStackHighWaterMark(handleTask));
			Serial.printf("Verificando nivel de stack do Count: %d\n", uxTaskGetStackHighWaterMark(NULL));
			vTaskResume(handleTask);
			count = 0;
		}
	}
}
