#include <utility>
#include <TinyXML.h>
#include "soundTouchXMLParser.hpp"
#include "statics.hpp"

namespace soundtouch
{
  using namespace alarmclock;
  using namespace logger;

  const char *SoundTouchXMLParser::tag{ "xmlparser" };

  SoundTouchXMLParser::SoundTouchXMLParser( XmlMessageList &_xmlList, DecodetMessageList &_msgList )
      : xmlList( _xmlList ), msgList( _msgList )
  {
    elog.log( logger::INFO, "%s: soundtouch xml parser create...", SoundTouchXMLParser::tag );
  }

  SoundTouchXMLParser::~SoundTouchXMLParser()
  {
    elog.log( logger::DEBUG, "%s: soundtouch xml parser delete...", SoundTouchXMLParser::tag );
  }

  bool SoundTouchXMLParser::decodeMessage()
  {
    using namespace tinyxml2;

    if ( xmlList.empty() )
      return true;
    //
    // get the message string
    //
    String msg = std::move( xmlList.front() );
    xmlList.erase( xmlList.begin() );
    elog.log( DEBUG, "%s: decode xml message <%s>...", SoundTouchXMLParser::tag, msg.c_str() );
    XMLDocument xmlDocument;
    return true;
  }

}  // namespace soundtouch