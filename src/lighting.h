#ifndef LIGHTING_H
#define LIGHTING_H

#include <Arduino.h>
#include <SHA256.h>
#include <Crypto.h>
#include <vector>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <AES.h>
#include <string.h>
#include <ArduinoJson.h>
#include <WiFi.h>

struct LightResult {
    bool success;
    String response;
};

struct LightInfo {
    bool device_on;
    int  brightness;
    int  hue;
    int  saturation;
    int  color_temp;
};

class lighting {
    private:
        String url;
        String cookie;
        std::vector<uint8_t> creds;
        std::vector<uint8_t> localHash;
        std::vector<uint8_t> key;
        std::vector<uint8_t> sig;
        std::vector<uint8_t> iv;
        uint32_t seq;

        std::vector<uint8_t> loadFile(String filePath);
        std::vector<uint8_t> SHA256Hash(const std::vector<uint8_t>& data);
        boolean checkResponse(int responseCode);
        std::vector<uint8_t> genLocalSeed();
        boolean handShake();
        void encryptionSetup();
        std::vector<uint8_t> seq_to_bytes();
        std::vector<uint8_t> iv_seq();
        std::vector<uint8_t> pkcs7_pad(const std::vector<uint8_t>& data, size_t block_size);
        void xorBlock(uint8_t* dst, const uint8_t* src, size_t length);
        std::vector<uint8_t> encrypt(const String& data);
        String decrypt(const std::vector<uint8_t>& encryptedData);

        LightResult executeRequest(String data);

    public:
        lighting(String url);
        lighting(String url , std::vector<uint8_t> creds);
        LightResult toggleLight(boolean lightState);
        LightInfo getLightInfo();
};

#endif // LIGHTING_H
