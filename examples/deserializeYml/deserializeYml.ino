#pragma GCC diagnostic ignored "-Wunused-variable"

#include <ArduinoJson.h>
#include <YAMLDuino.h>

#if ARDUINOJSON_VERSION_MAJOR<7
  #error "ArduinoJSON version is deprecated, please upgrade to 7.x"
#endif

const char* yaml_sample_str = R"_YAML_STRING_(
first: true
blah:
  nope: ["n","o","p","e"]
integer: 12345
float: 12.3323
qinteger: "12345"
qfloat: "12.3323"
last: true

)_YAML_STRING_";

// exact JSON representation of yaml_sample_str
const char* json_sample_str = R"_JSON_STRING_(
{
  "first": true,
  "blah": { "nope": [ "n", "o", "p", "e" ] },
  "integer": 12345,
  "float": 12.3323,
  "qinteger": "12345"
  "qfloat": "12.3323"
  "last": true
}

)_JSON_STRING_";


const size_t yaml_str_size = strlen(yaml_sample_str);
const size_t json_str_size = strlen(json_sample_str);
int test_number = 1;


// uncomment/comment as needed

#define TEST_YAML_Stream_To_ArduinoJsonObject
#define TEST_YAML_String_To_ArduinoJsonObject
#define TEST_YAML_Stream_To_ArduinoJsonDocument
#define TEST_YAML_String_To_ArduinoJsonDocument



void test_deserializeYml_JsonObject_YamlStream()
{
  #if defined TEST_YAML_Stream_To_ArduinoJsonObject
  String yaml_str = String( yaml_sample_str );
  StringStream yaml_stream( yaml_str );
  JsonDocument json_doc;
  JsonObject json_obj = json_doc.to<JsonObject>();
  auto err = deserializeYml( json_obj, yaml_stream ); // deserialize yaml stream to JsonObject
  if( err ) {
    Serial.printf("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
    return;
  }
  const size_t bytes_out = serializeJsonPretty( json_obj, Serial ); // print deserialized JsonObject
  #endif
}


void test_deserializeYml_JsonObject_YamlString()
{
  #if defined TEST_YAML_String_To_ArduinoJsonObject
  JsonDocument json_doc;
  JsonObject json_obj = json_doc.to<JsonObject>();
  auto err = deserializeYml( json_obj, yaml_sample_str ); // deserialize yaml string to JsonObject
  if( err ) {
    Serial.printf("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
    return;
  }
  const size_t bytes_out = serializeJsonPretty( json_obj, Serial ); // print deserialized JsonObject
  #endif
}


void test_deserializeYml_JsonDocument_YamlStream()
{
  #if defined TEST_YAML_Stream_To_ArduinoJsonDocument
  JsonDocument json_doc;
  String yaml_str = String( yaml_sample_str );
  StringStream yaml_stream( yaml_str );
  auto err = deserializeYml( json_doc, yaml_stream ); // deserialize yaml stream to JsonDocument
  if( err ) {
    Serial.printf("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
    return;
  }
  const size_t bytes_out = serializeJsonPretty( json_doc, Serial ); // print deserialized JsonObject
  #endif
}


void test_deserializeYml_JsonDocument_YamlString()
{
  #if defined TEST_YAML_String_To_ArduinoJsonDocument
  String yaml_str( yaml_sample_str );
  JsonDocument json_doc;
  auto err = deserializeYml( json_doc, yaml_str.c_str() ); // deserialize yaml string to JsonDocument
  if( err ) {
    Serial.printf("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
    return;
  }
  JsonObject json_obj = json_doc.as<JsonObject>();
  const size_t bytes_out = serializeJsonPretty( json_obj, Serial ); // print deserialized JsonObject
  #endif
}



void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.printf("Welcome to the YAML Test sketch\nRam free: %d bytes", HEAP_AVAILABLE() );

  // YAML::setLogLevel( YAML::LogLevelDebug ); // override sketch debug level (otherwise inherited)
  // YAML::setJSONIndent("  ", 8 ); // JSON -> two spaces per indent level, unfold objets up to 8 nesting levels
  // YAML::setYAMLIndent( 3 ); // annoy your friends with 3 spaces indentation

  test_deserializeYml_JsonDocument_YamlStream();
  test_deserializeYml_JsonDocument_YamlString();
  test_deserializeYml_JsonObject_YamlString();
  test_deserializeYml_JsonObject_YamlStream();

}


void loop()
{

}

