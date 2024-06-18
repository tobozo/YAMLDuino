
#include <ArduinoJson.h>

// very dirty but necessary for the CI test
// don't do that in your project!!
#if ARDUINOJSON_VERSION_MAJOR<7
  #define ARDUINOJSONDOC DynamicJsonDocument json_doc(2048)
#else
  #define ARDUINOJSONDOC JsonDocument json_doc
#endif

//#define YAML_DISABLE_CJSON // not needed here
//#define YAML_DISABLE_ARDUINOJSON // not needed here

// those defines should always be set *before* including YAMLDuino.h

//#define YAML_DISABLE_ARDUINOJSON
//#define YAML_DISABLE_CJSON

#include <YAMLDuino.h>

// sorry about the notation, but it looks nicer than chunk-splitting+quoting
const char* yaml_sample_str = R"_YAML_STRING_(
first: true
fourth: false
blah:
  multiline_string_with_trailing_lf: |
    omg I'm multiline!
    whelp "I'm quoted" ? ! () { } "\t\r\n"
    slash\ed
  array_of_indexed_multiline_strings:
    - with_trailing_lf: |
        one !
        two "?"
        three ...
    - without_trailing_lf: |-
        four @'"]
        five [[(()
        six +-_%^&
  just_a_string: "I am a string"
  array_of_strings:
# some unindented comments
  - oops
  # more indented comments
  - meh
  same_array_of_strings: [ oops, meh ]
  array_mixed: [1,2,3, "soleil!"]
  array_of_anonymous_objects:
    - prop1: wizz
      prop2: pop
      prop3: snap
    - prop1: foo
      prop3: bar
      prop2: baz
      prop4: wat
  integer: 1234567890
  quoted_integer: "1234567890"
  # this float value gives a memleak to cJSON
  float: 12.3323
  quoted_float: "12.3323"
  inline_json_for_the_haters: { "hello":"json", "nested":[3,2,"1","moon"] }
whatever:
  nope: ["n","o","p","e"]
last: true

)_YAML_STRING_";

// exact JSON representation of yaml_sample_str
const char* json_sample_str = R"_JSON_STRING_(
{
  "first": true,
  "fourth": false,
  "blah": {
    "multiline_string_with_trailing_lf": "omg I'm multiline!\nwhelp \"I'm quoted\" ? ! () { } \"\\t\\r\\n\"\nslash\\ed\n",
    "array_of_indexed_multiline_strings": [{ "with_trailing_lf": "one !\ntwo \"?\"\nthree ...\n" }, { "without_trailing_lf": "four @'\"]\nfive [[(()\nsix +-_%^&" } ],
    "just_a_string": "I am a string",
    "array_of_strings": [ "oops", "meh" ],
    "same_array_of_strings": [ "oops", "meh" ],
    "array_mixed": [ 1, 2, 3, "soleil!" ],
    "array_of_anonymous_objects": [{
        "prop1": "wizz",
        "prop2": "pop",
        "prop3": "snap"
      }, {
        "prop1": "foo",
        "prop3": "bar",
        "prop2": "baz",
        "prop4": "wat"
      }
    ],
    "integer": 1234567890,
    "quoted_integer": "1234567890",
    "float": 12.3323,
    "quoted_float": "12.3323",
    "inline_json_for_the_haters": { "hello": "json", "nested": [ 3, 2, "1", "moon" ] }
  },
  "whatever": { "nope": [ "n", "o", "p", "e" ] },
  "last": true
}

)_JSON_STRING_";


// some valid/invalid paths to test gettext
const char* testpaths[] =
{
  "first",
  "blah:just_a_string",
  "blah:array_of_anonymous_objects:0:prop2",
  "second",
  "blah:inline_json_for_the_haters:nested:3" ,
  "blah:array_of_indexed_multiline_strings:0:with_trailing_lf",
  "invalid:path", // should fail
  "last"
};

const size_t yaml_str_size = strlen(yaml_sample_str);
const size_t json_str_size = strlen(json_sample_str);


#include "test_utils.h" // test loader/logger


// YAML/JSON loading/parsing using yaml_document_t and YAMLNode


void test_Yaml2JsonPretty()
{
  YAMLNode yamlnode = YAMLNode::loadString( yaml_sample_str );
  serializeYml( yamlnode.getDocument(), Serial, OUTPUT_JSON_PRETTY );
}

void test_Yaml2Json()
{
  YAMLNode yamlnode = YAMLNode::loadString( yaml_sample_str );
  serializeYml( yamlnode.getDocument(), Serial, OUTPUT_JSON );
}

void test_Json2Yaml()
{
  YAMLNode yamlnode = YAMLNode::loadString( json_sample_str );
  serializeYml( yamlnode.getDocument(), Serial, OUTPUT_YAML );
}


void test_Yaml_gettext_trait()
{
  const char* blah = YAMLNode::loadString(yaml_sample_str).gettext("blah:just_a_string"); // value should be "true"
  YAML_LOG_n( "[%s][=>] %s", "blah:just_a_string", blah );
}



void test_Yaml_gettext_string()
{
  YAMLNode root = YAMLNode::loadString( yaml_sample_str );
  size_t paths_count = sizeof( testpaths ) / sizeof( const char*);
  for( int i=0;i<paths_count; i++ ) {
    const char* text = root.gettext( testpaths[i] );
    YAML_LOG_n( "[%s][=>] %s", testpaths[i], text );
  }
}

void test_Json_gettext_string()
{
  YAMLNode root = YAMLNode::loadString( json_sample_str );
  size_t paths_count = sizeof( testpaths ) / sizeof( const char*);
  for( int i=0;i<paths_count; i++ ) {
    const char* text = root.gettext( testpaths[i] );
    YAML_LOG_n( "[%s][=>] %s", testpaths[i], text );
  }
}


void test_Yaml_gettext_stream()
{
  String yaml_str = String( yaml_sample_str );
  StringStream yaml_stream( yaml_str );
  YAMLNode root = YAMLNode::loadStream( yaml_stream );
  size_t paths_count = sizeof( testpaths ) / sizeof( const char*);
  for( int i=0;i<paths_count; i++ ) {
    const char* text = root.gettext( testpaths[i] );
    YAML_LOG_n( "[%s][=>] %s", testpaths[i], text );
  }
}

void test_Json_gettext_stream()
{
  String json_str = String( json_sample_str );
  StringStream json_stream( json_str );
  YAMLNode root = YAMLNode::loadStream( json_stream );
  size_t paths_count = sizeof( testpaths ) / sizeof( const char*);
  for( int i=0;i<paths_count; i++ ) {
    const char* text = root.gettext( testpaths[i] );
    YAML_LOG_n( "[%s][=>] %s", testpaths[i], text );
  }
}



#if defined HAS_ARDUINOJSON

  // The following functions are tested using 'const char*' and 'Stream&' as input/output types:
  //   deserializeYml( JsonObject, input )
  //   deserializeYml( JsonDocument, input )
  //   serializeYml( JsonObject, output )
  //   serializeYml( JsonDocument, output )


  void test_deserializeYml_JsonObject_YamlStream()
  {
    String yaml_str = String( yaml_sample_str );
    StringStream yaml_stream( yaml_str );
    ARDUINOJSONDOC;
    JsonObject json_obj = json_doc.to<JsonObject>();
    auto err = deserializeYml( json_obj, yaml_stream ); // deserialize yaml stream to JsonObject
    if( err ) {
      YAML_LOG_n("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
      return;
    }
    String str_json_out = ""; // JSON output string
    const size_t bytes_out = serializeJsonPretty( json_obj, str_json_out ); // print deserialized JsonObject
    Serial.println( str_json_out );
    YAML_LOG_n("[YAML=>JsonObject] yaml bytes in=%d, json bytes out=%d\n\n", yaml_str_size, bytes_out);
  }


  void test_deserializeYml_JsonObject_YamlString()
  {
    ARDUINOJSONDOC;
    JsonObject json_obj = json_doc.to<JsonObject>();
    auto err = deserializeYml( json_obj, yaml_sample_str ); // deserialize yaml string to JsonObject
    if( err ) {
      YAML_LOG_n("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
      return;
    }
    String str_json_out = ""; // JSON output string
    const size_t bytes_out = serializeJsonPretty( json_obj, str_json_out ); // print deserialized JsonObject
    Serial.println( str_json_out );
    YAML_LOG_n("[YAML=>JsonObject] yaml bytes in=%d, json bytes out=%d\n\n", yaml_str_size, bytes_out);
  }

  void test_deserializeYml_JsonDocument_YamlStream()
  {
    ARDUINOJSONDOC;
    String yaml_str = String( yaml_sample_str );
    StringStream yaml_stream( yaml_str );
    auto err = deserializeYml( json_doc, yaml_stream ); // deserialize yaml stream to JsonDocument
    if( err ) {
      YAML_LOG_n("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
      return;
    }
    String str_json_out = ""; // JSON output string
    const size_t bytes_out = serializeJsonPretty( json_doc, str_json_out ); // print deserialized JsonObject
    Serial.println( str_json_out );
    YAML_LOG_n("[YAML=>JsonObject] yaml bytes in=%d, json bytes out=%d\n\n", yaml_str_size, bytes_out);
  }


  void test_deserializeYml_JsonDocument_YamlString()
  {
    String yaml_str( yaml_sample_str );
    ARDUINOJSONDOC;
    auto err = deserializeYml( json_doc, yaml_str.c_str() ); // deserialize yaml string to JsonDocument
    if( err ) {
      YAML_LOG_n("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
      return;
    }
    JsonObject json_obj = json_doc.as<JsonObject>();
    String str_json_out = ""; // JSON output string
    const size_t bytes_out = serializeJsonPretty( json_obj, str_json_out ); // print deserialized JsonObject
    Serial.println( str_json_out );
    YAML_LOG_n("[YAML=>JsonObject] yaml bytes in=%d, json bytes out=%d\n\n", yaml_str_size, bytes_out);
  }

  void test_serializeYml_JsonObject_YamlStream()
  {
    // Convert JsonObject to yaml
    String str_yaml_out = ""; // YAML output string
    String json_str = String( json_sample_str );
    StringStream yaml_stream_out( str_yaml_out ); // Stream to str_yaml_out
    ARDUINOJSONDOC; // create and populate a JsonObject
    auto err = deserializeJson( json_doc, json_str.c_str() );
    if( err ) {
      YAML_LOG_n("Unable to deserialize demo JSON to JsonObject: %s", err.c_str() );
      return;
    }
    JsonObject json_obj = json_doc.as<JsonObject>();
    const size_t bytes_out = serializeYml( json_obj, yaml_stream_out );
    Serial.println( str_yaml_out );
    YAML_LOG_n("[JsonObject=>YAML] json bytes in=%d, yaml bytes out=%d\n\n", json_str_size, bytes_out );
  }


  void test_serializeYml_JsonObject_YamlString()
  {
    // Convert JsonObject to yaml
    String str_yaml_out = ""; // YAML output string
    String json_str = String( json_sample_str );
    ARDUINOJSONDOC; // create and populate a JsonObject
    auto err = deserializeJson( json_doc, json_str.c_str() );
    if( err ) {
      YAML_LOG_n("Unable to deserialize demo JSON to JsonObject: %s", err.c_str() );
      return;
    }
    JsonObject json_obj = json_doc.as<JsonObject>();
    const size_t bytes_out = serializeYml( json_obj, str_yaml_out );
    Serial.println( str_yaml_out );
    YAML_LOG_n("[JsonObject=>YAML] json bytes in=%d, yaml bytes out=%d\n\n", json_str_size, bytes_out );
  }

#endif // defined HAS_ARDUINOJSON





#if defined HAS_CJSON

  // The following functions are tested using 'const char*' and 'Stream&' as input/output types:
  //   deserializeYml( cJSON*, input )
  //   serializeYml( cJSON*, output )

  void test_deserializeYml_cJson_String()
  {
    cJSON* objPtr;
    int ret = deserializeYml( &objPtr, yaml_sample_str ); // deserialize YAML string into cJSON object
    if (ret<0) {
      Serial.println("deserializeYml failed");
      return;
    }
    YAML_LOG_n("Printing json");
    char* json = cJSON_Print( objPtr );
    if( !json ) {
      YAML_LOG_e("emtpy output, aborting");
      return;
    }
    size_t bytes_out = strlen( json );
    Serial.println( json );
    cJSON_free( json );
    cJSON_Delete( objPtr );
    YAML_LOG_n("[YAML=>cJsonObject] yaml bytes in=%d, json bytes out=%d\n", yaml_str_size, bytes_out );
  }


  void test_deserializeYml_cJson_Stream()
  {
    String yaml_str = String( yaml_sample_str );
    StringStream yaml_stream( yaml_str );
    cJSON* objPtr;
    int ret = deserializeYml( &objPtr, yaml_stream ); // deserialize YAML stream into cJSON object
    if (ret<0) {
      Serial.println("deserializeYml failed");
      return;
    }
    YAML_LOG_n("Printing json");
    char* json = cJSON_Print( objPtr );
    if( !json ) {
      YAML_LOG_e("emtpy output, aborting");
      return;
    }
    size_t bytes_out = strlen( json );
    Serial.println( json );
    cJSON_free( json );
    cJSON_Delete( objPtr );
    YAML_LOG_n("[YAML=>cJsonObject] yaml bytes in=%d, json bytes out=%d\n", yaml_str.length(), bytes_out );
  }


  void test_serializeYml_cJson_Stream()
  {
    cJSON* objPtr = cJSON_Parse( json_sample_str );
    size_t bytes_out = serializeYml( objPtr, Serial );
    cJSON_Delete( objPtr );
    YAML_LOG_n("[YAML=>cJsonObject=>YAML] yaml bytes in=%d, json bytes out=%d\n", json_str_size, bytes_out);
  }


  void test_serializeYml_cJson_String()
  {
    cJSON* objPtr = cJSON_Parse( json_sample_str );
    String yaml_dest_str;
    size_t bytes_out = serializeYml( objPtr, yaml_dest_str );
    Serial.println( yaml_dest_str );
    cJSON_Delete( objPtr );
    YAML_LOG_n("[YAML=>cJsonObject=>YAML] yaml bytes in=%d, json bytes out=%d\n", json_str_size, bytes_out );
  }

#endif




void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.print("Welcome to the YAML Test sketch\nRam free: ");
  Serial.print( HEAP_AVAILABLE() );
  Serial.println(" bytes");

  YAML::setLogLevel( YAML::LogLevelDebug ); // override sketch debug level (otherwise inherited)

  // test logger, anything under the previously set level shoud be invisible in the console
  YAML_LOG_v("This is a verbose message");
  YAML_LOG_d("This is a debug message");
  YAML_LOG_i("This is an info message");
  YAML_LOG_w("This is a warning message");
  YAML_LOG_e("This is an error message");

  // YAMLNode 'gettext' 96 bytes memleak happens once, so force it now
  { YAMLNode::loadString("{\"blah\":{\"stuff\":\"true\"}}").gettext("blah:stuff"); }
  #if defined HAS_CJSON
    // cJSON 'float' 464 bytes memleak happens once, so force it now
    { cJSON* objPtr = cJSON_Parse( "{\"float\":12.3323}" ); serializeYml( objPtr, Serial ); cJSON_Delete( objPtr ); }
  #endif

  Serial.println("\n");
  YAML_LOG_n("### JSON<=>YAML using libyaml:\n");

  YAML::setJSONIndent("  ", 8 ); // JSON -> two spaces per indent level, unfold objets up to 8 nesting levels
  YAML::setYAMLIndent( 3 ); // annoy your friends with 3 spaces indentation

  test_fn( test_Yaml_gettext_trait,  "gettext",      "YAML gettext (trait)",  "YAMLNode::loadString(const char*).gettext(const char*)" );
  test_fn( test_Yaml_gettext_stream, "gettext",      "YAML gettext (Stream)", "YAMLNode::gettext(const char*)" );
  test_fn( test_Json_gettext_stream, "gettext",      "JSON gettext (Stream)", "YAMLNode::gettext(const char*)" );
  test_fn( test_Yaml_gettext_string, "gettext",      "YAML gettext (String)", "YAMLNode::gettext(const char*)" );
  test_fn( test_Json_gettext_string, "gettext",      "JSON gettext (String)", "YAMLNode::gettext(const char*)" );

  test_fn( test_Yaml2JsonPretty,     "serializeYml", "Yaml2JsonPretty",       "serializeYml(yaml_document_t*, Stream&, OUTPUT_JSON_PRETTY)" );
  test_fn( test_Yaml2Json,           "serializeYml", "Yaml2Json",             "serializeYml(yaml_document_t*, Stream&, OUTPUT_JSON)" );
  test_fn( test_Json2Yaml,           "serializeYml", "Json2Yaml",             "serializeYml(yaml_document_t*, Stream&, OUTPUT_YAML)" );


  YAML_LOG_n("### YAMLParser libyaml tests complete\n");

  #if defined HAS_ARDUINOJSON
    #pragma message "Enabling ArduinoJson tests"
    Serial.println("\n");
    YAML_LOG_n("### YAML=>JSON and JSON=>YAML using ArduinoJson\n");
    #if !defined ARDUINO_ARCH_AVR
      test_fn( test_deserializeYml_JsonDocument_YamlStream, "deserializeYml", "YAML stream to JsonDocument", "deserializeYml(JsonDocument, Stream&)");
      test_fn( test_deserializeYml_JsonDocument_YamlString, "deserializeYml", "YAML string to JsonDocument", "deserializeYml(JsonDocument, const char*)");
      test_fn( test_deserializeYml_JsonObject_YamlString,   "deserializeYml", "YAML string to JsonObject",   "deserializeYml(JsonObject, const char*)");
      test_fn( test_serializeYml_JsonObject_YamlString,     "serializeYml",   "JsonObject to YAML stream",   "serializeYml(JsonObject, Stream&)");
    #endif
    test_fn( test_deserializeYml_JsonObject_YamlStream, "deserializeYml", "YAML stream to JsonObject", "deserializeYml(JsonObject, Stream&)");
    test_fn( test_serializeYml_JsonObject_YamlStream,   "serializeYml",   "JsonObject to YAML stream", "serializeYml(JsonObject, Stream&)");

    YAML_LOG_n("### ArduinoJson tests complete\n");
  #endif


  #if defined HAS_CJSON
    #pragma message "Enabling cJSON tests"
    Serial.println("\n");
    YAML_LOG_n("### YAML=>JSON and JSON=>YAML using cJSON:\n");
    test_fn( test_deserializeYml_cJson_String, "deserializeYml", "YAML string to cJSON Object", "deserializeYml(cJSON_obj*, const char*)");
    test_fn( test_deserializeYml_cJson_Stream, "deserializeYml", "YAML stream to cJSON Object", "deserializeYml(cJSON_obj*, Stream&)");
    test_fn( test_serializeYml_cJson_Stream,   "serializeYml",   "cJSON Object to YAML stream", "serializeYml(cJSON_obj*, Stream&)");
    test_fn( test_serializeYml_cJson_String,   "serializeYml",   "cJSON Object to YAML string", "serializeYml(cJSON_obj*, String&)");

    YAML_LOG_n("### cJSON tests complete\n");
  #endif


  printGlobalReport();

}


void loop()
{

}
