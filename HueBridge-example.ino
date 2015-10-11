/*------------------------------------------------------------------------------
Example Sketch for Phillips Hue

This lib requires you to have the neopixelbus lib in your arduino libs folder,
for the colour management header files <RgbColor.h> & <HslColor.h>.
  https://github.com/Makuna/NeoPixelBus/tree/UartDriven 
***  Needs to be UARTDRIVEN branch, or Animator Branch ***


------------------------------------------------------------------------------*/




#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>

#include <ESP8266SSDP.h>
#include <NeoPixelBus.h>
#include <ArduinoJson.h>

#include "HueBridge.h"
#include <NeoPixelBus.h> // NeoPixelAnimator branch


const char* host = "Hue-Bridge";
const char* ssid = "SKY";
const char* pass = "wellcometrust";
const uint16_t aport = 8266;

WiFiServer TelnetServer(aport);
WiFiClient Telnet;
WiFiUDP OTA;

ESP8266WebServer HTTP(80);

//HueBridge* Hue = NULL; // used for dynamic memory allocation... 

 #define pixelCount 7 // Strip has 30 NeoPixels
 #define pixelPin 2 // Strip is attached to GPIO2 on ESP-01
 #define HUEgroups 3
 #define HUElights 10

 HueBridge Hue(&HTTP, HUElights, HUEgroups, &HandleHue);

 NeoPixelBus strip = NeoPixelBus(pixelCount, pixelPin);

 NeoPixelAnimator animator(&strip); // NeoPixel animation management object


void HandleHue(uint8_t Lightno, struct RgbColor rgb, HueLight* lightdata) {

// Hue callback....  

 // Serial.printf( "\n | Light = %u, Name = %s (%u,%u,%u) | ", Lightno, lightdata->Name, lightdata->Hue, lightdata->Sat, lightdata->Bri); 
          //temporarily holds data from vals
          char x[10];                
          char y[10];
          //4 is mininum width, 3 is precision; float value is copied onto buff
          dtostrf(lightdata->xy.x, 5, 4, x);
          dtostrf(lightdata->xy.y, 5, 4, y);


  Serial.printf( " | light = %3u, %15s , RGB(%3u,%3u,%3u), HSB(%5u,%3u,%3u), XY(%s,%s) | \n", 
      Lightno, lightdata->Name,  rgb.R, rgb.G, rgb.B, lightdata->Hue, lightdata->Sat, lightdata->Bri, x, y); 
 
        Lightno--;


         RgbColor original = strip.GetPixelColor(Lightno);
        
        AnimUpdateCallback animUpdate = [=](float progress)
        {
            RgbColor updatedColor = RgbColor::LinearBlend(original, rgb, progress);
            strip.SetPixelColor(Lightno, updatedColor);
        };

        animator.StartAnimation(Lightno, 1000, animUpdate);


}



// void HandleHue(uint8_t Light, uint16_t Time, RgbColor rgb) {

// // Hue callback....  

//   //Serial.printf( " | Callback-> Light = %u, Time = %u (%u,%u,%u) | ", Light, Time, rgb.R, rgb.G, rgb.B); 
  
//         Light--;

//          RgbColor original = strip.GetPixelColor(Light);
        
//         AnimUpdateCallback animUpdate = [=](float progress)
//         {
//             RgbColor updatedColor = RgbColor::LinearBlend(original, rgb, progress);
//             strip.SetPixelColor(Light, updatedColor);
//         };
//         animator.StartAnimation(Light, Time, animUpdate);


// }




void setup() {
  strip.Begin();
  strip.Show();

  Serial.begin(115200);
  
  delay(500);

  Serial.println("");
  Serial.println("Arduino HueBridge Test");

  Serial.printf("Sketch size: %u\n", ESP.getSketchSize());
  Serial.printf("Free size: %u\n", ESP.getFreeSketchSpace());


  //WiFi.begin(ssid, pass);

  //   if (WiFi.waitForConnectResult() != WL_CONNECTED) {
  //   Serial.println("WiFi Failed");
  //   infoLight(red);
  //   delay(100);
  //   ESP.reset();
  // }

  //Serial.print("\n\n");

      uint8_t i = 0;
      
      while (WiFi.status() != WL_CONNECTED ) {
        delay(500);
        i++;
        Serial.print(".");
        if (i == 20) { 
          Serial.print("Failed"); 
          ESP.reset(); 
          break; 
        } ;
      }


  if(WiFi.waitForConnectResult() == WL_CONNECTED){


    MDNS.begin(host);
    MDNS.addService("arduino", "tcp", aport);
    OTA.begin(aport);
    TelnetServer.begin();
    TelnetServer.setNoDelay(true);
    delay(10);
    Serial.print("IP address: ");
    //String IP = String(WiFi.localIP()); 
    //Serial.println(IP);
    Serial.println(WiFi.localIP());

  //Hue = new HueBridge(&HTTP, 10, 3, &HandleHue); 

    
      HTTP.on ( "/test", []() {
        Serial.println("TEST PAGE HIT");
    HTTP.send ( 200, "text/plain", "this works as well" );
      } );


    Hue.Begin();

    HTTP.begin();

  } else {
    Serial.println("CONFIG FAILED... rebooting");
    ESP.reset();
  }


}


void loop() {

  HTTP.handleClient();
  yield(); 

  OTA_handle();

  static unsigned long update_strip_time = 0;  //  keeps track of pixel refresh rate... limits updates to 33 Hz
  
  if (millis() - update_strip_time > 30)
  {
    if ( animator.IsAnimating() ) animator.UpdateAnimations(100);
    strip.Show();
    update_strip_time = millis();
  }


}



void OTA_handle(){

  if (OTA.parsePacket()) {
    IPAddress remote = OTA.remoteIP();
    int cmd  = OTA.parseInt();
    int port = OTA.parseInt();
    int size   = OTA.parseInt();

    Serial.print("Update Start: ip:");
    Serial.print(remote);
    Serial.printf(", port:%d, size:%d\n", port, size);
    uint32_t startTime = millis();

    WiFiUDP::stopAll();

    if(!Update.begin(size)){
      Serial.println("Update Begin Error");
      return;
    }

    WiFiClient client;

    bool connected = false; 

    delay(2000);
    
    //do {
      connected = client.connect(remote, port); 
    //} while (!connected || millis() - startTime < 20000);

    if (connected) {

      uint32_t written;
      while(!Update.isFinished()){
        written = Update.write(client);
        if(written > 0) client.print(written, DEC);
      }
      Serial.setDebugOutput(false);

      if(Update.end()){
        client.println("OK");
        Serial.printf("Update Success: %u\nRebooting...\n", millis() - startTime);
        ESP.restart();
      } else {
        Update.printError(client);
        Update.printError(Serial);
      }
    

    } else {
      Serial.printf("Connect Failed: %u\n", millis() - startTime);
      ESP.restart(); 
    }
  }
  //IDE Monitor (connected to Serial)
  if (TelnetServer.hasClient()){
    if (!Telnet || !Telnet.connected()){
      if(Telnet) Telnet.stop();
      Telnet = TelnetServer.available();
    } else {
      WiFiClient toKill = TelnetServer.available();
      toKill.stop();
    }
  }
  if (Telnet && Telnet.connected() && Telnet.available()){
    while(Telnet.available())
      Serial.write(Telnet.read());
  }
  if(Serial.available()){
    size_t len = Serial.available();
    uint8_t * sbuf = (uint8_t *)malloc(len);
    Serial.readBytes(sbuf, len);
    if (Telnet && Telnet.connected()){
      Telnet.write((uint8_t *)sbuf, len);
      yield();
    }
    free(sbuf);
  }
  //delay(1);
}