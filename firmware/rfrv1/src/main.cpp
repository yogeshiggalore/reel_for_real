#include <Arduino.h>
#include "app_api.h"

struct app_api_params	api_params;
int err;

void setup()
{
	delay(5000);
	Serial.begin(1500000);
	Serial.printf("reelforrealv1\n");

	app_api_init(&api_params);

}

void loop()
{
	// sprintf(api_params.data.filename, "/image_tst_%d.jpg", api_params.cfg.camera.img_cnt++);
	// err = app_take_photo_and_save(&api_params, api_params.data.filename);
	// if( err == 0 )
	// {
	// 	Serial.printf("Saved picture: %s\r\n", api_params.data.filename);
	// }
	// else
	// {
	// 	Serial.printf("Save picture Error: %d\r\n", err);
	// }
	app_api_read(&api_params);
	app_api_process(&api_params);
	app_api_write(&api_params);
	delay(1000);
}
