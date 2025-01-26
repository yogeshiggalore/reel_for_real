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
	// if( api_params.cfg.sd.init == true )
	// {
	// 	// Serial.println("\nListing files:");
	// 	// app_api_listDir(SD, "/", 0);

	// 	// Serial.println("\nDeleting all files...");
	// 	// app_api_deleteAllFiles(SD, "/");

	// 	// Serial.println("\nFiles after deletion:");
	// 	// app_api_listDir(SD, "/", 0);
	// 	app_api_delete_last_n_files(&(api_params.cfg.sd), 10);

	// 	Serial.println("\nFiles after deletion:");
	// }
	// else
	// {
	// 	Serial.println("SD Card initialization failed.");
	// }

	if( api_params.cfg.sd.init == true )
	{
		app_api_list_folders("/", 0);
	}
}

void loop()
{
	// sprintf(api_params.data.filename, "/image_tst_%d.jpg", api_params.cfg.camera.img_cnt++);
	// err = app_take_photo_and_save_sd(&api_params, api_params.data.filename);
	// if( err == 0 )
	// {
	// 	Serial.printf("Saved picture: %s\r\n", api_params.data.filename);
	// }
	// else
	// {
	// 	Serial.printf("Save picture Error: %d\r\n", err);
	// }
	
	app_api_check(&api_params);
}
