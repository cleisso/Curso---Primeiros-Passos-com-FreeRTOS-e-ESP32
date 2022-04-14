#ifndef MQTT_HPP
#define MQTT_HPP

/**
 * Bibliotecas
 */
#include "Arduino.h"
#include "WiFi.h"
#include "MQTT.h"

#include "stdio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

/**
 * Configurações de Rede
 */
#define WIFI_SSID				"CTB"
#define WIFI_PASSWORD			"49918451"

#define BROKER_ADDRESS			"mqtt3.thingspeak.com"
#define BROKER_USERNAME			"AzguCjgKGhIcPBovBAswOh4"
#define BROKER_PASSWORD			"/uamunmY9WMahWruMHbJVvSx"
#define CLIENT_ID				"AzguCjgKGhIcPBovBAswOh4"

#define THINGSPEAK_CH_ID		"1700282"
#define THINGSPEAK_PUB_TOPIC	"channels/" THINGSPEAK_CH_ID "/publish/fields/"
#define THINGSPEAK_SUB_TOPIC	"channels/" THINGSPEAK_CH_ID "/subscribe/fields/"
#define THINGSPEAK_TOPIC1		"field1"
#define THINGSPEAK_TOPIC2		"field2"
#define THINGSPEAK_TOPIC3		"field3"
#define THINGSPEAK_TOPIC4		"field4"
#define THINGSPEAK_TOPIC5		"field5"
#define THINGSPEAK_TOPIC6		"field6"
#define THINGSPEAK_TOPIC7		"field7"
#define THINGSPEAK_TOPIC8		"field8"

#define MQTT_PRIORITY		10

/**
 * Tipos de Dado
 */
typedef enum mqtt_status_t
{
	ERROR_QUEUE = -5,
	FULL_SUB_MQTT = -4,
	FULL_PUB_MQTT = -3,
	ERROR_MQTT = -2,
	ERROR_WIFI = -1,
	OK_MQTT = 0
} mqtt_status_t;

typedef struct mqtt_pubsub_t
{
	char topic[50];
	char payload[50];
} mqtt_pubsub_t;

/**
 * Protótipo das Funções
 */
mqtt_status_t BeginMQTT (void);
mqtt_status_t CloseMQTT (void);
mqtt_status_t PublishMQTT (mqtt_pubsub_t *publish);
mqtt_status_t SubscribeMQTT (char *topic, QueueHandle_t queueHandle);

#define DEBUG_MQTT	0

#endif