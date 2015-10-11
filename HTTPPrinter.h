/*--------------------------------------------------------------------
This lib is designed to stream out data to wificlient, in chunks.
At the moment the header is fixed, as HTTP 200 for JSON, but this will change.
To circumvent the issue of unknown size, send the data you want through it
after setting SetCountMode = true, once fininshed set it to false.  Now when 
you start to send data, content length is set to this value in header.  This
method is very fast, much faster than waiting for the client to disconnect. 
80K, of totally dynamic JSON, made on the fly, can be created and sent in 330ms. 

The buffer size defaults to HTTPPrinterSize, but can be changed at initialisation, 
for example HTTPPrinter printer(c, 500); will create a 500 byte send buffer. 
c, is a WiFiClient object reference , typically if you use c = (your HTTP instance).client(); 

Setcountmode returns size of the buffer at that time. 

Inspiration for design of this lib came from me-no-dev and probonopd. 

--------------------------------------------------------------------*/

#pragma once

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <functional>


#define HTTPPrinterSize 1000 // Sets the default size of buffer/packet to send out. 


class HTTPPrinter: public Print 
{

public:

  typedef std::function<void(void)> printer_callback;

  HTTPPrinter(); 
  void Begin(WiFiClient & c); 
  void Begin(WiFiClient & c, size_t memorysize); 
  void End();
  void Setsize(size_t s);                                                                                                                                                                                                                                          
  size_t GetSize();
  size_t SetCountMode(bool mode);
  
  size_t Send(WiFiClient client, int code, const char* content, printer_callback fn ); // uses printer callback
  size_t Send_Buffer(int code, const char* content); // just sends out the buffer as is... 

  void SetHeader(int code, const char* content);
  void Send_Header ();

private:
  virtual size_t write(uint8_t);  

  void Send_Header (int code, const char * content ); 
  uint8_t *_buffer;
  int _bufferCount;
  WiFiClient _client;
  size_t _size, _sizeTotal; 
  bool _headerSent; 
  bool _CountMode; 
  int _code; 
  const char* _headerContent; 
};




