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

extern "C" {
  #include "libyaml/yaml.h" // https://github.com/yaml/libyaml
}


class YAMLParser
{
public:
  YAMLParser() {};
  ~YAMLParser();
  YAMLParser( const char* yaml_str);
  void load( const char* yaml_str );
  void handle_parser_error();
  yaml_document_t *yaml_document();
private:
  yaml_document_t _document;
  yaml_parser_t _parser;
};


#if __has_include(<ArduinoJson.h>)

  #include <ArduinoJson.h>

  class YAMLToArduinoJson : public YAMLParser
  {
  public:
    YAMLToArduinoJson() {};
    ~YAMLToArduinoJson();
    YAMLToArduinoJson( const char* yaml_str );
    void toJson();
    void setYml( const char* yaml_str );
    void getJsonObject( JsonObject &dest );
    JsonObject& toJson( const char* yaml_str );
    JsonObject& getJsonObject();
    enum A2JNestingType_t { NONE, SEQ_KEY, MAP_KEY };
  private:
    void yaml2json_node( yaml_document_t * document, yaml_node_t * yamlNode, JsonObject &jsonNode, A2JNestingType_t nt=NONE, const char *nodename="", int depth=0 );
    void createJSONArray( JsonObject &dest, const char*name, const size_t size );
    void createJSONObject( JsonObject &dest, const char*name, const size_t size );
    DynamicJsonDocument *_doc = nullptr;
    JsonObject _root;
  };

#endif // __has_include(<ArduinoJson.h>)



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

#endif // __has_include(<cJSON.h>)
