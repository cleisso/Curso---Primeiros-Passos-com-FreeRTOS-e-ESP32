#include "mqtt.hpp"

static TaskHandle_t  taskPublishMQTT = NULL;
static TaskHandle_t  taskSubscribeMQTT[10] = {NULL};

static QueueHandle_t queuePublishMQTT = NULL;
static QueueHandle_t queueSubscribeMQTT = NULL;

static TimerHandle_t timerConnectMQTT = NULL;

static SemaphoreHandle_t semaphorePublishMQTT = NULL;

static WiFiClient wifiClient;
static MQTTClient mqttClient;

static void vTaskPublishMQTT (void *pvParameters);
static void vTaskSubscribeMQTT (void *pvParameters);
static void CallbackConnectMQTT (TimerHandle_t timerHandle);
static void CallbackSubscribeMQTT (String &topic, String &payload);

mqtt_status_t BeginMQTT (void)
{
	wl_status_t wifiStatus = WL_DISCONNECTED;
	bool mqttStatus = false;
	
	// Estabelecendo a conexão via WiFi
	#if (DEBUG_MQTT)
	Serial.printf("%s: Conectando o WiFi...\n", __FUNCTION__);
	#endif

	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	for (uint32_t timeout = 0; timeout < 10; ++timeout)
	{
		wifiStatus = WiFi.status();

		if (wifiStatus != WL_CONNECTED)
			vTaskDelay(pdMS_TO_TICKS(1000));
		else
			break;
	}

	if (wifiStatus != WL_CONNECTED)
		return ERROR_WIFI;

	// Estabelecendo a conexão ao Broker MQTT
	#if (DEBUG_MQTT)
	Serial.printf("%s: Conectando ao Broker MQTT\n", __FUNCTION__);
	#endif

	mqttClient.begin(BROKER_ADDRESS, wifiClient);
	for (uint32_t timeout = 0; timeout < 10; ++timeout)
	{
		mqttStatus = mqttClient.connect(CLIENT_ID, BROKER_USERNAME, BROKER_PASSWORD);

		if (mqttStatus == false)
			vTaskDelay(pdMS_TO_TICKS(1000));
		else
			break;
	}

	if (mqttStatus == false)
		return ERROR_MQTT;

	#if (DEBUG_MQTT)
	Serial.printf("%s: Salvando a funcao de Callback\n", __FUNCTION__);
	#endif

	mqttClient.onMessage(CallbackSubscribeMQTT);

	#if (DEBUG_MQTT)
	Serial.printf("%s: Configurando o Timer\n", __FUNCTION__);
	#endif

	timerConnectMQTT = xTimerCreate("Callback MQTT", pdMS_TO_TICKS(1000), pdTRUE, NULL, CallbackConnectMQTT);
	xTimerStart(timerConnectMQTT, 0);

	#if (DEBUG_MQTT)
	Serial.printf("%s: Configurando as Queues de Pub e Sub\n", __FUNCTION__);
	#endif

	queuePublishMQTT = xQueueCreate(20, sizeof(mqtt_pubsub_t *));
	queueSubscribeMQTT = xQueueCreate(1, 50*sizeof(char));

	semaphorePublishMQTT = xSemaphoreCreateMutex();

	#if (DEBUG_MQTT)
	Serial.printf("%s: Configurando a Task de Pub\n", __FUNCTION__);
	#endif

	xTaskCreate(vTaskPublishMQTT, "Publish MQTT", configMINIMAL_STACK_SIZE + 2000, NULL, MQTT_PRIORITY, &taskPublishMQTT);

	return OK_MQTT;
}

mqtt_status_t CloseMQTT (void)
{
	vTaskDelete(taskPublishMQTT);
	
	for (uint8_t taskIndex = 0; taskIndex < (sizeof(taskSubscribeMQTT) / sizeof(taskSubscribeMQTT[0])); ++taskIndex)
	{
		vTaskDelete(taskSubscribeMQTT[taskIndex]);
	}
	
	xTimerStop(timerConnectMQTT, 0);
	xTimerDelete(timerConnectMQTT, 0);

	vQueueDelete(queuePublishMQTT);
	vQueueDelete(queueSubscribeMQTT);

	vSemaphoreDelete(semaphorePublishMQTT);

	mqttClient.disconnect();
	WiFi.disconnect();

	return OK_MQTT;
}

mqtt_status_t PublishMQTT (mqtt_pubsub_t *publish)
{
	#if (DEBUG_MQTT)
	Serial.printf("%s: Publicando no topico: %s, payload: %s\n", __FUNCTION__, publish->topic, publish->payload);
	#endif

	if (uxQueueSpacesAvailable > 0)
	{
		if (xQueueSend(queuePublishMQTT, &publish, portMAX_DELAY) == pdTRUE)
		{
			#if (DEBUG_MQTT)
			Serial.printf("%s: Publicao Ok\n", __FUNCTION__);
			#endif
		}
		else
		{
			#if (DEBUG_MQTT)
			Serial.printf("%s: Erro\n", __FUNCTION__);
			#endif
		}

	}
	else
	{
		#if (DEBUG_MQTT)
		Serial.printf("%s: Nao ha espaco na Queue\n", __FUNCTION__);
		#endif

		return FULL_PUB_MQTT;
	}
	
	return OK_MQTT;
}

mqtt_status_t SubscribeMQTT (char *topic, QueueHandle_t queueHandle)
{
	uint8_t taskIndex;

	#if (DEBUG_MQTT)
	Serial.printf("%s: Subscrevendo ao topico: %s\n", __FUNCTION__, topic);
	#endif

	for (taskIndex = 0; taskIndex < 10; ++taskIndex)
	{
		if (taskSubscribeMQTT[taskIndex] == NULL)
			break;
	}

	if (taskIndex == 10)
	{
		#if (DEBUG_MQTT)
		Serial.printf("%s: Limite de topicos subscritos alcancado\n", __FUNCTION__);
		#endif

		return FULL_SUB_MQTT;
	}

	if (queueHandle == NULL)
	{
		#if (DEBUG_MQTT)
		Serial.printf("%s: Queue invalida\n", __FUNCTION__);
		#endif

		return ERROR_QUEUE;
	}

	#if (DEBUG_MQTT)
	Serial.printf("%s: Sub: %s\n", __FUNCTION__, topic);
	#endif

	mqttClient.subscribe(topic);

	#if (DEBUG_MQTT)
	Serial.printf("%s: Configurando a task do Sub\n", __FUNCTION__);
	#endif

	xTaskCreate(vTaskSubscribeMQTT, &topic[sizeof(THINGSPEAK_SUB_TOPIC) - 1], configMINIMAL_STACK_SIZE + 1000, queueHandle, MQTT_PRIORITY, &taskSubscribeMQTT[taskIndex]);

	return OK_MQTT;
}

mqtt_status_t UnsubscribeMQTT (char *topic)
{
	uint8_t taskIndex;

	for (taskIndex = 0; taskIndex < 10; ++taskIndex)
	{
		if (taskSubscribeMQTT[taskIndex] == NULL)
			continue;

		if (String(topic).indexOf( pcTaskGetTaskName(taskSubscribeMQTT[taskIndex]), sizeof(THINGSPEAK_SUB_TOPIC - 1) ) >= 0)
		{
			mqttClient.unsubscribe(topic);
			vTaskDelete(taskSubscribeMQTT[taskIndex]);
			taskSubscribeMQTT[taskIndex] = NULL;
			break;
		}
	}

	if (taskIndex < 10)
		return OK_MQTT;
	else
		return ERROR_MQTT;
}

static void vTaskPublishMQTT (void *pvParameters)
{
	(void)pvParameters;

	#if (DEBUG_MQTT)
	Serial.printf("%s: Inicializando a Task de Pub\n", __FUNCTION__);
	#endif

	while (1)
	{
		mqtt_pubsub_t *publishBuffer;

		if (xQueueReceive(queuePublishMQTT, &publishBuffer, portMAX_DELAY) == pdTRUE)
		{
			#if (DEBUG_MQTT)
			Serial.printf("%s: Mensagem recebida da Queue\n", __FUNCTION__);
			#endif
		}
		else
		{
			#if (DEBUG_MQTT)
			Serial.printf("%s: Mensagem nao recebida da Queue\n", __FUNCTION__);
			#endif
		}

		xSemaphoreTake(semaphorePublishMQTT, portMAX_DELAY);
		if (mqttClient.publish(publishBuffer->topic, publishBuffer->payload))
		{
			#if (DEBUG_MQTT)
			Serial.printf("%s: Pub. Topic: %s, Sucesso\n", __FUNCTION__, publishBuffer->topic);
			#endif
		}
		else
		{
			#if (DEBUG_MQTT)
			Serial.printf("%s: Pub. Topic: %s, Erro\n", __FUNCTION__, publishBuffer->topic);
			#endif
		}
		xSemaphoreGive(semaphorePublishMQTT);

		#if (DEBUG_MQTT)
		Serial.printf("%s: Pub. Topic: %s, Payload: %s\n", __FUNCTION__, publishBuffer->topic, publishBuffer->payload);
		#endif

		vTaskDelay(10);
	}
}

static void vTaskSubscribeMQTT (void *pvParameters)
{
	char payloadBuffer[50];
	QueueHandle_t queue = (QueueHandle_t)pvParameters;

	#if (DEBUG_MQTT)
	Serial.printf("%s: Inicializando a Task do Sub. Topic: %s\n", __FUNCTION__, pcTaskGetTaskName(NULL));
	#endif

	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		xQueueReceive(queueSubscribeMQTT, &payloadBuffer, portMAX_DELAY);
		xQueueSend(queue, &payloadBuffer, portMAX_DELAY);

		#if (DEBUG_MQTT)
		Serial.printf("%s: Sub. Topic: %s, Payload: %s\n", __FUNCTION__, pcTaskGetTaskName(NULL), payloadBuffer);
		#endif

		vTaskDelay(10);
	}
}

static void CallbackConnectMQTT (TimerHandle_t timerHandle)
{
	mqttClient.loop();
	vTaskDelay(pdMS_TO_TICKS(10));

	if (!mqttClient.connected())
	{
		mqttClient.connect(CLIENT_ID, BROKER_USERNAME, BROKER_PASSWORD);
	}
	else
	{
		#if (DEBUG_MQTT)
		Serial.printf("%s: Conectado\n", __FUNCTION__);
		#endif
	}
}

static void CallbackSubscribeMQTT (String &topic, String &payload)
{
	for (uint8_t taskIndex = 0; taskIndex < 10; ++taskIndex)
	{
		if (taskSubscribeMQTT[taskIndex] == NULL)
			continue;

		if (topic.indexOf( pcTaskGetTaskName(taskSubscribeMQTT[taskIndex]), sizeof(THINGSPEAK_SUB_TOPIC) -1 ) >= 0)
		{
			unsigned char payloadBuffer[50] = {0};

			payload.getBytes(payloadBuffer, 50);

			xQueueSend(queueSubscribeMQTT, &payloadBuffer, 100);
			xTaskNotifyGive(taskSubscribeMQTT[taskIndex]);
			break;
		}
	}
}