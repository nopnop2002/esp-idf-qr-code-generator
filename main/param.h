typedef struct {
	char qrText[256]; // String to be converted
	char qrFile[64]; // BMP file to create
	TaskHandle_t taskHandle; // for xTaskNotify
} PARAMETER_t;
