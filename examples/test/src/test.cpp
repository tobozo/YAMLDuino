#include <ArduinoJson.h>

// #define YAML_DISABLE_ARDUINOJSON
// #define YAML_DISABLE_CJSON

#include <YAMLDuino.h>


// sorry about the notation, but it looks nicer than chunk-splitting+quoting
const char* yaml_example_str = R"_YAML_STRING_(
first: true
fourth: false
blah:
  multiline_string: |
    omg I'm multiline!
    whelp "I'm quoted" ? ! () { } "\t\r\n"
    slash\ed
  array_of_indexed_multiline_strings:
    - multiline_string1: |
        one !
        two "?"
        three ...
    - multiline_string2: |
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
  integer: 12345
  # this float value gives a memleak to cJSON
  float: 12.3323
  inline_json_for_the_haters: { "hello":"json", "nested":[3,2,"1","moon"] }
whatever:
  nope: ["n","o","p","e"]
last: true

)_YAML_STRING_";


const char* json_example_str = R"_JSON_STRING_(
{
  "first": true,
  "fourth": false,
  "blah": {
    "multiline_string": "omg I'm multiline!\nwhelp \"I'm quoted\" ? ! () { } \"\\t\\r\\n\"\nslash\\ed",
    "array_of_indexed_multiline_strings": [{"multiline_string1": "one !\ntwo \"?\"\nthree ..."},{"multiline_string2": "four @'\"]\nfive [[(()\nsix +-_%^&"}],
    "just_a_string": "I am a string",
    "array_of_strings": ["oops", "meh" ],
    "same_array_of_strings": [ "oops", "meh" ],
    "array_mixed": [ 1, 2, 3, "soleil!" ],
    "array_of_anonymous_objects": [
      {
        "prop1": "wizz",
        "prop2": "pop",
        "prop3": "snap"
      },
      {
        "prop1": "foo",
        "prop3": "bar",
        "prop2": "baz",
        "prop4": "wat"
      }
    ],
    "integer": 12345,
    "float": 12.3323,
    "inline_json_for_the_haters": { "hello": "json", "nested": [ 3, 2, "1", "moon" ] }
  },
  "whatever": { "nope": [ "n", "o", "p", "e" ] },
  "last": true
}

)_JSON_STRING_";


const size_t yaml_str_size = strlen(yaml_example_str);
const size_t json_str_size = strlen(json_example_str);
int test_number = 1;



void test_Yaml_String_Parser()
{
  YAML_LOG_n( "[TEST #%d] parse JSON String to YAML Stream:", test_number++ );
  YAMLParser* parser = new YAMLParser();
  Stream* output_stream = &Serial;
  parser->setOutputStream( output_stream );
  parser->parse( json_example_str );
  delete parser;
  YAML_LOG_n("Tests complete");
}


void test_Yaml_Stream_Parser()
{
  YAML_LOG_n( "[TEST #%d] parse JSON Stream to YAML Stream:", test_number++ );
  YAMLParser* parser = new YAMLParser();
  Stream* output_stream = &Serial;
  parser->setOutputStream( output_stream );
  String json_in = String( json_example_str );
  StringStream input_stream( json_in );
  parser->parse( input_stream );
  delete parser;
  YAML_LOG_n("Tests complete");
}


void test_Yaml_String_Loader()
{
  YAML_LOG_n( "[TEST #%d] load JSON String and parse to YAML Stream:", test_number++ );
  YAMLParser* parser = new YAMLParser();
  Stream* output_stream = &Serial;
  parser->setOutputStream( output_stream );
  parser->load( json_example_str );
  // do something with parser->getDocument()
  parser->parse();
  delete parser;
  YAML_LOG_n("Tests complete");
}


void test_Yaml_Stream_Loader()
{
  YAML_LOG_n( "[TEST #%d] load JSON Stream and parse to YAML Stream:", test_number++ );
  YAMLParser* parser = new YAMLParser();
  Stream* output_stream = &Serial;
  parser->setOutputStream( output_stream );
  String json_in = String( json_example_str );
  StringStream input_stream( json_in );
  parser->load( input_stream );
  // do something with parser->getDocument()
  parser->parse();
  delete parser;
  YAML_LOG_n("Tests complete");
}


#if defined HAS_ARDUINOJSON

  void test_deserializeYml_JsonObject_YamlStream()
  {
    YAML_LOG_n( "[TEST #%d] YAML stream to JsonObject -> deserializeYml(json_obj, yaml_stream):", test_number++ );
    String yaml_str = String( yaml_example_str );
    StringStream yaml_stream( yaml_str );
    DynamicJsonDocument json_doc(2048);
    JsonObject json_obj = json_doc.as<JsonObject>();
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
    YAML_LOG_n( "[TEST #%d] YAML string to JsonObject -> deserializeYml(json_obj, yaml_example_str):", test_number++ );
    DynamicJsonDocument json_doc(2048);
    JsonObject json_obj = json_doc.as<JsonObject>();
    auto err = deserializeYml( json_obj, yaml_example_str ); // deserialize yaml string to JsonObject
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
    YAML_LOG_n( "[TEST #%d] YAML stream to JsonDocument -> deserializeYml(json_doc, yaml_stream):", test_number++ );
    DynamicJsonDocument json_doc(2048);
    String yaml_str = String( yaml_example_str );
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
    YAML_LOG_n( "[TEST #%d] YAML string to JsonDocument -> deserializeYml(json_doc, yaml_example_str):", test_number++ );
    String yaml_str( yaml_example_str );
    DynamicJsonDocument json_doc(2048);
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
    YAML_LOG_n( "[TEST #%d] JsonObject to YAML stream -> serializeYml(json_obj, yaml_stream_out):", test_number++ );
    String str_yaml_out = ""; // YAML output string
    String json_str = String( json_example_str );
    StringStream yaml_stream_out( str_yaml_out ); // Stream to str_yaml_out
    DynamicJsonDocument doc(2048); // create and populate a JsonObject
    auto err = deserializeJson( doc, json_str.c_str() );
    if( err ) {
      YAML_LOG_n("Unable to deserialize demo JSON to JsonObject: %s", err.c_str() );
      return;
    }
    JsonObject json_obj = doc.as<JsonObject>();
    const size_t bytes_out = serializeYml( json_obj, yaml_stream_out );
    Serial.println( str_yaml_out );
    YAML_LOG_n("[JsonObject=>YAML] json bytes in=%d, yaml bytes out=%d\n\n", json_str_size, bytes_out );
  }


  void test_serializeYml_JsonObject_YamlString()
  {
    // Convert JsonObject to yaml
    YAML_LOG_n( "[TEST #%d] JsonObject to YAML stream -> serializeYml(json_obj, str_yaml_out):", test_number++ );
    String str_yaml_out = ""; // YAML output string
    String json_str = String( json_example_str );
    DynamicJsonDocument doc(2048); // create and populate a JsonObject
    auto err = deserializeJson( doc, json_str.c_str() );
    if( err ) {
      YAML_LOG_n("Unable to deserialize demo JSON to JsonObject: %s", err.c_str() );
      return;
    }
    JsonObject json_obj = doc.as<JsonObject>();
    const size_t bytes_out = serializeYml( json_obj, str_yaml_out );
    Serial.println( str_yaml_out );
    YAML_LOG_n("[JsonObject=>YAML] json bytes in=%d, yaml bytes out=%d\n\n", json_str_size, bytes_out );
  }

#endif // defined HAS_ARDUINOJSON





#if defined HAS_CJSON


  void test_deserializeYml_cJson_String()
  {
    YAML_LOG_n( "[TEST #%d] YAML string to cJSON Object -> deserializeYml(cJSON_obj*, yaml_example_str):", test_number++ );
    cJSON* objPtr;
    int ret = deserializeYml( &objPtr, yaml_example_str ); // deserialize YAML string into cJSON object
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
    YAML_LOG_n( "[TEST #%d] YAML stream to cJSON Object -> deserializeYml(cJSON_obj*, yaml_stream):", test_number++ );
    String yaml_str = String( yaml_example_str );
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
    YAML_LOG_n( "[TEST #%d] cJSON Object to YAML stream -> serializeYml( objPtr, Serial ):", test_number++ );
    cJSON* objPtr = cJSON_Parse( json_example_str );
    size_t bytes_out = serializeYml( objPtr, Serial );
    cJSON_Delete( objPtr );
    YAML_LOG_n("[YAML=>cJsonObject=>YAML] yaml bytes in=%d, json bytes out=%d\n", json_str_size, bytes_out);
  }


  void test_serializeYml_cJson_String()
  {
    YAML_LOG_n( "[TEST #%d] cJSON Object to YAML string -> serializeYml( objPtr, yaml_dest_str ):", test_number++ );
    cJSON* objPtr = cJSON_Parse( json_example_str );
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

  YAMLParser::setLogLevel( YAML::LogLevelDebug ); // override sketch debug level (otherwise inherited)

  YAML_LOG_n("\n\nJSON=>YAML using libyaml:\n");

  test_Yaml_String_Parser();
  test_Yaml_Stream_Parser();
  test_Yaml_String_Loader();
  test_Yaml_Stream_Loader();

  YAML_LOG_n("YAMLParser libyaml tests complete");

  #if defined HAS_ARDUINOJSON
    #pragma message "Enabling ArduinoJson tests"
    YAML_LOG_n("YAML=>JSON and JSON=>YAML using ArduinoJson\n\n");
    test_deserializeYml_JsonObject_YamlString();
    test_deserializeYml_JsonObject_YamlStream();
    test_deserializeYml_JsonDocument_YamlStream();
    test_deserializeYml_JsonDocument_YamlString();
    test_serializeYml_JsonObject_YamlStream();
    test_serializeYml_JsonObject_YamlString();
    YAML_LOG_n("ArduinoJson tests complete");
  #endif


  #if defined HAS_CJSON
    #pragma message "Enabling cJSON tests"
    YAML_LOG_n("\n\nYAML=>JSON and JSON=>YAML using cJSON:\n");
    test_serializeYml_cJson_Stream();
    test_serializeYml_cJson_String();
    test_deserializeYml_cJson_Stream();
    test_deserializeYml_cJson_String();
    YAML_LOG_n("cJSON tests complete");
  #endif
}


void loop()
{

}
