# esp-idf-qr-code-generator
QR Code generator for esp-idf.
You can generate any QR code.   

I used [this](https://github.com/nayuki/QR-Code-generator) as components. It's GREAT WORK.   

# Installation overview
1. In this project directory, create a components directory.

2. In the components directory, clone QR-Code-generator:
```
git clone https://github.com/nayuki/QR-Code-generator

```

3. In the new QR-Code-generator directory, create a CMakeLists.txt file containing:
```
idf_component_register(SRCS "c/qrcodegen.c" INCLUDE_DIRS "c/")
```

4. Compile this project.


# Installation
```
git clone https://github.com/nopnop2002/esp-idf-qr-code-generator
cd esp-idf-qr-code-generator
mkdir -p components
cd components/
git clone https://github.com/nayuki/QR-Code-generator
echo "idf_component_register(SRCS \"c/qrcodegen.c\" INCLUDE_DIRS \"c/\")" > QR-Code-generator/CMakeLists.txt
cd ..
idf.py set-target {esp32/esp32s2/esp32s3/esp32c3}
idf.py menuconfig
idf.py flash monitor
```

# Software requiment
esp-idf ver4.4 or later.   
This is because this version supports ESP32-C3.

# Application Setting

![config-top](https://user-images.githubusercontent.com/6020549/183276602-6abd8b3b-4816-4f67-91cb-b3fc76439381.jpg)
![config-app](https://user-images.githubusercontent.com/6020549/183276605-b1372459-bf61-4712-a855-95b59223a116.jpg)

## Wifi Setting

![config-wifi](https://user-images.githubusercontent.com/6020549/183276665-607e2ca0-6caa-43b1-a431-5b4888858ea5.jpg)

You can use the mDNS hostname instead of the IP address.   

## HTTP Server Setting

![config-http](https://user-images.githubusercontent.com/6020549/183276684-676d6dcf-76c1-4b0a-b069-39bbe94fd38b.jpg)

# How to use
Open your brouser, and put address in address bar.   
You can use the mDNS hostname instead of the IP address.   
Default mDNS name is esp32.local.   
Input text and submit.   
After waiting for a while, a QR code will appear.   

![esp-idf-qr-code-generator-1](https://user-images.githubusercontent.com/6020549/183276725-faf1fe52-c380-4b26-83d4-b2e3bb31d84b.jpg)
![esp-idf-qr-code-generator-2](https://user-images.githubusercontent.com/6020549/183276723-9f9c9240-2e07-4109-aff4-3c2e1c5dd7cf.jpg)


# How to browse image data using built-in http server   
Even if there are image files in SPIFFS, the esp-idf http server does not support this:   
```
httpd_resp_sendstr_chunk(req, "<img src=\"/spiffs/picture.png\">");
```

You need to convert the image file to base64 string.   
```
httpd_resp_sendstr_chunk(req, "<img src=\"data:image/png;base64,");
httpd_resp_sendstr_chunk(req, (char *)BASE64_ENCODE_STRING);
httpd_resp_sendstr_chunk(req, "\">");
```

Images in png format are stored in the image folder.   
Images in base64 format are stored in the html folder.   
I converted using the base64 command.   
```
$ base64 image/ESP-IDF.png > html/ESP-IDF.txt
```

# Reference
https://github.com/nopnop2002/esp-idf-web-form

https://github.com/nopnop2002/esp-idf-pwm-slider

