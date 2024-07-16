#include <Arduino.h>
#include <WiFi.h>
#include <lighting.h>




void connectWifi(String ssid, String password){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Network connected");

}

void setup() {
  Serial.begin(9600);
  connectWifi("SSID", "PASSWORD");
  String url = "http://IP/app";
  Serial.println("Initalising Lighting");
  lighting light = lighting(url);


  LightInfo response = light.getLightInfo();
  LightResult response2 = light.toggleLight(!response.device_on);
  Serial.println(response2.response);
 
}

void loop() {
  // put your main code here, to run repeatedly:
}

