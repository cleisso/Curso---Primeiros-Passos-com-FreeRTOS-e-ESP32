#include <Arduino.h>
#include "stdio.h"

#include "mqtt.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define 			ADC_PIN				(34)
#define 			ADC_TOPIC			(THINGSPEAK_PUB_TOPIC THINGSPEAK_TOPIC1)
TaskHandle_t		taskHandleADC =		NULL;
QueueHandle_t		queueHandleADC = 	NULL;
SemaphoreHandle_t	semaphoreADC = 		NULL;

#define 			BUTTON_PIN			(23)
#define 			BUTTON_TOPIC 		(THINGSPEAK_PUB_TOPIC THINGSPEAK_TOPIC2)
TaskHandle_t		taskHandleButton = 	NULL;
SemaphoreHandle_t	semaphoreButton =	NULL;

#define 			LED_PIN				(2)
#define 			LED_TOPIC			(THINGSPEAK_PUB_TOPIC THINGSPEAK_TOPIC3)
TaskHandle_t		taskHandleLED =		NULL;

TaskHandle_t		taskHandlePubMQTT =	NULL;
QueueHandle_t		queuePubMQTT =		NULL;

SemaphoreHandle_t	semaphoreSerial =	NULL;

void vTaskADC (void *pvParameters);
void vTaskButton (void *pvParameters);
void vTaskLED (void *pvParameters);
void vTaskPubMQTT (void *pvParameters);

void setup()
{
	Serial.begin(9600);

	BeginMQTT();
	vTaskDelay(pdMS_TO_TICKS(1000));

	xTaskCreate(vTaskButton, "Task Button", configMINIMAL_STACK_SIZE + 1000, (void *)BUTTON_PIN, 4, &taskHandleButton);
	xTaskCreate(vTaskADC, "Task ADC", configMINIMAL_STACK_SIZE + 1000, (void *)ADC_PIN, 3, &taskHandleADC);
	xTaskCreate(vTaskLED, "Task LED", configMINIMAL_STACK_SIZE + 1000, (void *)LED_PIN, 2, &taskHandleLED);
	xTaskCreate(vTaskPubMQTT, "Task Pub MQTT", configMINIMAL_STACK_SIZE + 1000, NULL, 1, &taskHandlePubMQTT);

	queuePubMQTT = xQueueCreate(20, sizeof(mqtt_pubsub_t *));

	semaphoreADC = xSemaphoreCreateCounting(100, 0);
	semaphoreButton = xSemaphoreCreateCounting(100, 0);
	semaphoreSerial = xSemaphoreCreateMutex();
}

void loop()
{
	vTaskDelay(portMAX_DELAY);
}

void vTaskButton (void *pvParameters)
{
	uint32_t pinButton = (uint32_t)pvParameters;
	uint8_t flagPressButton = 0;
	mqtt_pubsub_t publishButton = {BUTTON_TOPIC};

	pinMode(pinButton, INPUT_PULLUP);
	vTaskDelay(pdMS_TO_TICKS(100));

	while (1)
	{
		if (digitalRead(pinButton) == 0 && flagPressButton == 0)
		{
			vTaskDelay(100);

			if (digitalRead(pinButton) == 0)
			{
				flagPressButton = 1;

				xSemaphoreTake(semaphoreSerial, portMAX_DELAY);
				Serial.printf("Botao Pressionado\r\n");
				xSemaphoreGive(semaphoreSerial);

				if (uxQueueSpacesAvailable(queuePubMQTT) > 0)
				{
					mqtt_pubsub_t *publish = &publishButton;
					
					snprintf(publishButton.payload, sizeof(publishButton.payload), "1");
					xQueueSend(queuePubMQTT, &publish, portMAX_DELAY);
				}
			}
		}
		else if (digitalRead(pinButton) == 1 && flagPressButton == 1)
		{
			vTaskDelay(100);

			if (digitalRead(pinButton) == 1)
			{
				flagPressButton = 0;

				xSemaphoreTake(semaphoreSerial, portMAX_DELAY);
				Serial.printf("Botao Solto\r\n");
				xSemaphoreGive(semaphoreSerial);

				if (uxQueueSpacesAvailable(queuePubMQTT) > 0)
				{
					mqtt_pubsub_t *publish = &publishButton;
					
					snprintf(publishButton.payload, sizeof(publishButton.payload), "0");
					xQueueSend(queuePubMQTT, &publish, portMAX_DELAY);
				}

				xSemaphoreGive(semaphoreButton);
			}
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void vTaskADC (void *pvParameters)
{
	uint32_t pinADC = (uint32_t)pvParameters;
	uint32_t valueADC;
	mqtt_pubsub_t publishADC = {ADC_TOPIC};

	vTaskDelay(pdMS_TO_TICKS(100));

	while (1)
	{
		valueADC = analogRead(pinADC);

		if (xSemaphoreTake(semaphoreSerial, pdMS_TO_TICKS(500)) == pdTRUE)
		{
			Serial.printf("ADC Value: %d\r\n", valueADC);
			xSemaphoreGive(semaphoreSerial);
		}

		if (xSemaphoreTake(semaphoreButton, pdMS_TO_TICKS(500)) == pdTRUE)
		{
			if (uxQueueSpacesAvailable(queuePubMQTT) > 0)
			{
				mqtt_pubsub_t *publish = &publishADC;
				
				snprintf(publishADC.payload, sizeof(publishADC.payload), "%d", valueADC);
				xQueueSend(queuePubMQTT, &publish, portMAX_DELAY);
			}

			xSemaphoreGive(semaphoreADC);
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void vTaskLED (void *pvParameters)
{
	uint32_t pinLED = (uint32_t)pvParameters;
	mqtt_pubsub_t publishLED = {LED_TOPIC};

	pinMode(pinLED, OUTPUT);
	digitalWrite(pinLED, 0);

	vTaskDelay(pdMS_TO_TICKS(100));

	while (1)
	{
		uint8_t statusLED;

		xSemaphoreTake(semaphoreADC, portMAX_DELAY);
		statusLED = !digitalRead(pinLED);
		digitalWrite(pinLED, statusLED);
		
		xSemaphoreTake(semaphoreSerial, portMAX_DELAY);
		Serial.printf("Status LED: %s\r\n", statusLED ? "ON" : "OFF");
		xSemaphoreGive(semaphoreSerial);

		if (uxQueueSpacesAvailable(queuePubMQTT) > 0)
		{
			mqtt_pubsub_t *publish = &publishLED;
			
			snprintf(publishLED.payload, sizeof(publishLED.payload), "%d", statusLED);
			xQueueSend(queuePubMQTT, &publish, portMAX_DELAY);
		}
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void vTaskPubMQTT (void *pvParameters)
{
	mqtt_pubsub_t *publish = NULL;

	vTaskDelay(pdMS_TO_TICKS(100));

	while (1)
	{
		xQueueReceive(queuePubMQTT, &publish, portMAX_DELAY);
		PublishMQTT(publish);
		publish = NULL;
		vTaskDelay(pdMS_TO_TICKS(4000));
	}
}
