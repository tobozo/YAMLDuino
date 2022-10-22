# ArduinoYaml A.K.A YAMLDuino


[![arduino-library-badge](https://www.ardu-badge.com/badge/YAMLDuino.svg?)](https://www.ardu-badge.com/YAMLDuino)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/tobozo/library/YAMLDuino.svg?)](https://registry.platformio.org/packages/libraries/tobozo/YAMLDuino)

![](https://raw.githubusercontent.com/tobozo/YAMLDuino/main/assets/sleazy-logo-with-title.png)


This arduino library is based on [libyaml](https://github.com/yaml/libyaml).

It provides several ways to convert YAML<=>JSON using libyaml, cJSON or ArduinoJson objects.

Supported platforms (some untested):

- ESP32
- RP2040
- ESP8266
- SAMD



### Usage

```cpp
#include <ArduinoYaml.h>

```

or

```cpp
#include <YAMLDuino.h>

```




#### pure libyaml

YAML is a superset of JSON, so native conversion from JSON is possible without any additional JSON library.

```cpp
#include <ArduinoYaml.h>

  // JSON stream to YAML stream
  size_t serializeYml( Stream &json_src_stream, Stream &yml_dest_stream );

```


#### ArduinoJson


```cpp
#include <ArduinoJson.h> // include this first or functions will be disabled
#include <ArduinoYaml.h>

  // ArduinoJSON object to YAML string
  size_t serializeYml( JsonVariant src_obj, String &dest_string );
  // ArduinoJSON object to YAML stream
  size_t serializeYml( JsonVariant src_obj, Stream &dest_stream );
  // Deserialize YAML string to ArduinoJSON object
  DeserializationError deserializeYml( JsonObject &dest_obj, const char* src_yaml_str );
  // Deserialize YAML stream to ArduinoJSON object
  DeserializationError deserializeYml( JsonObject &dest_obj, Stream &src_stream );
  // Deserialize YAML string to ArduinoJSON document
  DeserializationError deserializeYml( JsonDocument &dest_doc, Stream &src_stream );
  // Deserialize YAML string to ArduinoJSON document
  DeserializationError deserializeYml( JsonDocument &dest_doc, const char *src_yaml_str) ;

```



#### cJSON

⚠️ cJSON has a memory leak with floats, the leak happens only once though, and may be
avoided by quoting the float, which won't affect yaml output.


```cpp
// #include <cJSON.h> // no need to include, cJSON is built-in with esp32 and also bundled with ArduinoYaml
#include <ArduinoYaml.h>

  // cJSON object to YAML string
  size_t serializeYml( cJSON* src_obj, String &dest_string );
  // cJSON object to YAML stream
  size_t serializeYml( cJSON* src_obj, Stream &dest_stream );
  // YAML string to cJSON object
  int deserializeYml( cJSON* dest_obj, const char* src_yaml_str );
  // YAML stream to cJSON object
  int deserializeYml( cJSON* dest_obj, Stream &src_stream );

```





### Credits and special thanks to:

  - [@yaml](https://github.com/yaml)
  - [@DaveGamble](https://github.com/DaveGamble)
  - [@bblanchon](https://github.com/bblanchon)
  - [@vikman90](https://github.com/vikman90/yaml2json)

### Additional resources:

  - ArduinoJson : https://github.com/bblanchon/ArduinoJson
  - cJSON : https://github.com/DaveGamble/cJSON
  - libyaml : https://github.com/yaml/libyaml
