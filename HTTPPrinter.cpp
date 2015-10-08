#include "HTTPPrinter.h"

  HTTPPrinter::HTTPPrinter() {
    _bufferCount = 0;  
    _buffer = NULL;
    _size = 0; 
    _headerSent = false; 
    _CountMode = false; 
  }


  void HTTPPrinter::Begin(WiFiClient &c) {
    _bufferCount = 0;  
    _buffer = (uint8_t *)malloc(HTTPPrinterSize);
    _client = c;
    _size = 0; 
    _headerSent = false; 
    _CountMode = false; 
  }
  
  void HTTPPrinter::Begin(WiFiClient &c, size_t memory) {
    _bufferCount = 0;  
    _buffer = (uint8_t *)malloc(memory);
    _client = c;
    _size = 0; 
    _headerSent = false; 
    _CountMode = false; 
  }

  void HTTPPrinter::End() {
     free(_buffer);
   }

  void HTTPPrinter::Setsize(size_t s) {
    _size = s; 
  }

  size_t HTTPPrinter::SetCountMode(bool mode) {

    if (mode) {
      _CountMode = true;
      _size = 0;
    } else {
      _CountMode = false; 
      _sizeTotal = _size; 
    }

    return _size; 

  }

  size_t HTTPPrinter::GetSize() {
    return _sizeTotal; 
  }

  
  size_t HTTPPrinter::write(uint8_t data) {

    if (_buffer == NULL) return 0; 

    switch(_CountMode) {

      case true:
        _size++;
        break; 

      case false:

        _buffer[_bufferCount++] = data; // put byte into buffer 

    if(_bufferCount == HTTPPrinterSize || _size == _bufferCount ) { // send it if full, or remaining bytes = buffer


      if (!_headerSent) Send_Header(200, "text/json"); 

      size_t sent = _client.write(_buffer + 0, _bufferCount);
      _size -= _bufferCount; // keep track of remaining bytes.. 
      _bufferCount = 0; // reset the buffer to begining.. 
    } 

    break;
    } // end of switch

    return true; 

  } // end of write 

void HTTPPrinter::SetHeader(int code, const char* content) {
  if (code < 0) code = 0; 
  _code = code; 
  _headerContent = content; 
}


size_t HTTPPrinter::Send(WiFiClient client, int code, const char* content, printer_callback fn ){

      Begin(client); 
      SetHeader(code, content);
      SetCountMode(true);
      fn();
      size_t size = SetCountMode(false);
      fn();
      End(); 

     client.stop();
     while(client.connected()) yield();

     return size; 
  }

 void HTTPPrinter::Send_Header (int code, const char * content) {

        if (_headerSent) return; 

          uint8_t *headerBuff = (uint8_t*)malloc(128);
          sprintf((char*)headerBuff, "HTTP/1.1 %u OK\r\nContent-Type: %s\r\nContent-Length: %u\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\n\r\n", code, content,_size);
          size_t headerLen = strlen((const char*)headerBuff);
          _client.write((const uint8_t*)headerBuff, headerLen);
          free(headerBuff);
          _headerSent = true; 

 }

