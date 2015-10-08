/*--------------------------------------------------------------------
HueBridge for Arduino ESP8266
Andrew Melvin - Sticilface


Inspiration for design of this lib came from me-no-dev and probonopd. 

todo

user management
save settings


--------------------------------------------------------------------*/

#pragma once

#include "Arduino.h"

#include <ESP8266WiFi.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <functional>

//  These are from the neopixelbus lib.  https://github.com/Makuna/NeoPixelBus/tree/UartDriven 
//  Needs to be UARTDRIVEN branch, or Animator Branch

 #include <RgbColor.h>
 #include <HsbColor.h>


#include "HueBridgeStructs.h"
#include "HTTPPrinter.h"

#define cache ICACHE_FLASH_ATTR

static const char *HTTPMethod_text[] = { "HTTP_ANY", "HTTP_GET", "HTTP_POST", "HTTP_PUT", "HTTP_PATCH", "HTTP_DELETE" };

  class RgbColor;
  class HsbColor;

//typedef std::function<void(uint8_t Light, HueLight* currentlight)> HueHandlerFunction;


class HueBridge  
{
public:
	typedef std::function<void(uint8_t Light, uint16_t Time, struct RgbColor rgb)> HueHandlerFunction;
	//typedef std::function<void(uint8_t Light, HueLight* currentlight)> HueHandlerFunction;
    typedef std::function<void(void)>& GenericFunction; 

  	HueBridge(ESP8266WebServer * HTTP, uint8_t lights, uint8_t groups, HueHandlerFunction fn); 
  	~HueBridge(); 

  	void initHUE(uint8_t Lightcount, uint8_t Groupcount);
  	
  	void SetReply(bool value);  								
  	
  	bool SetLightState(uint8_t light, bool value);				
  	bool GetLightState(uint8_t light);			

  	bool SetLightRGB(uint8_t light, RgbColor color);                   //ToDo
  	RgbColor GetLightRGB(uint8_t light);          					   //ToDo

  	 bool GetGroupState(uint8_t group);                                 //ToDo
  	 bool SetGroupState(uint8_t group, bool value);                     //ToDo

  	
  	void SetGroupRGB(uint8_t group, uint8_t R, uint8_t G, uint8_t B);  //ToDo
  	void GetGroupRGB(uint8_t group);                                   //ToDo



	struct HueHSB rgb2HUEhsb(struct RgbColor color); 
	struct HueHSB xy2HUEhsb(struct HueXYColor xy, uint8_t bri); 
	struct HueHSB ct2hsb(long tmpKelvin, uint8_t bri);

	struct RgbColor HUEhsb2rgb(HueHSB color);	
	struct RgbColor XYtorgb(struct HueXYColor xy, uint8_t bri); 
	struct RgbColor ct2rbg(long tmpKelvin, uint8_t bri); 


	struct HueXYColor rgb2xy(struct RgbColor color); 
	struct HueXYColor HUEhsb2xy(struct HueHSB color);
	struct HueXYColor Ct2xy(long tmpKelvin, uint8_t bri); 


private:

	void Send_DataStore_Callback();
	void Send_Config_Callback();
	void Print_Lights();
	void Print_Groups();
	void Print_Config();
	void SendJson(JsonObject& root);
	void SendJson(JsonArray& root);
	void Handle_Description();
	void Send_HTTPprinter(int code, const char* content, GenericFunction Fn);
	void Put_light();
	uint8_t Extract_LightID();
	void AddSucessToArray(JsonArray& array, String item, String value);
	void AddSucessToArray(JsonArray& array, String item,  char* value);
	void initSSDP();
    void HandleWebRequest();


	String StringIPaddress(IPAddress myaddr);

 	HTTPPrinter printer; 
 	ESP8266WebServer* _HTTP; 
 	WiFiClient _client;


 	uint8_t _LightCount, _GroupCount; 

 	String user;
 	String _macString;
	String _ipString;
	String _netmaskString;
	String _gatewayString;
	bool isAuthorized, _returnJSON; 

 	HueLight * Lights = NULL; 
 	HueGroup * Groups = NULL; 
 
 	HueHandlerFunction _Handler; 

 	enum Hue_Commands { NO = 0, CREATE_USER, GET_CONFIG, GET_FULLSTATE, GET_LIGHT, GET_NEW_LIGHTS, PUT_LIGHT, GET_ALL_LIGHTS, PUT_LIGHT_ROOT,GET_LIGHT_ROOT }; 


};