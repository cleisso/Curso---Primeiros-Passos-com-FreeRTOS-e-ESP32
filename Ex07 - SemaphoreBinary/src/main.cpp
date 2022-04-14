#include <Arduino.h>

#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define LED_RED_PIN		2
#define LED_RED_TIME	500

typedef struct blink_led_t
{
	uint8_t pin;
	uint8_t state;
	uint32_t timeBlink;
	TaskHandle_t taskHandle;
} blink_led_t;

typedef struct adc_t
{
	uint8_t channel;
	uint16_t value;
	TaskHandle_t taskHandle;
} adc_t;

blink_led_t ledRed = {LED_RED_PIN, 0, LED_RED_TIME, NULL};
adc_t adc = {A11, 0, NULL};

SemaphoreHandle_t xSemaphore = NULL;

void vTaskADC (void *pvParameters);
void vTaskLED (void *pvParameters);

void setup()
{
	Serial.begin(9600);

	xSemaphore = xSemaphoreCreateBinary();
	if (xSemaphore == NULL)
	{
		Serial.printf("Nao foi possivel a criacao do Semaforo :(\r\n");
		while(1);
	}

	xTaskCreate(vTaskLED, "LED Task", configMINIMAL_STACK_SIZE + 1000, &ledRed, 1, &ledRed.taskHandle);
	xTaskCreate(vTaskADC, "ADC Task", configMINIMAL_STACK_SIZE + 1000, &adc, 1, &adc.taskHandle);
}

void loop()
{
	vTaskDelay(portMAX_DELAY);
}

void vTaskADC (void *pvParameters)
{
	adc_t *adc = (adc_t *)pvParameters;

	while (1)
	{
		xSemaphoreTake(xSemaphore, portMAX_DELAY);
		adc->value = analogRead(adc->channel);
		Serial.printf("ADC Value: %d\r\n", adc->value);
	}
}

void vTaskLED (void *pvParameters)
{
	blink_led_t *led = (blink_led_t *)pvParameters;

	pinMode(led->pin, OUTPUT);
	digitalWrite(led->pin, led->state);

	while (1)
	{
		digitalWrite(led->pin, !digitalRead(led->pin));
		Serial.printf("LED Status: %d\r\n", digitalRead(led->pin));
		vTaskDelay(pdMS_TO_TICKS(led->timeBlink));

		xSemaphoreGive(xSemaphore);
	}
}