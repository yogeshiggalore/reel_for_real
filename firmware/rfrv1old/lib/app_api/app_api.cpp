#include <Arduino.h>
#include "app_api.h"
#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>
#include "AsyncMqttClient.h"

AsyncMqttClient mqttClient;
struct app_api_params *ptr_app_params;

void WiFiEvent(WiFiEvent_t event)
{

	switch (event)
	{
	case WIFI_EVENT_STA_CONNECTED:
		ptr_app_params->cfg.wifi.status = WIFI_EVENT_STA_CONNECTED;
		ptr_app_params->cfg.wifi.status = WIFI_STATUS_CONNECTED;
		mqttClient.connect();
		break;

	case WIFI_EVENT_STA_DISCONNECTED:
		ptr_app_params->cfg.wifi.status = WIFI_STATUS_DISCONNECTED;
		WiFi.disconnect();
    	WiFi.reconnect();
		break;
	}
}

void onMqttConnect(bool sessionPresent)
{
	ptr_app_params->cfg.mqtt.status = MQTT_STATUS_CONNECTED;

	Serial.println("Connected to MQTT.");
	Serial.print("Session present: ");
	Serial.println(sessionPresent);
	
	uint16_t packetIdSub = mqttClient.subscribe(ptr_app_params->cfg.mqtt.cfg_get_topic, 2);
	Serial.print("Subscribing at QoS 2, packetId: ");
	Serial.println(packetIdSub);

	uint16_t packetIdPub1 = mqttClient.publish(ptr_app_params->cfg.mqtt.cfg_set_topic, 1, true, "test 2");
	Serial.print("Publishing at QoS 1, packetId: ");
	Serial.println(packetIdPub1);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
	Serial.println("Disconnected from MQTT.");
	ptr_app_params->cfg.mqtt.status = MQTT_STATUS_DISCONNECTED;
	mqttClient.connect();
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
	Serial.println("Subscribe acknowledged.");
	Serial.print("  packetId: ");
	Serial.println(packetId);
	Serial.print("  qos: ");
	Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId)
{
	Serial.println("Unsubscribe acknowledged.");
	Serial.print("  packetId: ");
	Serial.println(packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
	Serial.println("Publish received.");
	Serial.print("  topic: ");
	Serial.println(topic);
	Serial.print("  qos: ");
	Serial.println(properties.qos);
	Serial.print("  dup: ");
	Serial.println(properties.dup);
	Serial.print("  retain: ");
	Serial.println(properties.retain);
	Serial.print("  len: ");
	Serial.println(len);
	Serial.print("  index: ");
	Serial.println(index);
	Serial.print("  total: ");
	Serial.println(total);
}

void onMqttPublish(uint16_t packetId)
{
	Serial.println("Publish acknowledged.");
	Serial.print("  packetId: ");
	Serial.println(packetId);
}

void app_api_init(struct app_api_params *params)
{
	ptr_app_params = params;

	app_api_init_camera(&(params->cfg.camera));
	app_api_init_sd(&(params->cfg.sd));

	Serial.printf("app_api_init\n");
	Serial.printf("camera: init:%d\n", params->cfg.camera.init);
	Serial.printf("sd init:%d\n", params->cfg.sd.init);

	app_api_init_wifi(&(params->cfg.wifi));
	app_api_mqtt_start(&(params->cfg.mqtt));
}

void app_api_init_camera(struct camera_api_params *params)
{

	params->status = CAMERA_STATUS_NONE;
	params->err = CAMERA_ERR_NONE;
	params->init = false;

	params->cfg.ledc_channel = LEDC_CHANNEL_0;
	params->cfg.ledc_timer = LEDC_TIMER_0;
	params->cfg.pin_d0 = Y2_GPIO_NUM;
	params->cfg.pin_d1 = Y3_GPIO_NUM;
	params->cfg.pin_d2 = Y4_GPIO_NUM;
	params->cfg.pin_d3 = Y5_GPIO_NUM;
	params->cfg.pin_d4 = Y6_GPIO_NUM;
	params->cfg.pin_d5 = Y7_GPIO_NUM;
	params->cfg.pin_d6 = Y8_GPIO_NUM;
	params->cfg.pin_d7 = Y9_GPIO_NUM;
	params->cfg.pin_xclk = XCLK_GPIO_NUM;
	params->cfg.pin_pclk = PCLK_GPIO_NUM;
	params->cfg.pin_vsync = VSYNC_GPIO_NUM;
	params->cfg.pin_href = HREF_GPIO_NUM;
	params->cfg.pin_sscb_sda = SIOD_GPIO_NUM;
	params->cfg.pin_sscb_scl = SIOC_GPIO_NUM;
	params->cfg.pin_pwdn = PWDN_GPIO_NUM;
	params->cfg.pin_reset = RESET_GPIO_NUM;
	params->cfg.xclk_freq_hz = 20000000;
	params->cfg.frame_size = FRAMESIZE_UXGA;
	params->cfg.pixel_format = PIXFORMAT_JPEG; // for streaming
	params->cfg.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
	params->cfg.fb_location = CAMERA_FB_IN_PSRAM;
	params->cfg.jpeg_quality = 12;
	params->cfg.fb_count = 1;

	// if PSRAM IC present, init with UXGA resolution and higher JPEG quality for larger pre-allocated frame buffer.
	if (params->cfg.pixel_format == PIXFORMAT_JPEG)
	{
		if (psramFound())
		{
			params->cfg.jpeg_quality = 10;
			params->cfg.fb_count = 2;
			params->cfg.grab_mode = CAMERA_GRAB_LATEST;
		}
		else
		{
			// Limit the frame size when PSRAM is not available
			params->cfg.frame_size = FRAMESIZE_SVGA;
			params->cfg.fb_location = CAMERA_FB_IN_DRAM;
		}
	}
	else
	{
		// Best option for face detection/recognition
		params->cfg.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
		params->cfg.fb_count = 2;
#endif
	}

	// camera init
	esp_err_t err = esp_camera_init(&(params->cfg));
	if (err != ESP_OK)
	{
		params->err = CAMERA_ERR_INIT;
	}
	else
	{
		params->status = CAMERA_STATUS_INIT_SUCCESS;
		params->init = true;
	}
}

void app_api_init_sd(struct sd_api_params *params)
{
	params->init = false;

	// Initialize SD card
	if (!SD.begin(21))
	{
		params->err = SD_ERR_TYPE;
	}
	else
	{
		// Get card type
		params->cardType = SD.cardType();

		// Determine if the type of SD card is available
		if (params->cardType == CARD_NONE)
		{
			params->err = SD_ERR_TYPE;
		}
		else
		{
			params->status = SD_STATUS_INIT_SUCCESS;
			params->init = true;
		}
	}
}

int app_take_photo_and_save(struct app_api_params *params, const char *fileName)
{
	int err = -1;

	if (params->cfg.camera.init == true)
	{
		// Take a photo
		camera_fb_t *fb = esp_camera_fb_get();
		if (!fb)
		{
			params->cfg.camera.err = CAMERA_ERR_FRAME_BUF;
		}
		else
		{
			// Save photo to file
			app_write_to_file(params, SD, fileName, fb->buf, fb->len);

			// Release image buffer
			esp_camera_fb_return(fb);

			params->cfg.camera.status = CAMERA_STATUS_PHOTO_SUCCESS;
			err = 0;
			;
		}
	}
	else
	{
		params->cfg.camera.status = CAMERA_ERR_INIT;
	}

	return err;
}

void app_write_to_file(struct app_api_params *params, fs::FS &fs, const char *path, uint8_t *data, size_t len)
{
	if (params->cfg.sd.init == true)
	{
		File file = fs.open(path, FILE_WRITE);
		if (!file)
		{
			params->cfg.sd.err = SD_ERR_FILE_OPEN;
		}
		else
		{
			if ((file.write(data, len) == len))
			{
				params->cfg.sd.status = SD_STATUS_WRITE_SUCCESS;
			}
			else
			{
				params->cfg.sd.err = SD_ERR_FILE_WRITE;
			}

			file.close();
		}
	}
	else
	{
		params->cfg.sd.status = SD_ERR_INIT;
	}
}

void app_api_init_wifi(struct wifi_api_params *params)
{
	params->status = WIFI_STATUS_NONE;
	params->check_cnt = 10;
	params->err = WIFI_ERR_NONE;
	params->init = false;

	sprintf(params->ssid, "IOTDev");
	params->len_ssid = strlen(params->ssid);

	sprintf(params->password, "ammu1234");
	params->len_password = strlen(params->password);

	memset(params->mac, 0, sizeof(params->mac));

	Serial.printf("app_api_init_wifi\n");
	Serial.printf("ssid: %s\n", params->ssid);
	Serial.printf("pass: %s\n", params->password);

	WiFi.mode(WIFI_STA);
	app_api_wifi_start(params);
}

void app_api_wifi_start(struct wifi_api_params *params)
{
	esp_err_t ret;

	WiFi.begin(params->ssid, params->password);
	WiFi.onEvent(WiFiEvent);
	
	for (size_t i = 0; i < params->check_cnt; i++)
	{
		if (WiFi.status() == WL_CONNECTED)
		{
			params->status = WIFI_STATUS_CONNECTED;
			i = params->check_cnt + 1;
			params->local_ip = WiFi.localIP();
		}
		delay(500);
	}

	if (WiFi.status() != WL_CONNECTED)
	{
		params->err = WIFI_ERR_CONNECT;
	}
}

void app_api_mqtt_start(struct mqtt_api_params *params)
{
	sprintf(params->server, "broker.emqx.io");
	params->port = 1883;
	sprintf(params->user_name, "reelforreal");
	sprintf(params->password, "ammu1234");
	sprintf(params->client_id, "reelforreal1");
	sprintf(params->cfg_get_topic, "%s/rfr1/cfg/get", params->client_id);
	sprintf(params->cfg_set_topic, "%s/rfr1/cfg/set", params->client_id);

	mqttClient.onConnect(onMqttConnect);
  	mqttClient.onDisconnect(onMqttDisconnect);
  	mqttClient.onSubscribe(onMqttSubscribe);
  	mqttClient.onUnsubscribe(onMqttUnsubscribe);
  	mqttClient.onMessage(onMqttMessage);
  	mqttClient.onPublish(onMqttPublish);
  	mqttClient.setServer(params->server, params->port);

	Serial.println("Connecting to MQTT...");
	Serial.printf("server: %s\n", params->server);
	Serial.printf("port: %d\n", params->port);
	Serial.printf("user: %s\n", params->user_name);
	Serial.printf("pass: %s\n", params->password);
	Serial.printf("client_id: %s\n", params->client_id);
	Serial.printf("cfg_get_topic: %s\n", params->cfg_get_topic);
	Serial.printf("cfg_set_topic: %s\n", params->cfg_set_topic);
}

void app_api_read_wifi(struct app_api_params *params)
{

}

void app_api_read_mqtt(struct app_api_params *params)
{

}

void app_api_read(struct app_api_params *params)
{
	app_api_read_wifi(params);
	app_api_read_mqtt(params);
}

void app_api_process(struct app_api_params *params)
{
	
}

void app_api_write(struct app_api_params *params)
{

}