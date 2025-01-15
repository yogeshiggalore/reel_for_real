#ifndef APP_API_H
#define APP_API_H

#include <Arduino.h>
#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "WiFi.h"

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

#define LED_GPIO_NUM      21

#define EPOCHTIME_REFERENCE	1735669801

enum camera_err
{
	CAMERA_ERR_NONE=0,
	CAMERA_ERR_INIT,
	CAMERA_ERR_FRAME_BUF,
	CAMERA_ERR_CAPTURE,

};

enum sd_err
{
	SD_ERR_NONE=0,
	SD_ERR_INIT,
	SD_ERR_MOUNT,
	SD_ERR_TYPE,
	SD_ERR_FILE_OPEN,
	SD_ERR_FILE_WRITE,
};
enum wifi_err
{
	WIFI_ERR_NONE=0,
	WIFI_ERR_INIT,
	WIFI_ERR_MAC_READ,
	WIFI_ERR_CONNECT,	
};

enum mqtt_err
{
	MQTT_ERR_NONE=0,
	MQTT_ERR_INIT,
	MQTT_ERR_CONNECT,	
};

enum camera_status
{
	CAMERA_STATUS_NONE=0,
	CAMERA_STATUS_INIT_SUCCESS,
	CAMERA_STATUS_PHOTO_SUCCESS,
};

enum sd_status
{
	SD_STATUS_NONE=0,
	SD_STATUS_INIT_SUCCESS,
	SD_STATUS_WRITE_SUCCESS,
};

enum wifi_status
{
	WIFI_STATUS_NONE=0,
	WIFI_STATUS_INIT_SUCCESS,
	WIFI_STATUS_CONNECTED,
	WIFI_STATUS_DISCONNECTED,
};

enum mqtt_status
{
	MQTT_STATUS_NONE=0,
	MQTT_STATUS_INIT_SUCCESS,
	MQTT_STATUS_CONNECTED,
	MQTT_STATUS_DISCONNECTED,
};


struct camera_api_params
{
	uint8_t err;
	uint8_t init;
	uint8_t status;
	camera_config_t cfg;
	uint32_t img_cnt;
};

struct sd_api_params
{
	uint8_t err;
	uint8_t init;
	uint8_t status;
	uint8_t cardType;
};

struct wifi_api_params
{
	uint8_t err;
	uint8_t init;
	WiFiEvent_t event;
	uint8_t status;
	uint8_t check_cnt;
	char ssid[32];
	uint8_t len_ssid;
	char password[32];
	uint8_t len_password;
	IPAddress local_ip;
	uint8_t mac[6];
};

struct mqtt_api_params
{
	uint8_t err;
	uint8_t init;
	uint8_t status;
	uint16_t port;
	uint16_t keepalive;
	char server[32];
	char client_id[32];
	char user_name[32];
	char password[40];
	char cfg_get_topic[32];
	char cfg_set_topic[32];
	char will_topic[32];
};

struct app_api_config
{
	unsigned long epochtime;
	struct camera_api_params camera;
	struct sd_api_params sd;
	struct wifi_api_params wifi;
	struct mqtt_api_params mqtt;
};

struct app_api_data
{
	uint8_t test;
	char filename[32];
};

struct app_api_params
{
	struct app_api_config cfg;
	struct app_api_data data;
};

void app_api_init(struct app_api_params *params);
void app_api_init_camera(struct camera_api_params *params);
void app_api_init_sd(struct sd_api_params *params);
void app_api_init_wifi(struct wifi_api_params *params);
void app_api_wifi_start(struct wifi_api_params *params);
void app_api_mqtt_start(struct mqtt_api_params *params);

void app_api_check(struct app_api_params *params);
void app_api_check_wifi(struct app_api_params *params);
void app_api_check_mqtt(struct app_api_params *params);

void app_api_read(struct app_api_params *params);
void app_api_read_wifi(struct app_api_params *params);
void app_api_read_mqtt(struct app_api_params *params);

void app_api_process(struct app_api_params *params);
void app_api_write(struct app_api_params *params);


int app_take_photo_and_save(struct app_api_params *params, const char *fileName);
void app_write_to_file(struct app_api_params *params, fs::FS &fs, const char *path, uint8_t *data, size_t len);
void app_api_mqtt_callback(char* topic, byte* message, unsigned int length);

#endif