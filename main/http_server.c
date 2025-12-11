/* HTTP Server Example

	 This example code is in the Public Domain (or CC0 licensed, at your option.)

	 Unless required by applicable law or agreed to in writing, this
	 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	 CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <mbedtls/base64.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs.h"
#include "esp_http_server.h"

#include "http_server.h"
#include "param.h"

static const char *TAG = "HTTP";

extern QueueHandle_t xQueueHttp;

#define STORAGE_NAMESPACE "storage"

esp_err_t save_key_value(char * key, char * value)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	// Write
	err = nvs_set_str(my_handle, key, value);
	if (err != ESP_OK) return err;

	// Commit written value.
	// After setting any values, nvs_commit() must be called to ensure changes are written
	// to flash storage. Implementations may write to storage at other times,
	// but this is not guaranteed.
	err = nvs_commit(my_handle);
	if (err != ESP_OK) return err;

	// Close
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t load_key_value(char * key, char * value, size_t size)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	// Read
	size_t _size = size;
	err = nvs_get_str(my_handle, key, value, &_size);
	ESP_LOGD(__FUNCTION__, "nvs_get_str err=%d", err);
	//if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
	if (err != ESP_OK) return err;
	ESP_LOGD(__FUNCTION__, "err=%d key=[%s] value=[%s] _size=%d", err, key, value, _size);

	// Close
	nvs_close(my_handle);
	//return ESP_OK;
	return err;
}

int find_value(char * key, char * parameter, char * value) 
{
	//char * addr1;
	char * addr1 = strstr(parameter, key);
	if (addr1 == NULL) return 0;
	ESP_LOGD(__FUNCTION__, "addr1=%s", addr1);

	char * addr2 = addr1 + strlen(key);
	ESP_LOGD(__FUNCTION__, "addr2=[%s]", addr2);

	char * addr3 = strstr(addr2, "&");
	ESP_LOGD(__FUNCTION__, "addr3=%p", addr3);
	if (addr3 == NULL) {
		strcpy(value, addr2);
	} else {
		int length = addr3-addr2;
		ESP_LOGD(__FUNCTION__, "addr2=%p addr3=%p length=%d", addr2, addr3, length);
		strncpy(value, addr2, length);
		value[length] = 0;
	}
	ESP_LOGD(__FUNCTION__, "key=[%s] value=[%s]", key, value);
	return strlen(value);
}

#if 0
static esp_err_t TextToHtml(httpd_req_t *req, char * filename) {
	ESP_LOGI(__FUNCTION__, "Reading %s", filename);
	FILE* fhtml = fopen(filename, "r");
	if (fhtml == NULL) {
		ESP_LOGE(__FUNCTION__, "fopen fail. [%s]", filename);
		return ESP_FAIL;
	} else {
		char line[128];
		while (fgets(line, sizeof(line), fhtml) != NULL) {
			size_t linelen = strlen(line);
			//remove EOL (CR or LF)
			for (int i=linelen;i>0;i--) {
				if (line[i-1] == 0x0a) {
					line[i-1] = 0;
				} else if (line[i-1] == 0x0d) {
					line[i-1] = 0;
				} else {
					break;
				}
			}
			ESP_LOGD(__FUNCTION__, "line=[%s]", line);
			esp_err_t ret = httpd_resp_sendstr_chunk(req, line);
			if (ret != ESP_OK) {
				ESP_LOGE(__FUNCTION__, "httpd_resp_sendstr_chunk fail %d", ret);
			}
		}
		fclose(fhtml);
	}
	return ESP_OK;
}
#endif

// Calculate the size after conversion to base64
// http://akabanessa.blog73.fc2.com/blog-entry-83.html
int32_t calcBase64EncodedSize(int origDataSize)
{
	// Number of blocks in 6-bit units (rounded up in 6-bit units)
	int32_t numBlocks6 = ((origDataSize * 8) + 5) / 6;
	// Number of blocks in units of 4 characters (rounded up in units of 4 characters)
	int32_t numBlocks4 = (numBlocks6 + 3) / 4;
	// Number of characters without line breaks
	int32_t numNetChars = numBlocks4 * 4;
	// Size considering line breaks every 76 characters (line breaks are "\ r \ n")
	//return numNetChars + ((numNetChars / 76) * 2);
	return numNetChars;
}

// Convert from Image to BASE64
esp_err_t ImageToBase64(char * filename, size_t fsize, unsigned char * base64_buffer, size_t base64_buffer_len)
{
	unsigned char* image_buffer = NULL;
	//image_buffer = malloc(fsize + 1);
	image_buffer = malloc(fsize);
	if (image_buffer == NULL) {
		ESP_LOGE(__FUNCTION__, "malloc fail. image_buffer %d", fsize);
		return ESP_FAIL;
	}

	FILE * fp;
	if((fp=fopen(filename,"rb"))==NULL){
		ESP_LOGE(__FUNCTION__, "fopen fail. [%s]", filename);
		return ESP_FAIL;
	}else{
		for (int i=0;i<fsize;i++) {
			fread(&image_buffer[i],sizeof(char),1,fp);
		}
		fclose(fp);
	}

	size_t encord_len;
	esp_err_t ret = mbedtls_base64_encode(base64_buffer, base64_buffer_len, &encord_len, image_buffer, fsize);
	ESP_LOGI(__FUNCTION__, "mbedtls_base64_encode=%d encord_len=%d", ret, encord_len);
	free(image_buffer);
	return ret;
}

// Convert from Image to HTML
esp_err_t ImageToHtml(httpd_req_t *req, char * localFileName, char * type) 
{
	esp_err_t ret = ESP_FAIL;
	struct stat st;
	if (stat(localFileName, &st) != 0) {
		ESP_LOGE(__FUNCTION__, "[%s] not found", localFileName);
		//httpd_resp_sendstr_chunk(req, NULL);
		return ret;
	}

	ESP_LOGI(__FUNCTION__, "%s exist st.st_size=%ld", localFileName, st.st_size);
	int32_t base64Size = calcBase64EncodedSize(st.st_size);
	ESP_LOGI(__FUNCTION__, "base64Size=%"PRIi32, base64Size);

	/* Send HTML file header */
	httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><html><body>");

	// Convert from JPEG to BASE64
	unsigned char*	img_src_buffer = NULL;
	size_t img_src_buffer_len = base64Size + 1;
	img_src_buffer = malloc(img_src_buffer_len);
	if (img_src_buffer == NULL) {
		ESP_LOGE(__FUNCTION__, "malloc fail. img_src_buffer_len %d", img_src_buffer_len);
		return ret;
	}

	ret = ImageToBase64(localFileName, st.st_size, img_src_buffer, img_src_buffer_len);
	ESP_LOGI(__FUNCTION__, "Image2Base64=%d", ret);
	if (ret != 0) {
		ESP_LOGE(__FUNCTION__, "Error in mbedtls encode! ret = -0x%x", -ret);
	} else {
		// <img src="data:image/jpeg;base64,ENCORDED_DATA" />
		char buffer[128];
		sprintf(buffer, "<img src=\"data:image/%s;base64,", type);
		ESP_LOGI(__FUNCTION__, "buffer=[%s]", buffer);
		//httpd_resp_sendstr_chunk(req, "<img src=\"data:image/jpeg;base64,");
		httpd_resp_sendstr_chunk(req, buffer);
		httpd_resp_send_chunk(req, (char *)img_src_buffer, base64Size);
		httpd_resp_sendstr_chunk(req, "\" />");
	}
	if (img_src_buffer != NULL) free(img_src_buffer);
	return ret;
}

// Convert from Base64 to HTML
esp_err_t Base64ToHtml(httpd_req_t *req, char * filename, char * type)
{
	FILE * fhtml = fopen(filename, "r");
	if (fhtml == NULL) {
		ESP_LOGE(__FUNCTION__, "fopen fail. [%s]", filename);
		return ESP_FAIL;
	}else{
		char buffer[128];
		sprintf(buffer, "<img src=\"data:image/%s;base64,", type);
		ESP_LOGI(__FUNCTION__, "buffer=[%s]", buffer);
		httpd_resp_sendstr_chunk(req, buffer);

		while(1) {
			size_t bufferSize = fread(buffer, 1, sizeof(buffer), fhtml);
			ESP_LOGD(__FUNCTION__, "bufferSize=%d", bufferSize);
			if (bufferSize > 0) {
				httpd_resp_send_chunk(req, buffer, bufferSize);
			} else {
				break;
			}
		}
		fclose(fhtml);
		httpd_resp_sendstr_chunk(req, "\">");
	}
	return ESP_OK;
}

// from https://stackoverflow.com/questions/2673207/c-c-url-decode-library/2766963
void urldecode2(char *dst, const char *src)
{
	char a, b;
	while (*src) {
		if ((*src == '%') &&
			((a = src[1]) && (b = src[2])) &&
			(isxdigit(a) && isxdigit(b))) {
				if (a >= 'a')
					a -= 'a'-'A';
				if (a >= 'A')
					a -= ('A' - 10);
				else
					a -= '0';
				if (b >= 'a')
					b -= 'a'-'A';
				if (b >= 'A')
					b -= ('A' - 10);
				else
					b -= '0';
				*dst++ = 16*a+b;
				src+=3;
		} else if (*src == '+') {
			*dst++ = ' ';
			src++;
		} else {
			*dst++ = *src++;
		}
	}
	*dst++ = '\0';
}
/* HTTP get handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s]", req->uri);
	char* base_path = (char *)req->user_ctx;
	ESP_LOGI(__FUNCTION__, "base_path=[%s]", base_path);
	char key[64];
	char parameter[256];
	esp_err_t err;

	// Send HTML header
	httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><html>");
	httpd_resp_sendstr_chunk(req, "<head><meta charset=\"utf-8\"></head>");
	//TextToHtml(req, "/html/head.html");

	httpd_resp_sendstr_chunk(req, "<body>");
	httpd_resp_sendstr_chunk(req, "<h1>QR Code Generator using ESP-IDF</h1>");

	strcpy(key, "bmpText");
	err = load_key_value(key, parameter, sizeof(parameter));
	ESP_LOGI(__FUNCTION__, "load_key_value %s=%d", key, err);
	assert (err == ESP_OK);
	ESP_LOGI(__FUNCTION__, "parameter=%d [%s]", strlen(parameter), parameter);

	httpd_resp_sendstr_chunk(req, "<h2>input text</h2>");
	httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/post\">");
	httpd_resp_sendstr_chunk(req, "<input type=\"text\" name=\"text\" size=\"80\" value=\"");
	if (strlen(parameter)) httpd_resp_sendstr_chunk(req, parameter);
	httpd_resp_sendstr_chunk(req, "\">");
	httpd_resp_sendstr_chunk(req, "<br>");
	httpd_resp_sendstr_chunk(req, "<input type=\"submit\" name=\"submit\" value=\"submit\">");

#if 0
	httpd_resp_sendstr_chunk(req, key);
	httpd_resp_sendstr_chunk(req, ":");
	httpd_resp_sendstr_chunk(req, parameter);
#endif

	httpd_resp_sendstr_chunk(req, "</form><br>");

	strcpy(key, "bmpError");
	err = load_key_value(key, parameter, sizeof(parameter));
	ESP_LOGI(__FUNCTION__, "load_key_value %s=%d", key, err);
	assert (err == ESP_OK);
	ESP_LOGI(__FUNCTION__, "parameter=%d [%s]", strlen(parameter), parameter);
	if (strlen(parameter) > 0) {
		httpd_resp_sendstr_chunk(req, "<br>");
		httpd_resp_sendstr_chunk(req, parameter);
		httpd_resp_sendstr_chunk(req, "<br>");
	}

	strcpy(key, "bmpFile");
	err = load_key_value(key, parameter, sizeof(parameter));
	ESP_LOGI(__FUNCTION__, "load_key_value %s=%d", key, err);
	assert (err == ESP_OK);
	ESP_LOGI(__FUNCTION__, "parameter=%d [%s]", strlen(parameter), parameter);
	if (strlen(parameter) > 0) {
		ImageToHtml(req, parameter, "bmp");
	}

#if 0
	/* Convert from Base64 to HTML */
	Base64ToHtml(req, "/html/ESP-LOGO.txt", "png");

	/* Convert from Image to HTML */
	char imageFileName[64];
	strcpy(imageFileName, "/html/ESP-LOGO.png");
	ImageToHtml(req, imageFileName, "png");
#endif

	/* Send remaining chunk of HTML file to complete it */
	httpd_resp_sendstr_chunk(req, "</body></html>");

	/* Send empty chunk to signal HTTP response completion */
	httpd_resp_sendstr_chunk(req, NULL);

	return ESP_OK;
}

void qrcode(void *pvParameters);

/* HTTP post handler */
static esp_err_t root_post_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "req->uri=[%s]", req->uri);
	ESP_LOGI(__FUNCTION__, "content length %d", req->content_len);
	char* base_path = (char *)req->user_ctx;
	ESP_LOGI(__FUNCTION__, "base_path=[%s]", base_path);
	char* buf = malloc(req->content_len + 1);
	size_t off = 0;
	while (off < req->content_len) {
		/* Read data received in the request */
		int ret = httpd_req_recv(req, buf + off, req->content_len - off);
		if (ret <= 0) {
			if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
				httpd_resp_send_408(req);
			}
			free (buf);
			return ESP_FAIL;
		}
		off += ret;
		ESP_LOGI(__FUNCTION__, "root_post_handler recv length %d", ret);
	}
	buf[off] = '\0';
	ESP_LOGI(__FUNCTION__, "buf=[%s]", buf);

	char *_buf = malloc(strlen(buf)+1);
	urldecode2(_buf, buf);
	ESP_LOGI(__FUNCTION__, "_buf=[%s] len=%d", _buf, strlen(_buf));
	free(buf);

	/* Create QR Generate Task */
	PARAMETER_t param;
	char *find = strstr(_buf,"submit=");
	int pos = find - _buf;
	ESP_LOGI(__FUNCTION__, "pos=%d", pos);
	esp_err_t err;
	if (pos < 256) {
		memset(param.qrText, 0, sizeof(param.qrText));
		strncpy(param.qrText, &_buf[5], pos-6);
		ESP_LOGI(__FUNCTION__, "param.qrText=[%s]", param.qrText);
	
		sprintf(param.qrFile, "%s/qrcode.bmp", base_path);
		param.taskHandle = xTaskGetCurrentTaskHandle();
		xTaskCreate(&qrcode, "QRCODE", 1024*10, (void *)&param, 2, NULL);
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		ESP_LOGI(__FUNCTION__, "ulTaskNotifyTake");
	
		// save key & value to NVS
		err = save_key_value("bmpText", param.qrText);
		if (err != ESP_OK) {
			ESP_LOGE(__FUNCTION__, "Error (%s) saving to NVS", esp_err_to_name(err));
		}

		err = save_key_value("bmpFile", param.qrFile);
		if (err != ESP_OK) {
			ESP_LOGE(__FUNCTION__, "Error (%s) saving to NVS", esp_err_to_name(err));
		}

		err = save_key_value("bmpError", "");
		if (err != ESP_OK) {
			ESP_LOGE(__FUNCTION__, "Error (%s) saving to NVS", esp_err_to_name(err));
		}
	} else {
		err = save_key_value("bmpFile", "");
		if (err != ESP_OK) {
			ESP_LOGE(__FUNCTION__, "Error (%s) saving to NVS", esp_err_to_name(err));
		}

		err = save_key_value("bmpError", "input text is too long");
		if (err != ESP_OK) {
			ESP_LOGE(__FUNCTION__, "Error (%s) saving to NVS", esp_err_to_name(err));
		}
		ESP_LOGE(__FUNCTION__, "input text is too long");
	}
	free(_buf);


	/* Redirect onto root to see the updated file list */
	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/");
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
	httpd_resp_set_hdr(req, "Connection", "close");
#endif
	httpd_resp_sendstr(req, "post successfully");
	return ESP_OK;
}

/* favicon get handler */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
	ESP_LOGI(__FUNCTION__, "favicon_get_handler req->uri=[%s]", req->uri);
	return ESP_OK;
}

/* Function to start the web server */
esp_err_t start_server(char *base_path, int port)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = port;

	/* Use the URI wildcard matching function in order to
	 * allow the same handler to respond to multiple different
	 * target URIs which match the wildcard scheme */
	config.uri_match_fn = httpd_uri_match_wildcard;

	ESP_LOGI(__FUNCTION__, "Starting HTTP Server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) != ESP_OK) {
		ESP_LOGE(__FUNCTION__, "Failed to start file server!");
		return ESP_FAIL;
	}

	/* URI handler for get */
	httpd_uri_t _root_get_handler = {
		.uri		 = "/",
		.method		 = HTTP_GET,
		.handler	 = root_get_handler,
		.user_ctx	 = base_path // Pass server data as context
	};
	httpd_register_uri_handler(server, &_root_get_handler);

	/* URI handler for post */
	httpd_uri_t _root_post_handler = {
		.uri		 = "/post",
		.method		 = HTTP_POST,
		.handler	 = root_post_handler,
		.user_ctx	 = base_path // Pass server data as context
	};
	httpd_register_uri_handler(server, &_root_post_handler);

	/* URI handler for favicon.ico */
	httpd_uri_t _favicon_get_handler = {
		.uri		 = "/favicon.ico",
		.method		 = HTTP_GET,
		.handler	 = favicon_get_handler,
		//.user_ctx  = server_data	// Pass server data as context
	};
	httpd_register_uri_handler(server, &_favicon_get_handler);

	return ESP_OK;
}


void http_server_task(void *pvParameters)
{
	char *task_parameter = (char *)pvParameters;
	ESP_LOGI(TAG, "Start task_parameter=%s", task_parameter);
	char url[64];
	sprintf(url, "http://%s:%d", task_parameter, CONFIG_WEB_PORT);

	esp_err_t err;
	err = save_key_value("bmpText", "https://github.com/nopnop2002/");
	if (err != ESP_OK) {
		ESP_LOGE(__FUNCTION__, "Error (%s) saving to NVS", esp_err_to_name(err));
	}

	err = save_key_value("bmpFile", "");
	if (err != ESP_OK) {
		ESP_LOGE(__FUNCTION__, "Error (%s) saving to NVS", esp_err_to_name(err));
	}

	err = save_key_value("bmpError", "");
	if (err != ESP_OK) {
		ESP_LOGE(__FUNCTION__, "Error (%s) saving to NVS", esp_err_to_name(err));
	}

	// Start Server
	ESP_LOGI(TAG, "Starting server on %s", url);
	ESP_ERROR_CHECK(start_server("/html", CONFIG_WEB_PORT));
	
	while(1) {
		vTaskDelay(1);
	}

	// Never reach here
	ESP_LOGI(TAG, "finish");
	vTaskDelete(NULL);
}
