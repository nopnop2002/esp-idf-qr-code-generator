typedef struct {
	char qrText[64]; // String to be converted
	char qrFile[64]; // BMP file to create
	TaskHandle_t taskHandle; // for xTaskNotify
} PARAMETER_t;
