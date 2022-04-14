#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED (2)

void TaskPiscaLED(void *pvParameters);
void TaskSerial(void *pvParameters);

void setup()
{
	// put your setup code here, to run once:
	xTaskCreate(TaskPiscaLED, "PiscaLED", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(TaskSerial, "Serial", configMINIMAL_STACK_SIZE + 1000, NULL, 2, NULL);
}

void loop()
{
	// put your main code here, to run repeatedly:
	vTaskDelay(pdMS_TO_TICKS(1000));
}

void TaskPiscaLED(void *pvParameters)
{
	(void)pvParameters;

	pinMode(LED, OUTPUT);

	while (1)
	{
		digitalWrite(LED, HIGH);
		vTaskDelay(pdMS_TO_TICKS(200));
		digitalWrite(LED, LOW);
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}
void TaskSerial(void *pvParameters)
{
	(void)pvParameters;

	uint32_t count = 0;
	Serial.begin(9600);

	while (1)
	{
		Serial.println("Contagem: " + String(++count));
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}