/*\
 *
 * @file
 * @version 1.0
 * @author tobozo <tobozo@users.noreply.github.com>
 * @section DESCRIPTION
 *
 * YAML <=> JSON converter and l10n parser
 *
 * YAMLDuino
 * Project Page: https://github.com/tobozo/YAMLDuino
 *
 * @section LICENSE
 *
 * Copyright 2022 tobozo http://github.com/tobozo
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files ("YAMLDuino"), to deal in the Software without
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
\*/

#include "ArduinoYaml.hpp"



namespace YAML
{

  namespace helpers
  {

    /*\
     * @brief Create yaml_document_t with shared pointer
     *
     * Attaches yaml_document_delete to the destructor.
     *
    \*/
    std::shared_ptr<yaml_document_t> CreateDocument()
    {
      return std::shared_ptr<yaml_document_t>(new yaml_document_t, [](yaml_document_t *doc) {
          yaml_document_delete(doc);
          delete doc;
        }
      );
    }


    /*\
     * @brief Confirm a string only contains one given char
     *
     * setJSONIndent helper, checks if haystack
     * is filled with needle.
    \*/
    bool is_filled_with( char needle, const char* haystack )
    {
      if( !haystack ) return false;
      size_t len = strlen( haystack );
      for( size_t i=0;i<len;i++ ) {
        if( haystack[i] != needle ) {
          return false;
        }
      }
      return true;
    }


    /*\
     * @brief Dummy utf8 comparaison
     *
     * YAMLNode helper for [] operator
     *
    \*/
    bool utf8_equal( const char *str1, size_t len1, const char *str2, size_t len2 )
    {
      // @todo proper utf8 comparason
      if (len1 != len2) return false;
      return memcmp(str1, str2, len1) == 0;
    }


    /*\
     * @brief Parser auto destructor
     *
     * Parser delete callback, used in a shared ptr
     * so it can self-destruct.
     *
    \*/
    struct ParserDelete {
      void operator () ( yaml_parser_t *parser ) const {
        yaml_parser_delete(parser);
      }
    };


    /*\
     * @brief YAML Traverser
     *
     * Wrapper struct passed when recursively parsing yaml_document
     *
    \*/
    struct yaml_traverser_t
    {
      yaml_document_t* document;
      yaml_node_t* node;
      Stream* stream;
      YAMLNode::Type type;
      int depth;
    };



    /*\
     * @brief Uniform YAML Stream Handler
     *
     * Struct for passing stream with read/written bytes_count.
     * For some reason libyaml only supports bytes_read.
     *
    \*/
    struct yaml_stream_handler_data_t
    {
      Stream* streamPtr;
      size_t *size;
    };



    /*\
     * @brief YAML input parser stream reader callback
     *
     * Custom stream reader callback, holds both Stream and total bytes read in data pointer
     *
    \*/
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


    String _indent_str; // used by indent();

    /*\
     * @brief Indentation string builder
     *
     * Creates a string with multiple occurences of a given char array.
     *
    \*/
    const char* indent( int size, const char* indstr )
    {
      if( size<=0 ) return "";
      _indent_str = "";
      for( int i=0;i<size;i++ ) _indent_str += indstr;
      return _indent_str.c_str();
    }


    // array or object index (prefixed by a stroke) translated to indent level
    #define YAML_INDEX_STROKE "- "
    String _index_str;

    /*\
     * @brief Indentation string builder
     *
     * Creates a string with multiple occurences of a given char array, prefixed by a stroke.
     * Friend of indent() function.
     *
    \*/
    const char* index()
    {
      if( YAML::YAMLIndentDepth <= 2 ) return YAML_INDEX_STROKE;
      String current_indent = _indent_str; // memoize _indent_str as it'll be reset by the next call
      _index_str = String( indent(YAML::YAMLIndentDepth-2, " ") ) + String( YAML_INDEX_STROKE );
      _indent_str = current_indent; // restore _indent_str
      return _index_str.c_str();
    }


    /*\
     * @brief String content test on truthy value
     *
     * YAML is very inclusive with booleans :-)
     * https://yaml.org/type/bool.html
     *
    \*/
    bool string_has_truthy_value( String &_scalar )
    {
      return _scalar == "y"    ||  _scalar == "Y"    ||  _scalar == "yes" || _scalar == "Yes" || _scalar == "YES"
        ||  _scalar == "true" ||  _scalar == "True" ||  _scalar == "TRUE"
        ||  _scalar == "on"   ||  _scalar == "On"   ||  _scalar == "ON";
    }


    /*\
     * @brief String content test on truthy value
     *
     * YAML is very inclusive with booleans :-)
     * https://yaml.org/type/bool.html
     *
    \*/
    bool string_has_falsy_value( String &_scalar )
    {
      return _scalar == "n"     ||  _scalar == "N"     ||  _scalar == "no" || _scalar == "No" || _scalar == "NO"
        ||  _scalar == "false" ||  _scalar == "False" ||  _scalar == "FALSE"
        ||  _scalar == "off"   ||  _scalar == "Off"   ||  _scalar == "OFF";
    }


    /*\
     * @brief String content test on truthy/falsy value
     *
     * YAML is very inclusive with booleans :-)
     * https://yaml.org/type/bool.html
     *
    \*/
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


    /*\
     * @brief Node content test on truthy/falsy value
     *
     * YAML is very inclusive with booleans :-)
     * https://yaml.org/type/bool.html
     *
    \*/
    bool yaml_node_is_bool( yaml_node_t * yamlNode, bool *value_out )
    {
      switch (yamlNode->data.scalar.style) {
        case YAML_PLAIN_SCALAR_STYLE:
        {
          String _scalar = String( SCALAR_s(yamlNode) );
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


    /*\
     * @brief YAML multiline entities escaper for YAML output
     *
     * Helper for serializers.
     *
    \*/
    void yaml_multiline_escape_string( Stream* stream, const char* str, size_t length, size_t *bytes_out, size_t depth )
    {
      size_t i;
      char c;
      char l = '\0';
      String _str = String(str);
      char last_char = str[length-1];
      bool has_multiline = _str.indexOf('\n') > -1;
      bool has_ending_lf = last_char == '\n';
      if( has_ending_lf ) { /*_str = _str.substring( 0, -1 );*/ length--; } // remove trailing lf
      if( has_multiline ) *bytes_out += stream->printf("|%s\n%s", has_ending_lf?"":"-", indent(depth, YAML::YAML_INDENT) );

      const char *str_ = _str.c_str(); // modified version of the string

      for (i = 0; i < length; i++) {
        c = *(str_ + i);
        if( c== '\r' || c=='\n' ) {
          if( l != '\\' && has_multiline ) { // unescaped \r or \n
            if(c == '\r') *bytes_out += 1;  // ignore \r
            else          *bytes_out += stream->printf("\n%s",  indent(depth, YAML::YAML_INDENT) ); // print CRLF
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


    /*\
     * @brief YAML string escaper for JSON output
     *
     * Helper for serializers when escaping to JSON.
     *
    \*/
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
        else if (c == '"')  *bytes_out += stream->printf("\\\"");
        else                *bytes_out += stream->printf("%c", c);
      }
    }


    /*\
     * @brief Node content checker for JSON output.
     *
     * Checks if scalar contents needs to be quoted
     * when serializing to JSON.
     *
    \*/
    bool scalar_needs_quote( yaml_node_t *node )
    {
      if( node->type != YAML_SCALAR_NODE ) return false;
      bool needs_quotes = true;
      bool is_bool = false;
      bool bool_value = false;
      bool is_string = false;
      __attribute__((unused)) double number;
      char* scalar;
      char* end;
      scalar = SCALAR_s(node);
      number = strtod(scalar, &end);
      is_string = (end == scalar || *end);
      if( is_string && yaml_node_is_bool( node, &bool_value ) ) {
        is_bool = true;
      }
      if(is_bool)        needs_quotes = false;
      else if(is_string) needs_quotes = true;
      else               needs_quotes = false; // number
      return needs_quotes;
    }


  };



  /*\
   * @brief Set YAML indentation depth
   *
   * Minimum indentation is 2 spaces.
   * Maximum is 16 spaces for retina screens :)
  \*/
  void setYAMLIndent( int spaces_per_indent )
  {
    if( spaces_per_indent < 2 ) spaces_per_indent = 2;
    if( spaces_per_indent > MAX_INDENT_DEPTH ) spaces_per_indent = MAX_INDENT_DEPTH;
    YAMLIndentDepth = spaces_per_indent;
    YAML_INDENT_STRING = "";
    for( int i=0;i<spaces_per_indent;i++ ) {
      YAML_INDENT_STRING += String(YAML_SCALAR_SPACE);
    }
  }


  /*\
   * @brief Set JSON indentation depth
   *
   * Sanitize 'spaces_or_tabs' input var:
   *  - must have spaces only or tabs only
   *  - min/max requirements
   *  - fallback values
  \*/
  void setJSONIndent( const char* spaces_or_tabs, int folding_depth )
  {
    bool is_valid_str = ( is_filled_with(' ', spaces_or_tabs ) || is_filled_with('\t', spaces_or_tabs ) );
    if( !is_valid_str ) JSON_INDENT_STRING = JSON_SCALAR_TAB;
    else                JSON_INDENT_STRING = String( spaces_or_tabs );

    bool is_in_range  = strlen( spaces_or_tabs ) < MAX_INDENT_DEPTH;
    if( !is_in_range ) JSONFoldindDepth = JSON_FOLDING_DEPTH;
    else               JSONFoldindDepth = folding_depth;
  }





  /*\
   * @brief YAML document copier
   *
   * Creates a copy of a given document.
   *
  \*/
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



  namespace libyaml_native {

    /*\
     * @brief YAML document to YAML/JSON string
     *
     * Serializes yaml_document to JSON minified, JSON pretty or YAML string.
     *
    \*/
    size_t serializeYml( yaml_document_t* src_doc, String &dest_string, OutputFormat_t format )
    {
      StringStream dest_stream( dest_string );
      return serializeYml( src_doc, dest_stream, format );
    }


    /*\
     * @brief YAML document to YAML/JSONL stream
     *
     * Serializes yaml_document to JSON minified, JSON pretty or YAML stream.
     *
    \*/
    size_t serializeYml( yaml_document_t* src_doc, Stream &dest_stream, OutputFormat_t format )
    {
      yaml_node_t* node;
      if (node = yaml_document_get_root_node(src_doc), !node) { YAML_LOG_w("No document defined."); return 0; }
      yaml_traverser_t doc = { src_doc, node, &dest_stream, YAMLNode::Type::Null, 0 };
      size_t bytes_out = 0;
      switch(format) {
        case OutputFormat_t::OUTPUT_JSON: YAML::JSONFoldindDepth = -1; break;
        case OutputFormat_t::OUTPUT_JSON_PRETTY: if( YAML::JSONFoldindDepth<0) YAML::JSONFoldindDepth = JSON_FOLDING_DEPTH; break;
        default:
        case OutputFormat_t::OUTPUT_YAML: break;
      }
      bytes_out = format==OutputFormat_t::OUTPUT_YAML? YAMLNode::toYAML( &doc ) : YAMLNode::toJSON( &doc );
      return bytes_out;
    }


    /*\
     * @brief YAML string to YAMLNode
     *
     * Deserializes YAML or JSON string to YAMLNode.
     * Returns the read data size.
     *
    \*/
    int deserializeYml( YAMLNode& dest_obj, const char* src_yaml_str )
    {
      dest_obj = YAMLNode::loadString( src_yaml_str );
      return strlen( src_yaml_str );
    }


    /*\
     * @brief YAML stream to YAMLNode
     *
     * Deserializes YAML or JSON stream to YAMLNode.
     * Returns the read data size.
     *
    \*/
    int deserializeYml( YAMLNode& dest_obj, Stream &src_stream )
    {
      size_t bytes_read = 0;
      yaml_stream_handler_data_t shd = { &src_stream, &bytes_read };
      dest_obj = YAMLNode::loadStream( shd );
      return bytes_read;
    }

  };



  namespace YAMLNode_Class
  {

    #if defined __cpp_exceptions
      #define YAMLNode_Fail( msg ) throw std::runtime_error(msg);
    #else
      #define YAMLNode_Fail( msg ) YAML_LOG_e(msg); return YAMLNode{};
    #endif


    /*\
     * @brief Get the node type
     *
    \*/
    YAMLNode::Type YAMLNode::type() const
    {
      if (mNode == nullptr) return Type::Null;

      switch (mNode->type) {
        case YAML_NO_NODE:
          return Type::Null;
        case YAML_SCALAR_NODE:
          return Type::Scalar;
        case YAML_SEQUENCE_NODE:
          return Type::Sequence;
        case YAML_MAPPING_NODE:
          return Type::Map;
      }
      return Type::Null;
    }


    /*\
     * @brief Get scalar value
     *
    \*/
    const char* YAMLNode::scalar() const
    {
      if (type() != Type::Scalar) return nullptr;

      const auto &scalar = mNode->data.scalar;
      return (const char*)scalar.value;
    }


    /*\
     * @brief Node Accessor (sequence)
     *
    \*/
    YAMLNode YAMLNode::operator [] ( int i ) const
    {
      if (type() != Type::Sequence) return YAMLNode{};
      if (i < 0) return YAMLNode{};

      const auto &sequence = mNode->data.sequence;
      yaml_node_item_t *item = sequence.items.start + i;
      // is we out of range?
      if (sequence.items.top <= item) return YAMLNode{};

      yaml_node_t *node = yaml_document_get_node(mDocument.get(), *item);

      return YAMLNode{mDocument, node};
    }


    /*\
     * @brief Node Accessor (map)
     *
    \*/
    YAMLNode YAMLNode::operator [] ( const char *str ) const
    {
      if (type() != Type::Map) return YAMLNode{};

      size_t len = strlen(str);

      const auto &mapping = mNode->data.mapping;
      for (yaml_node_pair_t *iter = mapping.pairs.start; iter < mapping.pairs.top; ++iter) {
        yaml_node_t *key = yaml_document_get_node(mDocument.get(), iter->key);
        if (key == nullptr) continue;
        if (key->type != YAML_SCALAR_NODE) continue;

        const auto &scalar = key->data.scalar;
        if (!utf8_equal(str, len, (const char*) scalar.value, scalar.length))  continue;

        yaml_node_t *value = yaml_document_get_node(mDocument.get(), iter->value);
        return YAMLNode(mDocument, value);
      }

      return YAMLNode{};
    }


    /*\
     * @brief Node size
     *
    \*/
    size_t YAMLNode::size() const
    {
      switch (type()) {
        case Type::Null:
        case Type::Scalar:
          return 0;
        case Type::Sequence:
        {
          const auto &sequence = mNode->data.sequence;
          return sequence.items.top - sequence.items.start;
        }
        case Type::Map:
        {
          const auto &mapping = mNode->data.mapping;
          return mapping.pairs.top - mapping.pairs.start;
        }
      }
      return 0;
    }


    /*\
     * @brief YAML String loader
     *
     * Note: this is a static method
     *
    \*/
    YAMLNode YAMLNode::loadString( const char *str )
    {
      size_t len = strlen(str);
      return loadString(str, len);
    }


    /*\
     * @brief YAML String loader
     *
     * Note: this is a static method
     *
    \*/
    YAMLNode YAMLNode::loadString( const char *str, size_t len )
    {
      yaml_parser_t parser;
      std::unique_ptr<yaml_parser_t, ParserDelete> parserDelete(&parser);

      if (yaml_parser_initialize(&parser) != 1) {
        handle_parser_error( &parser );
        YAMLNode_Fail("Failed to initialize yaml parser");
      }
      yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
      yaml_parser_set_input_string(&parser, (const unsigned char*)str, len);

      std::shared_ptr<yaml_document_t> document = CreateDocument();
      if (yaml_parser_load(&parser, document.get()) != 1) {
        handle_parser_error( &parser );
        YAMLNode_Fail("Failed to load yaml document!");
      }

      yaml_node_t *root = yaml_document_get_root_node(document.get());
      return YAMLNode(document, root);
    }


    /*\
     * @brief YAML Stream loader (stream handler data)
     *
     * Note: this is a static method
     *
    \*/
    YAMLNode YAMLNode::loadStream( yaml_stream_handler_data_t &shd )
    {
      yaml_parser_t parser;
      std::unique_ptr<yaml_parser_t, ParserDelete> parserDelete(&parser);

      if (yaml_parser_initialize(&parser) != 1) {
        handle_parser_error( &parser );
        YAMLNode_Fail("Failed to initialize yaml parser");
      }

      yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
      yaml_parser_set_input(&parser, &_yaml_stream_reader, &shd);

      std::shared_ptr<yaml_document_t> document = CreateDocument();
      if (yaml_parser_load(&parser, document.get()) != 1) {
        handle_parser_error( &parser );
        YAMLNode_Fail("Failed to load yaml document!");
      }

      yaml_node_t *root = yaml_document_get_root_node(document.get());
      return YAMLNode(document, root);
    }


    /*\
     * @brief YAML Stream loader (basic stream)
     *
     * Note: this is a static method
     *
    \*/
    YAMLNode YAMLNode::loadStream( Stream &stream )
    {
      size_t bytes_read = 0;
      yaml_stream_handler_data_t shd = { &stream, &bytes_read };
      return loadStream( shd );
    }


    /*\
     * @brief l10n style gettext
     *
     * L10N: Return localized string when given a path e.g. 'blah:some:property:count:1'
     *
    \*/
    const char* YAMLNode::gettext( const char* path, char delimiter)
    {
      if( !path ) return "";
      const char delimStr[2] = { delimiter, 0 };
      char* pathCopy = strdup( path );
      char *found = NULL;
      yaml_node_t* root_node = mNode;
      void* retPtr = (void*)path;

      if( this->isNull()) goto _not_found; // uh-oh, language not loaded

      if( strchr( path, delimiter ) == NULL ) { // no delimiter found, just a key
        YAMLNode tmp = (*this)[path];
        if( tmp.isNull() ) goto _not_found; // no property under this name
        this->mNode = tmp.mNode;
        goto _success;
      }

      found = strtok( pathCopy, delimStr ); // walk through delimited properties

      while( found != NULL ) {
        YAMLNode tmpMap = (*this)[found];
        if( !tmpMap.isNull() ) { // map key->val
          this->mNode = tmpMap.mNode;
        } else {
          int idx = atoi( found );
          YAMLNode tmpSeq = (*this)[idx];
          if( idx >= 0 && !tmpSeq.isNull() ) { // array index->val
            this->mNode = tmpSeq.mNode;
          } else {
            goto _not_found; // delimited string/index not in yaml tree
          }
        }
        found = strtok( NULL, delimStr ); // get next token
      }

      if( this->isNull() ) goto _not_found; // no match

      _success:
        free( pathCopy );
        retPtr = (void*)this->scalar();
        this->mNode = root_node;
        return (const char*)retPtr;

      _not_found:
        free( pathCopy );
        return path; // not found
    }


    /*\
     * @brief Parser error handler
     *
    \*/
    void YAMLNode::handle_parser_error(yaml_parser_t *p)
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


    /*\
     * @brief Emitter error handler
     *
    \*/
    void YAMLNode::handle_emitter_error(yaml_emitter_t *e)
    {
      switch (e->error) {
        case YAML_MEMORY_ERROR:    YAML_LOG_e("[MEM]: Not enough memory for emitting"); break;
        case YAML_WRITER_ERROR:    YAML_LOG_e("[WRI]: %s", e->problem); break;
        case YAML_EMITTER_ERROR:   YAML_LOG_e("[EMI]: %s", e->problem); break;
        default:/*Couldn't happen*/YAML_LOG_e("[INT]"); break;
      }
    }


    /*\
     * @brief yaml_document_t traverser (not really a deconstructor)
     *
     * Output format: JSON
     *
    \*/
    size_t YAMLNode::toJSON( yaml_traverser_t *it )
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
      int node_count = 0;
      int node_max = 0;
      bool is_seq = false;
      bool needs_folding = false;
      bool needs_quotes = false;

      switch (node->type) {
        case YAML_SCALAR_NODE:
          needs_quotes = scalar_needs_quote( node );
          if( needs_quotes ) bytes_out += stream->printf("\"");
          yaml_escape_quoted_string( stream, SCALAR_c(node), strlen(SCALAR_c(node)), &bytes_out );
          if( needs_quotes ) bytes_out += stream->printf("\"");
        break;
        case YAML_SEQUENCE_NODE:
          bytes_out += stream->printf("[");
          node_max = node->data.sequence.items.top - node->data.sequence.items.start;
          for (auto item_i = node->data.sequence.items.start; item_i < node->data.sequence.items.top; ++item_i) {
            auto node_item = yaml_document_get_node(document, *item_i);
            int child_level = node_item->type == YAML_MAPPING_NODE ? depth+1 : depth-1;
            yaml_traverser_t seq_item = { document, yaml_document_get_node(document, *item_i), stream, YAMLNode::Type::Sequence, child_level };
            bytes_out += toJSON( &seq_item );
            node_count++;
            if( node_count < node_max ) {
              bytes_out += stream->printf(", ");
            }
          }
          bytes_out += stream->printf("]");
        break;
        case YAML_MAPPING_NODE:
          is_seq = ( depth>0 && nest_type == YAMLNode::Type::Sequence );
          needs_folding = (depth>YAML::JSONFoldindDepth);
          bytes_out += stream->printf("{");
          if( !needs_folding ) bytes_out += stream->printf("\n%s", indent(depth+1, YAML::JSON_INDENT) );
          node_max = node->data.mapping.pairs.top - node->data.mapping.pairs.start;
          for (auto pair_i = node->data.mapping.pairs.start; pair_i < node->data.mapping.pairs.top; ++pair_i) {
            auto key   = yaml_document_get_node(document, pair_i->key);
            auto value = yaml_document_get_node(document, pair_i->value);
            if (key->type != YAML_SCALAR_NODE) {
              YAML_LOG_e("Mapping key is not scalar (line %lu).", key->start_mark.line);
              continue;
            }
            yaml_traverser_t map_item = { document, value, stream, YAMLNode::Type::Map, depth+1 };
            bytes_out += stream->printf("\"%s\": ", SCALAR_c(key) );
            bytes_out += toJSON( &map_item );
            node_count++;
            if( node_count < node_max ) {
              if( !needs_folding ) bytes_out += stream->printf(",\n%s", indent(depth+1, YAML::JSON_INDENT) );
              else bytes_out += stream->printf(", ");
            }
          }
          if( !needs_folding ) bytes_out += stream->printf("\n%s", indent( is_seq ? depth-1 : depth, YAML::JSON_INDENT) );
          bytes_out += stream->printf("}");
        break;
        case YAML_NO_NODE: break;
        default: YAML_LOG_e("Unknown node type (line %lu).", node->start_mark.line); break;
      }
      return bytes_out;
    }


    /*\
     * @brief yaml_document_t traverser (not really a deconstructor)
     *
     * Output format: YAML
     *
    \*/
    size_t YAMLNode::toYAML( yaml_traverser_t *it )
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
          if ( nest_type == YAMLNode::Type::Sequence ) bytes_out += stream->printf("\n%s%s",  indent(depth, YAML::YAML_INDENT), index() );
          yaml_multiline_escape_string( stream, SCALAR_c(node), strlen(SCALAR_c(node)), &bytes_out, depth );
        break;
        case YAML_SEQUENCE_NODE:
          for (auto item_i = node->data.sequence.items.start; item_i < node->data.sequence.items.top; ++item_i) {
            auto node_item = yaml_document_get_node(document, *item_i);
            int child_level = node_item->type == YAML_MAPPING_NODE ? depth+1 : depth-1;
            yaml_traverser_t seq_item = { document, yaml_document_get_node(document, *item_i), stream, YAMLNode::Type::Sequence, child_level };
            bytes_out += toYAML( &seq_item );
          }
        break;
        case YAML_MAPPING_NODE:
          for (auto pair_i = node->data.mapping.pairs.start; pair_i < node->data.mapping.pairs.top; ++pair_i) {
            bool is_seqfirst = ( i++==0 && nest_type == YAMLNode::Type::Sequence );
            auto key   = yaml_document_get_node(document, pair_i->key);
            auto value = yaml_document_get_node(document, pair_i->value);
            if (key->type != YAML_SCALAR_NODE) {
              YAML_LOG_e("Mapping key is not scalar (line %lu).", key->start_mark.line);
              continue;
            }
            yaml_traverser_t map_item = { document, value, stream, YAMLNode::Type::Map, depth+1 };
            bytes_out += stream->printf("\n%s%s%s: ", is_seqfirst ? indent(parent_level, YAML::YAML_INDENT) : indent(depth, YAML::YAML_INDENT), is_seqfirst ? index() : "", SCALAR_c(key) );
            bytes_out += toYAML( &map_item );
          }
        break;
        case YAML_NO_NODE: break;
        default: YAML_LOG_e("Unknown node type (line %lu).", node->start_mark.line); break;
      }
      return bytes_out;
    }

  };



  #if defined HAS_ARDUINOJSON

    namespace libyaml_arduinojson
    {
      /*\
       * @brief yaml_node_t deconstructor => JsonObject
       *
       * Input: yaml_document_t
       * Output: ArduinoJSON JsonObject
       *
      \*/
      DeserializationError deserializeYml_JsonObject( yaml_document_t* document, yaml_node_t* yamlNode, JsonObject &jsonNode, YAMLNode::Type nt, const char *nodename, int depth )
      {
        bool isRootNode = ( strlen(nodename)<=0 );

        switch (yamlNode->type) {
          case YAML_SCALAR_NODE:
          {
            double number;
            char* scalar;
            char* end;
            scalar = SCALAR_s(yamlNode);
            number = strtod(scalar, &end);
            bool is_double = false;
            bool is_bool = false;
            bool bool_value = false;
            bool is_string = (end == scalar || *end);
            if( is_string && yaml_node_is_bool( yamlNode, &bool_value ) ) {
              is_bool = true;
            }
            if( SCALAR_Quoted(yamlNode) ) {
              is_string = true;
            }
            if( !is_bool && !is_string ) {
              is_double = String(scalar).indexOf(".") > 0;
            }
            switch( nt ) {
              case YAMLNode::Type::Sequence:
              {
                JsonArray array = jsonNode[nodename];
                if(is_bool)        array.add( bool_value );
                else if(is_string) array.add( scalar );
                else if(is_double) array.add( number );
                else               array.add( (int64_t)number );
              }
              break;
              case YAMLNode::Type::Map:
                if(is_bool)        jsonNode[nodename] = bool_value;
                else if(is_string) jsonNode[nodename] = scalar;
                else if(is_double) jsonNode[nodename] = number;
                else               jsonNode[nodename] = (int64_t)number;
              break;
              default: YAML_LOG_e("Error invalid nesting type"); break;
            }
          }
          break;
          case YAML_SEQUENCE_NODE:
          {
            #if ARDUINOJSON_VERSION_MAJOR<7

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
                  deserializeYml_JsonObject( document, itemNode, tmpObj, YAMLNode::Type::Sequence, _nodeItemName.c_str(), depth+1 ); // go recursive using temporary node name
                  jsonNode[nodename][tmpArray.size()-1] = tmpObj[_nodeItemName.c_str()]; // remove temporary name and make object anonymous
                } else { // array of sequences or values
                  _nodeItemName = "" + String( nodename );
                  deserializeYml_JsonObject( document, itemNode, jsonNode, YAMLNode::Type::Sequence, _nodeItemName.c_str(), depth+1 );
                }
              }

            #else

              jsonNode[(char*)nodename].to<JsonArray>();
              JsonArray nodeArray = jsonNode[(char*)nodename];//.to<JsonArray>();

              yaml_node_item_t * item_i;
              yaml_node_t *itemNode;
              String _nodeItemName;
              JsonDocument copyDoc;
              JsonObject tmpObj;
              for (item_i = yamlNode->data.sequence.items.start; item_i < yamlNode->data.sequence.items.top; ++item_i) {
                itemNode = yaml_document_get_node(document, *item_i);
                if( itemNode->type == YAML_MAPPING_NODE ) { // array of anonymous objects
                  tmpObj = nodeArray.add<JsonObject>(); // insert empty nested object
                  _nodeItemName = ROOT_NODE + String( nodename ) + String( nodeArray.size() ); // generate a temporary nodename
                  tmpObj[(char*)_nodeItemName.c_str()].to<JsonObject>();
                  deserializeYml_JsonObject( document, itemNode, tmpObj, YAMLNode::Type::Sequence, _nodeItemName.c_str(), depth+1 ); // go recursive using temporary node name
                  copyDoc.set(tmpObj[_nodeItemName]); // make object anonymous, remove temporary nodename
                  nodeArray[nodeArray.size()-1].set(copyDoc); // replace array item by reparented node
                } else { // array of sequences or values
                  _nodeItemName = "" + String( nodename );
                  deserializeYml_JsonObject( document, itemNode, jsonNode, YAMLNode::Type::Sequence, _nodeItemName.c_str(), depth+1 );
                }
              }

            #endif
          }
          break;
          case YAML_MAPPING_NODE:
          {
            #if ARDUINOJSON_VERSION_MAJOR<7

              JsonObject tmpNode = isRootNode ? jsonNode : jsonNode.createNestedObject((char*)nodename);
              yaml_node_pair_t* pair_i;
              yaml_node_t* key;
              yaml_node_t* value;
              for (pair_i = yamlNode->data.mapping.pairs.start; pair_i < yamlNode->data.mapping.pairs.top; ++pair_i) {
                key   = yaml_document_get_node(document, pair_i->key);
                value = yaml_document_get_node(document, pair_i->value);
                if (key->type != YAML_SCALAR_NODE) {
                  YAML_LOG_e("Mapping key is not scalar (line %lu, val=%s).", key->start_mark.line, SCALAR_c(value) );
                  continue;
                }
                tmpNode.createNestedObject( SCALAR_s(key) );
                deserializeYml_JsonObject( document, value, tmpNode, YAMLNode::Type::Map, SCALAR_c(key), depth+1 );
              }

            #else

              JsonObject tmpNode = isRootNode ? jsonNode : jsonNode[(char*)nodename].to<JsonObject>();
              yaml_node_pair_t* pair_i;
              yaml_node_t* key;
              yaml_node_t* value;
              for (pair_i = yamlNode->data.mapping.pairs.start; pair_i < yamlNode->data.mapping.pairs.top; ++pair_i) {
                key   = yaml_document_get_node(document, pair_i->key);
                value = yaml_document_get_node(document, pair_i->value);
                if (key->type != YAML_SCALAR_NODE) {
                  YAML_LOG_e("Mapping key is not scalar (line %lu, val=%s).", key->start_mark.line, SCALAR_c(value) );
                  continue;
                }
                tmpNode[SCALAR_s(key)].add<JsonObject>();
                deserializeYml_JsonObject( document, value, tmpNode, YAMLNode::Type::Map, SCALAR_c(key), depth+1 );
              }

            #endif
          }
          break;
          case YAML_NO_NODE: YAML_LOG_e("YAML_NO_NODE");

          break;
          default: YAML_LOG_e("Unknown node type (line %lu).", yamlNode->start_mark.line); break;
        }
        return DeserializationError::Ok;
      }


      /*\
       * @brief JsonVariant deconstructor => YAML stream
       *
       * Input: ArduinoJSON JsonVariant
       * Output: YAML Stream
       *
      \*/
      size_t serializeYml_JsonVariant( JsonVariant root, Stream &out, int depth, YAMLNode::Type nt )
      {
        int parent_level = depth>0?depth-1:0;
        size_t out_size = 0;
        if (root.is<JsonArray>()) {
          JsonArray array = root;
          for( size_t i=0; i<array.size(); i++ ) {
            size_t child_depth = array[i].is<JsonObject>() ? depth+1 : depth-1;
            out_size += serializeYml_JsonVariant(array[i], out, child_depth, YAMLNode::Type::Sequence);
          }
        } else if (root.is<JsonObject>()) {
          JsonObject object = root;
          int i = 0;
          for (JsonPair pair : object) {
            bool is_seqfirst = (i++==0 && nt==YAMLNode::Type::Sequence);
            out_size += out.printf("\n%s%s%s: ", is_seqfirst ? indent(parent_level, YAML::YAML_INDENT) : indent(depth, YAML::YAML_INDENT), is_seqfirst ? index() : "", pair.key().c_str() );
            out_size += serializeYml_JsonVariant( pair.value(), out, depth+1, YAMLNode::Type::Map );
          }
        } else if( !root.isNull() ) {
          if( nt == YAMLNode::Type::Sequence ) out_size += out.printf("\n%s%s",  indent(depth, YAML::YAML_INDENT), index() );
          yaml_multiline_escape_string(&out, root.as<String>().c_str(), root.as<String>().length(), &out_size, depth);
        } else {
          YAML_LOG_e("Error, root is null");
        }
        return out_size;
      }


      /*\
       * @brief JsonVariant serializer => YAML string
       *
       * Input: ArduinoJSON JsonVariant
       * Output: YAML string
       *
      \*/
      size_t serializeYml( JsonVariant src_obj, String &dest_string )
      {
        StringStream dest_stream( dest_string );
        return serializeYml_JsonVariant( src_obj, dest_stream, 0, YAMLNode::Type::Null );
      }


      /*\
       * @brief JsonVariant serializer => YAML stream
       *
       * Input: ArduinoJSON JsonVariant
       * Output: YAML string
       *
      \*/
      size_t serializeYml( JsonVariant src_obj, Stream &dest_stream )
      {
        return serializeYml_JsonVariant( src_obj, dest_stream, 0, YAMLNode::Type::Null );
      }


      /*\
       * @brief JsonDocument deserializer <= YAML or JSON stream
       *
       * Input: ArduinoJSON JsonDocument
       * Output: YAML string
       *
      \*/
      DeserializationError deserializeYml( JsonDocument &dest_doc, Stream &src )
      {
        JsonObject dest_obj = dest_doc.to<JsonObject>();
        auto ret = YAMLToArduinoJson::toJsonObject( src, dest_obj );
        return ret;
      }


      /*\
       * @brief JsonDocument deserializer <= YAML or JSON string
       *
       * Input: ArduinoJSON JsonDocument
       * Output: YAML string
       *
      \*/
      DeserializationError deserializeYml( JsonDocument &dest_doc, const char* src )
      {
        JsonObject dest_obj = dest_doc.to<JsonObject>();
        auto ret = YAMLToArduinoJson::toJsonObject( src, dest_obj );
        return ret;
      }


      /*\
       * @brief JsonObject deserializer <= YAML or JSON stream
       *
       * Input: ArduinoJSON JsonObject
       * Output: YAML string
       *
      \*/
      DeserializationError deserializeYml( JsonObject &dest_obj, Stream &src)
      {
        auto ret = YAMLToArduinoJson::toJsonObject( src, dest_obj );
        return ret;
      }


      /*\
       * @brief JsonObject deserializer <= YAML or JSON string
       *
       * Input: ArduinoJSON JsonObject
       * Output: YAML string
       *
      \*/
      DeserializationError deserializeYml( JsonObject &dest_obj,  const char* src)
      {
        auto ret = YAMLToArduinoJson::toJsonObject( src, dest_obj );
        return ret;
      }

    };


    /*\
     * @brief JsonObject deserializer <= YAML or JSON stream
     *
     * Input: ArduinoJSON JsonObject
     * Output: YAML string
     *
    \*/
    DeserializationError YAMLToArduinoJson::toJsonObject( Stream &src, JsonObject& output )
    {
      YAMLNode yamlnode = YAMLNode::loadStream( src );
      auto ret = deserializeYml_JsonObject(yamlnode.getDocument(), yamlnode.getNode(), output);
      return ret;
    }


    /*\
     * @brief JsonObject deserializer <= YAML or JSON string
     *
     * Input: ArduinoJSON JsonObject
     * Output: YAML string
     *
    \*/
    DeserializationError YAMLToArduinoJson::toJsonObject( const char* src, JsonObject& output )
    {
      YAMLNode yamlnode = YAMLNode::loadString( src );
      auto ret = deserializeYml_JsonObject(yamlnode.getDocument(), yamlnode.getNode(), output);
      return ret;
    }


  #endif // HAS_ARDUINOJSON





  #if defined HAS_CJSON

    namespace libyaml_cjson
    {
      /*\
       * @brief yaml_node_t deconstructor => cJSON Object
       *
       * Input: yaml_document_t
       * Returns: cJSON object
       *
      \*/
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
            scalar = SCALAR_s(yamlNode);
            number = strtod(scalar, &end);
            if( (end == scalar || *end) || SCALAR_Quoted(yamlNode) ) { // string or bool
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
                YAML_LOG_e("Mapping key is not scalar (line %lu).", key->start_mark.line);
                continue;
              }
              cJSON_AddItemToObject(object, SCALAR_c(key), deserializeYml_cJSONObject(document, value));
            }
          }
          break;
          default:
            YAML_LOG_e("Unknown node type (line %lu).", yamlNode->start_mark.line);
            object = NULL;
        }
        return object;
      }


      /*\
       * @brief cJSON deconstructor => YAML stream
       *
       * Input: cJSON object
       * Output: YAML Stream
       *
      \*/
      size_t serializeYml_cJSONObject( cJSON *root, Stream &out, int depth, YAMLNode::Type nt )
      {
        assert(root);
        int parent_level = depth>0?depth-1:0;
        size_t out_size = 0;

        if ( root->type == cJSON_Array ) {
          cJSON *current_element = root->child;
          while (current_element != NULL) {
            size_t child_depth = current_element->type == cJSON_Object ? depth+1 : depth-1;
            out_size += serializeYml_cJSONObject( current_element, out, child_depth, YAMLNode::Type::Sequence );
            current_element = current_element->next;
          }
        } else if (root->type == cJSON_Object ) {
          cJSON *current_item = root->child;
          int i = 0;
          while (current_item) {
            bool is_seqfirst = (i++==0 && nt==YAMLNode::Type::Sequence);
            char* key = current_item->string;
            out_size += out.printf("\n%s%s%s: ", is_seqfirst ? indent(parent_level, YAML::YAML_INDENT) : indent(depth, YAML::YAML_INDENT), is_seqfirst ? index() : "", key );
            out_size += serializeYml_cJSONObject( current_item, out, depth+1, YAMLNode::Type::Map );
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
          if( nt == YAMLNode::Type::Sequence ) out_size += out.printf("\n%s%s",  indent(depth, YAML::YAML_INDENT), index() );
          yaml_multiline_escape_string(&out, value, value_len, &out_size, depth);
          if( value_needs_free ) cJSON_free( value );
        }
        return out_size;
      }


      /*\
       * @brief cJSON object to YAML string
       *
       * Input: cJSON object
       * Output: YAML string
       *
      \*/
      size_t serializeYml( cJSON* src_obj, String &dest_string )
      {
        assert( src_obj );
        StringStream dest_stream( dest_string );
        return serializeYml_cJSONObject( src_obj, dest_stream, 0, YAMLNode::Type::Null );
      }


      /*\
       * @brief cJSON object to YAML stream
       *
       * Input: cJSON object
       * Output: YAML stream
       *
      \*/
      size_t serializeYml( cJSON* src_obj, Stream &dest_stream )
      {
        assert( src_obj );
        return serializeYml_cJSONObject( src_obj, dest_stream, 0, YAMLNode::Type::Null );
      }


      /*\
       * @brief string to cJSON object
       *
       * Input: YAML string
       * Output: ptr to cJSON object
       *
      \*/
      int deserializeYml( cJSON** dest_obj, const char* src_yaml_str )
      {
        *dest_obj = YAMLToCJson::toJson( src_yaml_str );
        return *dest_obj != NULL ? 1 : -1;
      }


      /*\
       * @brief stream to cJSON object
       *
       * Input: YAML stream
       * Output: ptr to cJSON object
       *
      \*/
      int deserializeYml( cJSON** dest_obj, Stream &src_stream )
      {
        *dest_obj = YAMLToCJson::toJson( src_stream );
        return *dest_obj != NULL ? 1 : -1;
      }


      /*\
       * @brief yaml_document to cJSON object
       *
       * Input: yaml_document_t
       * Output: ptr to cJSON object
       *
      \*/
      int deserializeYml( cJSON** dest_obj, yaml_document_t* src_document )
      {
        yaml_node_t* node;
        if (node = yaml_document_get_root_node(src_document), !node) { YAML_LOG_w("No document defined."); return -1; }
        *dest_obj = YAMLToCJson::toJson( src_document, node );
        return *dest_obj != NULL ? 1 : -1;
      }

      /*\
       * @brief yaml_document to cJSON object
       *
       * Input: yaml_document_t
       * Return: cJSON object
       *
      \*/
      cJSON* YAMLToCJson::toJson( yaml_document_t* document, yaml_node_t * node )
      {
        return deserializeYml_cJSONObject(document, node);
      };


    };

  #endif // HAS_CJSON




  #if defined I18N_SUPPORT

    namespace libyaml_i18n
    {




      /*\
       * @brief Constructor with YAMLNode
       *
      \*/
      i18n_t::i18n_t( const char* localeStr, YAMLNode &node )
      {
        setLocale( localeStr, node );
      };


      #if defined I18N_SUPPORT_FS
        /*\
         * @brief Filesystem setter
         *
         * Accepts any fs::FS
         *
        \*/
        void i18n_t::setFS( fs::FS *_fs )
        {
          if( _fs ) {
            fs = _fs;
          } else {
            YAML_LOG_d("No filesystem attached");
          }
        }


        /*\
         * @brief Constructor with FS
         *
         * Accepts any fs::FS
         *
        \*/
        i18n_t::i18n_t( fs::FS *_fs)
        {
          setFS( _fs);
        }
      #endif



      /*\
       * @brief Destructor
       *
      \*/
      i18n_t::~i18n_t()
      {
        clearLocale();
      }



      /*\
       * @brief zero fill the deconstructed locale
       *
      \*/
      void i18n_t::clearLocale()
      {
        memset( locale.language,  0, sizeof(i18n_locale_t::language) );
        memset( locale.country,   0, sizeof(i18n_locale_t::country) );
        memset( locale.variant,   0, sizeof(i18n_locale_t::variant) );
        memset( locale.delimiter, 0, sizeof(i18n_locale_t::delimiter) );
      }


      /*\
       * @brief Access l10n YAMLNode by path
       *
       * Return: scalar value
       *
      \*/
      const char* i18n_t::gettext( const char* l10npath, char delimiter )
      {
        return l10n.gettext( l10npath, delimiter );
      }


      /*\
       * @brief Reconstruct the locale in a string
       *
      \*/
      const std::string i18n_t::getLocale()
      {
        assert( locale.language[0] != 0 );
        std::string ret = "";
        ret += locale.language;
        if( locale.country[0] != 0 ) {
          ret += locale.delimiter;
          ret += locale.country;
        }
        if( locale.variant[0] != 0 ) {
          ret += locale.delimiter;
          ret += locale.variant;
        }
        return ret;
      }


      /*\
       * @brief Dummy string test based on length
       *
      \*/
      bool i18n_t::isValidISO( char* maybe_iso, size_t min, size_t max )
      {
        if( !maybe_iso ) return false;
        size_t len = strlen( maybe_iso );
        return ( len>=min && len<=max );
      }


      /*\
       * @brief Named tests using isValidISO()
       *
       * TODO: enforce validation with lowercase/uppercase check
      \*/
      bool i18n_t::isValidLocale(  char* maybe_locale )   { return isValidISO( maybe_locale,   2, sizeof(i18n_locale_t) );   }
      bool i18n_t::isValidLang(    char* maybe_language ) { return isValidISO( maybe_language, 2, sizeof(i18n_locale_t::language) ); }
      bool i18n_t::isValidVariant( char* maybe_variant )  { return isValidISO( maybe_variant,  2, sizeof(i18n_locale_t::variant) );  }
      bool i18n_t::isValidCountry( char* maybe_country )  { return isValidISO( maybe_country,  2, sizeof(i18n_locale_t::country) );  }


      #define goto_error(x) { if(x[0]!=0) YAML_LOG_e(x); goto _error; }


      /*\
       * @brief Locale validation and deconstruction
       *
       * Guesses the locale format.
       * Performs deconstruction into lang/country/version/separator when applicable.
       *
      \*/
      bool i18n_t::presetLocale( const char* localeStr )
      {
        assert( localeStr );
        assert( strlen(localeStr) >= 2 && strlen(localeStr)<=32 );

        char* localeCopy;    // Copy of localeStr;
        char* localename;    // Pointer used for filepath deconstructor
        char *found = NULL;  // Pointer used for string search

        clearLocale(); // Clear any previously loaded locale
        localeCopy = strdup( localeStr ); // Allocate a copy of localeStr

        found = strrchr( localeCopy, '.' ); // Look for a dot: is locale a filename?
        if( found != NULL ) { // Locale string is a file name, deconstruct file extension
          #if defined I18N_SUPPORT_FS
            this->extension = found+1; // Update file extension property
          #endif
          *found = '\0'; // Truncate file extension, only keep the locale
        }

        found = strrchr( localeCopy, '/' ); // Look for a path separator: is locale prexifed with a path?
        if( found != NULL ) { // Locale string has a path separator, deconstruct [path][locale]
          localename = found+1; // Locale name starts after the last path separator
          if( !isValidLocale( localename ) ) goto_error(""); // Perform a naive locale length validation
          #if defined I18N_SUPPORT_FS
            int pathlen = found-localeCopy+1; // Get the last path separator position
            this->path = std::string( localeCopy ).substr( 0, pathlen ); // Update path property
          #endif
          snprintf( localeCopy, strlen( localename  )+1, "%s", localename ); // Update localeCopy with value trimmed from path
        } else { // Locale string has no path separator
          if( !isValidLocale( localeCopy ) ) goto_error(""); // Perform a naive locale length validation
        }

        found = NULL; // Reset search pointer as it'll now be used to find a locale delimiter

        for( int i=0;i<sizeof(i18n_t::delimiters);i++ ) { // Search for known locale delimiters in order to guess if locale is xx-XX, or xx_XX or xx
          found = strchr( localeCopy, delimiters[i] );
          if( found != NULL ) { // Found a known locale delimiter
            locale.delimiter[0] = delimiters[i]; // Update delimiter property
            break;
          }
        }

        if( found == NULL ) { // No delimiter found, assume it's a language-only locale of at least 2 chars
          if( !isValidLang( localeCopy ) ) goto_error("Bad ISO-639 language code"); // Perform a naive ISO-639 length validation
          memcpy( locale.language, localeCopy, strlen(localeCopy) );// Update language property
          goto _success;
        }

        // A delimiter was found! assume it's at least language+country, and at most language+country+variant
        found = strtok( localeCopy, locale.delimiter ); // split by delimiter, first element is language code
        if( !isValidLang( found ) ) goto_error("Bad ISO-639 language code"); // Perform a naive ISO-639 length validation
        memcpy( locale.language, found, strlen(found) );

        found = strtok( NULL, locale.delimiter ); // Get next delimited element, should be country code
        if( !isValidCountry( found ) ) goto_error("Bad ISO-3166 country code"); // Perform a naive ISO-639 length validation
        memcpy( locale.country, found, strlen(found) );

        found = strtok( NULL, locale.delimiter ); // Get next optional delimited element, should be country variant code
        if( found == NULL ) goto _success; // no variant provided
        if( !isValidVariant( found ) ) goto_error("Bad ISO-3166 variant code"); // Perform a naive ISO-639 length validation
        memcpy( locale.variant, found, strlen(found) );

        _success:
          YAML_LOG_d("Loaded locale: %s", localeCopy );
          free(localeCopy);
          return true;

        _error:
          YAML_LOG_e("[Error] Malformed locale, given='%s', filtered='%s'", localeStr, localeCopy );
          free(localeCopy);
          return false;
      }


      /*\
       * @brief i18n style 'setLocale', YAMLNode based
       *
       * Calls presetLocale() and loads the locale from the provided YAMLNode
       *
      \*/
      bool i18n_t::setLocale( const char* localeStr, YAMLNode &node )
      {
        if( presetLocale( localeStr ) ) {
          if( node.size()==1 ) { // only one root node, does it match locale or language ?
            if( !node[localeStr].isNull() ) l10n=node[localeStr];
            if( !node[locale.language].isNull() )   l10n=node[locale.language];
          } else {
            l10n = node;
          }
          return true;
        }
        return false;
      }


      /*\
       * @brief Filesystem Locale stream loader
       *
       * Called by loadLocale()
       *
      \*/
      bool i18n_t::loadLocaleStream( Stream& stream, size_t size )
      {
        l10n = YAMLNode::loadStream( stream );
        return !l10n.isNull();
      }


      #if defined I18N_SUPPORT_FS
        /*\
         * @brief i18n style 'setLocale', filesystem based
         *
         * Calls presetLocale() and loads the locale from the filesystem upon success.
         * Use filePath if filename differs from locale e.g. setLocale("en-UD", "/path/to/arbitrary_non_locale_filename_yml")
         * 'localeStr' must be valid, it can be "xx_XX" or "/path/to/xx_XX.yml"
         *
        \*/
        bool i18n_t::setLocale( const char* localeStr, const char* filePath )
        {
          if( filePath == nullptr ) return presetLocale( localeStr ) ? loadFsLocale() : false; // no path provided, guess
          if( fs == nullptr || ! fs->exists( filePath ) ) return false; // a filePath was provided but filesystem or file doesn't exist
          if( ! presetLocale( localeStr ) ) return false; // localeStr is invalid
          fs::File localeFile = fs->open( filePath, "r" );
          if( ! localeFile ) return false; // file is invalid
          bool ret = loadLocaleStream( localeFile, localeFile.size()*2 ); // TODO: better size evaluation
          localeFile.close();
          return ret;
        }


        /*\
         * @brief Filesystem Locale loader
         *
         * Calls loadLocaleStream() from file stream.
         *  Reparents the root node if root map key==localeStr
         *
        \*/
        bool i18n_t::loadFsLocale()
        {
          if( fs == nullptr ) {
            YAML_LOG_n("No filesystem attached, use setFS() e.g. i18n.setFS( &LiffleFS );");
            return false;
          }
          std::string localeStr = getLocale();
          std::string fileName = this->path + localeStr + "." + this->extension;
          if( ! fs->exists( fileName.c_str() ) ) return false; // this test is useless with SPIFFS
          fs::File localeFile = fs->open( fileName.c_str(), "r" );
          if( ! localeFile ) return false;
          bool ret = loadLocaleStream( localeFile, localeFile.size()*2 ); // TODO: better size evaluation
          localeFile.close();
          if( !ret ) return false;
          // perf+mem optimization: optionally reparent tree if root key is unique and value equals locale or language
          if( l10n.isNull() ) return false;
          if( l10n.size()==1 ) { // only one root node, does it match locale or language ?
            if( !l10n[localeStr.c_str()].isNull() ) l10n=l10n[localeStr.c_str()];
            if( !l10n[locale.language].isNull() )   l10n=l10n[locale.language];
          }
          return true;
        }
      #endif


    };
  #endif // if defined I18N_SUPPORT


};
