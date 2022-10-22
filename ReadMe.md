# ArduinoYaml A.K.A YAMLDuino


[![arduino-library-badge](https://www.ardu-badge.com/badge/YAMLDuino.svg?)](https://www.ardu-badge.com/YAMLDuino)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/tobozo/library/YAMLDuino.svg?)](https://registry.platformio.org/packages/libraries/tobozo/YAMLDuino)

![](https://raw.githubusercontent.com/tobozo/YAMLDuino/main/assets/sleazy-logo-with-title.png)


This arduino library is based on [libyaml](https://github.com/yaml/libyaml).


### Supported platforms:

- ESP32
- RP2040
- ESP8266
- SAMD

### Features:

- YAML➔JSON and JSON➔YAML conversion
- ArduinoJson serializers/deserializers
- cJSON serializers/deserializers


----------------------------


## Usage

```cpp
#include <ArduinoYaml.h>

```

or

```cpp
#include <YAMLDuino.h>

```

----------------------------


## Pure libyaml implementation

YAML is a superset of JSON, so native conversion from/to JSON is possible without any additional JSON library.

```cpp
  // JSON <=> YAML stream to stream conversion (both ways!), accepts valid JSON or YAML as the input
  // Available values for output format:
  //   YAMLParser::OUTPUT_YAML
  //   YAMLParser::OUTPUT_JSON
  //   YAMLParser::OUTPUT_JSON_PRETTY
  size_t serializeYml( Stream &source, Stream &destination, OutputFormat_t format );

```


**Convert YAML to JSON**
```cpp

  String yaml_str = "hello: world\nboolean: true\nfloat: 1.2345";
  StringStream yaml_stream( yaml_str );

  serializeYml( yaml_stream, Serial, YAMLParser::OUTPUT_JSON_PRETTY );

```



**Convert JSON to YAML**
```cpp

  String json_str = "{\"hello\": \"world\", \"boolean\": true, \"float\":1.2345}";
  StringStream json_stream( json_str );

  serializeYml( json_stream, Serial, YAMLParser::OUTPUT_YAML );

```

----------------------------

## Bindings

ArduinoJson and cJSON bindings operate differently depending on the platform.


|                 | ArduinoJson support |         cJSON support        |
|-----------------|---------------------|------------------------------|
|        ESP32    |    detected (*)     |  implicit (built-in esp-idf) |
|        ESP8266  |    implicit         |  implicit (bundled)          |
|        RP2040   |    implicit         |  implicit (bundled)          |
|        SAMD     |    implicit         |  implicit (bundled)          |


(*) On ESP32 platform, the detection depends on `__has_include(<ArduinoJson.h>)` macro.
So all ArduinoJson functions will be disabled unless `#include <ArduinoJson.h>` is found **before** `#include <ArduinoYaml.h>`.

On ESP8266/RP2040/SAMD platforms it is assumed that ArduinoJson is already available as a dependency.


In order to save flash space and/or memory, the defaults bindings can be disabled independently by setting one or all of the
following macros before including ArduinoYaml:

```cpp
#define YAML_DISABLE_ARDUINOJSON // disable all ArduinoJson functions
#define YAML_DISABLE_CJSON       // disable all cJSON functions
```

Note to self: this should probably be the other way around e.g. explicitely enabled by user.

Note to readers: should ArduinoJson and/or cJSON be implicitely loaded?
[Feedback is welcome!](https://github.com/tobozo/YAMLDuino/issues)


----------------------------

## ArduinoJson bindings

The support is implicitely enabled on most platforms.


```cpp
  #include <ArduinoJson.h> // ESP32 plaforms must include this before ArduinoYaml or functions will be disabled
  #include <ArduinoYaml.h>
```

Enabling support will expose the following functions:

```cpp
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

----------------------------

## cJSON bindinds

The support is implicitely enabled on most platforms and will use the bundled cJSON version.
ESP32 will use the built-in version.


⚠️ both versions of cJSON have a memory leak with floats, the leak happens only once though, and
may be avoided by quoting the float, which won't affect yaml output.


Enabling support will expose the following functions:

```cpp

  // cJSON object to YAML string
  size_t serializeYml( cJSON* src_obj, String &dest_string );
  // cJSON object to YAML stream
  size_t serializeYml( cJSON* src_obj, Stream &dest_stream );
  // YAML string to cJSON object
  int deserializeYml( cJSON* dest_obj, const char* src_yaml_str );
  // YAML stream to cJSON object
  int deserializeYml( cJSON* dest_obj, Stream &src_stream );

```

----------------------------

## String/Stream helper

Although `const char*` is an acceptable source type for conversion, using `Stream` is recommended as it is more memory efficient.

The `StringStream` class is provided with this library as a helper.

```cpp

  String my_json = "{\"blah\":true}";

  StringStream json_input_stream(my_json);

  String my_output;
  StringStream output_stream(my_output);

```

The `StringStream` bundled class is based on Arduino `String` and can easily be replaced by any class inheriting from `Stream`.

```cpp
class StringStream : public Stream
{
public:
  StringStream(String &s) : str(s), pos(0) {}
  virtual ~StringStream() {};
  virtual int available() { return str.length() - pos; }
  virtual int read() { return pos<str.length() ? str[pos++] : -1; }
  virtual int peek() { return pos<str.length() ? str[pos] : -1; }
  virtual size_t write(uint8_t c) { str += (char)c; return 1; }
  virtual void flush() {}
private:
  String &str;
  unsigned int pos;
};
```

See [ArduinoStreamUtils](https://github.com/bblanchon/ArduinoStreamUtils) for other types of streams (i.e. buffered).


----------------------------


## Output decorators


JSON and YAML indentation levels can be customized:


```cpp
  void YAML::setYAMLIndent( int spaces_per_indent=2 ); // min=2, max=16
  void YAML::setJSONIndent( const char* spaces_or_tabs=JSON_SCALAR_TAB, int folding_depth=JSON_FOLDING_DEPTH );

```


Set custom JSON indentation and folding depth:

```cpp
// two spaces per indentation level, unfold up to 8 nesting levels
YAMLParser::setJSONIndent("  ", 8 ); // lame fact: folds on objects, not on arrays

```


Set custom YAML indentation (minimum=2, max=16):

```cpp
// annoy your friends with 3 spaces indentation, totally valid in YAML
YAML::setYAMLIndent( 3 );

```


----------------------------


## Debug


The debug level can be changed at runtime:


```cpp
  void YAML::setLogLevel( LogLevel_t level );
```

Set library debug level:

```cpp
//
// Accepted values:
//   LogLevelNone    : No logging
//   LogLevelError   : Errors
//   LogLevelWarning : Errors+Warnings
//   LogLevelInfo    : Errors+Warnings+Info
//   LogLevelDebug   : Errors+Warnings+Info+Debug
//   LogLevelVerbose : Errors+Warnings+Info+Debug+Verbose
YAMLParser::setLogLevel( YAML::LogLevelDebug );
```

----------------------------

## Credits and special thanks to:

  - [@yaml](https://github.com/yaml)
  - [@DaveGamble](https://github.com/DaveGamble)
  - [@bblanchon](https://github.com/bblanchon)
  - [@vikman90](https://github.com/vikman90/yaml2json)

## Additional resources:

  - ArduinoJson : https://github.com/bblanchon/ArduinoJson
  - ArduinoStreamUtils : https://github.com/bblanchon/ArduinoStreamUtils
  - cJSON : https://github.com/DaveGamble/cJSON
  - libyaml : https://github.com/yaml/libyaml
