# esp-idf-qr-code-generator
QR Code generator for esp-idf.   
You can generate any QR code.   

This project use [this](https://github.com/nayuki/QR-Code-generator) as components. It's a GREAT job.   

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

# Software requiment
ESP-IDF V4.4/V5.0.   
ESP-IDF V5.0 is required when using ESP32-C2.   

# Installation
```
git clone https://github.com/nopnop2002/esp-idf-qr-code-generator
cd esp-idf-qr-code-generator
mkdir -p components
cd components/
git clone https://github.com/nayuki/QR-Code-generator
echo "idf_component_register(SRCS \"c/qrcodegen.c\" INCLUDE_DIRS \"c/\")" > QR-Code-generator/CMakeLists.txt
cd ..
idf.py set-target {esp32/esp32s2/esp32s3/esp32c2/esp32c3/esp32c6}
idf.py menuconfig
idf.py flash monitor
```

# Application Setting

![config-top](https://user-images.githubusercontent.com/6020549/183276602-6abd8b3b-4816-4f67-91cb-b3fc76439381.jpg)
![config-app](https://user-images.githubusercontent.com/6020549/183276605-b1372459-bf61-4712-a855-95b59223a116.jpg)

## Wifi Setting

![config-wifi](https://user-images.githubusercontent.com/6020549/183276665-607e2ca0-6caa-43b1-a431-5b4888858ea5.jpg)

You can use the mDNS hostname instead of the IP address.   

## HTTP Server Setting

![config-http](https://user-images.githubusercontent.com/6020549/183277176-2ed5a850-d339-43d4-835f-a00ec021e661.jpg)

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
httpd_resp_sendstr_chunk(req, "<img src=\"/spiffs/qr-code.bmp\">");
```

You need to convert the image file to base64 string.   
```
httpd_resp_sendstr_chunk(req, "<img src=\"data:image/bmp;base64,");
httpd_resp_sendstr_chunk(req, (char *)BASE64_ENCODE_STRING);
httpd_resp_sendstr_chunk(req, "\">");
```


# Reference
https://github.com/nopnop2002/esp-idf-web-form

https://github.com/nopnop2002/esp-idf-pwm-slider

