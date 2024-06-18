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

#pragma once

#include "logger.hpp"

extern "C"
{
  #include "libyaml/yaml.h" // https://github.com/yaml/libyaml
}
#include <memory>   // for std::shared_ptr


#if defined ARDUINO_ARCH_SAMD || defined ARDUINO_ARCH_RP2040 || defined ESP8266 || defined ARDUINO_ARCH_AVR || defined CORE_TEENSY
  #include <Arduino.h>
  #include <assert.h>
#endif

//#define YAML_DISABLE_ARDUINOJSON
//#define YAML_DISABLE_CJSON

#define I18N_SUPPORT

#if __has_include(<FS.h>)
  #define I18N_SUPPORT_FS
#endif

#if !defined YAML_DISABLE_CJSON && !defined ARDUINO_ARCH_AVR // define this from sketch if cJSON isn't needed
  #define HAS_CJSON // built-in (esp32) or bundled, except for AVR
#endif

#if !defined YAML_DISABLE_ARDUINOJSON
  #if defined ARDUINO_ARCH_SAMD || defined ARDUINO_ARCH_RP2040 || defined ESP8266 || defined ARDUINO_ARCH_AVR || defined CORE_TEENSY
    // those platforms don't have built-in cJSON and __has_include() macro is limited to
    // the sketch folder, so assume ArduinoJson is in use
    //#include <ArduinoJson.h>
    #define HAS_ARDUINOJSON
  #endif
  #if !defined HAS_ARDUINOJSON && __has_include(<ArduinoJson.h>)
    // esp32 __has_include() macro works outside the sketch folder, so it's possible to guess
    #define HAS_ARDUINOJSON
  #endif
#endif

#if defined I18N_SUPPORT_FS
  #include <FS.h>
#endif

#if defined HAS_ARDUINOJSON
  #include <ArduinoJson.h>
#endif

#if defined HAS_CJSON
  #if defined ESP32
    #include <cJSON.h> //  built-in with esp32
  #else
    #include <cJSON/cJSON.h> // bundled with this library
  #endif
#endif




#define YAML_SCALAR_SPACE " " // YAML is indented with spaces (2 or more), not tabs
#define JSON_SCALAR_TAB "\t"  // JSON is indented with one tab as a default, this can be changed later
#define JSON_FOLDING_DEPTH 4  // lame fact: folds on objects, not on arrays
#define MAX_INDENT_DEPTH 16   // max amount of space/tab per indent level, doesn't seem reasonable to do more on embedded projects


namespace YAML
{

  using logger::setLogLevel;

  // defaults
  __attribute__((unused)) static int JSONFoldindDepth = 4;
  __attribute__((unused)) static int YAMLIndentDepth = 2;
  __attribute__((unused)) static String YAML_INDENT_STRING = "  ";
  __attribute__((unused)) static String JSON_INDENT_STRING = "\t";

  #define JSON_INDENT JSON_INDENT_STRING.c_str()
  #define YAML_INDENT YAML_INDENT_STRING.c_str()

  // shorthand to libyaml scalar values
  #define SCALAR_c(x) (const char*)x->data.scalar.value
  #define SCALAR_s(x)       (char*)x->data.scalar.value
  #define SCALAR_Quoted(n) n->data.scalar.style == YAML_SINGLE_QUOTED_SCALAR_STYLE || n->data.scalar.style == YAML_DOUBLE_QUOTED_SCALAR_STYLE

  void setYAMLIndent( int spaces_per_indent=2 ); // min=2, max=16
  void setJSONIndent( const char* spaces_or_tabs=JSON_SCALAR_TAB, int folding_depth=JSON_FOLDING_DEPTH );

  namespace helpers
  {
    struct yaml_traverser_t;
    struct yaml_stream_handler_data_t;
    struct ParserDelete;
    struct yaml_traverser_t;
    struct yaml_stream_handler_data_t;
    // available output formats
    enum OutputFormat_t { OUTPUT_YAML, OUTPUT_JSON, OUTPUT_JSON_PRETTY };
    std::shared_ptr<yaml_document_t> CreateDocument();
    bool is_filled_with( char needle, const char* haystack );
    bool utf8_equal( const char *str1, size_t len1, const char *str2, size_t len2 );
    int _yaml_stream_reader(void *data, unsigned char *buffer, size_t size, size_t *size_read);
    const char* indent( int size, const char* indstr=YAML::YAML_INDENT );
    const char* index();
    bool string_has_truthy_value( String &_scalar );
    bool string_has_falsy_value( String &_scalar );
    bool string_has_bool_value( String &_scalar, bool *value_out );
    bool yaml_node_is_bool( yaml_node_t * yamlNode, bool *value_out );
    void yaml_multiline_escape_string( Stream* stream, const char* str, size_t length, size_t *bytes_out, size_t depth );
    void yaml_escape_quoted_string( Stream* stream, const char* str, size_t length, size_t *bytes_out );
    bool scalar_needs_quote( yaml_node_t *node );
  };


  // provide a default String::Stream reader/writer for internals
  class StringStream : public Stream
  {
  public:
    StringStream(String &s) : str(s), pos(0) {}
    virtual ~StringStream() {};
    virtual int available() { return str.length() - pos; }
    virtual int read() { return pos < str.length() ? str[pos++] : -1; }
    virtual int peek() { return pos < str.length() ? str[pos] : -1; }
    virtual size_t write(uint8_t c) { str += (char)c; return 1; }
    virtual void flush() {}
  private:
    String &str;
    unsigned int pos;
  };


  namespace YAMLNode_Class
  {

    using namespace helpers;

    // provide traversable node for reading
    class YAMLNode
    {
      std::shared_ptr<yaml_document_t> mDocument;
      yaml_node_t *mNode = nullptr;
    public:
      enum class Type {
        Null,
        Scalar,
        Sequence,
        Map
      };

    public:
      YAMLNode() = default;
      YAMLNode( const YAMLNode& ) = default;
      YAMLNode( YAMLNode&& ) = default;
      ~YAMLNode() = default;

      YAMLNode( std::shared_ptr<yaml_document_t> document, yaml_node_t *node ) :
        mDocument(document),
        mNode(node)
      {}

      Type type() const;

      static YAMLNode loadString( const char *str );
      static YAMLNode loadString( const char *str, size_t len );
      static YAMLNode loadStream( Stream &stream );
      static YAMLNode loadStream( yaml_stream_handler_data_t &stream_handler_data );

      // serialization
      static size_t toJSON( yaml_traverser_t *it );
      static size_t toYAML( yaml_traverser_t *it );
      // error handling
      static void handle_parser_error(yaml_parser_t *parser);
      static void handle_emitter_error(yaml_emitter_t* emitter);


      const char* scalar() const;
      const char* gettext( const char* path, char delimiter=':' );

      YAMLNode& operator = ( const YAMLNode& ) = default;
      YAMLNode& operator = ( YAMLNode&& ) = default;
      YAMLNode operator [] ( int i ) const;
      YAMLNode operator [] ( const char *str ) const;

      size_t size() const;

      bool isScalar() const { return type() == Type::Scalar; }
      bool isSequence() const { return type() == Type::Sequence; }
      bool isMap() const { return type() == Type::Map; }
      bool isNull() const { return type() == Type::Null; }

      yaml_document_t* getDocument() { return mDocument.get(); }
      std::shared_ptr<yaml_document_t> getDocumentSharedPtr() { return mDocument; };
      yaml_node_t *getNode() { return mNode; }

      void setNode( yaml_node_t *n ) { mNode = n; }

    };

  };



  namespace libyaml_native
  {
    using namespace logger;
    using namespace helpers;
    using YAMLNode_Class::YAMLNode;
    // Pure libyaml JSON <-> YAML stream-to-stream seralization
    // size_t serializeYml( Stream &json_src_stream, Stream &yml_dest_stream, OutputFormat_t format=YAMLParser::OUTPUT_YAML );

    // JSON/YAML document to YAML/JSON string
    size_t serializeYml( yaml_document_t* src_doc, String &dest_string, OutputFormat_t format=OUTPUT_YAML );
    // JSON/YAML object to YAML/JSON stream
    size_t serializeYml( yaml_document_t* src_doc, Stream &dest_stream, OutputFormat_t format=OUTPUT_YAML );

    // YAML stream to YAML document
    int deserializeYml( YAMLNode& dest_obj, const char* src_yaml_str );
    // YAML string to YAML document
    int deserializeYml( YAMLNode& dest_obj, Stream &src_stream );
  };


  #if defined HAS_ARDUINOJSON

    namespace libyaml_arduinojson
    {
      using namespace logger;
      using namespace helpers;
      using YAMLNode_Class::YAMLNode;
      // ArduinoJson friendly functions and derivated class

      // recursion helpers for json variant
      // https://github.com/bblanchon/ArduinoJson/issues/1505#issuecomment-782825946
      template <typename Key>
      JsonVariantConst resolvePath(JsonVariantConst variant, Key key) {
        return variant[key];
      }

      template <typename Key, typename... Tail>
      JsonVariantConst resolvePath(JsonVariantConst variant, Key key, Tail... tail) {
        return resolvePath(variant[key], tail...);
      }

      // default name for the topmost temporary JsonObject
      #define ROOT_NODE "_root_"
      // deconstructors
      DeserializationError deserializeYml_JsonObject( yaml_document_t*, yaml_node_t* , JsonObject&, YAMLNode::Type nt=YAMLNode::Type::Null, const char *nodename="", int depth=0 );
      size_t serializeYml_JsonVariant( JsonVariant root, Stream &out, int depth_level, YAMLNode::Type nt );

      class YAMLToArduinoJson
      {
      public:
        YAMLToArduinoJson() {};
        ~YAMLToArduinoJson() { if( _doc) delete _doc; }
        #if ARDUINOJSON_VERSION_MAJOR<7
          void setJsonDocument( const size_t capacity ) { _doc = new DynamicJsonDocument(capacity); _root = _doc->to<JsonObject>(); }
        #else
          void setJsonDocument( const size_t capacity ) { _doc = new JsonDocument; _root = _doc->to<JsonObject>(); }
        #endif
        JsonObject& getJsonObject() { return _root; }
        static DeserializationError toJsonObject( Stream &src, JsonObject& output );
        static DeserializationError toJsonObject( const char* src, JsonObject& output );

      private:
        #if ARDUINOJSON_VERSION_MAJOR<7
          DynamicJsonDocument *_doc = nullptr;
        #else
          JsonDocument *_doc = nullptr;
        #endif
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

      // Deserialize YAML string to ArduinoJSON Document
      DeserializationError deserializeYml( JsonObject &dest_doc, const char* src_yaml_str );
      // Deserialize YAML stream to ArduinoJSON Document
      DeserializationError deserializeYml( JsonObject &dest_doc, Stream &src_stream );

    };
  #endif // HAS_ARDUINOJSON



  #if defined HAS_CJSON

    namespace libyaml_cjson
    {
      using namespace logger;
      using namespace helpers;
      using YAMLNode_Class::YAMLNode;

      // cJSON friendly functions and derivated class

      // deconstructors
      cJSON* deserializeYml_cJSONObject(yaml_document_t* document, yaml_node_t * yamlNode);
      size_t serializeYml_cJSONObject( cJSON *root, Stream &out, int depth, YAMLNode::Type nt );

      class YAMLToCJson
      {
      public:
        YAMLToCJson() {};
        ~YAMLToCJson() {};
        static cJSON* toJson( yaml_document_t* document, yaml_node_t* node );
        static cJSON* toJson( const char* yaml_str ) { YAMLNode yamlnode = YAMLNode::loadString( yaml_str );    auto ret = toJson( yamlnode.getDocument(), yamlnode.getNode() ); return ret; }
        static cJSON* toJson( Stream &yaml_stream )  { YAMLNode yamlnode = YAMLNode::loadStream( yaml_stream ); auto ret = toJson( yamlnode.getDocument(), yamlnode.getNode() ); return ret; }
      };

      // cJSON object to YAML string
      size_t serializeYml( cJSON* src_obj, String &dest_string );
      // cJSON object to YAML stream
      size_t serializeYml( cJSON* src_obj, Stream &dest_stream );

      // YAML stream to cJSON object
      int deserializeYml( cJSON** dest_obj, const char* src_yaml_str );
      // YAML string to cJSON object
      int deserializeYml( cJSON** dest_obj, Stream &src_stream );
      // YAML document to cJSON object
      int deserializeYml( cJSON** dest_obj, yaml_document_t* src_document );

    };
  #endif // HAS_CJSON





  #if defined I18N_SUPPORT

    namespace libyaml_i18n
    {

      using YAMLNode_Class::YAMLNode;
      using namespace logger;
      using namespace helpers;

      // Deconstructed locale
      struct i18n_locale_t
      {
        char language[5]  {0}; // ISO-639 language code
        char country[5]   {0}; // ISO-3166 country code
        char variant[5]   {0}; // ISO-3166 variant code
        char delimiter[2] {0}; // lang-country-variant delimiter, either stroke or underscore
        i18n_locale_t() = default;
      };


      // I18N setlocale() and gettext()
      class i18n_t
      {


      #if defined I18N_SUPPORT_FS
        public:
            i18n_t( fs::FS* _fs=nullptr );
            void setFS( fs::FS *_fs ); // set filesystem
            bool setLocale( const char* localeStr, const char* filePath=nullptr );
        private:
            fs::FS *fs = nullptr;            // Filesystem
            bool loadFsLocale();
            std::string path = "/lang/";     // Deconstructed path where the l10n files can be found, with trailing slash
            std::string extension = "yml";   // Deconstructed file extension (yml, yaml, json)
      #endif


      public:
        i18n_t() { };
        ~i18n_t();
        i18n_t( const char* localeStr, YAMLNode &Node );

        bool setLocale( const char* localeStr, YAMLNode &node );
        const char* gettext( const char* l10npath, char delimiter=':' );

      private:

        i18n_locale_t locale; // Deconstructed locale
        YAMLNode l10n;        // Localization set, deserialized as YAMLNode

        constexpr static const char delimiters[2] = {'-', '_'}; // Supported locale delimiters

        const std::string getLocale(); // reconstruct locale

        bool presetLocale( const char* localeStr );
        void clearLocale();

        bool loadLocaleStream( Stream& stream, size_t size );

        // Note: validation is not made on *values*. Only string length is checked for overflow protection
        bool isValidISO( char* maybe_iso, size_t min, size_t max );
        bool isValidLocale( char* maybe_locale );
        bool isValidLang( char* maybe_iso3166 );
        bool isValidVariant( char* maybe_iso3166 );
        bool isValidCountry( char* maybe_iso639 );

      };

    };

  #endif

};


using YAML::YAMLNode_Class::YAMLNode;
using YAML::StringStream;

using namespace YAML::libyaml_native;


#if defined I18N_SUPPORT
  using YAML::libyaml_i18n::i18n_t;
#endif

#if defined HAS_CJSON
  using namespace YAML::libyaml_cjson;
#endif

#if defined HAS_ARDUINOJSON
  using namespace YAML::libyaml_arduinojson;
#endif
