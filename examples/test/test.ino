#include <ArduinoJson.h> // optional
#include <cJSON.h>       // implicit with esp32, otherwise optional
#include <esp32-yaml.hpp>


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


void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.printf("[DEBUG] YAML (in):\n%s\n\n", yaml_example_str);

  YAMLParser::setLogLevel( YAML::LogLevelVerbose );


  #if __has_include(<ArduinoJson.h>)
    Serial.println("YAML to JSON ArduinoJson\n");
    YAMLToArduinoJson *Y2J = new YAMLToArduinoJson( yaml_example_str );
    JsonObject obj = Y2J->getJsonObject();
    serializeJsonPretty(obj, Serial);
  #endif


  #if __has_include(<cJSON.h>)
    Serial.println("\n\nYAML to cJSON:\n");
    YAMLToCJson *Y2CJ = new YAMLToCJson();
    cJSON * object = Y2CJ->toJson( yaml_example_str );
    if (object) {
      char * json = cJSON_Print(object);
      Serial.print( json );
      free(json);
    }
  #endif

}


void loop()
{

}
