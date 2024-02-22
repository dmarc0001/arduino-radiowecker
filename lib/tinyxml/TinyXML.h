#pragma once

#include <inttypes.h>
typedef void ( *XMLcallback )( uint8_t errorflag, char *nameBuffer, uint16_t namebuflen, char *dataBuffer, uint16_t databuflen );

// inline member now #define isAlpha( ch ) ( ( ch >= 'A' && ch <= 'Z' ) || ( ch >= 'a' && ch <= 'z' ) )
// inline member now #define isNumeric( ch ) ( ch >= '0' && ch <= '9' )

//
// Status flags
//
#define STATUS_START_TAG 0x01
#define STATUS_TAG_TEXT 0x02
#define STATUS_ATTR_TEXT 0x04
#define STATUS_END_TAG 0x08
#define STATUS_ERROR 0x10

#define DEFAULT_BUFFER_SIZE 256
#define TAGBUFFERMAX 128
#define ATTRBUFFERMAX 64
#define CHECKTAGMAX 64

class TinyXML
{
  private:
  XMLcallback Xcb;
  uint8_t tagBuffer[ TAGBUFFERMAX + 1 ];  // allow for terminating zero
  uint16_t tagBufferPtr;
  uint8_t attrBuffer[ ATTRBUFFERMAX + 1 ];  // allow for terminating zero
  uint16_t attrBufferPtr;
  uint8_t currentState;
  uint8_t matchQuote;
  uint8_t LTCount;
  int8_t tagCount;
  uint8_t *dataBuffer;
  uint16_t maxDataLen;
  uint16_t dataBufferPtr;
  uint8_t checkTagBuffer[ CHECKTAGMAX + 1 ];  // allow for terminating zero
  uint16_t checkTagBufferPtr;

  void action( uint8_t ch, uint8_t actionType );

  public:
  TinyXML();  // constructor
  void init( uint8_t *buffer, uint16_t maxbuflen, XMLcallback XMLcb );
  void reset();
  void processChar( uint8_t ch );
  inline bool isAlpha( char ch )
  {
    return ( ( ch >= 'A' && ch <= 'Z' ) || ( ch >= 'a' && ch <= 'z' ) );
  }
  inline bool isNumeric( char ch )
  {
    return ( ch >= '0' && ch <= '9' );
  }
};
