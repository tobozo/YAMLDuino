#include <ArduinoJson.h>
//#define YAML_DISABLE_ARDUINOJSON
#include <YAMLDuino.h>

const char* yaml_sample_str = R"_YAML_STRING_(
first: true
blah:
  nope: ["n","o","p","e"]
integer: 12345
float: 12.3323
hex: 0x1234
dq_hex: "0x1234"
sq_hex: '0x1234'
last: true

)_YAML_STRING_";

// exact JSON representation of yaml_sample_str
const char* json_sample_str = R"_JSON_STRING_(
{
  "first": true,
  "blah": { "nope": [ "n", "o", "p", "e" ] },
  "integer": 12345,
  "float": 12.3323,
  "hex": 4660,
  "dq_hex": "0x1234",
  "sq_hex": "0x1234",
  "last": true
}

)_JSON_STRING_";


const size_t yaml_str_size = strlen(yaml_sample_str);
const size_t json_str_size = strlen(json_sample_str);


int test_number = 1;


// uncomment/comment as needed

#define TEST_YAML_TO_JSON
#define TEST_YAML_TO_JSON_PRETTY
#define TEST_JSON_TO_YAML
#define TEST_ArduinoJsonObject_TO_YAML_Stream
#define TEST_ArduinoJsonObject_TO_YAML_String


void test_Yaml2JsonPretty()
{
  #if defined TEST_YAML_TO_JSON_PRETTY
  Serial.println("serializeYml (OUTPUT_JSON_PRETTY)");
  YAMLNode yamlnode = YAMLNode::loadString( yaml_sample_str );
  serializeYml( yamlnode.getDocument(), Serial, OUTPUT_JSON_PRETTY );
  Serial.println();
  #endif
}

void test_Yaml2Json()
{
  #if defined TEST_YAML_TO_JSON
  Serial.println("serializeYml (OUTPUT_JSON)");
  YAMLNode yamlnode = YAMLNode::loadString( yaml_sample_str );
  serializeYml( yamlnode.getDocument(), Serial, OUTPUT_JSON );
  Serial.println();
  #endif
}


void test_Json2Yaml()
{
  #if defined TEST_JSON_TO_YAML
  Serial.println("serializeYml (OUTPUT_YAML)");
  YAMLNode yamlnode = YAMLNode::loadString( json_sample_str );
  serializeYml( yamlnode.getDocument(), Serial, OUTPUT_YAML );
  Serial.println();
  #endif
}


void test_serializeYml_JsonObject_YamlStream()
{
  #if defined TEST_ArduinoJsonObject_TO_YAML_Stream
  // Convert JsonObject to yaml
  String json_str = String( json_sample_str );
  JsonDocument doc; // create and populate a JsonObject
  auto err = deserializeJson( doc, json_str.c_str() );
  if( err ) {
    Serial.printf("Unable to deserialize demo JSON to JsonObject: %s", err.c_str() );
    return;
  }
  JsonObject json_obj = doc.as<JsonObject>();
  const size_t bytes_out = serializeYml( json_obj, Serial );
  #endif
}


void test_serializeYml_JsonObject_YamlString()
{
  #if defined TEST_ArduinoJsonObject_TO_YAML_String
  // Convert JsonObject to yaml
  String str_yaml_out = ""; // YAML output string
  String json_str = String( json_sample_str );
  JsonDocument doc; // create and populate a JsonObject
  auto err = deserializeJson( doc, json_str.c_str() );
  if( err ) {
    Serial.printf("Unable to deserialize demo JSON to JsonObject: %s", err.c_str() );
    return;
  }
  JsonObject json_obj = doc.as<JsonObject>();
  const size_t bytes_out = serializeYml( json_obj, str_yaml_out );
  Serial.println( str_yaml_out );
  #endif
}




void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.printf("Welcome to the YAML Test sketch\nRam free: %d bytes\n", HEAP_AVAILABLE() );

  // YAML::setLogLevel( YAML::LogLevelDebug ); // override sketch debug level (otherwise inherited)
  // YAML::setJSONIndent("  ", 8 ); // JSON -> two spaces per indent level, unfold objets up to 8 nesting levels
  // YAML::setYAMLIndent( 3 ); // annoy your friends with 3 spaces indentation

  test_Yaml2JsonPretty();
  test_Yaml2Json();
  test_Json2Yaml();
  test_serializeYml_JsonObject_YamlString();
  test_serializeYml_JsonObject_YamlStream();
}


void loop()
{

}

