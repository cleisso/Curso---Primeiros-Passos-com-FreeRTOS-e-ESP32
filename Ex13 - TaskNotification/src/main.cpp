#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_PIN		23

typedef struct button_t
{
	uint8_t pin;
	void (*Callback) (void);
	TaskHandle_t taskHandle;
} button_t;

void vTaskButton (void *pvParameters);
void PinISR (void);

button_t button = {BUTTON_PIN, PinISR, NULL};

void setup()
{
	Serial.begin(9600);

	xTaskCreate(vTaskButton, "Task Button", configMINIMAL_STACK_SIZE + 1000, &button, 1, &button.taskHandle);
}

void loop()
{
	vTaskDelay(portMAX_DELAY);
}

void vTaskButton (void *pvParameters)
{
	button_t *button = (button_t *)pvParameters;
	uint32_t quantityNotification = 0;
	pinMode(button->pin, INPUT_PULLUP);
	attachInterrupt(button->pin, button->Callback, FALLING);

	while (1)
	{
		quantityNotification = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
		Serial.printf("Tratamento do: %s com %d notificacoes\r\n", pcTaskGetTaskName(button->taskHandle), quantityNotification);
		vTaskDelay(500);
	}
}

void PinISR (void)
{
	vTaskNotifyGiveFromISR(button.taskHandle, NULL);
	vTaskDelay(10);
}
