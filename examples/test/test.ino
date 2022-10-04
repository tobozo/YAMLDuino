#include <ArduinoJson.h>
#include <cJSON.h>
#include <esp32-yaml.hpp>

const char* yaml_example_str = R"DBG_FMT(

blah:
  main:
# some unindented comments
  - oops
  # more indented comments
  - meh
  integer: 12345
  float: 12.3323
  array: [1,2,3, "soleil!"]
  just_a_string: "I am a string"
  inline_json_for_the_haters: { "hello":"json", "nested":[3,2,1,"moon"] }
whatever:
  nope: ["n","o","p","e"]
first: "true"
)DBG_FMT";



void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.printf("[DEBUG] YAML (in):\n%s\n\n", yaml_example_str);


  Serial.println("YAML to JSON ArduinoJson\n");
  YAMLToArduinoJson *Y2J = new YAMLToArduinoJson( yaml_example_str );
  JsonObject obj = Y2J->getJsonObject();
  serializeJsonPretty(obj, Serial);


  Serial.println("\n\nYAML to cJSON:\n");
  YAMLToCJson *Y2CJ = new YAMLToCJson();
  cJSON * object = Y2CJ->toJson( yaml_example_str );
  if (object) {
    char * json = cJSON_Print(object);
    Serial.print( json );
    free(json);
  }
}


void loop()
{

}
