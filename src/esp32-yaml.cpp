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

#include "esp32-yaml.hpp"

YAMLParser::YAMLParser()
{
  YAML_LOG_n("Current debug level: %s", YAML::logLevelStr() );
  YAML_LOG_e("this is an error message");
  YAML_LOG_w("this is a warning  message");
  YAML_LOG_i("this is an info message");
  YAML_LOG_d("this is a debug message");
  YAML_LOG_v("this is a verbose message");
}


YAMLParser::YAMLParser( const char* yaml_or_json_str )
{
  load( yaml_or_json_str );
}


YAMLParser::~YAMLParser()
{
  yaml_document_delete(&_document);
}


yaml_document_t *YAMLParser::yaml_document()
{
  return &_document;
}


void YAMLParser::setLogLevel( YAML::LogLevel_t level )
{
  YAML::setLogLevel( level );
  YAML::setLoggerFunc( YAML::_LOG ); // re-attach logger
}


void YAMLParser::load( const char* yaml_or_json_str )
{
  assert( yaml_or_json_str );
  YAML_LOG_i("Loading %d bytes", strlen(yaml_or_json_str) );
  if( !yaml_parser_initialize(&_parser) ) {
    handle_parser_error();
    YAML_LOG_e("[FATAL] could not initialize parser");
  } else {
    yaml_parser_set_input_string(&_parser, (const unsigned char*)yaml_or_json_str, strlen(yaml_or_json_str) );
    if (!yaml_parser_load(&_parser, &_document)) {
      handle_parser_error();
      YAML_LOG_e("[FATAL] Failed to load YAML document at line %lu", _parser.problem_mark.line);
      return;
    }
  }
  yaml_parser_delete(&_parser);
  return;
}


void YAMLParser::handle_parser_error()
{
  switch (_parser.error) {
    case YAML_MEMORY_ERROR:
      YAML_LOG_e( "Memory error: Not enough memory for parsing");
    break;
    case YAML_READER_ERROR:
      if (_parser.problem_value != -1) {
        YAML_LOG_e( "[READER ERROR]: %s: #%X at %ld", _parser.problem, _parser.problem_value, (long)_parser.problem_offset);
      } else {
        YAML_LOG_e( "[READER ERROR]: %s at %ld", _parser.problem, (long)_parser.problem_offset);
      }
    break;
    case YAML_SCANNER_ERROR:
      if (_parser.context) {
        YAML_LOG_e( "[SCANNER ERROR]: %s at line %d, column %d\n" "%s at line %d, column %d", _parser.context,
          (int)_parser.context_mark.line+1, (int)_parser.context_mark.column+1, _parser.problem, (int)_parser.problem_mark.line+1, (int)_parser.problem_mark.column+1
        );
      } else {
        YAML_LOG_e( "[SCANNER ERROR]: %s at line %d, column %d", _parser.problem, (int)_parser.problem_mark.line+1, (int)_parser.problem_mark.column+1 );
      }
    break;
    case YAML_PARSER_ERROR:
      if (_parser.context) {
        YAML_LOG_e( "[PARSER ERROR]: %s at line %d, column %d\n" "%s at line %d, column %d", _parser.context,
          (int)_parser.context_mark.line+1, (int)_parser.context_mark.column+1, _parser.problem, (int)_parser.problem_mark.line+1, (int)_parser.problem_mark.column+1
        );
      } else {
        YAML_LOG_e( "[PARSER ERROR]: %s at line %d, column %d", _parser.problem, (int)_parser.problem_mark.line+1, (int)_parser.problem_mark.column+1 );
      }
    break;
    default:
      /* Couldn't happen. */
      YAML_LOG_e( "[INTERNAL ERROR]");
      break;
  }
}



#if __has_include(<ArduinoJson.h>)


  YAMLToArduinoJson::YAMLToArduinoJson(const char* yaml_str)
  {
    toJson(yaml_str);
  }


  YAMLToArduinoJson::~YAMLToArduinoJson()
  {
    if( _doc) delete _doc;
  }

  void YAMLToArduinoJson::setYml( const char* yaml_str )
  {
    load( yaml_str );
    // TODO: better buffer size evaluation
    const size_t CAPACITY = strlen(yaml_str)*2;
    _doc = new DynamicJsonDocument(CAPACITY);
    _root = _doc->to<JsonObject>();
  }

  void YAMLToArduinoJson::toJson()
  {
    yaml_node_t * node;
    if (node = yaml_document_get_root_node(yaml_document()), !node) {
      YAML_LOG_e("No document defined.");
      return;
    }
    YAML_LOG_i("Converting to ArduinoJson");
    toJsonNode(yaml_document(), node, _root);
  };


  JsonObject& YAMLToArduinoJson::toJson( const char* yaml_str )
  {
    setYml( yaml_str );
    toJson();
    return _root;
  }

  void YAMLToArduinoJson::getJsonObject( JsonObject &dest )
  {
    dest = _root;
  }


  JsonObject& YAMLToArduinoJson::getJsonObject()
  {
    return _root;
  }


  void YAMLToArduinoJson::createJSONArray( JsonObject &dest, const char*name, const size_t size )
  {
    if( dest[name].size() == size ) return;
    const size_t CAPACITY = JSON_ARRAY_SIZE( size );
    DynamicJsonDocument arrayDoc(CAPACITY);
    JsonArray array = arrayDoc.createNestedArray();
    dest[name] = array;
  }

  void YAMLToArduinoJson::createJSONObject( JsonObject &dest, const char*name, const size_t size )
  {
    if( dest[name].size() == size ) return;
    const size_t CAPACITY = JSON_OBJECT_SIZE( size );
    DynamicJsonDocument objDoc(CAPACITY);
    JsonObject obj = objDoc.createNestedObject();
    dest[name] = obj;
  }

  JsonObject YAMLToArduinoJson::createNestedObject( const char* name )
  {
    const size_t CAPACITY = JSON_OBJECT_SIZE( 1 );
    DynamicJsonDocument rootDoc(CAPACITY);
    rootDoc.createNestedObject(name);
    JsonObject rootObj = rootDoc.as<JsonObject>();
    return rootObj;
  }


  void YAMLToArduinoJson::toJsonNode( yaml_document_t * document, yaml_node_t * yamlNode, JsonObject &jsonNode, A2JNestingType_t nt, const char *nodename, int depth )
  {
    std::string indent = std::string(depth*2, ' ' );
    switch (yamlNode->type) {
      case YAML_NO_NODE:
        YAML_LOG_v("YAML_NO_NODE");
      break;
      case YAML_SCALAR_NODE:
      {
        double number;
        char * scalar;
        char * end;
        scalar = (char *)yamlNode->data.scalar.value;
        number = strtod(scalar, &end);
        bool is_string = (end == scalar || *end);
        switch( nt ) {
          case SEQ_KEY:
          {
            JsonArray array = jsonNode[nodename];
            if(is_string) array.add( scalar );
            else          array.add( number );
          }
          break;
          case MAP_KEY:
            if(is_string) jsonNode[nodename] = scalar;
            else          jsonNode[nodename] = number;
          break;
          default:
            YAML_LOG_e("Error invalid nesting type");
        }
        YAML_LOG_v("%s YAML_SCALAR_NODE: %s(%s) nodename:%s", indent.c_str(), (end == scalar || *end) ? "string" : "number",  (end == scalar || *end) ? scalar : String(number).c_str(), nodename );
      }
      break;
      case YAML_SEQUENCE_NODE:
      {
        size_t array_size = yamlNode->data.sequence.items.top-yamlNode->data.sequence.items.start;
        YAML_LOG_v("%s YAML_SEQUENCE_NODE: %lu items, parent=%s", indent.c_str(), array_size, A2JNestingTypeStr[nt] );
        createJSONArray( jsonNode, nodename, array_size );
        yaml_node_item_t * item_i;
        for (item_i = yamlNode->data.sequence.items.start; item_i < yamlNode->data.sequence.items.top; ++item_i) {
          yaml_node_t *itemNode = yaml_document_get_node(document, *item_i);
          if( itemNode->type == YAML_MAPPING_NODE ) { // array of anonymous objects
            YAML_LOG_v("%s SEQ_KEY: %d, type=%d, nodename:%s", indent.c_str(), *item_i, itemNode->type, nodename );
            JsonObject rootObj = createNestedObject("root");   // create empty object
            jsonNode[nodename].add( rootObj["root"] );         // append ref to list
            size_t nodeIndex = jsonNode[nodename].size()-1;    // current list index
            JsonObject tmpObj = jsonNode[nodename][nodeIndex]; // alias item to object
            toJsonNode( document, itemNode, tmpObj, SEQ_KEY, "root", depth+1 ); // go recursive
            jsonNode[nodename][nodeIndex] = tmpObj["root"];    // reassign object to anonymous
          } else { // array of sequences or values
            YAML_LOG_v("%s SEQ_KEY: %s, type=%d, nodename:%s", indent.c_str(), (char *)itemNode->data.scalar.value, itemNode->type, nodename );
            toJsonNode( document, itemNode, jsonNode, SEQ_KEY, nodename, depth+1 );
          }
        }
      }
      break;
      case YAML_MAPPING_NODE:
      {
        size_t object_size = yamlNode->data.mapping.pairs.top-yamlNode->data.mapping.pairs.start;
        YAML_LOG_v("%s YAML_MAPPING_NODE: %lu pairs, parent=%s", indent.c_str(), object_size, A2JNestingTypeStr[nt] );
        createJSONObject( jsonNode, nodename, object_size );
        JsonObject tmpObj = jsonNode[nodename];
        if( nt == NONE ) jsonNode = tmpObj; // root object has no parent

        yaml_node_pair_t * pair_i;
        for (pair_i = yamlNode->data.mapping.pairs.start; pair_i < yamlNode->data.mapping.pairs.top; ++pair_i) {
          yaml_node_t * key   = yaml_document_get_node(document, pair_i->key);
          yaml_node_t * value = yaml_document_get_node(document, pair_i->value);
          if (key->type != YAML_SCALAR_NODE) {
            YAML_LOG_w("%s Mapping key is not scalar (line %lu, val=%s).", indent.c_str(), key->start_mark.line, (const char*)value->data.scalar.value);
            continue;
          }
          YAML_LOG_v("%s MAP_KEY: %s, nodename:%s", indent.c_str() ,(char *)key->data.scalar.value, nodename );
          toJsonNode( document, value, tmpObj, MAP_KEY, (const char*)key->data.scalar.value, depth+1 );
        }
      }
      break;
      default:
        YAML_LOG_w("Unknown node type (line %lu).", yamlNode->start_mark.line);
      break;
    }
  }


#endif // __has_include(<ArduinoJson.h>)




#if __has_include(<cJSON.h>)


  YAMLToCJson::YAMLToCJson( const char * yaml_str )
  {
    toJson( yaml_str);
  }


  YAMLToCJson::~YAMLToCJson()
  {
    if(_root) cJSON_Delete(_root);
  }


  void YAMLToCJson::setYml( const char* yaml_str )
  {
    load( yaml_str );
  }


  cJSON * YAMLToCJson::toJson(yaml_document_t * document) {
    yaml_node_t * node;
    if (node = yaml_document_get_root_node(document), !node) {
      YAML_LOG_w("No document defined.");
      return NULL;
    }
    return toJsonNode(document, node);
  }


  cJSON * YAMLToCJson::toJson( const char* yaml_str )
  {
    load( yaml_str );
    YAML_LOG_i("Converting to cJSON");
    _root = toJson( yaml_document() );
    return _root;
  }


  cJSON* YAMLToCJson::getJsonObject()
  {
    return _root;
  }


  cJSON * YAMLToCJson::toJsonNode(yaml_document_t * document, yaml_node_t * node)
  {

      cJSON * object;

      switch (node->type) {
      case YAML_NO_NODE:
        object = cJSON_CreateObject();
      break;
      case YAML_SCALAR_NODE:
      {
        double number;
        char * scalar;
        char * end;
        scalar = (char *)node->data.scalar.value;
        number = strtod(scalar, &end);
        object = (end == scalar || *end) ? cJSON_CreateString(scalar) : cJSON_CreateNumber(number);
      }
      break;
      case YAML_SEQUENCE_NODE:
      {
        object = cJSON_CreateArray();
        yaml_node_item_t * item_i;
        for (item_i = node->data.sequence.items.start; item_i < node->data.sequence.items.top; ++item_i) {
          cJSON_AddItemToArray(object, toJsonNode(document, yaml_document_get_node(document, *item_i)));
        }
      }
      break;
      case YAML_MAPPING_NODE:
      {
        object = cJSON_CreateObject();
        yaml_node_pair_t * pair_i;
        for (pair_i = node->data.mapping.pairs.start; pair_i < node->data.mapping.pairs.top; ++pair_i) {
          yaml_node_t * key = yaml_document_get_node(document, pair_i->key);
          yaml_node_t * value = yaml_document_get_node(document, pair_i->value);

          if (key->type != YAML_SCALAR_NODE) {
            YAML_LOG_w("Mapping key is not scalar (line %lu).", key->start_mark.line);
            continue;
          }

          cJSON_AddItemToObject(object, (char *)key->data.scalar.value, toJsonNode(document, value));
        }
      }
      break;
      default:
        YAML_LOG_w("Unknown node type (line %lu).", node->start_mark.line);
        object = NULL;
      }
      return object;
  }



#endif // __has_include(<cJSON.h>)
