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

// shorthand to libyaml scalar values
#define SCALAR_c(x) (const char*)x->data.scalar.value
#define SCALAR_s(x)       (char*)x->data.scalar.value

// indent to string
String _indent_str;
const char* indent( int size )
{
  if( size<=0 ) return "";
  _indent_str = "";
  for( int i=0;i<size;i++ ) _indent_str += "  ";
  return _indent_str.c_str();
}



// YAML is very inclusive with booleans :-)
// https://yaml.org/type/bool.html

bool string_has_truthy_value( String &_scalar )
{
  return _scalar == "y"    ||  _scalar == "Y"    ||  _scalar == "yes" || _scalar == "Yes" || _scalar == "YES"
     ||  _scalar == "true" ||  _scalar == "True" ||  _scalar == "TRUE"
     ||  _scalar == "on"   ||  _scalar == "On"   ||  _scalar == "ON";
}


bool string_has_falsy_value( String &_scalar )
{
  return _scalar == "n"     ||  _scalar == "N"     ||  _scalar == "no" || _scalar == "No" || _scalar == "NO"
     ||  _scalar == "false" ||  _scalar == "False" ||  _scalar == "FALSE"
     ||  _scalar == "off"   ||  _scalar == "Off"   ||  _scalar == "OFF";
}


bool string_has_bool_value( String &_scalar, bool *value_out )
{
  bool has_truthy = string_has_truthy_value(_scalar);
  bool has_falsy  = string_has_falsy_value(_scalar);
  if( has_truthy || has_falsy ) {
    *value_out = has_truthy;
    return true;
  }
  return false;
}


bool yaml_node_is_bool( yaml_node_t * yamlNode, bool *value_out )
{
  switch (yamlNode->data.scalar.style) {
    case YAML_PLAIN_SCALAR_STYLE:
    {
      String _scalar = String( (char*)yamlNode->data.scalar.value );
      if( string_has_bool_value(_scalar, value_out) ) {
        return true;
      }
    }
    break;
    case YAML_SINGLE_QUOTED_SCALAR_STYLE: /*YAML_LOG_e(" '")*/; break;
    case YAML_DOUBLE_QUOTED_SCALAR_STYLE: /*YAML_LOG_e(" \"")*/; break;
    case YAML_LITERAL_SCALAR_STYLE: /*YAML_LOG_e(" |")*/; break;
    case YAML_FOLDED_SCALAR_STYLE: /*YAML_LOG_e(" >")*/; break;
    case YAML_ANY_SCALAR_STYLE: /*abort(); */ break;
  }
  return false;
}


void yaml_multiline_escape_string( Stream* stream, const char* str, size_t length, size_t *bytes_out, size_t depth )
{
  size_t i;
  char c;
  char l = '\0';

  String _str = String(str);
  bool has_multiline = _str.indexOf('\n') > -1;
  if( has_multiline ) *bytes_out += stream->printf("|\n%s",  indent(depth) );

  for (i = 0; i < length; i++) {
    c = *(str + i);
    if( c== '\r' || c=='\n' ) {
      if( l != '\\' && has_multiline ) { // unescaped \r or \n
        if(c == '\r') *bytes_out += 1;  // ignore \r
        else          *bytes_out += stream->printf("\n%s",  indent(depth) ); // print CRLF
      } else {
        if(c == '\n') *bytes_out += stream->printf("\\n");
        else          *bytes_out += stream->printf("\\r");
      }
    } else {
      if( has_multiline ) {
        *bytes_out += stream->printf("%c", c);
      } else {
        if      (c == '\\') *bytes_out += stream->printf("\\\\");
        else if (c == '\0') *bytes_out += stream->printf("\\0");
        else if (c == '\b') *bytes_out += stream->printf("\\b");
        else if (c == '\t') *bytes_out += stream->printf("\\t");
        else                *bytes_out += stream->printf("%c", c);
      }
    }
    l = c; // memoize last char to spot escaped entities
  }
}


// when escaping to JSON
void yaml_escape_quoted_string( Stream* stream, const char* str, size_t length, size_t *bytes_out )
{
  size_t i;
  char c;
  for (i = 0; i < length; i++) {
    c = *(str + i);
    if      (c == '\\') *bytes_out += stream->printf("\\\\");
    else if (c == '\0') *bytes_out += stream->printf("\\0");
    else if (c == '\b') *bytes_out += stream->printf("\\b");
    else if (c == '\n') *bytes_out += stream->printf("\\n");
    else if (c == '\r') *bytes_out += stream->printf("\\r");
    else if (c == '\t') *bytes_out += stream->printf("\\t");
    else                *bytes_out += stream->printf("%c", c);
  }
}



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


// struct for recursively parsing yaml_document
struct yaml_traverser_t
{
  yaml_document_t* document;
  yaml_node_t* node;
  Stream* stream;
  JNestingType_t type;
  int depth;
};



// stream reader callback
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


// dummy writer callback to count bytes
int _yaml_stream_dummy_writer(void *data, __attribute__((unused)) unsigned char *buffer, size_t size)
{
  yaml_stream_handler_data_t *shd = (yaml_stream_handler_data_t*)data;
  *shd->size += size;
  return size > 0 ? 1 : 0;
}


// stream writer callback
int _yaml_stream_writer(void *data, unsigned char *buffer, size_t size)
{
  yaml_stream_handler_data_t *shd = (yaml_stream_handler_data_t*)data;
  Stream* stream = shd->streamPtr;
  int bytes_written = stream->write( buffer, size );
  if( bytes_written > 0 ) *shd->size += bytes_written;
  return bytes_written > 0 ? 1 : 0;
}



size_t deserialize_YamlDocument( yaml_traverser_t *it )
{
  assert( it );
  assert( it->node );
  assert( it->document );
  assert( it->stream );
  // just some aliasing
  auto      node = it->node;
  auto  document = it->document;
  auto    stream = it->stream;
  auto     depth = it->depth;
  auto nest_type = it->type;

  size_t bytes_out = 0;
  int i = 0, parent_level = depth>0 ? depth-1 : 0;

  switch (node->type) {
    case YAML_SCALAR_NODE:
      if ( nest_type == YAMLParser::SEQ_KEY ) bytes_out += stream->printf("\n%s%s",  indent(depth), "- " );
      yaml_multiline_escape_string( stream, SCALAR_c(node), strlen(SCALAR_c(node)), &bytes_out, depth );
    break;
    case YAML_SEQUENCE_NODE:
      for (auto item_i = node->data.sequence.items.start; item_i < node->data.sequence.items.top; ++item_i) {
        auto node_item = yaml_document_get_node(document, *item_i);
        int child_level = node_item->type == YAML_MAPPING_NODE ? depth+1 : depth-1;
        yaml_traverser_t seq_item = { document, yaml_document_get_node(document, *item_i), stream, YAMLParser::SEQ_KEY, child_level };
        bytes_out += deserialize_YamlDocument( &seq_item );
      }
    break;
    case YAML_MAPPING_NODE:
      for (auto pair_i = node->data.mapping.pairs.start; pair_i < node->data.mapping.pairs.top; ++pair_i) {
        bool is_seqfirst = ( i++==0 && nest_type == YAMLParser::SEQ_KEY );
        auto key   = yaml_document_get_node(document, pair_i->key);
        auto value = yaml_document_get_node(document, pair_i->value);
        if (key->type != YAML_SCALAR_NODE) {
          YAML_LOG_w("Mapping key is not scalar (line %lu).", key->start_mark.line);
          continue;
        }
        yaml_traverser_t map_item = { document, value, stream, YAMLParser::MAP_KEY, depth+1 };
        bytes_out += stream->printf("\n%s%s%s: ", is_seqfirst ? indent(parent_level) : indent(depth), is_seqfirst ? "- " : "", SCALAR_c(key) );
        bytes_out += deserialize_YamlDocument( &map_item );
      }
    break;
    case YAML_NO_NODE: break;
    default: YAML_LOG_w("Unknown node type (line %lu).", node->start_mark.line); break;
  }
  return bytes_out;
}



// pure libyaml JSON->YAML stream-to-stream seralization
size_t serializeYml( Stream &stream_in, Stream &stream_out )
{
  YAMLParser* parser = new YAMLParser();
  parser->setOutputStream( &stream_out );
  parser->parse( stream_in );
  auto ret = parser->bytesWritten();
  delete parser;
  return ret;
}



YAMLParser::YAMLParser()
{
  _yaml_string = "";
  _yaml_string_stream_ptr = new StringStream(_yaml_string);
  _yaml_stream = _yaml_string_stream_ptr;
}


YAMLParser::~YAMLParser()
{
  yaml_parser_delete(&parser);
  yaml_document_delete(&document);
  if( _yaml_string_stream_ptr ) delete _yaml_string_stream_ptr;
  _yaml_string = "";
}


void YAMLParser::setLogLevel( YAML::LogLevel_t level )
{
  YAML::setLogLevel( level );
  YAML::setLoggerFunc( YAML::_LOG ); // re-attach logger
}


void YAMLParser::parse()
{
  yaml_node_t* node;
  if (node = yaml_document_get_root_node(&document), !node) {
    YAML_LOG_w("No document defined.");
    return;
  }

  yaml_traverser_t doc = { &document, node, _yaml_stream, YAMLParser::NONE, 0 };
  size_t bytes_out = deserialize_YamlDocument( &doc );

  YAML_LOG_d("written %d bytes", bytes_out );
}


void YAMLParser::parse( Stream &yaml_or_json_stream  )
{
  if (!yaml_parser_initialize(&parser)) {
    YAML_LOG_e("Failed to initialize parser!\n", stderr);
    return;
  }

  yaml_stream_handler_data_t shd = { &yaml_or_json_stream, &_bytes_read };
  yaml_parser_set_input(&parser, &_yaml_stream_reader, &shd);

  if (!yaml_parser_load(&parser, &document)) {
    handle_parser_error(&parser);
    return;
  }

  yaml_node_t* node;
  if (node = yaml_document_get_root_node(&document), !node) {
    YAML_LOG_w("No document defined.");
    return;
  }

  yaml_traverser_t doc = { &document, node, _yaml_stream, YAMLParser::NONE, 0 };
  size_t bytes_out = deserialize_YamlDocument( &doc );

  YAML_LOG_d("written %d bytes", bytes_out );
}



void YAMLParser::parse( const char* yaml_or_json_str )
{
  assert( yaml_or_json_str );

  if (!yaml_parser_initialize(&parser)) {
    YAML_LOG_e("Failed to initialize parser!\n", stderr);
    return;
  }

  yaml_parser_set_input_string(&parser, (const unsigned char*)yaml_or_json_str, strlen(yaml_or_json_str) );

  if (!yaml_parser_load(&parser, &document)) {
    handle_parser_error(&parser);
    return;
  }

  yaml_node_t* node;
  if (node = yaml_document_get_root_node(&document), !node) {
    YAML_LOG_w("No document defined.");
    return;
  }

  yaml_traverser_t doc = { &document, node, _yaml_stream, YAMLParser::NONE, 0 };
  size_t bytes_out = deserialize_YamlDocument( &doc );

  YAML_LOG_d("written %d bytes", bytes_out );
}


void YAMLParser::load( Stream &yaml_or_json_stream )
{
  _yaml_string = "";  // reset internal output stream
  _bytes_read = 0;    // length will be known when the stream is consumed
  _bytes_written = 0; // reset yaml output length

  if ( !yaml_parser_initialize(&parser) ) {
    handle_parser_error(&parser);
    YAML_LOG_e("[FATAL] could not initialize parser");
    return;
  }

  yaml_stream_handler_data_t shd = { &yaml_or_json_stream, &_bytes_read };
  yaml_parser_set_input(&parser, &_yaml_stream_reader, &shd);
  loadDocument();
}


void YAMLParser::load( const char* yaml_or_json_str )
{
  assert( yaml_or_json_str );
  _yaml_string = "";  // reset internal output stream
  _bytes_read = strlen(yaml_or_json_str); // length is already known
  _bytes_written = 0; // reset yaml output length
  if ( !yaml_parser_initialize(&parser) ) {
    handle_parser_error(&parser);
    YAML_LOG_e("[FATAL] could not initialize parser");
    return;
  }
  yaml_parser_set_input_string(&parser, (const unsigned char*)yaml_or_json_str, _bytes_read );
  loadDocument();
}


// called by load(const char*) and load(Stream&)
void YAMLParser::loadDocument()
{
  yaml_document_t _tmpdoc;
  yaml_emitter_t emitter;
  assert(yaml_emitter_initialize(&emitter));
  yaml_stream_handler_data_t shd = { nullptr, &_bytes_written };
  yaml_emitter_set_canonical(&emitter, 1);
  yaml_emitter_set_unicode(&emitter, 1);
  yaml_emitter_set_output(&emitter, &_yaml_stream_dummy_writer, &shd);
  yaml_emitter_open(&emitter);
  if (!yaml_parser_load(&parser, &document)) {
    handle_parser_error(&parser);
    YAML_LOG_e("[FATAL] Failed to load YAML document at line %lu", parser.problem_mark.line);
    goto _emitter_delete;
  }
  assert( yaml_copy_document(&_tmpdoc, &document) ); // copy into local document for later parsing
  assert( yaml_emitter_dump(&emitter, &_tmpdoc) ); // dump to emitter for input length evaluation
  yaml_document_delete(&_tmpdoc);

  _emitter_delete:
    yaml_emitter_close(&emitter);
    yaml_emitter_delete(&emitter);
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
  switch (e->error) {
    case YAML_MEMORY_ERROR: YAML_LOG_e("[MEMORY ERROR]: Not enough memory for emitting"); break;
    case YAML_WRITER_ERROR: YAML_LOG_e("[WRITER ERROR]: %s", e->problem); break;
    case YAML_EMITTER_ERROR: YAML_LOG_e("[EMITTER ERROR]: %s", e->problem); break;
    default: /* Couldn't happen. */ YAML_LOG_e( "[INTERNAL ERROR]"); break;
  }
}



#if defined HAS_ARDUINOJSON


  // yaml_node_t deconstructor => JsonObject
  void deserializeYml_JsonObject( yaml_document_t* document, yaml_node_t* yamlNode, JsonObject &jsonNode, JNestingType_t nt, const char *nodename, int depth )
  {
    switch (yamlNode->type) {
      case YAML_SCALAR_NODE:
      {
        double number;
        char* scalar;
        char* end;
        scalar = (char *)yamlNode->data.scalar.value;
        number = strtod(scalar, &end);
        bool is_bool = false;
        bool bool_value = false;
        bool is_string = (end == scalar || *end);
        if( is_string && yaml_node_is_bool( yamlNode, &bool_value ) ) {
          is_bool = true;
        }
        switch( nt ) {
          case YAMLParser::SEQ_KEY:
          {
            JsonArray array = jsonNode[nodename];
            if(is_bool)        array.add( bool_value );
            else if(is_string) array.add( scalar );
            else               array.add( number );
            //YAML_LOG_d("[SEQ][%s][%d] => %s(%s)", nodename, array.size()-1, is_string?"string":"number", is_string?scalar:String(number).c_str() );
          }
          break;
          case YAMLParser::MAP_KEY:
            if(is_bool)        jsonNode[nodename] = bool_value;
            else if(is_string) jsonNode[nodename] = scalar;
            else               jsonNode[nodename] = number;
            //YAML_LOG_d("[MAP][%d][%s] => %s(%s)", jsonNode.size()-1, nodename, is_string?"string":"number", is_string?scalar:String(number).c_str() );
          break;
          default: YAML_LOG_e("Error invalid nesting type"); break;
        }
      }
      break;
      case YAML_SEQUENCE_NODE:
      {
        JsonArray tmpArray = jsonNode.createNestedArray((char*)nodename);
        yaml_node_item_t * item_i;
        yaml_node_t *itemNode;
        String _nodeItemName;
        JsonObject tmpObj;
        for (item_i = yamlNode->data.sequence.items.start; item_i < yamlNode->data.sequence.items.top; ++item_i) {
          itemNode = yaml_document_get_node(document, *item_i);
          if( itemNode->type == YAML_MAPPING_NODE ) { // array of anonymous objects
            tmpObj = tmpArray.createNestedObject(); // insert empty nested object
            _nodeItemName = ROOT_NODE + String( nodename ) + String( tmpArray.size() ); // generate a temporary nodename
            tmpObj.createNestedObject((char*)_nodeItemName.c_str());
            deserializeYml_JsonObject( document, itemNode, tmpObj, YAMLParser::SEQ_KEY, _nodeItemName.c_str(), depth+1 ); // go recursive using temporary node name
            jsonNode[nodename][tmpArray.size()-1] = tmpObj[_nodeItemName.c_str()]; // remove temporary name and make object anonymous
          } else { // array of sequences or values
            _nodeItemName = "" + String( nodename );
            deserializeYml_JsonObject( document, itemNode, jsonNode, YAMLParser::SEQ_KEY, _nodeItemName.c_str(), depth+1 );
          }
        }
        //YAML_LOG_d("[ARR][%s] has %d items", nodename, tmpArray.size() );
      }
      break;
      case YAML_MAPPING_NODE:
      {
        JsonObject tmpNode = jsonNode.createNestedObject((char*)nodename);
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
          tmpNode.createNestedObject((char*)key->data.scalar.value);
          deserializeYml_JsonObject( document, value, tmpNode, YAMLParser::MAP_KEY, (const char*)key->data.scalar.value, depth+1 );
        }
      }
      break;
      case YAML_NO_NODE: YAML_LOG_e("YAML_NO_NODE"); break;
      default: YAML_LOG_w("Unknown node type (line %lu).", yamlNode->start_mark.line); break;
    }
  }


  // JsonVariant deconstructor => YAML stream
  size_t serializeYml_JsonVariant( JsonVariant root, Stream &out, int depth, JNestingType_t nt )
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
      if( nt == YAMLParser::SEQ_KEY ) out_size += out.printf("\n%s%s",  indent(depth), "- " );
      yaml_multiline_escape_string(&out, root.as<String>().c_str(), root.as<String>().length(), &out_size, depth);
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


  DeserializationError deserializeYml( JsonDocument &dest_doc, Stream &src )
  {
    YAMLToArduinoJson *parser = new YAMLToArduinoJson();
    JsonObject tmpObj = parser->toJson( src ); // decode yaml stream/string
    DeserializationError ret = DeserializationError::Ok;
    if( !dest_doc.set( tmpObj[ROOT_NODE] ) ) {
      ret = DeserializationError::NoMemory;
    }
    delete parser;
    return DeserializationError::Ok;
  }


  DeserializationError deserializeYml( JsonDocument &dest_doc, const char *src )
  {
    YAMLToArduinoJson *parser = new YAMLToArduinoJson();
    JsonObject tmpObj = parser->toJson( src ); // decode yaml stream/string
    DeserializationError ret = DeserializationError::Ok;
    if( !dest_doc.set( tmpObj[ROOT_NODE] ) ) {
      ret = DeserializationError::NoMemory;
    }
    delete parser;
    return DeserializationError::Ok;
  }


#endif // HAS_ARDUINOJSON





#if defined HAS_CJSON


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
        if( (end == scalar || *end) ) { // string or bool
          bool bool_value;
          if( yaml_node_is_bool( yamlNode, &bool_value ) ) {
            object = cJSON_CreateBool( bool_value );
          } else {
            object = cJSON_CreateString( scalar );
          }
        } else object = cJSON_CreateNumber(number); // leaky !!
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
        yaml_node_pair_t* pair_i;
        yaml_node_t* key;
        yaml_node_t* value;
        for (pair_i = yamlNode->data.mapping.pairs.start; pair_i < yamlNode->data.mapping.pairs.top; ++pair_i) {
          key   = yaml_document_get_node(document, pair_i->key);
          value = yaml_document_get_node(document, pair_i->value);
          if (key->type != YAML_SCALAR_NODE) {
            YAML_LOG_w("Mapping key is not scalar (line %lu).", key->start_mark.line);
            continue;
          }
          cJSON_AddItemToObject(object, (const char *)key->data.scalar.value, deserializeYml_cJSONObject(document, value));
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
  size_t serializeYml_cJSONObject( cJSON *root, Stream &out, int depth, JNestingType_t nt )
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
      char *value = root->valuestring;
      bool value_needs_free = false; // at this point value is just a pointer to root object property
      if( !value ) { // not a string, maybe number or a bool
        value = cJSON_PrintUnformatted( root ); // now the value has been malloc'ed, will need free
        value_needs_free = true;
        if( !value ) {
          YAML_LOG_e("node has no value!");
          return 0;
        }
      }
      size_t value_len = strlen(value);
      if( nt == YAMLParser::SEQ_KEY ) out_size += out.printf("\n%s%s",  indent(depth), "- " );
      yaml_multiline_escape_string(&out, value, value_len, &out_size, depth);
      if( value_needs_free ) cJSON_free( value );
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


#endif // HAS_CJSON
