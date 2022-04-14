#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#define BUTTON_PIN		23
#define BUTTON_FLAG		(1 << 0)

#define LED_RED_PIN		2
#define LED_RED_STATE	0
#define LED_RED_TIME	500

#define MESSAGE1_FLAG	(1 << 1)
#define MESSAGE2_FLAG	(1 << 2)

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
button_t button = {BUTTON_PIN};

TimerHandle_t timerCallback = NULL;
EventGroupHandle_t eventGroup = NULL;

void vTaskButton (void *pvParameters);
void vTaskBlinkLED (void *pvParameters);
void vTaskSendMessage1 (void *pvParameters);
void vTaskSendMessage2 (void *pvParameters);

void vTimerCallback (TimerHandle_t pxTimer);

void setup()
{
	Serial.begin(9600);

	eventGroup = xEventGroupCreate();
	timerCallback = xTimerCreate("Callback Timer", pdMS_TO_TICKS(1000), pdTRUE, NULL, vTimerCallback);
	xTimerStart(timerCallback, 0);

	xTaskCreate(vTaskButton, "Task Button", configMINIMAL_STACK_SIZE + 1000, &button, 1, &button.taskHandle);
	xTaskCreate(vTaskBlinkLED, "Blink Task", configMINIMAL_STACK_SIZE + 1000, &ledRed, 1, &ledRed.taskHandle);
	xTaskCreate(vTaskSendMessage1, "Send Message 1", configMINIMAL_STACK_SIZE + 1000, (void *)"Teste 1", 1, NULL);
	xTaskCreate(vTaskSendMessage2, "Send Message 2", configMINIMAL_STACK_SIZE + 1000, (void *)"Teste 2", 1, NULL);

}

void loop()
{
	vTaskDelay(portMAX_DELAY);
}

void vTaskButton (void *pvParameters)
{
	button_t *button = (button_t *)pvParameters;
	uint8_t buttonFlag = 0;

	pinMode(button->pin, INPUT_PULLUP);

	while (1)
	{
		if (digitalRead(button->pin) == 0 && buttonFlag == 0)
		{
			vTaskDelay(10);
			if (digitalRead(button->pin) == 0)
				buttonFlag = 1;
		}
		else if (digitalRead(button->pin) == 1 && buttonFlag == 1)
		{
			vTaskDelay(10);
			buttonFlag = 0;
			xEventGroupSetBits(eventGroup, BUTTON_FLAG);
		}

		vTaskDelay(1);
	}
}

void vTaskBlinkLED (void *pvParameters)
{
	blink_led_t *led = (blink_led_t *)pvParameters;

	pinMode(led->pin, OUTPUT);
	digitalWrite(led->pin, led->state);

	while (1)
	{
		vTaskDelay(led->timeBlink);
		digitalWrite(led->pin, !digitalRead(led->pin));
	}
}

void vTaskSendMessage1 (void *pvParameters)
{
	char *buffer = (char *)pvParameters;

	while (1)
	{
		xEventGroupWaitBits(eventGroup, MESSAGE1_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);
		Serial.printf("%s\r\n", buffer);
	}
}

void vTaskSendMessage2 (void *pvParameters)
{
	char *buffer = (char *)pvParameters;

	while (1)
	{
		xEventGroupWaitBits(eventGroup, BUTTON_FLAG | MESSAGE2_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);
		Serial.printf("%s\r\n", buffer);
	}
}

void vTimerCallback (TimerHandle_t pxTimer)
{
	static uint32_t count = 0;

	++count;

	if (count == 5)
		xEventGroupSetBits(eventGroup, MESSAGE1_FLAG);
	else if (count == 10)
		xEventGroupSetBits(eventGroup, MESSAGE2_FLAG);
	else if (count >= 15)
		count = 0;
}