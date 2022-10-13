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

#include "ArduinoYaml.hpp"

// used for internals, mainly because yaml_emitter_dump() destroys data after emitting
int yaml_copy_document(yaml_document_t *dest, yaml_document_t *src)
{
  yaml_node_t *node;
  yaml_node_item_t *item;
  yaml_node_pair_t *pair;

  if (!yaml_document_initialize(dest, src->version_directive, src->tag_directives.start, src->tag_directives.end, src->start_implicit, src->end_implicit)) return 0;

  for (node = src->nodes.start; node < src->nodes.top; node ++) {
    switch (node->type) {
      case YAML_SCALAR_NODE:   if (!yaml_document_add_scalar(dest, node->tag, node->data.scalar.value, node->data.scalar.length, node->data.scalar.style)) goto _error; break;
      case YAML_SEQUENCE_NODE: if (!yaml_document_add_sequence(dest, node->tag, node->data.sequence.style)) goto _error; break;
      case YAML_MAPPING_NODE:  if (!yaml_document_add_mapping(dest, node->tag, node->data.mapping.style)) goto _error; break;
      default: assert(0); break;
    }
  }

  for (node = src->nodes.start; node < src->nodes.top; node ++) {
    switch (node->type) {
      case YAML_SEQUENCE_NODE:
        for (item = node->data.sequence.items.start; item < node->data.sequence.items.top; item ++) {
          if (!yaml_document_append_sequence_item(dest, node - src->nodes.start + 1, *item)) goto _error;
        }
      break;
      case YAML_MAPPING_NODE:
        for (pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair ++) {
          if (!yaml_document_append_mapping_pair(dest, node - src->nodes.start + 1, pair->key, pair->value)) goto _error;
        }
      break;
      default: break;
    }
  }
  return 1;

  _error:
    YAML_LOG_e("Error while copying node, deleting document");
    yaml_document_delete(dest);
  return 0;
}


// struct for passing stream
struct yaml_stream_handler_data_t
{
  Stream* streamPtr;
  size_t *size;
};


int _yaml_stream_reader(void *data, unsigned char *buffer, size_t size, size_t *size_read)
{
  yaml_stream_handler_data_t *shd = (yaml_stream_handler_data_t*)data;
  Stream* stream = shd->streamPtr;
  int bytes_read = stream->readBytes( buffer, size );
  if( bytes_read >= 0 ) {
    *size_read = bytes_read;
    *shd->size += bytes_read;
  }
  return bytes_read>=0?1:0;
}


// typedef int yaml_write_handler_t(void *data, unsigned char *buffer, size_t size);
int _yaml_stream_writer(void *data, unsigned char *buffer, size_t size)
{
  yaml_stream_handler_data_t *shd = (yaml_stream_handler_data_t*)data;
  Stream* stream = shd->streamPtr;
  int bytes_written = stream->write( buffer, size );
  if( bytes_written > 0 ) *shd->size += bytes_written;
  return bytes_written > 0 ? 1 : 0;
}


YAMLParser::~YAMLParser()
{
  yaml_parser_delete(&_parser);
  yaml_emitter_delete(&_emitter);
  yaml_document_delete(&_document);
}


void YAMLParser::setLogLevel( YAML::LogLevel_t level )
{
  YAML::setLogLevel( level );
  YAML::setLoggerFunc( YAML::_LOG ); // re-attach logger
}


void YAMLParser::load( Stream &yaml_or_json_stream )
{
  _yaml_string = "";  // reset internal output stream
  _bytes_read = 0;    // length will be known when the stream is consumed
  _bytes_written = 0; // reset yaml output length
  yaml_stream_handler_data_t shd = { &yaml_or_json_stream, &_bytes_read };
  if( !yaml_parser_initialize(&_parser) ) {
    handle_parser_error(&_parser);
    YAML_LOG_e("[FATAL] could not initialize parser");
    goto _parser_delete;
  }
  yaml_parser_set_input(&_parser, &_yaml_stream_reader, &shd);
  loadDocument();
  _parser_delete:
    yaml_parser_delete(&_parser);
}


void YAMLParser::load( const char* yaml_or_json_str )
{
  assert( yaml_or_json_str );
  _yaml_string = "";  // reset internal output stream
  _bytes_read = strlen(yaml_or_json_str); // length is already known
  _bytes_written = 0; // reset yaml output length
  if( !yaml_parser_initialize(&_parser) ) {
    handle_parser_error(&_parser);
    YAML_LOG_e("[FATAL] could not initialize parser");
    goto _parser_delete;
  }
  yaml_parser_set_input_string(&_parser, (const unsigned char*)yaml_or_json_str, _bytes_read );
  loadDocument();
  _parser_delete:
    yaml_parser_delete(&_parser);
}


// called by load(const char*) and load(Stream&)
void YAMLParser::loadDocument()
{
  yaml_document_t document;
  assert(yaml_emitter_initialize(&_emitter));
  yaml_stream_handler_data_t shd = { &_yaml_stream, &_bytes_written };
  yaml_emitter_set_output(&_emitter, &_yaml_stream_writer, &shd);
  yaml_emitter_open(&_emitter);
  if (!yaml_parser_load(&_parser, &document)) {
    handle_parser_error(&_parser);
    YAML_LOG_e("[FATAL] Failed to load YAML document at line %lu", _parser.problem_mark.line);
    goto _emitter_delete;
  }
  assert( yaml_copy_document(&_document, &document) ); // copy into local document for later parsing
  assert( yaml_emitter_dump(&_emitter, &document) ); // dump to emitter for output length evaluation
  _emitter_delete:
    yaml_emitter_close(&_emitter);
    yaml_emitter_delete(&_emitter);
}


void YAMLParser::handle_parser_error(yaml_parser_t *p)
{
  switch (p->error) {
    case YAML_MEMORY_ERROR: YAML_LOG_e( "Memory error: Not enough memory for parsing"); break;
    case YAML_READER_ERROR:
      if (p->problem_value != -1)
        YAML_LOG_e("[R]: %s: #%X at %ld", p->problem, p->problem_value, p->problem_offset);
      else
        YAML_LOG_e("[R]: %s at %ld", p->problem, p->problem_offset);
    break;
    case YAML_SCANNER_ERROR:
      if (p->context)
        YAML_LOG_e("[S]: %s at line %d, column %d\n" "%s at line %d, column %d", p->context, p->context_mark.line+1, p->context_mark.column+1, p->problem, p->problem_mark.line+1, p->problem_mark.column+1 );
      else
        YAML_LOG_e("[S]: %s at line %d, column %d", p->problem, p->problem_mark.line+1, p->problem_mark.column+1 );
    break;
    case YAML_PARSER_ERROR:
      if (p->context)
        YAML_LOG_e("[P]: %s at line %d, column %d\n" "%s at line %d, column %d", p->context, p->context_mark.line+1, p->context_mark.column+1, p->problem, p->problem_mark.line+1, p->problem_mark.column+1 );
      else
        YAML_LOG_e("[P]: %s at line %d, column %d", p->problem, p->problem_mark.line+1, p->problem_mark.column+1 );
    break;
    default: /* Couldn't happen. */ YAML_LOG_e( "[INTERNAL ERROR]"); break;
  }
}


void YAMLParser::handle_emitter_error(yaml_emitter_t *e)
{
  switch (e->error)
  {
    case YAML_MEMORY_ERROR: YAML_LOG_e("[MEMORY ERROR]: Not enough memory for emitting"); break;
    case YAML_WRITER_ERROR: YAML_LOG_e("[WRITER ERROR]: %s", e->problem); break;
    case YAML_EMITTER_ERROR: YAML_LOG_e("[EMITTER ERROR]: %s", e->problem); break;
    default: /* Couldn't happen. */ YAML_LOG_e( "[INTERNAL ERROR]"); break;
  }
}



#if __has_include(<ArduinoJson.h>)


  // yaml_node_t deconstructor => JsonObject
  void deserializeYml_JsonObject( yaml_document_t* document, yaml_node_t* yamlNode, JsonObject &jsonNode, YAMLParser::JNestingType_t nt, const char *nodename, int depth )
  {
    // Prevent "use after free" situations in JsonObject key
    // ArduinoJson only aliases const char* so use char* to force a copy
    char* _nodename = (char*)malloc( strlen(nodename)+1 );
    strcpy(_nodename, nodename);

    switch (yamlNode->type) {
      case YAML_NO_NODE: /*YAML_LOG_v("YAML_NO_NODE");*/ break;
      case YAML_SCALAR_NODE:
      {
        double number;
        char* scalar;
        char* end;
        bool is_string;
        scalar = (char *)yamlNode->data.scalar.value;
        number = strtod(scalar, &end);
        is_string = (end == scalar || *end);
        switch( nt ) {
          case YAMLParser::SEQ_KEY:
          {
            if(is_string) jsonNode[_nodename].add( scalar );
            else          jsonNode[_nodename].add( number );
          }
          break;
          case YAMLParser::MAP_KEY:
            if(is_string) jsonNode[_nodename] = scalar;
            else          jsonNode[_nodename] = number;
          break;
          default: YAML_LOG_e("Error invalid nesting type"); break;
        }
      }
      break;
      case YAML_SEQUENCE_NODE:
      {
        jsonNode.createNestedArray(_nodename);
        yaml_node_item_t * item_i;
        for (item_i = yamlNode->data.sequence.items.start; item_i < yamlNode->data.sequence.items.top; ++item_i) {
          yaml_node_t *itemNode = yaml_document_get_node(document, *item_i);
          if( itemNode->type == YAML_MAPPING_NODE ) { // array of anonymous objects
            JsonObject tmpObj = jsonNode[_nodename].createNestedObject(); // insert empty nested object
            deserializeYml_JsonObject( document, itemNode, tmpObj, YAMLParser::SEQ_KEY, "root", depth+1 ); // go recursive using temporary node name
            jsonNode[_nodename][jsonNode[_nodename].size()-1].set( tmpObj["root"] ); // remove temporary name and make object anonymous
          } else { // array of sequences or values
            deserializeYml_JsonObject( document, itemNode, jsonNode, YAMLParser::SEQ_KEY, _nodename, depth+1 );
          }
        }
      }
      break;
      case YAML_MAPPING_NODE:
      {
        jsonNode.createNestedObject(_nodename);
        JsonObject tmpObj = jsonNode[_nodename];
        if( nt == YAMLParser::NONE ) {
          jsonNode = tmpObj; // topmost object, nodename is empty so apply reparenting
        }
        yaml_node_pair_t* pair_i;
        yaml_node_t* key;
        yaml_node_t* value;
        for (pair_i = yamlNode->data.mapping.pairs.start; pair_i < yamlNode->data.mapping.pairs.top; ++pair_i) {
          key   = yaml_document_get_node(document, pair_i->key);
          value = yaml_document_get_node(document, pair_i->value);
          if (key->type != YAML_SCALAR_NODE) {
            YAML_LOG_w("Mapping key is not scalar (line %lu, val=%s).", key->start_mark.line, (const char*)value->data.scalar.value);
            continue;
          }
          deserializeYml_JsonObject( document, value, tmpObj, YAMLParser::MAP_KEY, (const char*)key->data.scalar.value, depth+1 );
        }
      }
      break;
      default:
        YAML_LOG_w("Unknown node type (line %lu).", yamlNode->start_mark.line);
      break;
    }
    free(_nodename);
  }


  // JsonVariant deconstructor => YAML stream
  size_t serializeYml_JsonVariant( JsonVariant root, Stream &out, int depth, YAMLParser::JNestingType_t nt )
  {
    int parent_level = depth>0?depth-1:0;
    size_t out_size = 0;
    if (root.is<JsonArray>()) {
      JsonArray array = root;
      for( size_t i=0; i<array.size(); i++ ) {
        size_t child_depth = array[i].is<JsonObject>() ? depth+1 : depth-1;
        out_size += serializeYml_JsonVariant(array[i], out, child_depth, YAMLParser::SEQ_KEY);
      }
    } else if (root.is<JsonObject>()) {
      JsonObject object = root;
      int i = 0;
      for (JsonPair pair : object) {
        bool is_seqfirst = (i++==0 && nt==YAMLParser::SEQ_KEY);
        out_size += out.printf("\n%s%s%s: ", is_seqfirst ? indent(parent_level) : indent(depth), is_seqfirst ? "- " : "", pair.key().c_str() );
        out_size += serializeYml_JsonVariant( pair.value(), out, depth+1, YAMLParser::MAP_KEY );
      }
    } else if( !root.isNull() ) {
      switch(nt) {
        case YAMLParser::SEQ_KEY:
          out_size += out.printf("\n%s%s%s",  indent(depth), "- ", root.as<String>().c_str() );
        break;
        default:
          out_size += out.printf("%s",  root.as<String>().c_str() );
        break;
      }
    } else {
      YAML_LOG_e("Error, root is null");
    }
    return out_size;
  }


  size_t serializeYml( JsonVariant src_obj, String &dest_string )
  {
    StringStream dest_stream( dest_string );
    return serializeYml_JsonVariant( src_obj, dest_stream, 0, YAMLParser::NONE );
  }


  size_t serializeYml( JsonVariant src_obj, Stream &dest_stream )
  {
    return serializeYml_JsonVariant( src_obj, dest_stream, 0, YAMLParser::NONE );
  }

  #if defined USE_STREAM_TO_STREAM
    size_t serializeYml( Stream &json_src_stream, Stream &yaml_dest_stream )
    {
      JsonObject src_obj;
      if ( deserializeYml( src_obj, json_src_stream ) != DeserializationError::Ok ) {
        YAML_LOG_e("unable to deserialize to temporary JsonObject, aborting");
        return 0;
      }
      return serializeYml_JsonVariant( src_obj, yaml_dest_stream, 0, YAMLParser::NONE );
    }
  #endif


  DeserializationError deserializeYml( JsonDocument &dest_doc, Stream &src)
  {
    YAMLToArduinoJson *parser = new YAMLToArduinoJson();
    JsonObject tmpObj = parser->toJson( src ); // decode yaml stream/string
    dest_doc.set( tmpObj );
    delete parser;
    return DeserializationError::Ok;
  }


  DeserializationError deserializeYml( JsonDocument &dest_doc, const char *src)
  {
    YAMLToArduinoJson *parser = new YAMLToArduinoJson();
    JsonObject tmpObj = parser->toJson( src ); // decode yaml stream/string
    dest_doc.set( tmpObj );
    delete parser;
    return DeserializationError::Ok;
  }


#endif // __has_include(<ArduinoJson.h>)





#if __has_include(<cJSON.h>)


  // yaml_node_t deconstructor => cJSON Object
  cJSON* deserializeYml_cJSONObject(yaml_document_t * document, yaml_node_t * yamlNode)
  {
    assert( yamlNode );
    assert( document );
    cJSON * object;

    switch (yamlNode->type) {
      case YAML_NO_NODE:
        object = cJSON_CreateObject();
      break;
      case YAML_SCALAR_NODE:
      {
        double number;
        char * scalar;
        char * end;
        scalar = (char *)yamlNode->data.scalar.value;
        number = strtod(scalar, &end);
        object = (end == scalar || *end) ? cJSON_CreateString(scalar) : cJSON_CreateNumber(number);
      }
      break;
      case YAML_SEQUENCE_NODE:
      {
        object = cJSON_CreateArray();
        yaml_node_item_t * item_i;
        for (item_i = yamlNode->data.sequence.items.start; item_i < yamlNode->data.sequence.items.top; ++item_i) {
          cJSON_AddItemToArray(object, deserializeYml_cJSONObject(document, yaml_document_get_node(document, *item_i)));
        }
      }
      break;
      case YAML_MAPPING_NODE:
      {
        object = cJSON_CreateObject();
        yaml_node_pair_t * pair_i;
        for (pair_i = yamlNode->data.mapping.pairs.start; pair_i < yamlNode->data.mapping.pairs.top; ++pair_i) {
          yaml_node_t * key = yaml_document_get_node(document, pair_i->key);
          yaml_node_t * value = yaml_document_get_node(document, pair_i->value);

          if (key->type != YAML_SCALAR_NODE) {
            YAML_LOG_w("Mapping key is not scalar (line %lu).", key->start_mark.line);
            continue;
          }
          cJSON_AddItemToObject(object, (char *)key->data.scalar.value, deserializeYml_cJSONObject(document, value));
        }
      }
      break;
      default:
        YAML_LOG_w("Unknown node type (line %lu).", yamlNode->start_mark.line);
        object = NULL;
    }
    return object;
  }


  // cJSON deconstructor => YAML stream
  size_t serializeYml_cJSONObject( cJSON *root, Stream &out, int depth, YAMLParser::JNestingType_t nt )
  {
    assert(root);
    int parent_level = depth>0?depth-1:0;
    size_t out_size = 0;

    if ( root->type == cJSON_Array ) {
      cJSON *current_element = root->child;
      while (current_element != NULL) {
        size_t child_depth = current_element->type == cJSON_Object ? depth+1 : depth-1;
        out_size += serializeYml_cJSONObject( current_element, out, child_depth, YAMLParser::SEQ_KEY );
        current_element = current_element->next;
      }
    } else if (root->type == cJSON_Object ) {
      cJSON *current_item = root->child;
      int i = 0;
      while (current_item) {
        bool is_seqfirst = (i++==0 && nt==YAMLParser::SEQ_KEY);
        char* key = current_item->string;
        out_size += out.printf("\n%s%s%s: ", is_seqfirst ? indent(parent_level) : indent(depth), is_seqfirst ? "- " : "", key );
        out_size += serializeYml_cJSONObject( current_item, out, depth+1, YAMLParser::MAP_KEY );
        current_item = current_item->next;
      }
    } else {
      char *value = cJSON_PrintUnformatted( root );
      if( !value ) {
        YAML_LOG_e("node has no value!");
        return 0;
      }
      switch(nt) {
        case YAMLParser::SEQ_KEY:
          out_size += out.printf("\n%s%s%s",  indent(depth), "- ", value );
        break;
        default:
          out_size += out.printf("%s",  value );
        break;
      }
      free( value );
    }
    return out_size;
  }


  // cJSON object to YAML string
  size_t serializeYml( cJSON* src_obj, String &dest_string )
  {
    assert( src_obj );
    StringStream dest_stream( dest_string );
    return serializeYml_cJSONObject( src_obj, dest_stream, 0, YAMLParser::NONE );
  }


  // cJSON object to YAML stream
  size_t serializeYml( cJSON* src_obj, Stream &dest_stream )
  {
    assert( src_obj );
    return serializeYml_cJSONObject( src_obj, dest_stream, 0, YAMLParser::NONE );
  }


#endif // __has_include(<cJSON.h>)