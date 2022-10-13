#include <ArduinoJson.h> // optional
//#include <cJSON.h>       // implicit with esp32, otherwise optional
#include <YAMLDuino.h>


// sorry about the notation, but it looks nicer than chunk-splitting+quoting
const char* yaml_example_str = R"_YAML_STRING_(
first: "true"
blah:
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
  float: 12.3323
  inline_json_for_the_haters: { "hello":"json", "nested":[3,2,1,"moon"] }
whatever:
  nope: ["n","o","p","e"]
last: "true"

)_YAML_STRING_";


const char* json_example_str = R"_JSON_STRING_(
{
  "first": "true",
  "blah": {
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
    "inline_json_for_the_haters": { "hello": "json", "nested": [ 3, 2, 1, "moon" ] }
  },
  "whatever": { "nope": [ "n", "o", "p", "e" ] },
  "last": "true"
}
)_JSON_STRING_";


void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.print("Welcome to the YAML Test sketch\nRam free: ");
  Serial.print( HEAP_AVAILABLE() );
  Serial.println(" bytes");

  const size_t yaml_str_size = strlen(yaml_example_str);
  const size_t json_str_size = strlen(json_example_str);
  int test_number = 1;

  Serial.printf("[DEBUG] YAML (in):\n%s\n\n", yaml_example_str);

  YAMLParser::setLogLevel( YAML::LogLevelDebug ); // override sketch debug level (otherwise inherited)


  #if defined HAS_ARDUINOJSON
    #pragma message "Enabling ArduinoJson tests"

    YAML_LOG_n("YAML=>JSON and JSON=>YAML using ArduinoJson\n\n");

    #if !defined ARDUINO_ARCH_SAMD // samd with 32kb ram can oom after this, so only enable for specific test
    {
      YAML_LOG_n( "[TEST #%d] YAML stream to JsonObject -> deserializeYml(json_doc, yaml_stream):", test_number++ );
      DynamicJsonDocument json_doc(yaml_str_size*2);
      String yaml_str = String( yaml_example_str );
      StringStream yaml_stream( yaml_str );
      auto err = deserializeYml( json_doc, yaml_stream ); // deserialize yaml stream to JsonDocument
      if( err ) {
        YAML_LOG_n("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
        return;
      }
      const size_t bytes_out = serializeJsonPretty( json_doc, Serial ); // print deserialized JsonObject
      YAML_LOG_n("[YAML=>JsonObject] yaml bytes in=%d, json bytes out=%d\n\n", yaml_str_size, bytes_out);
    }
    #endif


    {
      YAML_LOG_n( "[TEST #%d] YAML string to JsonObject -> deserializeYml(json_doc, yaml_example_str):", test_number++ );
      DynamicJsonDocument json_doc(yaml_str_size*2);
      auto err = deserializeYml( json_doc, yaml_example_str ); // deserialize yaml string to JsonDocument
      if( err ) {
        YAML_LOG_n("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
        return;
      }
      JsonObject json_obj = json_doc.as<JsonObject>();
      const size_t bytes_out = serializeJsonPretty( json_obj, Serial ); // print deserialized JsonObject
      YAML_LOG_n("[YAML=>JsonObject] yaml bytes in=%d, json bytes out=%d\n\n", yaml_str_size, bytes_out);
    }





    {
      // Convert JsonObject to yaml
      YAML_LOG_n( "[TEST #%d] JsonObject to YAML stream -> serializeYml(json_obj, yaml_stream_out):", test_number++ );
      String str_yaml_out = ""; // YAML output string
      StringStream yaml_stream_out( str_yaml_out ); // Stream to str_yaml_out
      // create and populate a JsonObject
      DynamicJsonDocument doc(json_str_size*2);
      auto err = deserializeJson( doc, json_example_str );
      if( err ) {
        YAML_LOG_n("Unable to deserialize demo JSON to JsonObject: %s", err.c_str() );
        return;
      }
      JsonObject json_obj = doc.as<JsonObject>();
      const size_t bytes_out = serializeYml( json_obj, yaml_stream_out );
      Serial.println( str_yaml_out );
      YAML_LOG_n("[JsonObject=>YAML] json bytes in=%d, yaml bytes out=%d\n\n", json_str_size, bytes_out );
    }


    {
      // Convert JsonObject to yaml
      YAML_LOG_n( "[TEST #%d] JsonObject to YAML stream -> serializeYml(json_obj, str_yaml_out):", test_number++ );
      String str_yaml_out = ""; // YAML output string
      //StringStream yaml_stream_out( str_yaml_out ); // Stream to str_yaml_out
      // create and populate a JsonObject
      DynamicJsonDocument doc(json_str_size*2);
      auto err = deserializeJson( doc, json_example_str );
      if( err ) {
        YAML_LOG_n("Unable to deserialize demo JSON to JsonObject: %s", err.c_str() );
        return;
      }
      JsonObject json_obj = doc.as<JsonObject>();
      const size_t bytes_out = serializeYml( json_obj, str_yaml_out );
      Serial.println( str_yaml_out );
      YAML_LOG_n("[JsonObject=>YAML] json bytes in=%d, yaml bytes out=%d\n\n", json_str_size, bytes_out );
    }


    #if defined USE_STREAM_TO_STREAM // stream to stream unavailable on esp8266 (not enough memory)
    #pragma message "Enabling ArduinoJson stream<=>stream tests"
    {
      YAML_LOG_n( "[TEST #%d] JSON stream to JsonObject to YAML stream -> serializeYml(stream_in, Serial):", test_number++ );
      String str_json = String( json_example_str );
      StringStream stream_in( str_json );
      const size_t bytes_out = serializeYml( stream_in, Serial );
      YAML_LOG_n("[JSON=>JsonObject=>YAML] json bytes in=%d, yaml bytes out=%d\n", json_str_size, bytes_out);
    }
    #endif


    YAML_LOG_n("ArduinoJson tests complete");

  #endif




  #if __has_include(<cJSON.h>)
    #pragma message "Enabling cJSON tests"



    YAML_LOG_n("\n\nYAML=>JSON and JSON=>YAML using cJSON:\n");


    {
      YAML_LOG_n( "YAML string to cJSON Object -> deserializeYml(cJSON_obj*, yaml_example_str):" );
      // deserialize YAML string into cJSON object
      cJSON* objPtr = (cJSON*)malloc( sizeof(cJSON) ); // allocate minimal memory to empty object
      int ret = deserializeYml( objPtr, yaml_example_str );
      if (!ret) {
        Serial.println("deserializeYml failed");
        return;
      }
      YAML_LOG_n("Printing json");
      char* json = cJSON_Print( objPtr );
      if( !json ) {
        YAML_LOG_e("emtpy output, aborting");
        return;
      }
      String str_json_out = String( json );
      free(json);
      Serial.print( str_json_out );
      cJSON_Delete( objPtr );
      YAML_LOG_n("[YAML=>cJsonObject] yaml bytes in=%d, json bytes out=%d\n", yaml_str_size, str_json_out.length() );
    }



    {
      YAML_LOG_n( "YAML stream to cJSON Object -> deserializeYml(cJSON_obj*, yaml_stream):" );
      String yaml_str = String( yaml_example_str );
      StringStream yaml_stream( yaml_str );
      cJSON* objPtr = (cJSON*)malloc( sizeof(cJSON) ); // allocate minimal memory to empty object
      // deserialize YAML stream into cJSON object
      int ret = deserializeYml( objPtr, yaml_stream );
      if (!ret) {
        Serial.println("deserializeYml failed");
        return;
      }
      YAML_LOG_n("Printing json");
      char* json = cJSON_Print( objPtr );
      if( !json ) {
        YAML_LOG_e("emtpy output, aborting");
        return;
      }
      size_t bytes_out = strlen(json);
      Serial.print( json );
      free(json);
      cJSON_Delete( objPtr );
      YAML_LOG_n("[YAML=>cJsonObject] yaml bytes in=%d, json bytes out=%d\n", yaml_str.length(), bytes_out );
    }


    {
      YAML_LOG_n( "cJSON Object to YAML stream -> serializeYml( objPtr, Serial ):" );
      cJSON* objPtr = cJSON_Parse( json_example_str );
      size_t bytes_out = serializeYml( objPtr, Serial );
      cJSON_Delete( objPtr );
      YAML_LOG_n("[YAML=>cJsonObject=>YAML] yaml bytes in=%d, json bytes out=%d\n", json_str_size, bytes_out);
    }


    {
      YAML_LOG_n( "cJSON Object to YAML string -> serializeYml( objPtr, yaml_dest_str ):" );
      cJSON* objPtr = cJSON_Parse( json_example_str );
      String yaml_dest_str;
      size_t bytes_out = serializeYml( objPtr, yaml_dest_str );
      Serial.println( yaml_dest_str );
      cJSON_Delete( objPtr );
      YAML_LOG_n("[YAML=>cJsonObject=>YAML] yaml bytes in=%d, json bytes out=%d\n", json_str_size, bytes_out );
    }


    YAML_LOG_n("cJSON tests complete");


  #endif


}


void loop()
{

}
