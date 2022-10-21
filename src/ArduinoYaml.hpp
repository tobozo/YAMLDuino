#pragma once
/*
 *
 * ESP32-yaml
 * Project Page: https://github.com/tobozo/esp32-yaml
 *
 * Copyright 2022 tobozo http://github.com/tobozo
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files ("ESP32-yaml"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


// fuck you espressif, this warning is treaded as error
//#pragma GCC diagnostic ignored "-Wunused-variable"
//#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include "logger.hpp"

extern "C" {
  #include "libyaml/yaml.h" // https://github.com/yaml/libyaml
}

#if !defined YAML_DISABLE_CJSON // define this from sketch if cJSON isn't needed
  #define HAS_CJSON // built-in (esp32) or bundled
#endif

#if !defined YAML_DISABLE_ARDUINOJSON

  #if defined ARDUINO_ARCH_SAMD || defined ARDUINO_ARCH_RP2040 || defined ESP8266
    // those platforms don't have built-in cJSON so assume ArduinoJson is in use
    #include <Arduino.h>
    #include <assert.h>
    #include <ArduinoJson.h>
    #define HAS_ARDUINOJSON
  #endif

  #if !defined HAS_ARDUINOJSON && __has_include(<ArduinoJson.h>)
    // esp32 __has_include() macro works outside the sketch folder, so it's possible to guess
    #define HAS_ARDUINOJSON
  #endif

#endif


// provide a default String::Stream reader/writer for internals
class StringStream : public Stream
{
public:
  StringStream(String &s) : str(s), pos(0) {}
  virtual ~StringStream() { };
  virtual int available() { return str.length() - pos; }
  virtual int read() { return pos < str.length() ? str[pos++] : -1; }
  virtual int peek() { return pos < str.length() ? str[pos] : -1; }
  virtual void flush() {}
  virtual size_t write(uint8_t c) { str += (char)c; return 1; }
private:
  String &str;
  unsigned int pos;
};


// the base class
class YAMLParser
{
public:
  YAMLParser();
  ~YAMLParser();

  void setOutputStream( Stream* stream ) { _yaml_stream = stream; }

  void load( const char* yaml_or_json_str );
  void load( Stream &yaml_or_json_stream );

  void parse();
  void parse( const char* yaml_or_json_str );
  void parse( Stream &yaml_or_json_stream );

  yaml_document_t* getDocument() { return &document; }
  String getYamlString() { return _yaml_string; }
  size_t bytesWritten() { return _bytes_written; }
  size_t bytesRead()    { return _bytes_read; }

  enum JNestingType_t { NONE, SEQ_KEY, MAP_KEY };
  static void handle_parser_error(yaml_parser_t *parser);
  static void handle_emitter_error(yaml_emitter_t* emitter);
  static void setLogLevel( YAML::LogLevel_t level );
private:
  size_t _bytes_read;
  size_t _bytes_written;
  String _yaml_string;
  Stream *_yaml_stream = nullptr;
  StringStream *_yaml_string_stream_ptr = nullptr;
  void loadDocument();
  yaml_document_t document;
  yaml_parser_t     parser;
};

typedef YAMLParser::JNestingType_t JNestingType_t;

// JSON stream to JsonObject to YAML stream
size_t serializeYml( Stream &json_src_stream, Stream &yml_dest_stream );


#if defined HAS_ARDUINOJSON

  // ArduinoJson friendly functions and derivated class

  #include <ArduinoJson.h>

  // default name for the topmost temporary JsonObject
  #define ROOT_NODE "_root_"
  // deconstructors
  void deserializeYml_JsonObject( yaml_document_t* document, yaml_node_t* yamlNode, JsonObject &jsonNode, JNestingType_t nt=YAMLParser::NONE, const char *nodename=ROOT_NODE, int depth=0 );
  size_t serializeYml_JsonVariant( JsonVariant root, Stream &out, int depth_level, JNestingType_t nt );

  class YAMLToArduinoJson : public YAMLParser
  {
  public:
    YAMLToArduinoJson() {};
    ~YAMLToArduinoJson() { if( _doc) delete _doc; }
    void setJsonDocument( const size_t capacity ) { _doc = new DynamicJsonDocument(capacity); _root = _doc->to<JsonObject>(); }
    JsonObject& getJsonObject() { return _root; }
    void toJson() {
      yaml_node_t * node;
      if( !_doc || _root.isNull() ) {
        YAML_LOG_e("No destination JsonObject defined.");
        return;
      }
      // YAML_LOG_i("JsonDocument capacity: %d (%d/%d yaml r/w)", _doc->capacity(), bytesRead(), bytesWritten() );
      // dafuq is that if( a=b, !a ) notation ??
      if (node = yaml_document_get_root_node(getDocument()), !node) {
        YAML_LOG_e("No document defined.");
        return;
      }
      deserializeYml_JsonObject(getDocument(), node, _root);
    }
    template<typename T>
    JsonObject& toJson( T &yaml )
    {
      load( yaml );
      if( bytesWritten() > 0 ) {
        setJsonDocument( bytesWritten()*2 );
        toJson();
      }
      return _root;
    }
  private:
    DynamicJsonDocument *_doc = nullptr;
    JsonObject _root;

  };

  // ArduinoJSON object to YAML string
  size_t serializeYml( JsonVariant src_obj, String &dest_string );
  // ArduinoJSON object to YAML stream
  size_t serializeYml( JsonVariant src_obj, Stream &dest_stream );

  // Deserialize YAML string to ArduinoJSON document
  DeserializationError deserializeYml( JsonDocument &dest_doc, Stream &src);
  // Deserialize YAML stream to ArduinoJSON document
  DeserializationError deserializeYml( JsonDocument &dest_doc, const char *src);

  // [templated] Deserialize YAML string to ArduinoJSON object
  // DeserializationError deserializeYml( JsonObject &dest_obj, const char* src_yaml_str );
  // [templated] Deserialize YAML stream to ArduinoJSON object
  // DeserializationError deserializeYml( JsonObject &dest_obj, Stream &src_stream );
  template<typename T>
  DeserializationError deserializeYml( JsonObject &dest_obj, T &src)
  {
    static_assert(std::is_same<Stream, T>::value || std::is_same<StringStream, T>::value || std::is_same<const char*, T>::value, "src must be const char* or Stream*");
    YAMLToArduinoJson *parser = new YAMLToArduinoJson();
    JsonObject _dest_obj = parser->toJson( src ); // decode yaml stream/string
    dest_obj = _dest_obj[ROOT_NODE];
    size_t capacity = parser->bytesWritten()*2;
    delete parser;
    if( capacity == 0 ) {
      return DeserializationError::InvalidInput;
    }
    if( dest_obj.isNull() ) {
      return DeserializationError::NoMemory;
    }
    return DeserializationError::Ok;
  }


#endif // HAS_ARDUINOJSON



#if defined HAS_CJSON

  // cJSON friendly functions and derivated class

  #if defined ESP32
    #include <cJSON.h> //  built-in with esp32
  #else
    #include <cJSON/cJSON.h> // bundled with this library
  #endif


  // deconstructors
  cJSON* deserializeYml_cJSONObject(yaml_document_t * document, yaml_node_t * yamlNode);
  size_t serializeYml_cJSONObject( cJSON *root, Stream &out, int depth, JNestingType_t nt );


  class YAMLToCJson : public YAMLParser
  {
  public:
    YAMLToCJson() {};
    ~YAMLToCJson() {};
    cJSON *toJson( yaml_document_t * document ) {
      yaml_node_t * node;
      if (node = yaml_document_get_root_node(document), !node) { YAML_LOG_w("No document defined."); return NULL; }
      return deserializeYml_cJSONObject(document, node);
    };
    cJSON *toJson( const char* yaml_str ) { load( yaml_str );    return toJson( getDocument() ); };
    cJSON* toJson( Stream &yaml_stream )  { load( yaml_stream ); return toJson( getDocument() ); }
  };


  // cJSON object to YAML string
  size_t serializeYml( cJSON* src_obj, String &dest_string );
  // cJSON object to YAML stream
  size_t serializeYml( cJSON* src_obj, Stream &dest_stream );

  // [templated] YAML string to cJSON object
  // int deserializeYml( cJSON* dest_obj, const char* src_yaml_str );
  // [templated] YAML stream to cJSON object
  // int deserializeYml( cJSON* dest_obj, Stream &src_stream );
  template<typename T>
  int deserializeYml( cJSON** dest_obj, T &src_yaml )
  {
    YAMLToCJson *parser = new YAMLToCJson();
    *dest_obj = parser->toJson( src_yaml );
    delete parser;
    return *dest_obj != NULL ? 1 : -1;
  }


#endif // HAS_CJSON

