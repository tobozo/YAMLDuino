# ArduinoYaml

This library is based on [libyaml](https://github.com/yaml/libyaml).
It provides two ways to convert YAML strings to JSON (cJSON or ArduinoJson) objects.

Supported platforms (some untested):

- esp32
- esp8266
- samd
- rp2040


### Usage

#### cJSON

```cpp
#include <cJSON.h> // note: cJSON is built-in with esp32
#include <ArduinoYaml.h>

  // cJSON object to YAML string
  size_t serializeYml( cJSON* src_obj, String &dest_string );
  // cJSON object to YAML stream
  size_t serializeYml( cJSON* src_obj, Stream &dest_stream );
  // JSON stream to cJSON object to YAML stream
  size_t serializeYml( Stream &json_src_stream, Stream &yml_dest_stream );

  // YAML string to cJSON object
  int deserializeYml( cJSON* dest_obj, const char* src_yaml_str );
  // YAML stream to cJSON object
  int deserializeYml( cJSON* dest_obj, Stream &src_stream );
```

#### ArduinoJson


```cpp

#include <ArduinoJson.h>
#include <ArduinoYaml.h>

  // ArduinoJSON object to YAML string
  size_t serializeYml( JsonVariant src_obj, String &dest_string );
  // ArduinoJSON object to YAML stream
  size_t serializeYml( JsonVariant src_obj, Stream &dest_stream );
  // JSON stream to JsonObject to YAML stream
  size_t serializeYml( Stream &json_src_stream, Stream &yml_dest_stream );
  // Deserialize YAML string to ArduinoJSON object
  DeserializationError deserializeYml( JsonObject &dest_obj, const char* src_yaml_str );
  // Deserialize YAML stream to ArduinoJSON object
  DeserializationError deserializeYml( JsonObject &dest_obj, Stream &src_stream );
  // Deserialize YAML string to ArduinoJSON document
  DeserializationError deserializeYml( JsonDocument &dest_doc, Stream &src_stream );
  // Deserialize YAML string to ArduinoJSON document
  DeserializationError deserializeYml( JsonDocument &dest_doc, const char *src_yaml_str) ;

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
