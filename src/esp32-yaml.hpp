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


class YAMLParser
{
public:
  YAMLParser();
  ~YAMLParser();
  YAMLParser( const char* yaml_or_json_str);
  void load( const char* yaml_or_json_str );
  yaml_document_t *yaml_document();
  static void setLogLevel( YAML::LogLevel_t level );
private:
  void handle_parser_error();
  yaml_document_t _document;
  yaml_parser_t _parser;
};


#if __has_include(<ArduinoJson.h>)

  #include <ArduinoJson.h>

  enum JNestingType_t { NONE, SEQ_KEY, MAP_KEY };
  #define indent(x) std::string(x*2, ' ').c_str()

  static void serializeYml( JsonVariant root, Stream &out, int depth_level=0, JNestingType_t nt=NONE )
  {
    int parent_level = depth_level>0?depth_level-1:0;

    if (root.is<JsonArray>()) {
      JsonArray array = root;
      for( int i=0; i<array.size(); i++ ) {
        size_t child_depth_level = array[i].is<JsonObject>() ? depth_level+1 : depth_level-1;
        serializeYml(array[i], out, child_depth_level, SEQ_KEY);
      }
    } else if (root.is<JsonObject>()) {
      JsonObject object = root;
      int i = 0;
      for (JsonPair pair : object) {
        const char* _indent = (i==0 && nt==SEQ_KEY) ? indent(parent_level) : indent(depth_level);
        const char* _prefix = (i==0 && nt==SEQ_KEY) ? "- " : "";
        out.printf("\n%s%s%s: ", _indent, _prefix, pair.key().c_str() );
        serializeYml( pair.value(), out, depth_level+1, MAP_KEY );
        i++;
      }
    } else if( !root.isNull() ) {
      switch(nt) {
        case SEQ_KEY: out.printf("\n%s%s%s",  indent(depth_level), "- ", root.as<String>().c_str() ); break;
        default:  out.printf("%s",  root.as<String>().c_str() ); break;
      }
    } else {
      out.println("Error, unhandled type");
    }
  }


  class YAMLToArduinoJson : public YAMLParser
  {
  public:
    YAMLToArduinoJson() {};
    ~YAMLToArduinoJson();
    YAMLToArduinoJson( const char* yaml_str );
    void setYml( const char* yaml_str );
    void toJson();
    void getJsonObject( JsonObject &dest );
    JsonObject& toJson( const char* yaml_str );
    JsonObject& getJsonObject();
    const char *A2JNestingTypeStr[3] = { "NONE", "SEQ_KEY", "MAP_KEY" };
  private:
    void toJsonNode( yaml_document_t * document, yaml_node_t * yamlNode, JsonObject &jsonNode, JNestingType_t nt=NONE, const char *nodename="", int depth=0 );
    void createJSONArray( JsonObject &dest, const char*name, const size_t size );
    void createJSONObject( JsonObject &dest, const char*name, const size_t size );
    JsonObject createNestedObject( const char* name );
    DynamicJsonDocument *_doc = nullptr;
    JsonObject _root;
  };

#endif



#if __has_include(<cJSON.h>)

  #include <cJSON.h> //  built-in with esp32

  class YAMLToCJson : public YAMLParser
  {
  public:
    YAMLToCJson() {};
    YAMLToCJson( const char * yaml_str );
    ~YAMLToCJson();
    void setYml( const char* yaml_str );
    cJSON *toJson( yaml_document_t * document );
    cJSON *toJson( const char* yaml_str );
    cJSON *getJsonObject();
  private:
    cJSON *toJsonNode( yaml_document_t * document, yaml_node_t * node );
    cJSON *_root = nullptr;
  };

#endif
