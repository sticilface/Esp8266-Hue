/*--------------------------------------------------------------------
HueBridge for Arduino ESP8266
Andrew Melvin - Sticilface


Inspiration for design of this lib came from me-no-dev and probonopd. 

todo

FIX nan for xy colour convertion.. breaks the JSON...
Add /format for spiffs...
user management
save settings - kind of done.  groups not done properly.  just saves all to SPIFFS
Testing of colour conversions + optimising/fixing them. 
Delete groups/ then adding them -> finding first free group slot.. etc... 
Shift over to SPIFFS filesystem, for everything, so whole array is not kept in RAM!  

***
Think about adding a function callback to a light... that can be called on on/off change or colour. 
maybe as an override to the generic... ie.. if blah != NULL then.... do this function instead... 
Could allow lights to do different things.  such as call an MQTT function / UDP function.  then the bridge can act as a bridge.
***


// max groups in hue is 16 per bridge... 
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

#define MaxLightMembersPerGroup 10 // limits the size of the array holding lights in each group.  


#include "HueBridgeStructs.h"
#include "HTTPPrinter.h"

#define cache ICACHE_FLASH_ATTR // not being used yet... 

#define EXPERIMENTAL // stuff for me to play with...

static const char *HTTPMethod_text[] = { "HTTP_ANY", "HTTP_GET", "HTTP_POST", "HTTP_PUT", "HTTP_PATCH", "HTTP_DELETE" };

 class RgbColor;
 class HsbColor;

//typedef std::function<void(uint8_t Light, HueLight* currentlight)> HueHandlerFunction;


class HueBridge  
{
public:

	typedef std::function<void(uint8_t Light, struct RgbColor rgb, HueLight* currentlight)> HueHandlerFunctionNEW;    
    typedef std::function<void(void)>& GenericFunction; 

  	HueBridge(ESP8266WebServer * HTTP, uint8_t lights, uint8_t groups, HueHandlerFunctionNEW fn); 
  	~HueBridge(); 

  	void Begin();
  	
  	void SetReply(bool value);  								
  	
  	bool SetLightState(uint8_t light, bool value);				
  	bool GetLightState(uint8_t light);			

  	bool SetLightRGB(uint8_t light, RgbColor color);                   //ToDo
  	struct RgbColor GetLightRGB(uint8_t light);          			   //ToDo

  	 bool GetGroupState(uint8_t group);                                 //ToDo
  	 bool SetGroupState(uint8_t group, bool value);                     //ToDo

  	
  	void SetGroupRGB(uint8_t group, uint8_t R, uint8_t G, uint8_t B);  //ToDo
  	void GetGroupRGB(uint8_t group);                                   //ToDo

	void Get_Light_Root(); 											   //  NOT IMPLEMENTED 
	void Put_Light_Root();

	void Name_Light(uint8_t i, const char* name);
	void Name_Light(uint8_t i,  String &name);
    void Name_Group(uint8_t i, const char* name);
    void Name_Group(HueGroup * currentgroup, const char* name);


	void Name_Group(uint8_t i,  String &name);

	bool Add_Group();

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
	void Put_group(); 
	uint8_t Extract_LightID();
	void AddSucessToArray(JsonArray& array, String item, String value);
	void AddSucessToArray(JsonArray& array, String item,  char* value);
	void initSSDP();
    void HandleWebRequest();
    char* subStr(const char* str, char *delim, int index);
  	void initHUE(uint8_t Lightcount, uint8_t Groupcount);
  	uint8_t find_nextfreegroup();
	bool SPIFFS_LIGHT(uint8_t no, HueLight *L, bool save); 


	String StringIPaddress(IPAddress myaddr);

 	HTTPPrinter printer; 
 	ESP8266WebServer* _HTTP; 
 	WiFiClient _client;

 	uint8_t _LightCount, _GroupCount, _nextfreegroup; 

 	String user;
 	String _macString;
	String _ipString;
	String _netmaskString;
	String _gatewayString;
	bool isAuthorized, _returnJSON; 

	long _lastrequest = 0; 

 	HueLight * Lights = NULL; 
 	HueGroup * Groups = NULL; 
 
 //	HueHandlerFunction _Handler; 
	HueHandlerFunctionNEW _HandlerNEW;

	File L_File;
	File G_File;

 	//enum Hue_Commands { NO = 0, CREATE_USER, GET_CONFIG, GET_FULLSTATE, GET_LIGHT, GET_NEW_LIGHTS, PUT_LIGHT, 
 	//	GET_ALL_LIGHTS, PUT_LIGHT_ROOT, GET_LIGHT_ROOT, PUT_GROUP, GET_GROUP, ADD_GROUP }; 


};