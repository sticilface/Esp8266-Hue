
#include "HueBridge.h"

  	HueBridge::HueBridge(ESP8266WebServer * HTTP, uint8_t lights, uint8_t groups, HueBridge::HueHandlerFunction fn): 
  		_HTTP(HTTP),
  		_LightCount(lights),
  		_GroupCount(groups),
  		_Handler(fn),
  		_returnJSON(false), 
  		_macString (String(WiFi.macAddress())), // toDo turn to 4 byte array..
  		_ipString (StringIPaddress(WiFi.localIP())),
  		_netmaskString (StringIPaddress(WiFi.subnetMask())),
  		_gatewayString (StringIPaddress(WiFi.gatewayIP()))  		

  	{

  	 	initHUE(_LightCount, _GroupCount); 

  	 	initSSDP();

		_HTTP->onNotFound(std::bind(&HueBridge::HandleWebRequest, this)); //  register the onNotFound for Webserver to HueBridge
  	
  	}

  	HueBridge::~HueBridge()   	{

		if (Lights) delete[] Lights;
		if (Groups) delete[] Groups;

		Lights = NULL;
		Groups = NULL; // Must check these.... 

		_HTTP->onNotFound(NULL); 
  	}

void HueBridge::initHUE(uint8_t Lightcount, uint8_t Groupcount) {

		if (Lights) delete[] Lights;
		if (Groups) delete[] Groups;

 		Lights = new HueLight[Lightcount];
 		Groups = new HueGroup[Groupcount];
 			Serial.println("Philips Hue Started....");
 			Serial.print("Size Lights = ");
 			Serial.println(sizeof(HueLight) * Lightcount);
 			Serial.print("Size Groups = ");
 			Serial.println(sizeof(HueGroup) * Groupcount);

 		for (uint8_t i = 0; i < Lightcount; i++) {
    		HueLight* currentlight = &Lights[i];
    		sprintf(currentlight->Name, "Hue Light %u", i+1); 
 		}

		for (uint8_t i = 0; i < Groupcount; i++) {
  			HueGroup *g = &Groups[i];
  			sprintf(g->Name, "Group %u", i+1); 
  			g->LightsCount = 5 ; 

      		for (uint8_t i = 0; i < 5; i++) {
        		uint8_t randomlight = random(1,Lightcount + 1 );
        		g->LightMembers[i] = randomlight; 
      		}

 		}

}


void HueBridge::HandleWebRequest() {


    //------------------------------------------------------
    //                    Initial web request handles  .... 
    //------------------------------------------------------
     HTTPMethod RequestMethod = _HTTP->method(); 
     long startime = millis(); 
//     Serial.print("Uri: ");
//     Serial.println(_HTTP->uri());

    /////////////////////////////////


  	//------------------------------------------------------
    //                    Extract User    
    //------------------------------------------------------

    Hue_Commands Command;  // holds command issued by client 


    if ( _HTTP->uri() != "/api" && _HTTP->uri().charAt(4) == '/' && _HTTP->uri() != "/api/config"  ) {
      user = _HTTP->uri().substring(5, _HTTP->uri().length()); 
      if (user.indexOf("/") > 0) {
        user = user.substring(0, user.indexOf("/"));
      } 
    }

    //Serial.print("Session user = ");
    //Serial.println(user);

    isAuthorized = true; 

    if (!isAuthorized) return;  // exit if none authorised user... toDO... 

    //------------------------------------------------------
    //                    Determine command   
    //------------------------------------------------------

//     /api/JPnfsdoKSVacEA0f/lights/8   --> renames light  
    Serial.print("  ");

    if        ( _HTTP->uri() == "/description.xml") {
        Handle_Description(); 
        return; 
    } else if ( _HTTP->uri() == "/api"  ) {
        Serial.print("CREATE_USER"); 
        Command = CREATE_USER;
         
    } else if ( _HTTP->uri().endsWith("/config") ) {
        Command = GET_CONFIG; 
        Serial.print("GET_CONFIG"); 
    } else if (  _HTTP->uri() == "/api/" + user ) {
        Command = GET_FULLSTATE; 
        Serial.print("GET_FULLSTATE"); 
    } else if ( _HTTP->uri().indexOf(user) && _HTTP->uri().endsWith("/state") ) {

        if (RequestMethod == HTTP_PUT) { 
          Command = PUT_LIGHT;
          Serial.print("PUT_LIGHT"); 
        } else if (RequestMethod == HTTP_GET) {
          Command = GET_LIGHT; 
          Serial.print("GET_LIGHT"); // ToDo
        } else {
          Serial.print("Unknown req: ");
          Serial.print(HTTPMethod_text[RequestMethod]);
        }

    } else if ( _HTTP->uri().substring(0, _HTTP->uri().lastIndexOf("/") ) == "/api/" + user + "/lights" ) {

      if (RequestMethod == HTTP_PUT) { 
          Command = PUT_LIGHT_ROOT;
          Serial.print("PUT_LIGHT_ROOT"); 
        } else {
          Command = GET_LIGHT_ROOT;
          Serial.print("GET_LIGHT_ROOT"); 

        }

    } else if  ( _HTTP->uri() == "/api/" + user + "/lights" ) {
         Command = GET_ALL_LIGHTS; 
         Serial.print("GET_ALL_LIGHTS"); 
    
    }



    else 

    {

        Serial.print("UnknownUri: ");
        Serial.print(_HTTP->uri());
        Serial.print(" ");
        Serial.println(HTTPMethod_text[RequestMethod]); 

        if (_HTTP->arg("plain") != "" ) {
          Serial.print("BODY: ");
          Serial.println(_HTTP->arg("plain"));
        }
        return; 

    }

    //------------------------------------------------------
    //                    Process command   
    //------------------------------------------------------

    size_t sent = 0; 

 //   FuncDelegate f_delegate;
 //   f_delegate = MakeDelegate(this, &HueBridge::Send_Config_Callback);

    //timer.setTimerDelg(1, f_delegate, 1);

  //void (HueBridge::*my_memfunc_ptr)(); 
  // my_memfunc_ptr = &Send_Config_Callback; 


  switch (Command) {

    case GET_CONFIG:
        sent = printer.Send( _HTTP->client() , 200, "text/json", std::bind(&HueBridge::Send_Config_Callback, this) ); // Send_Config_Callback
        //sent = printer.Send( _HTTP->client() , 200, "text/json", Send_Config_Callback ); // Send_Config_Callback
     //      Send_HTTPprinter(200, "text/json", (this*)Send_Config_Callback); 
        break; 
    case GET_FULLSTATE:
        sent = printer.Send( _HTTP->client() , 200, "text/json", std::bind(&HueBridge::Send_DataStore_Callback, this) ); 
        break;
    case CREATE_USER:
//        Create_User();
        break;
    case PUT_LIGHT:
        Put_light(); 
        break;
    case PUT_LIGHT_ROOT:
//        Put_Light_Root();
         break;
    case GET_LIGHT_ROOT:
//        Get_Light_Root();
        break;
    case GET_ALL_LIGHTS:
//        Send_all_lights();
        break;

  }

   if (sent) Serial.printf(" %uB", sent); 

    //------------------------------------------------------
    //                    END OF NEW command PARSING
    //------------------------------------------------------


  long _time = millis() - startime; 
  Serial.print(" "); 
  Serial.print(_time);
  Serial.println("ms");

  	}



/*-------------------------------------------------------------------------------------------

Functions to send out JSON data

-------------------------------------------------------------------------------------------*/


void HueBridge::Send_DataStore_Callback() {

            printer.print("{"); // START of JSON 
              printer.print(F("\"lights\":{ "));
                  Print_Lights();  // Lights
              printer.print("},");
              printer.print(F("\"config\":{ "));
                  Print_Config();  // Config
              printer.print("},");
              printer.print(F("\"groups\": { "));
                  Print_Groups();  // Config
              printer.print(F("},")); 
              printer.print(F("\"scenes\" : { }"));
              printer.print(","); 
              printer.print(F("\"schedules\" : { }"));       
          printer.print("}"); //  END of JSON
}


void HueBridge::Send_Config_Callback () {

          printer.print("{"); // START of JSON 
            Print_Config();  // Config      
          printer.print("}"); //  END of JSON

}

void HueBridge::Print_Lights() {

      for (uint8_t i = 0; i < _LightCount; i++) {
          
          if (i > 0 ) printer.print(",");

          uint8_t LightNumber = i + 1; 
          HueLight* currentlight = &Lights[i];

          printer.print(F("\""));
          printer.print(LightNumber);
          printer.print(F("\":{"));
          printer.print(F("\"type\":\"Extended color light\","));
          printer.printf("\"name\":\"%s\",",currentlight->Name);
          printer.print(F("\"modelid\":\"LST001\","));
          printer.print(F("\"state\":{"));
          ( currentlight->State == true ) ? printer.print(F("\"on\":true,")) : printer.print(F("\"on\":false,"));
          printer.printf("\"bri\":%u,", currentlight->Bri);
          printer.printf("\"hue\":%u,", currentlight->Hue);
          printer.printf("\"sat\":%u,", currentlight->Sat);

          //temporarily holds data from vals
          char x[10];                
          char y[10];
          //4 is mininum width, 3 is precision; float value is copied onto buff
          dtostrf(currentlight->xy.x, 5, 4, x);
          dtostrf(currentlight->xy.y, 5, 4, y);

          printer.printf("\"xy\":[%s,%s],", x, y );
          printer.printf("\"ct\":%u,", currentlight->Ct);
          printer.print(F("\"alert\":\"none\","));
          printer.print(F("\"effect\":\"none\","));
          printer.printf("\"colormode\":\"%s\",", currentlight->Colormode);
          printer.print(F("\"reachable\":true}"));
          printer.print("}");
      }

}

void HueBridge::Print_Groups(){

      for (uint8_t i = 0; i < _GroupCount; i++) {
          
          if (i > 0 ) printer.print(",");

          uint8_t GroupNumber = i + 1; 
          HueGroup* currentgroup = &Groups[i];

          printer.print(F("\""));
          printer.print(GroupNumber);
          printer.print(F("\":{"));
          printer.printf("\"name\":\"%s\",",currentgroup->Name);
          printer.print(F("\"lights\": ["));

            for (uint8_t i = 0; i < currentgroup->LightsCount; i++) {
              if (i>0) printer.print(",");
              printer.printf("\"%u\"", currentgroup->LightMembers[i]);
            }

          printer.print(F("],"));
          printer.print(F("\"type\":\"LightGroup\","));
          printer.print(F("\"action\": {"));
            ( currentgroup->State == true ) ? printer.print(F("\"on\":true,")) : printer.print(F("\"on\":false,"));
          printer.printf("\"bri\":%u,", currentgroup->Bri);
          printer.printf("\"hue\":%u,", currentgroup->Hue);
          printer.printf("\"sat\":%u,", currentgroup->Sat);
          printer.print("\"effect\": \"none\",");

          //temporarily holds data from vals
          char x[10];                
          char y[10];
          //4 is mininum width, 3 is precision; float value is copied onto buff
          dtostrf(currentgroup->xy.x, 5, 4, x);
          dtostrf(currentgroup->xy.y, 5, 4, y);

          printer.printf("\"xy\":[%s,%s],", x, y );
          printer.printf("\"ct\":%u,", currentgroup->Ct);
          printer.print(F("\"alert\":\"none\","));
          printer.printf("\"colormode\":\"%s\"", currentgroup->Colormode);
          printer.print("}}");

  }
      
}


void HueBridge::Print_Config() {

  printer.print("\"name\":\"Hue Emulator\",");
  printer.print("\"swversion\":\"0.1\","); 
  printer.printf("\"mac\": \"%s\",", (char*)_macString.c_str() );
  printer.print("\"dhcp\":true,"); 
  printer.printf("\"ipaddress\": \"%s\",", (char*)_ipString.c_str() );
  printer.printf("\"netmask\": \"%s\",", (char*)_netmaskString.c_str() );
  printer.printf("\"gateway\": \"%s\",", (char*)_gatewayString.c_str() );
  printer.print(F("\"swupdate\":{\"text\":\"\",\"notify\":false,\"updatestate\":0,\"url\":\"\"},")); 
  printer.print(F("\"whitelist\":{\"xyz\":{\"name\":\"clientname#devicename\"}}"));
          
}


void HueBridge::SendJson(JsonObject& root){

  size_t size = 0; // root.measureLength(); 
  Serial.print("  JSON ");
  WiFiClient c = _HTTP->client();
  printer.Begin(c); 
  printer.SetHeader(200, "text/json");
  printer.SetCountMode(true);
  root.printTo(printer);
  size  = printer.SetCountMode(false);
  root.printTo(printer);
  c.stop();
  while(c.connected()) yield();
  printer.End(); // free memory...
  Serial.printf(" %uB", size ); 

}

void HueBridge::SendJson(JsonArray& root){


  size_t size = 0; 
  Serial.print("  JSON ");
  WiFiClient c = _HTTP->client();
  printer.Begin(c); 
  printer.SetHeader(200, "text/json");
  printer.SetCountMode(true);
  root.printTo(printer);
  size = printer.SetCountMode(false);
  root.printTo(printer);
  c.stop();
  while(c.connected()) yield();
  printer.End(); // free memory...
  Serial.printf(" %uB", size); 

}


void HueBridge::Handle_Description() {

  String str = F("<root><specVersion><major>1</major><minor>0</minor></specVersion><URLBase>http://"); 
  str += _ipString; 
  str += F(":80/</URLBase><device><deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType><friendlyName>Philips hue ("); 
  str += _ipString; 
  str += F(")</friendlyName><manufacturer>Royal Philips Electronics</manufacturer><manufacturerURL>http://www.philips.com</manufacturerURL><modelDescription>Philips hue Personal Wireless Lighting</modelDescription><modelName>Philips hue bridge 2012</modelName><modelNumber>929000226503</modelNumber><modelURL>http://www.meethue.com</modelURL><serialNumber>00178817122c</serialNumber><UDN>uuid:2f402f80-da50-11e1-9b23-00178817122c</UDN><presentationURL>index.html</presentationURL><iconList><icon><mimetype>image/png</mimetype><height>48</height><width>48</width><depth>24</depth><url>hue_logo_0.png</url></icon><icon><mimetype>image/png</mimetype><height>120</height><width>120</width><depth>24</depth><url>hue_logo_3.png</url></icon></iconList></device></root>");
  _HTTP->send(200, "text/plain", str);
  Serial.println("/Description.xml SENT"); 

}


/*-------------------------------------------------------------------------------------------

Functions to process known web request 

-------------------------------------------------------------------------------------------*/

void HueBridge::Put_light () {

  if (_HTTP->arg("plain") == "")
    {
      return; 
    }

    //------------------------------------------------------
    //                    Extract Light ID ==>  Map to LightNumber (toDo)
    //------------------------------------------------------

    uint8_t numberOfTheLight = Extract_LightID();  
    String lightID = String(numberOfTheLight + 1); 
 
    HueLight* currentlight;
    currentlight = &Lights[numberOfTheLight];

    struct RgbColor rgb;
    struct HueHSB hsb; 


    //------------------------------------------------------
    //                    JSON set up IN + OUT 
    //------------------------------------------------------

    DynamicJsonBuffer jsonBufferOUT; // create buffer 
    DynamicJsonBuffer jsonBufferIN; // create buffer 
    JsonObject& root = jsonBufferIN.parseObject(_HTTP->arg("plain"));
    JsonArray& array = jsonBufferOUT.createArray(); // new method

    //------------------------------------------------------
    //                    ON / OFF 
    //------------------------------------------------------

    bool onValue = false; 

    if (root.containsKey("on"))
        {
          onValue = root["on"]; 
          String response = "/lights/" + lightID + "/state/on"; // + onoff; 

          if (onValue) {
//            Serial.print("  ON :");
            currentlight->State = true;
            if (_returnJSON) AddSucessToArray (array, response, F("on") ); 

          } else {
//            Serial.print("  OFF :");
            currentlight->State = false;
            rgb = RgbColor(0,0,0);
            if (_returnJSON) AddSucessToArray (array, response, F("off") ) ; 

          }

        }   

    //------------------------------------------------------
    //              HUE / SAT / BRI 
    //------------------------------------------------------
        //    To Do Colormode..... 

     bool hasHue{false}, hasBri{false}, hasSat{false};  
     uint16_t hue = currentlight->Hue ;
     uint8_t sat = currentlight->Sat ; 
     uint8_t bri = currentlight->Bri ; 
        
    if (root.containsKey("hue"))
        {
          hue = root["hue"]; 
          String response = "/lights/" + lightID + "/state/hue"; // + onoff; 
//          Serial.print("  HUE -> ");
//          Serial.print(hue);
//          Serial.print(", "); 
          hasHue = true; 
          if (_returnJSON) AddSucessToArray (array, response, root["hue"] ); 
        } 

    if (root.containsKey("sat"))
        {
          sat = root["sat"]; 
          String response = "/lights/" + lightID + "/state/sat"; // + onoff; 
//          Serial.print("  SAT -> ");
//          Serial.print(sat);
//          Serial.print(", "); 
          hasSat = true; 
          if (_returnJSON) AddSucessToArray (array, response, root["sat"]); 
        } 

    if (root.containsKey("bri"))
        {
          bri = root["bri"]; 
          String response = "/lights/" + lightID + "/state/bri"; // + onoff; 
//          Serial.print("  BRI -> ");
//          Serial.print(bri);
//          Serial.print(", "); 
          hasBri = true; 
          if (_returnJSON) AddSucessToArray (array, response, root["bri"]); 
        } 

    //------------------------------------------------------
    //              XY Color Space 
    //------------------------------------------------------

     HueXYColor xy_instance; 
     bool hasXy{false}; 

    if (root.containsKey("xy"))
        {
           xy_instance.x = root["xy"][0];
           xy_instance.y = root["xy"][1];
           currentlight->xy = xy_instance;
              // Serial.print("  XY (");
              // Serial.print(xy_instance.x);
              // Serial.print(",");
              // Serial.print(xy_instance.y);
              // Serial.print(") "); 
            hasXy = true; 
        if (_returnJSON) {
          JsonObject& nestedObject = array.createNestedObject();          
          JsonObject& sucess = nestedObject.createNestedObject("success");
          JsonArray&  xyObject  = sucess.createNestedArray("xy");
          xyObject.add(xy_instance.x, 4);  // 4 digits: "3.1415"
          xyObject.add(xy_instance.y, 4);  // 4 digits: "3.1415"
      	  }
        }

    //------------------------------------------------------
    //              Ct Colour space
    //------------------------------------------------------

    if (root.containsKey("ct"))
        {




		}

    //------------------------------------------------------
    //              Apply recieved Color data 
    //------------------------------------------------------




    if (hasHue || hasBri || hasSat) {
      currentlight->Hue = hsb.H = hue;
      currentlight->Sat = hsb.S = sat;
      currentlight->Bri = hsb.B = bri; 

      rgb = HUEhsb2rgb(hsb); //  designed to handle philips hue RGB ALLOCATED FROM JSON REQUEST
      currentlight->xy = HUEhsb2xy(hsb); // COPYED TO LED STATE incase onother applciation requests colour
    
   } else if (hasXy) {

      rgb = XYtorgb(xy_instance, bri);  // set the color to return
      hsb = xy2HUEhsb(xy_instance, bri);  // converts for storage...
      currentlight->Hue = hsb.H; ///floor(hsb.H * 182.04 * 360.0); 
      currentlight->Sat = hsb.S; //floor(hsb.S * 254);
      currentlight->Bri = hsb.B; // floor(hsb.B * 254);  
   }    


    //------------------------------------------------------
    //              SEND Back changed JSON REPLY 
    //------------------------------------------------------

      // char *msgStr = aJson.print(reply);
      // aJson.deleteItem(reply);
      // Serial.print("\nJSON REPLY = ");
      // //Serial.println(millis());
      // Serial.println(msgStr);
      // HTTP.send(200, "text/plain", msgStr);
      // free(msgStr);


  //  Serial.println();
//    array.prettyPrintTo(Serial);
//    Serial.println(); 

    if (_returnJSON) SendJson(array);


    //------------------------------------------------------
    //              Print Saved State Buffer to serial 
    //------------------------------------------------------

      // Serial.print("Saved = Hue:");
      // Serial.print(StripHueData[numberOfTheLight].Hue); 
      // Serial.print(", Sat:"); 
      // Serial.print(StripHueData[numberOfTheLight].Sat); 
      // Serial.print(", Bri:");   
      // Serial.print(StripHueData[numberOfTheLight].Bri); 
      // Serial.println(); 


    //------------------------------------------------------
    //              Set up LEDs... rock and roll.... 
    //------------------------------------------------------

    //Serial.println();

    // for (uint8_t i = 0; i < pixelCount; i++) {
    // // define the effect to apply, in this case linear blend
    //   RgbColor originalColor = strip.GetPixelColor(i); // FIXME: In lambda function: error: 'originalColor' was not declared in this scope - potential reason for crashes?

    //     AnimUpdateCallback animUpdate = [ onValue, originalColor, rgb, i ](float progress)
    //     {
    //       // progress will start at 0.0 and end at 1.0
    //       RgbColor updatedColor; 
    //       if (onValue) updatedColor = RgbColor::LinearBlend(originalColor, rgb, progress);
    //       if (!onValue) updatedColor = RgbColor::LinearBlend(originalColor, black, progress);
    //       strip.SetPixelColor(i, updatedColor);
    //     };

    //     animator.StartAnimation(i, transitionTime, animUpdate);

    // }

    	uint8_t Light = (uint8_t)lightID.toInt();

    	_Handler(Light, 2000, rgb); 

    //	_Handler(Light, currentlight); 



}


uint8_t HueBridge::Extract_LightID() {

    String lightID = (_HTTP->uri().substring(_HTTP->uri().indexOf("/lights/") + 8 ,_HTTP->uri().indexOf("/state")));
    uint8_t light  = lightID.toInt() - 1; 
    //Serial.print(" LIGHT ID = ");
    //Serial.println(lightID);
    return light ; 
}

// New method using arduinoJSON
void HueBridge::AddSucessToArray(JsonArray& array, String item, String value) {

      JsonObject& nestedObject = array.createNestedObject(); 
      JsonObject& sucess = nestedObject.createNestedObject("success");
      sucess[item] = value; 

}

void HueBridge::AddSucessToArray(JsonArray& array, String item,  char* value) {

      JsonObject& nestedObject = array.createNestedObject(); 
      JsonObject& sucess = nestedObject.createNestedObject("success");
      sucess[item] = value; 

}

String HueBridge::StringIPaddress(IPAddress myaddr) {


  String LocalIP = "";
  for (int i = 0; i < 4; i++)
  {
    LocalIP += String(myaddr[i]);
    if (i < 3) LocalIP += ".";
  }
  return LocalIP;

}

void HueBridge::initSSDP() {

  Serial.printf("Starting SSDP...");
  SSDP.begin();
  SSDP.setSchemaURL((char*)"description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName((char*)"Philips hue clone");
  SSDP.setSerialNumber((char*)"001788102201");
  SSDP.setURL((char*)"index.html");
  SSDP.setModelName((char*)"Philips hue bridge 2012");
  SSDP.setModelNumber((char*)"929000226503");
  SSDP.setModelURL((char*)"http://www.meethue.com");
  SSDP.setManufacturer((char*)"Royal Philips Electronics");
  SSDP.setManufacturerURL((char*)"http://www.philips.com");
  Serial.println("SSDP Started");
}

void HueBridge::SetReply(bool value) {
	_returnJSON = value; 
}

bool HueBridge::SetLightState(uint8_t light, bool value){

	if(light == 0) return false;
	light--;
	if (light < 0 || light > _LightCount) return false; 
	HueLight* currentlight = &Lights[light]; 
    currentlight->State = value; 
    return true; 
}

bool HueBridge::GetLightState(uint8_t light) {

	if(light == 0) return false;
	light--;
	if (light < 0 || light > _LightCount) return false; 
	HueLight* currentlight = &Lights[light]; 
	return currentlight->State;

}

bool HueBridge::SetLightRGB(uint8_t light, RgbColor color) {

//
//					To Do
//

}

RgbColor HueBridge::GetLightRGB(uint8_t light) {


//
//					To Do
//

}




bool HueBridge::SetGroupState(uint8_t group, bool value){

	if(group == 0) return false;
	group--;
	if (group < 0 || group > _GroupCount) return false; 
	HueGroup* currentgroup = &Groups[group];
    currentgroup->State = value; 
    return true; 
}



bool HueBridge::GetGroupState(uint8_t group) {

	if(group == 0) return false;
	group--;
	if (group < 0 || group > _GroupCount) return false; 
	HueGroup* currentgroup = &Groups[group];
    return currentgroup->State; 
}




/*-------------------------------------------------------------------------------------------

Colour Management
Thanks to probonopd

-------------------------------------------------------------------------------------------*/


struct HueHSB HueBridge::rgb2HUEhsb(RgbColor color)
{

  HsbColor hsb = HsbColor(color);
  int hue, sat, bri;

  hue = floor(hsb.H * 182.04 * 360.0);
  sat = floor(hsb.S * 254);
  bri = floor(hsb.B * 254);

  HueHSB hsb2;
  hsb2.H = hue;
  hsb2.S = sat;
  hsb2.B = bri;

  return (hsb2);
}



struct RgbColor HueBridge::HUEhsb2rgb(HueHSB color)
{

  float H, S, B;
  H = color.H / 182.04 / 360.0;
  S = color.S / 254.0; 
  B = color.B / 254.0; 
  return HsbColor(H, S, B);
}

struct HueXYColor HueBridge::rgb2xy(RgbColor color) {

        float red   =  float(color.R) / 255.0f;
        float green =  float(color.G) / 255.0f;
        float blue  =  float(color.B) / 255.0f;

        // Wide gamut conversion D65
        float r = ((red > 0.04045f) ? (float) pow((red + 0.055f)
                / (1.0f + 0.055f), 2.4f) : (red / 12.92f));
        float g = (green > 0.04045f) ? (float) pow((green + 0.055f)
                / (1.0f + 0.055f), 2.4f) : (green / 12.92f);
        float b = (blue > 0.04045f) ? (float) pow((blue + 0.055f)
                / (1.0f + 0.055f), 2.4f) : (blue / 12.92f);

        float x = r * 0.649926f + g * 0.103455f + b * 0.197109f;
        float y = r * 0.234327f + g * 0.743075f + b * 0.022598f;
        float z = r * 0.0000000f + g * 0.053077f + b * 1.035763f;

        // Calculate the xy values from the XYZ values
        
        HueXYColor xy_instance;

        xy_instance.x = x / (x + y + z);
        xy_instance.y = y / (x + y + z);

        if (isnan(xy_instance.x) ) {
            xy_instance.x = 0.0f;
        }
        if (isnan(xy_instance.y)) {
            xy_instance.y = 0.0f;
        }
 
  return xy_instance; 


}

struct HueXYColor HueBridge::HUEhsb2xy(HueHSB color) {

	RgbColor rgb = HUEhsb2rgb(color);
	double r = rgb.R / 255.0;
	double g = rgb.G / 255.0;
	double b = rgb.B / 255.0;
	double X = r * 0.649926f + g * 0.103455f + b *0.197109f;
	double Y = r * 0.234327f + g * 0.743075f + b * 0.022598f;
	double Z = r * 0.0000000f + g * 0.053077f + b * 1.035763f;
	HueXYColor xy;
	xy.x = X / (X + Y + Z);
	xy.y = Y / (X + Y + Z);
	return xy;
}

struct HueHSB HueBridge::xy2HUEhsb(HueXYColor xy, uint8_t bri) {

	double x = xy.x;
	double y = xy.y; 

	double z = 1.0f - xy.x - xy.y;
	double Y = (double)(bri / 254.0); // The given brightness value
	double X = (Y / xy.y) * xy.x;
	double Z = (Y / xy.y) * z;
	double r = X * 1.4628067f - Y * 0.1840623f - Z * 0.2743606f;
	double g = -X * 0.5217933f + Y* 1.4472381f + Z *  0.0677227f;
	double b = X * 0.0349342f - Y * 0.0968930f + Z * 1.2884099f;
	uint8_t R = abs(r) * 255;
	uint8_t G = abs(g) * 255;
	uint8_t B = abs(b) * 255;
	struct HueHSB hsb;

	double mi, ma, delta, h;
	mi = (R<G)?R:G; mi = (mi<B)?mi:B; ma = (R>G)?R:G;
	ma = (ma>B)?ma:B;
	delta = ma - mi;
	
	if(ma <= 0.0){
		hsb.H = 0xFFFF;
		hsb.S = 1;
		hsb.B = bri;
	return hsb;
	}

 	if (R >= ma) h = (G - B) / delta; // between yellow & magenta
	else if(G >= ma) h = 2.0 + (B - R) / delta; // between cyan & yellow
	else h = 4.0 + ( R - G ) / delta; // between magenta & cyan
	h *= 60.0; // degrees
	if(h < 0.0) h += 360.0;
	hsb.H = (uint16_t)floor(h * 182.04);
	hsb.S = (uint16_t)floor((delta / ma) * 254);
	hsb.B = bri;
	return hsb;
}

// struct HueHSB ct2hsb(long kelvin, uint8_t bri) {
// 	double r, g, b;
// 	long temperature = kelvin / 10;
// 	if(temperature <= 66) {
// 		r = 255;
// 	}
// 	else {
// 		r = temperature - 60;
// 		r = 329.698727446 * pow(r, -0.1332047592);
// 		if(r < 0) r = 0;
// 		if(r > 255) r = 255;
// 	}

// 	if(temperature <= 66) {
// 		g = temperature;
// 		g = 99.4708025861 log(g) - 161.1195681661;
// 		if(g < 0) g = 0;
// 		if(g > 255) g = 255;
// 		}
// 		else {
// 			g = temperature - 60;
// 			g = 288.1221695283 pow(g, -0.0755148492);
// 			if(g < 0) g = 0;
// 			if(g > 255) g = 255;
// 		}

// 		if(temperature >= 66) {
// 			b = 255;
// 		}
// 		else {
// 			if(temperature <= 19) {
// 			b = 0;
// 			}
// 		else {
// 			b = temperature - 10;
// 			b = 138.5177312231 * log(b) - 305.0447927307;
// 			if(b < 0) b = 0;
// 			if(b > 255) b = 255;
// 		}
// 		}

// 	uint8_t R = abs(r) 255;
// 	uint8_t G = abs(g) 255;
// 	uint8_t B = abs(b) * 255;
// 	struct HueHSB hsb;
// 	double mi, ma, delta, h;
// 	mi = (R<G)?R:G; mi = (mi<B)?mi:B; ma = (R>G)?R:G;
// 	ma = (ma>B)?ma:B;
// 	delta = ma - mi;
// 	if(ma <= 0.0){
// 	hsb.H = 0xFFFF;
// 	hsb.S = 1;
// 	hsb.B = bri;
// return hsb;
// }

// if(R >= ma) h = (G - B) / delta; // between
// }

//http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
// 'Given a temperature (in Kelvin), estimate an RGB equivalent

struct RgbColor HueBridge::ct2rbg(long tmpKelvin, uint8_t bri) {

     if (tmpKelvin < 1000) tmpKelvin = 1000;
     if (tmpKelvin > 40000)tmpKelvin = 40000;

     double tmpCalc; 
     RgbColor rgb; 
     tmpKelvin = tmpKelvin / 100; 

// RED.
     if (tmpKelvin <= 66) {
     	rgb.R = 255; 
     } else {
        tmpCalc = tmpKelvin - 60;
        tmpCalc = 329.698727446 * pow(tmpCalc, -0.1332047592);
        rgb.R = tmpCalc;
        if (rgb.R < 0) rgb.R = 0;
        if (rgb.R > 255) rgb.R = 255;

     }
// green
	if (tmpKelvin <= 66) {
//         'Note: the R-squared value for this approximation is .996
         tmpCalc = tmpKelvin;
         tmpCalc = 99.4708025861 * log(tmpCalc) - 161.1195681661;
         rgb.G = tmpCalc;
         if (rgb.G < 0) rgb.G = 0;
         if (rgb.G > 255) rgb.G = 255;
   } else {
//         'Note: the R-squared value for this approximation is .987
	         tmpCalc = tmpKelvin - 60; 
	         tmpCalc = 288.1221695283 * pow(tmpCalc,-0.0755148492);
         	 rgb.G = tmpCalc;
         	if (rgb.G < 0) rgb.G = 0;
         	if (rgb.G > 255) rgb.G = 255;
	}

return rgb; 


     }

struct HueHSB HueBridge::ct2hsb(long tmpKelvin, uint8_t bri) {

		RgbColor rgb = ct2rbg(tmpKelvin, bri);
		return (rgb2HUEhsb(rgb)); 
}


struct HueXYColor HueBridge::Ct2xy(long tmpKelvin, uint8_t bri) {

		RgbColor rgb = ct2rbg(tmpKelvin, bri);
		return rgb2xy(rgb); 

	} 

struct RgbColor HueBridge::XYtorgb(struct HueXYColor xy, uint8_t bri) {

 	HueHSB hsb = xy2HUEhsb(xy,bri); 
	return HUEhsb2rgb(hsb);

}

