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


#include "logger.hpp"

extern "C" {
  #include "libyaml/yaml.h" // https://github.com/yaml/libyaml
}


class StringStream : public Stream
{
public:
  StringStream(String &s) : str(s), pos(0) {}
  virtual int available() { return str.length() - pos; }
  virtual int read() { return pos < str.length() ? str[pos++] : -1; }
  virtual int peek() { return pos < str.length() ? str[pos] : -1; }
  virtual void flush() {}
  virtual size_t write(uint8_t c) { str += (char)c; return 1; }
private:
  String &str;
  //unsigned int length = 0;
  unsigned int pos;
};



class YAMLParser
{
public:
  YAMLParser() { };
  ~YAMLParser();
  void load( const char* yaml_or_json_str );
  void load( Stream &yaml_or_json_stream );
  yaml_document_t* getDocument() { return &_document; }
  static void setLogLevel( YAML::LogLevel_t level );
  size_t bytesWritten() { return _bytes_written; }
  size_t bytesRead()    { return _bytes_read; }
  String getYamlString() { return _yaml_string; }
  enum JNestingType_t { NONE, SEQ_KEY, MAP_KEY };
private:
  size_t _bytes_read;
  size_t _bytes_written;
  String _yaml_string;
  StringStream _yaml_stream = StringStream(_yaml_string);
  void loadDocument();
  void handle_parser_error();
  void handle_emitter_error();
  yaml_document_t _document;
  yaml_emitter_t _emitter;
  yaml_parser_t _parser;
  yaml_event_t _event;
};




#if __has_include(<ArduinoJson.h>)

  #include <ArduinoJson.h>

  void deserializeYml_JsonObject( yaml_document_t* document, yaml_node_t* yamlNode, JsonObject &jsonNode, YAMLParser::JNestingType_t nt=YAMLParser::NONE, const char *nodename="", int depth=0 );
  size_t serializeYml_JsonVariant( JsonVariant root, Stream &out, int depth_level, YAMLParser::JNestingType_t nt );

  class YAMLToArduinoJson : public YAMLParser
  {
  public:
    YAMLToArduinoJson() {};
    ~YAMLToArduinoJson() { if( _doc) delete _doc; };
    void setJsonDocument( const size_t capacity ) { _doc = new DynamicJsonDocument(capacity); _root = _doc->to<JsonObject>(); };
    JsonObject& getJsonObject() { return _root; }
    void toJson() {
      yaml_node_t * node;
      if( !_doc || _root.isNull() ) {
        YAML_LOG_e("No destination JsonObject defined.");
        return;
      }
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
  // JSON stream to JsonObject to YAML stream
  size_t serializeYml( Stream &json_src_stream, Stream &yml_dest_stream );
  // Deserialize YAML string to ArduinoJSON object
  // DeserializationError deserializeYml( JsonObject &dest_obj, const char* src_yaml_str );
  // Deserialize YAML stream to ArduinoJSON object
  // DeserializationError deserializeYml( JsonObject &dest_obj, Stream &src_stream );
  template<typename T>
  DeserializationError deserializeYml( JsonObject &dest_obj, T &src)
  {
    YAMLToArduinoJson *parser = new YAMLToArduinoJson();
    JsonObject tmpObj = parser->toJson( src ); // decode yaml stream/string
    size_t capacity = parser->bytesWritten()*2;
    if( capacity ==0 ) {
      delete parser;
      return DeserializationError::InvalidInput;
    }
    DynamicJsonDocument tmpDoc( capacity ); // prepare object copy
    tmpDoc.set( tmpObj ); // copy values to temporary document
    dest_obj = tmpDoc.as<JsonObject>(); // copy temporary document into destination object
    delete parser;
    if( dest_obj.isNull() ) {
      return DeserializationError::NoMemory;
    }
    return DeserializationError::Ok;
  }


#endif



#if __has_include(<cJSON.h>)

  #include <cJSON.h> //  built-in with esp32

  cJSON* deserializeYml_cJSONObject(yaml_document_t * document, yaml_node_t * yamlNode);
  size_t serializeYml_cJSONObject( cJSON *root, Stream &out, int depth, YAMLParser::JNestingType_t nt );


  class YAMLToCJson : public YAMLParser
  {
  public:
    YAMLToCJson() {};
    ~YAMLToCJson() { if(_root) cJSON_Delete(_root); };
    //void toJson();
    cJSON *toJson( yaml_document_t * document ){
      yaml_node_t * node;
      if (node = yaml_document_get_root_node(document), !node) { YAML_LOG_w("No document defined."); return NULL; }
      return deserializeYml_cJSONObject(document, node);
    };
    cJSON *toJson( const char* yaml_str ){ load( yaml_str ); return toJson( getDocument() ); };
    cJSON* toJson( Stream &yaml_stream ) { load(yaml_stream); return toJson( getDocument() ); }
    cJSON *getJsonObject() { return _root; };
  private:
    cJSON *_root = nullptr;
  };


  // cJSON object to YAML string
  size_t serializeYml( cJSON* src_obj, String &dest_string );
  // cJSON object to YAML stream
  size_t serializeYml( cJSON* src_obj, Stream &dest_stream );

  // YAML string to cJSON object
  // int deserializeYml( cJSON* dest_obj, const char* src_yaml_str );
  // YAML stream to cJSON object
  // int deserializeYml( cJSON* dest_obj, Stream &src_stream );

  // YAML string/stream to cJSON object
  template<typename T>
  int deserializeYml( cJSON* dest_obj, T &src_yaml )
  {
    YAMLToCJson *parser = new YAMLToCJson();
    cJSON* _dest_obj = parser->toJson( src_yaml );
    *dest_obj = *_dest_obj;
    delete parser;
    return dest_obj ? 1 : -1;
  }


#endif



// this macro does not like to be defined early (especially before ArduinoJson.h is included)
#define indent(indent_size) std::string(indent_size*2, ' ').c_str()
