# esp32-yaml

This library is based on [libyaml](https://github.com/yaml/libyaml).
It provides two ways to convert YAML strings to JSON (cJSON or ArduinoJson) objects.


### Usage

#### cJSON

```cpp
#include <cJSON.h> // built-in with esp32
#include <esp32-yaml.hpp>

void setup()
{
  // ...


  String yaml = http.getString(); // put your YAML here

  YAMLToCJson *Y2CJ = new YAMLToCJson();

  cJSON *object = Y2CJ->toJson( yaml.c_str() ); // get converted yaml as cJSON object

  if (object) {
    char *json = cJSON_Print(object);
    Serial.print( json );
    free(json);
  }
}
```

#### ArduinoJson

```cpp
#include <ArduinoJson.h>
#include <esp32-yaml.hpp>

void setup()
{
  // ...

  String yaml = http.getString(); // put your YAML here

  YAMLToArduinoJson *Y2J = new YAMLToArduinoJson( yaml.c_str() );

  JsonObject obj = Y2J->getJsonObject(); // get converted yaml as ArduinoJson object
  serializeJsonPretty(obj, Serial);
}
```



### Credits and special thanks to:

  - [libyaml](https://github.com/yaml/libyaml)
  - [yaml2json](https://github.com/vikman90/yaml2json)
  - [@bblanchon](https://github.com/bblanchon)
  - [@DaveGamble](https://github.com/DaveGamble)
  - [@espressif](https://github.com/espressif)

### Additional resources:

  - ArduinoJson : https://github.com/bblanchon/ArduinoJson
  - cJSON : https://github.com/DaveGamble/cJSON
