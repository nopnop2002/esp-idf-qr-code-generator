/*
	QR Code generator for esp-idf

	This code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "cmd.h"
#include "param.h"
#include "bmpfile.h"
#include "qrcodegen.h"

static const char *TAG = "QRCODE";

// Prints the given QR Code to the console.
static void printQr(const uint8_t qrcode[]) {
	int size = qrcodegen_getSize(qrcode);
	ESP_LOGI(TAG, "size=%d", size);
	int border = 4;
	for (int y = -border; y < size + border; y++) {
		for (int x = -border; x < size + border; x++) {
			fputs((qrcodegen_getModule(qrcode, x, y) ? "##" : "  "), stdout);
		}
		fputs("\n", stdout);
	}
	fputs("\n", stdout);
}

static esp_err_t makeQr(char * file, const uint8_t qrcode[]) {
	int size = qrcodegen_getSize(qrcode);
	ESP_LOGI(__FUNCTION__, "size=%d", size);
	int border = 4;

	uint32_t imageSize = (size+border*2) * 8;
	ESP_LOGI(__FUNCTION__, "imageSize=%"PRIu32, imageSize);
	uint32_t rowSize =((imageSize + 31 ) / 32 ) * 4;
	ESP_LOGI(__FUNCTION__, "rowSize=%"PRIu32, rowSize);
	
	// open output file
	esp_err_t ret;
	FILE* fp = fopen(file, "wb");
	if (fp == NULL) {
		ESP_LOGE(__FUNCTION__, "File open fail [%s]", file);
		return ESP_FAIL;
	}

	uint32_t bmp_header_sz = 14;
	//uint32_t bmp_dib_v3_header_sz = 40;
	uint32_t bmp_dib_v4_header_sz = 108;
	uint32_t bmp_color_pallet_sz = 8;
	bmpfile_t *result = (bmpfile_t*)malloc(sizeof(bmpfile_t));
	if (result == NULL) {
		ESP_LOGE(__FUNCTION__, "result allocate error");
		fclose(fp);
		return ESP_FAIL;
	}
	result->header.magic[0]=0x42; // B
	result->header.magic[1]=0x4D; // M 
	result->header.creator1=0x0;
	result->header.creator2=0x0;
	result->header.offset=bmp_header_sz + bmp_dib_v4_header_sz + bmp_color_pallet_sz;
	result->dib.header_sz=bmp_dib_v4_header_sz;
	result->dib.width=imageSize;
	result->dib.height=imageSize;
	result->dib.nplanes=1;
	result->dib.depth=1;
	result->dib.compress_type=0;
	result->dib.bmp_bytesz=rowSize * result->dib.height;
	ESP_LOGI(__FUNCTION__, "result->dib.bmp_bytesz=%"PRIu32, result->dib.bmp_bytesz);

	result->dib.hres=0xec3;
	result->dib.vres=0xec3;
	result->dib.ncolors=2;
	result->dib.nimpcolors=2;
	result->dib.rmask=0xff0000;
	result->dib.gmask=0xff00;
	result->dib.bmask=0xff;
	result->dib.amask=0xff000000;
	result->dib.cstype=0x73524742;
	memset(result->dib.space, 0x00, sizeof(result->dib.space));
	result->dib.rgamma=0x40000000;
	result->dib.ggamma=0x00;
	result->dib.bgamma=0x00;
	result->header.filesz=bmp_header_sz+bmp_dib_v4_header_sz+result->dib.bmp_bytesz;
	ESP_LOGI(__FUNCTION__, "result->header.filesz=%"PRIu32, result->header.filesz);
	

	// write bitmap file header
	ret = fwrite(&result->header, 1, sizeof(bmp_header_t), fp);
	ESP_LOGI(__FUNCTION__, "fwrite ret=%d", ret);
	assert(ret == sizeof(bmp_header_t));

	// write bitmap information header(BITMAPV4HEADER)
	ret = fwrite(&result->dib, 1, sizeof(bmp_dib_v4_header_t), fp);
	ESP_LOGI(__FUNCTION__, "fwrite ret=%d", ret);
	assert(ret == sizeof(bmp_dib_v4_header_t));

	// write color pallet 
	uint8_t pallet[8] = {0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00};
	ret = fwrite(&pallet, 1, sizeof(pallet), fp);
	ESP_LOGI(__FUNCTION__, "fwrite ret=%d", ret);
	assert(ret == sizeof(pallet));

	// write pixel data
	uint8_t *sdbuffer = (uint8_t*)malloc(rowSize); // pixel buffer
	if (sdbuffer == NULL) {
		ESP_LOGE(__FUNCTION__, "sdbuffer allocate error");
		free(result);
		fclose(fp);
		return ESP_FAIL;
	}
	memset(sdbuffer, 0xff, rowSize);
	for (int row=0; row<result->dib.height; row++) { // For each scanline...
		int pos = result->header.offset + row * rowSize;
		ESP_LOGD(__FUNCTION__, "pos=%d row=%d", pos, row);
		ret = fwrite(sdbuffer, 1, rowSize, fp);
		assert(ret == rowSize);
	}

	int row = 0;
/*
	for (int row=0; row<result->dib.height; row++) { // For each scanline...
		int pos = result->header.offset + (result->dib.height - 1 - row) * rowSize;
		ESP_LOGI(__FUNCTION__, "pos=%d row=%d", pos, result->dib.height - 1 - row);
*/

	// write qr code pixel
	for (int y = -border; y < size + border; y++) {
		int index = 0;
		for (int x = -border; x < size + border; x++) {
			sdbuffer[index] = 0xff;
			if (qrcodegen_getModule(qrcode, x, y)) sdbuffer[index]=0x0;
			index++;
		}
		// write the same value to 8 rows
		for (int _row=0;_row<8;_row++) {
			int pos = result->header.offset + (result->dib.height - 1 - row) * rowSize;
			ESP_LOGD(__FUNCTION__, "pos=%d row=%"PRIu32" y=%d size + border=%d", pos, result->dib.height - 1 - row, y, size + border);
			fseek(fp, pos, SEEK_SET);
			ret = fwrite(sdbuffer, 1, rowSize, fp);
			assert(ret == rowSize);
			row++;
		}
	}
	free(sdbuffer);
	free(result);
	fclose(fp);
	return ESP_OK;
}

static void showBMP(char * file) {
	esp_err_t ret;
	FILE* fp = fopen(file, "rb");
	if (fp == NULL) {
		ESP_LOGW(__FUNCTION__, "File not found [%s]", file);
		return;
	}

	// read bmp header
	bmpfile_t *result = (bmpfile_t*)malloc(sizeof(bmpfile_t));
	ret = fread(result->header.magic, 1, 2, fp);
	assert(ret == 2);
	ESP_LOGI(__FUNCTION__, "result->header.magicu=%x %x", result->header.magic[0], result->header.magic[1]);

	if (result->header.magic[0]!='B' || result->header.magic[1] != 'M') {
		ESP_LOGW(__FUNCTION__, "File is not BMP");
		free(result);
		fclose(fp);
		return;
	}
	ret = fread(&result->header.filesz, 4, 1 , fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->header.filesz=%"PRIu32, result->header.filesz);
	ret = fread(&result->header.creator1, 2, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->header.creator1=0x%x", result->header.creator1);
	ret = fread(&result->header.creator2, 2, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->header.creator2=0x%x", result->header.creator2);
	ret = fread(&result->header.offset, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->header.offset=%"PRIu32, result->header.offset);

	// read dib header
	ret = fread(&result->dib.header_sz, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.header_sz=%"PRIu32, result->dib.header_sz);
	ret = fread(&result->dib.width, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.width=%"PRIu32, result->dib.width);
	ret = fread(&result->dib.height, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.height=%"PRIu32, result->dib.height);
	ret = fread(&result->dib.nplanes, 2, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.nplanes=%d", result->dib.nplanes);
	ret = fread(&result->dib.depth, 2, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.depth=%d", result->dib.depth);
	ret = fread(&result->dib.compress_type, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.compress_type=%"PRIu32, result->dib.compress_type);
	ret = fread(&result->dib.bmp_bytesz, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.bmp_bytesz=%"PRIu32, result->dib.bmp_bytesz);
	ret = fread(&result->dib.hres, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.hres=%"PRIu32, result->dib.hres);
	ret = fread(&result->dib.vres, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.vres=%"PRIu32, result->dib.vres);
	ret = fread(&result->dib.ncolors, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.ncolors=%"PRIu32, result->dib.ncolors);
	ret = fread(&result->dib.nimpcolors, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGI(__FUNCTION__, "result->dib.nimpcolors=%"PRIu32, result->dib.nimpcolors);

	uint32_t rowSize = result->dib.bmp_bytesz / result->dib.height;
	ESP_LOGI(__FUNCTION__ ,"rowSize=%"PRIi32, rowSize);

	for (int row=0; row<result->dib.height; row++) { // For each scanline...
		int pos = result->header.offset + (result->dib.height - 1 - row) * rowSize;
		ESP_LOGI(__FUNCTION__, "pos=%d row=%"PRIi32, pos, result->dib.height - 1 - row);
	}

	free(result);
	fclose(fp);

	return;
}

void qrcode(void *pvParameters)
{
	PARAMETER_t *task_parameter = pvParameters;
	PARAMETER_t param;
	memcpy((char *)&param, task_parameter, sizeof(PARAMETER_t));
	//TaskHandle_t taskHandle = (TaskHandle_t)pvParameters;
	//TaskHandle_t taskHandle = param.taskHandle;
	ESP_LOGI(pcTaskGetName(0), "Start");
	ESP_LOGI(pcTaskGetName(0), "param.qrText=[%s]", param.qrText);
	ESP_LOGI(pcTaskGetName(0), "param.qrFile=[%s]", param.qrFile);
	//const char *text = "Hello, world!"; // User-supplied text
	//enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW; // Error correction level
	enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_HIGH; // Error correction level
	
	// Make and print the QR Code symbol
	uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
	//bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl,
	bool ok = qrcodegen_encodeText(param.qrText, tempBuffer, qrcode, errCorLvl,
		qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

	if (ok) {
		//printQr(qrcode);
		//char newFileName[64];
		//strcpy(newFileName, "/spiffs/qrcode.bmp");
		//esp_err_t ret = makeQr(newFileName, qrcode);
		esp_err_t ret = makeQr(param.qrFile, qrcode);
		ESP_LOGI(pcTaskGetName(0), "makeQr=%d", ret);
	} else {
		ESP_LOGE(pcTaskGetName(0), "qrcodegen_encodeText fail");
		while(1) { vTaskDelay(1); }
	}
	//xTaskNotifyGive( taskHandle );
	xTaskNotifyGive( param.taskHandle );
	ESP_LOGI(pcTaskGetName(0), "End");
	vTaskDelete( NULL );
}
