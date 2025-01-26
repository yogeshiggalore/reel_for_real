#include <Arduino.h>
#include "app_api.h"
#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>
#include "AsyncMqttClient.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <base64.h>
#include <WiFiClientSecure.h>
#include <ESP32Time.h>
#include "Base64.h"

#define NTP_OFFSET 19800	   // In seconds
#define NTP_INTERVAL 10 * 1000 // In miliseconds
#define NTP_ADDRESS "1.asia.pool.ntp.org"

AsyncMqttClient mqttClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
WiFiClientSecure gglClient;
ESP32Time rtc(0);
hw_timer_t *hwtimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

struct app_api_params *ptr_app_params;

String myScript = "/macros/s/AKfycbxsJlUAtG1nauw5uq2r_auO2XD94IAGdZSIVUjqMpPo9cq1I9Xpea9oEVzfazaGAdD_dw/exec";
String myFoldername = "&myFoldername=rfrv1";
String myFilename = "&myFilename=test1.jpg";
String myImage = "&myFile=";

void IRAM_ATTR onTimer()
{
	portENTER_CRITICAL_ISR(&timerMux);
	ptr_app_params->data.timer.curr_timecntr++;
	portEXIT_CRITICAL_ISR(&timerMux);
}

void WiFiEvent(WiFiEvent_t event)
{
	Serial.printf("[WiFi-event] event: %d\n", event);

	switch (event)
	{
	case ARDUINO_EVENT_WIFI_READY:
		Serial.println("WiFi interface ready");
		break;
	case ARDUINO_EVENT_WIFI_SCAN_DONE:
		Serial.println("Completed scan for access points");
		break;
	case ARDUINO_EVENT_WIFI_STA_START:
		Serial.println("WiFi client started");
		break;
	case ARDUINO_EVENT_WIFI_STA_STOP:
		Serial.println("WiFi clients stopped");
		break;
	case ARDUINO_EVENT_WIFI_STA_CONNECTED:
		Serial.println("Connected to access point");
		break;
	case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
		Serial.println("Disconnected from WiFi access point");
		WiFi.disconnect();
		WiFi.reconnect();
		timeClient.end();
		break;
	case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
		Serial.println("Authentication mode of access point has changed");
		break;
	case ARDUINO_EVENT_WIFI_STA_GOT_IP:
		ptr_app_params->cfg.wifi.local_ip = WiFi.localIP();
		Serial.println("got IP address");
		timeClient.begin();
		app_api_epochtime_check(ptr_app_params);
		// mqttClient.connect();
		break;
	case ARDUINO_EVENT_WIFI_STA_LOST_IP:
		Serial.println("Lost IP address and IP address is reset to 0");
		break;
	case ARDUINO_EVENT_WPS_ER_SUCCESS:
		Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
		break;
	case ARDUINO_EVENT_WPS_ER_FAILED:
		Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
		break;
	case ARDUINO_EVENT_WPS_ER_TIMEOUT:
		Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
		break;
	case ARDUINO_EVENT_WPS_ER_PIN:
		Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
		break;
	case ARDUINO_EVENT_WIFI_AP_START:
		Serial.println("WiFi access point started");
		break;
	case ARDUINO_EVENT_WIFI_AP_STOP:
		Serial.println("WiFi access point  stopped");
		break;
	case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
		Serial.println("Client connected");
		break;
	case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
		Serial.println("Client disconnected");
		break;
	case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
		Serial.println("Assigned IP address to client");
		break;
	case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
		Serial.println("Received probe request");
		break;
	case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
		Serial.println("AP IPv6 is preferred");
		break;
	case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
		Serial.println("STA IPv6 is preferred");
		break;
	case ARDUINO_EVENT_ETH_GOT_IP6:
		Serial.println("Ethernet IPv6 is preferred");
		break;
	case ARDUINO_EVENT_ETH_START:
		Serial.println("Ethernet started");
		break;
	case ARDUINO_EVENT_ETH_STOP:
		Serial.println("Ethernet stopped");
		break;
	case ARDUINO_EVENT_ETH_CONNECTED:
		Serial.println("Ethernet connected");
		break;
	case ARDUINO_EVENT_ETH_DISCONNECTED:
		Serial.println("Ethernet disconnected");
		break;
	case ARDUINO_EVENT_ETH_GOT_IP:
		Serial.println("Obtained IP address");
		break;
	default:
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

	packetIdSub = mqttClient.subscribe(ptr_app_params->cfg.mqtt.img_take_topic, 1);
	Serial.print("Subscribing at QoS 1, packetId: ");
	Serial.println(packetIdSub);

	uint16_t packetIdPub1 = mqttClient.publish(ptr_app_params->cfg.mqtt.cfg_set_topic, 1, true, "test 2");
	Serial.print("Publishing at QoS 1, packetId: ");
	Serial.println(packetIdPub1);

	packetIdPub1 = mqttClient.publish(ptr_app_params->cfg.mqtt.will_topic, 1, true, "online");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
	Serial.printf("Disconnected from MQTT. reason:%d\n", reason);
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
	if (strcmp(topic, ptr_app_params->cfg.mqtt.img_take_topic) == 0)
	{
		ptr_app_params->data.mqtt.img_take = int(strtol(payload, NULL, 10));
		Serial.printf("received img_take:%d\n", ptr_app_params->data.mqtt.img_take);
	}
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

	params->data.img_start_time = 0;
	params->data.img_start_time |= IMG_CAP_START_HOUR;
	params->data.img_start_time <<= 8;
	params->data.img_start_time |= IMG_CAP_START_MIN;
	params->data.img_start_time <<= 8;
	params->data.img_start_time |= IMG_CAP_START_SEC;

	params->data.img_end_time = 0;
	params->data.img_end_time |= IMG_CAP_END_HOUR;
	params->data.img_end_time <<= 8;
	params->data.img_end_time |= IMG_CAP_END_MIN;
	params->data.img_end_time <<= 8;
	params->data.img_end_time |= IMG_CAP_END_SEC;

	app_api_timer_init(&(params->data.timer));

	app_api_init_camera(&(params->cfg.camera));
	app_api_init_sd(&(params->cfg.sd));

	Serial.printf("app_api_init\n");
	Serial.printf("camera: init:%d\n", params->cfg.camera.init);
	Serial.printf("sd init:%d\n", params->cfg.sd.init);

	// app_api_mqtt_start(&(params->cfg.mqtt));
	app_api_init_wifi(&(params->cfg.wifi));

	app_api_google_start(&(params->cfg.ggl));
}

void app_api_timer_init(struct timer_params *params)
{
	params->curr_timecntr = 0;
	params->lst_timecntr = 0;
	params->sec_10_cntr = 0;
	params->sec_30_cntr = 0;
	params->sec_60_cntr = 0;
	params->sec_300_cntr = 0;
	params->flag_1_sec = 0;
	params->flag_10_sec = 0;
	params->flag_30_sec = 0;
	params->flag_60_sec = 0;
	params->flag_300_sec = 0;

	params->img_timeout_cntr = IMG_TAKE_TIMEOUT;

	hwtimer = timerBegin(0, 80, true);			   // timer 0, prescalar: 80, UP counting
	timerAttachInterrupt(hwtimer, &onTimer, true); // Attach interrupt
	timerAlarmWrite(hwtimer, 1000000, true);	   // Match value= 1000000 for 1 sec. delay.
	timerAlarmEnable(hwtimer);					   // Enable Timer with interrupt (Alarm Enable)
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
	params->cfg.frame_size = FRAMESIZE_XGA;
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
		params->img_timeout = IMG_TAKE_TIMEOUT;
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

int app_take_photo_and_save_sd(struct app_api_params *params, const char *fileName)
{
	int err = -1;

	Serial.printf("taking img %s and saving in sd card \n", fileName);
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
			Serial.printf("image size %u\n", fb->len);
			app_write_to_file(params, SD, fileName, fb->buf, fb->len);

			// Release image buffer
			esp_camera_fb_return(fb);

			params->cfg.camera.status = CAMERA_STATUS_PHOTO_SUCCESS;
			err = 0;
			Serial.println("photo saved in sd card");
		}
	}
	else
	{
		params->cfg.camera.status = CAMERA_ERR_INIT;
		Serial.println("camera not init");
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

int app_take_photo_and_send_mqtt(struct app_api_params *params)
{
	int err = -1;
	uint16_t packetIdPub1;

	if (params->cfg.camera.init == true)
	{
		// Take a photo
		camera_fb_t *fb = esp_camera_fb_get();
		if (!fb)
		{
			params->cfg.camera.err = CAMERA_ERR_FRAME_BUF;
			Serial.println("camera frame err");
		}
		else
		{
			Serial.println("sending image to server");
			// String buffer = base64::encode((uint8_t *)fb->buf, fb->len);

			// packetIdPub1 = mqttClient.publish(ptr_app_params->cfg.mqtt.img_send_topic, 1, true, buffer.c_str());
			// Serial.printf("sending img buf len:%u", buffer.length());

			// // Release image buffer
			// esp_camera_fb_return(fb);

			// params->cfg.camera.status = CAMERA_STATUS_PHOTO_SUCCESS;
			err = 0;
		}
	}
	else
	{
		params->cfg.camera.status = CAMERA_ERR_INIT;
		Serial.println("camera not init");
	}

	return err;
}

int app_take_photo_and_send_google(struct app_api_params *params)
{
	int err = -1;
	uint16_t packetIdPub1;
	unsigned long totl_time = millis();
	unsigned long camera_time = millis();
	unsigned long ggl_time = millis();

	gglClient.setInsecure();

	if (params->cfg.camera.init == true)
	{
		// Take a photo
		camera_fb_t *fb = esp_camera_fb_get();
		if (!fb)
		{
			params->cfg.camera.err = CAMERA_ERR_FRAME_BUF;
			Serial.println("camera frame err");
		}
		else
		{
			camera_time = millis() - camera_time;
			ggl_time = millis();
			Serial.println("sending image to google drive");
			if (gglClient.connect(params->cfg.ggl.domain, params->cfg.ggl.port))
			{
				Serial.println("connected to domain");
				Serial.println("Size: " + String(fb->len) + "byte");

				gglClient.println("POST " + String(params->cfg.ggl.url) + " HTTP/1.1");
				gglClient.println("Host: " + String(params->cfg.ggl.domain));
				gglClient.println("Transfer-Encoding: chunked");
				gglClient.println();

				int fbLen = fb->len;
				char *input = (char *)fb->buf;
				int chunkSize = 3 * 1000; //--> must be multiple of 3.
				int chunkBase64Size = base64_enc_len(chunkSize);
				char output[chunkBase64Size + 1];

				Serial.println();
				int chunk = 0;
				for (int i = 0; i < fbLen; i += chunkSize)
				{
					int l = base64_encode(output, input, min(fbLen - i, chunkSize));
					gglClient.print(l, HEX);
					gglClient.print("\r\n");
					gglClient.print(output);
					gglClient.print("\r\n");
					delay(100);
					input += chunkSize;
					Serial.print(".");
					chunk++;
					if (chunk % 50 == 0)
					{
						Serial.println();
					}
				}
				gglClient.print("0\r\n");
				gglClient.print("\r\n");
			}
			else
			{
				Serial.println("connection to google domain failed");
			}
			ggl_time = millis() - ggl_time;
			gglClient.stop();
			// Release image buffer
			esp_camera_fb_return(fb);

			params->cfg.camera.status = CAMERA_STATUS_PHOTO_SUCCESS;
			err = 0;
		}
	}
	else
	{
		params->cfg.camera.status = CAMERA_ERR_INIT;
		Serial.println("camera not init");
	}

	totl_time = millis() - totl_time;

	Serial.printf("\ntotal time:%s\n", String(totl_time));
	Serial.printf("camera time:%s\n", String(camera_time));
	Serial.printf("ggl time:%s\n", String(ggl_time));
	return err;
}
void app_api_init_wifi(struct wifi_api_params *params)
{
	params->status = WIFI_STATUS_NONE;
	params->check_cnt = 10;
	params->err = WIFI_ERR_NONE;
	params->init = false;

	// sprintf(params->ssid, "IOTDev");
	// params->len_ssid = strlen(params->ssid);

	// sprintf(params->password, "ammu1234");
	// params->len_password = strlen(params->password);

	sprintf(params->ssid, "JioFiber-ya24");
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

	WiFi.onEvent(WiFiEvent);
	WiFi.begin(params->ssid, params->password);

	// for (size_t i = 0; i < params->check_cnt; i++)
	// {
	// 	if (WiFi.status() == WL_CONNECTED)
	// 	{
	// 		params->status = WIFI_STATUS_CONNECTED;
	// 		i = params->check_cnt + 1;
	// 		params->local_ip = WiFi.localIP();
	// 	}
	// 	delay(500);
	// }

	// if (WiFi.status() != WL_CONNECTED)
	// {
	// 	params->err = WIFI_ERR_CONNECT;
	// }
}

void app_api_mqtt_start(struct mqtt_api_params *params)
{
	char willtopic[32];

	// sprintf(params->server, "%s","io.adafruit.com");
	// params->port = 1883;
	// sprintf(params->user_name, "%s","yogi_baba");
	// sprintf(params->password, "%s","95e927f24f414acca6589f99818c46ed");

	sprintf(params->server, "%s", "herin.in");
	params->port = 8883;
	sprintf(params->user_name, "%s", "annonymous");
	sprintf(params->password, "%s", "herin@2425");
	sprintf(willtopic, "offline");
	sprintf(params->client_id, "rfr1D%06llx", ESP.getEfuseMac());
	sprintf(params->cfg_get_topic, "%s/rfr1/cfg/get", params->client_id);
	sprintf(params->cfg_set_topic, "%s/rfr1/cfg/set", params->client_id);
	sprintf(params->img_take_topic, "%s/rfr1/img/take", params->client_id);
	sprintf(params->img_send_topic, "%s/rfr1/img/send", params->client_id);
	sprintf(params->will_topic, "%s/rfr1/status", params->client_id);

	mqttClient.onConnect(onMqttConnect);
	mqttClient.onDisconnect(onMqttDisconnect);
	mqttClient.onSubscribe(onMqttSubscribe);
	mqttClient.onUnsubscribe(onMqttUnsubscribe);
	mqttClient.onMessage(onMqttMessage);
	mqttClient.onPublish(onMqttPublish);

	mqttClient.setServer(params->server, params->port);
	mqttClient.setCredentials(params->user_name, params->password);
	mqttClient.setClientId(params->client_id);
	mqttClient.setKeepAlive(params->keepalive);
	mqttClient.setCleanSession(true);
	mqttClient.setMaxTopicLength(128);

	mqttClient.setWill(ptr_app_params->cfg.mqtt.will_topic, 0, 1, willtopic, strlen(willtopic));
	delay(1000);

	Serial.printf("pass: %s\n", params->password);
	Serial.println("Connecting to MQTT...");
	Serial.printf("server: %s\n", params->server);
	Serial.printf("port: %d\n", params->port);
	Serial.printf("user: %s\n", params->user_name);
	Serial.printf("pass: %s\n", params->password);
	Serial.printf("client_id: %s\n", params->client_id);
	Serial.printf("keepalive: %d\n", params->keepalive);
	Serial.printf("cfg_get_topic: %s\n", params->cfg_get_topic);
	Serial.printf("cfg_set_topic: %s\n", params->cfg_set_topic);
	Serial.printf("will topic: %s\n", params->will_topic);

	delay(1000);
}

void app_api_google_start(struct ggl_api_params *params)
{
	sprintf(params->domain, "%s", "script.google.com");
	params->port = 443;
	sprintf(params->url, "%s", "/macros/s/AKfycbw3lw-59dYEDBehsweJWzQsjvhm2QBX-rzrQLVcuWrUA-z5wsRHxWUSj7iy0U-0fLPfoA/exec?folder=rfrv1");
	params->lstepochtime = 0;
	params->timediff = 60;
}

void app_api_read_wifi(struct app_api_params *params)
{
}

void app_api_read_mqtt(struct app_api_params *params)
{
}

void app_api_check_timer(struct app_api_params *params)
{
	uint8_t time_diff = 0;

	if (params->data.timer.curr_timecntr != params->data.timer.lst_timecntr)
	{
		time_diff = params->data.timer.curr_timecntr - params->data.timer.lst_timecntr;

		params->data.timer.lst_timecntr = params->data.timer.curr_timecntr;

		if (time_diff >= 1)
		{
			params->data.timer.flag_1_sec = 1;
		}

		params->data.timer.sec_10_cntr += time_diff;
		if (params->data.timer.sec_10_cntr >= 10)
		{
			params->data.timer.flag_10_sec = 1;
			params->data.timer.sec_10_cntr = 0;
		}

		params->data.timer.sec_30_cntr += time_diff;
		if (params->data.timer.sec_30_cntr >= 30)
		{
			params->data.timer.flag_30_sec = 1;
			params->data.timer.sec_30_cntr = 0;
		}

		params->data.timer.sec_60_cntr += time_diff;
		if (params->data.timer.sec_60_cntr >= 60)
		{
			params->data.timer.flag_60_sec = 1;
			params->data.timer.sec_60_cntr = 0;
		}

		params->data.timer.sec_300_cntr += time_diff;
		if (params->data.timer.sec_300_cntr >= 300)
		{
			params->data.timer.flag_300_sec = 1;
			params->data.timer.sec_300_cntr = 0;
		}

		params->data.timer.img_timeout_cntr += time_diff;
		if (params->data.timer.img_timeout_cntr >= IMG_TAKE_TIMEOUT)
		{
			params->data.timer.flag_img_timeout = 1;
			params->data.timer.img_timeout_cntr = 0;
		}
	}
}
void app_api_check(struct app_api_params *params)
{
	app_api_check_timer(params);

	if (params->data.timer.flag_1_sec)
	{
		params->data.timer.flag_1_sec = 0;
		struct tm tinf = rtc.getTimeStruct();
		sprintf(params->data.timer.date_str, "%02d%02d%02d", (tinf.tm_year - 100), tinf.tm_mon, tinf.tm_mday);
		sprintf(params->data.timer.time_str, "%02d%02d%02d", tinf.tm_hour, tinf.tm_min, tinf.tm_sec);
		params->data.curr_time = tinf.tm_hour;
		params->data.curr_time <<= 8;
		params->data.curr_time |= tinf.tm_min;
		params->data.curr_time <<= 8;
		params->data.curr_time |= tinf.tm_sec;

		Serial.printf("date:%s time:%s capt:%d send:%d\n", params->data.timer.date_str, params->data.timer.time_str, params->data.capture_active, params->data.sending_active);
		app_api_read(params);
		app_api_process(params);
		app_api_write(params);
	}

	if (params->data.timer.flag_10_sec)
	{
		params->data.timer.flag_10_sec = 0;
		app_api_check_wifi(params);
		app_api_check_mqtt(params);
	}

	if (params->data.timer.flag_30_sec)
	{
		params->data.timer.flag_30_sec = 0;
		app_api_epochtime_check(params);
	}
}

void app_api_check_wifi(struct app_api_params *params)
{
}

void app_api_check_mqtt(struct app_api_params *params)
{
}

void app_api_read(struct app_api_params *params)
{
	app_api_read_wifi(params);
	app_api_read_mqtt(params);
}

void app_api_process(struct app_api_params *params)
{
	app_api_run_apps(params);
}

void app_api_epochtime_check(struct app_api_params *params)
{
	timeClient.update();
	params->cfg.epochtime = timeClient.getEpochTime();
	if (params->cfg.epochtime > EPOCHTIME_REFERENCE)
	{
		rtc.setTime(params->cfg.epochtime, 0);
	}
}
void app_api_write(struct app_api_params *params)
{

	// if( params->data.mqtt.img_take )
	// {
	// 	if( params->cfg.mqtt.status == MQTT_STATUS_CONNECTED )
	// 	{
	// 		Serial.println("sending image to server");
	// 		params->data.mqtt.img_take = false;
	// 		app_take_photo_and_send_mqtt(params);
	// 	}
	// }

	// if( params->cfg.epochtime > EPOCHTIME_REFERENCE )
	// {
	// 	if( params->cfg.ggl.lstepochtime == 0 )
	// 	{
	// 		params->cfg.ggl.lstepochtime = params->cfg.epochtime;
	// 		app_take_photo_and_send_google(params);
	// 	}
	// 	else
	// 	{
	// 		if( (params->cfg.epochtime - params->cfg.ggl.lstepochtime) > params->cfg.ggl.timediff )
	// 		{
	// 			app_take_photo_and_send_google(params);
	// 			params->cfg.ggl.lstepochtime = params->cfg.epochtime;
	// 		}
	// 	}
	// }

	// params->cfg.epochtime = time(nullptr);
	//  Serial.printf("\nTime synchronized: %s", ctime((time_t *)&(ptr_app_params->cfg.epochtime)));

	// struct tm timeinfo;
	// //gmtime_r(&(params->cfg.epochtime), &timeinfo);
	// Serial.printf("Time: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

String app_api_getLatestFileName(struct sd_api_params *params)
{
	String latestFileName = "";
	time_t latestTime = 0;
	File root;

	if (params->init == true)
	{
		root = SD.open("/");
		if (!root)
		{
			Serial.println("Failed to open directory");
			return "";
		}
	}

	while (true)
	{
		File file = root.openNextFile();
		if (!file)
		{
			break; // No more files
		}

		String fileName = file.name();
		Serial.println("Checking file: " + fileName);

		if (!file.isDirectory())
		{
			int startIdx = fileName.indexOf('_') + 1; // Assuming "image_1700000000.jpg"
			int endIdx = fileName.indexOf('.');

			if (startIdx > 0 && endIdx > startIdx)
			{
				String timestampStr = fileName.substring(startIdx, endIdx);
				time_t fileTime = atol(timestampStr.c_str());

				if (fileTime > latestTime)
				{
					latestTime = fileTime;
					latestFileName = fileName;
				}
			}
		}
		file.close();
	}

	root.close();
	return latestFileName;
}

void app_api_run_apps(struct app_api_params *params)
{
	uint8_t capture_active=0;

	if (params->data.lst_time != params->data.curr_time)
	{
		params->data.lst_time = params->data.curr_time;
		if (params->data.curr_time > params->data.img_start_time)
		{
			if (params->data.curr_time < params->data.img_end_time)
			{
				if (params->data.timer.flag_img_timeout)
				{
					params->data.timer.flag_img_timeout = 0;
					sprintf(params->data.folder_name, "/%s", params->data.timer.date_str);
					sprintf(params->data.file_name, "%s/img_%s%s.jpg", params->data.folder_name, params->data.timer.date_str, params->data.timer.time_str);
					Serial.printf("folder_name:%s file_name:%s\n", params->data.folder_name, params->data.file_name);
					SD.mkdir(params->data.folder_name);
					app_take_photo_and_save_sd(params, params->data.file_name);
					params->data.capture_active = 1;
					params->data.sending_active = 1;
					delay(1000);
					app_take_photo_and_send_google(params);
				}
			}
			else
			{
				params->data.capture_active = 0;
			}
		}
		else
		{
			params->data.capture_active = 0;
		}

		if(params->data.capture_active == 0)
		{
			if( params->data.sending_active == 1 )
			{
				//app_read_file_and_send_google(params);		
			}
		}
	}
}

// Function to list all files and directories
void app_api_listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
	Serial.printf("Listing directory: %s\n", dirname);
	File root = fs.open(dirname);
	if (!root)
	{
		Serial.println("Failed to open directory");
		return;
	}
	if (!root.isDirectory())
	{
		Serial.println("Not a directory");
		return;
	}

	File file = root.openNextFile();
	while (file)
	{
		if (file.isDirectory())
		{
			Serial.printf("  DIR : %s\n", file.name());
			if (levels)
			{
				app_api_listDir(fs, file.name(), levels - 1);
			}
		}
		else
		{
			Serial.printf("  FILE: %s  SIZE: %d bytes\n", file.name(), file.size());
		}
		file = root.openNextFile();
	}
}

// Function to delete all files in the directory
void app_api_deleteAllFiles(fs::FS &fs, const char *dirname)
{
	Serial.printf("Deleting all files in: %s\n", dirname);
	File root = fs.open(dirname);
	if (!root)
	{
		Serial.println("Failed to open directory");
		return;
	}
	if (!root.isDirectory())
	{
		Serial.println("Not a directory");
		return;
	}

	File file = root.openNextFile();
	while (file)
	{
		if (file.isDirectory())
		{
			Serial.printf("Skipping directory: %s\n", file.name());
		}
		else
		{
			Serial.printf("Deleting file: %s\n", file.name());
			fs.remove(file.name());
		}
		file = root.openNextFile();
	}
}

void app_api_delete_last_n_files(struct sd_api_params *params, int cnt)
{
	String latestFileName = "";
	time_t latestTime = 0;
	File root;

	if (params->init == true)
	{
		root = SD.open("/");
		if (!root)
		{
			Serial.println("Failed to open directory");
		}
	}

	while (true)
	{
		cnt--;
		File file = root.openNextFile();
		if (!file)
		{
			break; // No more files
		}

		char fileName[32];
		sprintf(fileName, "/%s", file.name());
		Serial.printf("Checking deleting file: %s\n", fileName);
		file.close();
		SD.remove(fileName);
	}
	root.close();
}

void app_api_list_folders(const char *dirname, uint8_t levels)
{
	Serial.printf("Scanning directory: %s\n", dirname);

	File root = SD.open(dirname);
	if (!root)
	{
		Serial.println("Failed to open directory");
		return;
	}
	if (!root.isDirectory())
	{
		Serial.println("Not a directory");
		return;
	}

	File file = root.openNextFile();
	while (file)
	{
		if (file.isDirectory())
		{
			Serial.printf("  [DIR]  %s\n", file.name());
			// Recursively list subfolders if levels > 0
			if (levels)
			{
				app_api_list_folders(file.name(), levels - 1);
			}
		}
		file = root.openNextFile();
	}
	root.close();
}

String urlencode(String str) {
  const char *msg = str.c_str();
  const char *hex = "0123456789ABCDEF";
  String encodedMsg = "";
  while (*msg != '\0') {
    if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9') || *msg == '-' || *msg == '_' || *msg == '.' || *msg == '~') {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[(unsigned char)*msg >> 4];
      encodedMsg += hex[*msg & 0xf];
    }
    msg++;
  }
  return encodedMsg;
}


void app_read_file_and_send_google(struct app_api_params *params)
{
	char fileName[32];
	char deletepath[32];

	File root = SD.open(params->data.folder_name);
	if (!root)
	{
		Serial.println("Failed to open directory");
	}

	if (root.isDirectory())
	{
		File file = root.openNextFile();
		if (file)
		{
			
			sprintf(fileName, "%s", file.name());
			Serial.printf("sending image %s to google drive", fileName);
			unsigned int fileSize = file.size();
			Serial.printf("File size: %d bytes\n", fileSize);
			

			uint8_t *fileinput;
			char *input;
			
			fileinput = (uint8_t*)malloc(fileSize + 1);
			file.read(fileinput, fileSize);
			fileinput[fileSize] = '\0';
			file.close();

			input = (char *)fileinput;
			String imageFile = "data:image/jpeg;base64,";
			char output[base64_enc_len(3)];
			for (int i=0;i<fileSize;i++) {
				base64_encode(output, (input++), 3);
				if (i%3==0) imageFile += urlencode(String(output));
			}
			
			String Data = myFoldername+myFilename+myImage;  
			
			const char* myDomain = "script.google.com";
			String getAll="", getBody = "";

			Serial.println("Connect to " + String(myDomain));
			
			gglClient.setInsecure();
			if (gglClient.connect(myDomain, 443)) {
				Serial.println("Connection successful");

				gglClient.println("POST " + myScript + " HTTP/1.1");
				gglClient.println("Host: " + String(myDomain));
				gglClient.println("Content-Length: " + String(Data.length()+imageFile.length()));
				gglClient.println("Content-Type: application/x-www-form-urlencoded");
				gglClient.println("Connection: close");
				gglClient.println();
				
				gglClient.print(Data);
				int Index;
				for (Index = 0; Index < imageFile.length(); Index = Index+1000)
				{
					gglClient.print(imageFile.substring(Index, Index+1000));
				}
				
				int waitTime = 10000;   // timeout 10 seconds
				long startTime = millis();
				boolean state = false;
				
				while ((startTime + waitTime) > millis())
				{
					Serial.print(".");
					delay(100);      
					while (gglClient.available()) 
					{
						char c = gglClient.read();
						if (state==true) getBody += String(c);        
						if (c == '\n') 
						{
							if (getAll.length()==0) state=true; 
							getAll = "";
						} 
						else if (c != '\r')
							getAll += String(c);
						startTime = millis();
					}
					if (getBody.length()>0) break;
				}
				gglClient.stop();
				Serial.println(getBody);
			}
			else
			{
				getBody="Connected to " + String(myDomain) + " failed.";
				Serial.println("Connected to " + String(myDomain) + " failed.");
			}

			free(fileinput);  
			input[0] = '\0';

			sprintf(deletepath, "%s/%s", params->data.folder_name, fileName);
			SD.remove(deletepath);
		}
		else
		{
			params->data.sending_active = 0;
		}
	}	
}