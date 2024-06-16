#include <ArduinoJson.h>
#include <YAMLDuino.h>

#if defined ARDUINO_M5Stack_Core_ESP32
  #include <M5Stack.h>
  #define FS_t fs::FS
  #define File_t fs::File
  #define delay_fn vTaskDelay
#elif defined CORE_TEENSY
  #include <FS.h>
  #include <SD.h>
  #define FS_t FS
  #define File_t File
  #define delay_fn delay
#else
  #error "please implement"
#endif


const char* yaml_example_str = R"_YAML_STRING_(

my_setting: false

flag1: true
flag2: true

settings1:
  just_a_string: "I am a string"
  integer: 12345
  float: 12.3323

settings2:
  nope: ["n","o","p","e"]


)_YAML_STRING_";


const char* config_file = "/config.yml";
const char* nodename = "my_setting"; // property name in the config
const bool default_value = false; // default value for property
bool current_value = default_value;
bool config_loaded = false; // prevent updates if config isn't loaded
DynamicJsonDocument json_doc(2048);
JsonObject myConfig; // json accessor


bool writeTestYaml( FS_t &fs, const char* path )
{
  File_t file = fs.open( path, FILE_WRITE );
  if( !file ) {
    Serial.println("Can't open file for writing");
    return false;
  }
  size_t written = file.write( (const uint8_t*)yaml_example_str, strlen( yaml_example_str ) );
  file.close();
  //Serial.printf("Example file created (%d bytes)\n", written);
  return written > 0;
}


bool loadYamlConfig()
{
  File_t file = SD.open( config_file );
  if( !file ) {
    Serial.println("Can't open test file for writing :-(");
    return false;
  }
  auto err = deserializeYml( json_doc, file ); // convert yaml to json
  file.close();
  if( err ) {
    Serial.printf("Unable to deserialize YAML to JsonDocument: %s\n", err.c_str() );
    return false;
  }
  myConfig = json_doc.as<JsonObject>();
  current_value = myConfig[nodename].as<bool>();
  //serializeJson( myConfig, Serial );
  //Serial.println();
  return true;
}


bool saveYamlConfig()
{
  File_t file = SD.open( config_file, FILE_WRITE);
  if( !file ) {
    Serial.println("Can't open file for writing");
    return false;
  }
  const size_t bytes_out = serializeYml( myConfig, file );
  file.close();
  //Serial.printf("Written %d bytes\n", bytes_out );
  return bytes_out > 0;
}


bool toggleYamlProperty()
{
  if( !config_loaded ) return false;
  //Serial.printf("Initial value: [%s] = %s\n", nodename, current_value ? "true" : "false" );
  current_value = !current_value;
  Serial.printf("New value: [%s] = %s\n", nodename, current_value ? "true" : "false" );
  myConfig[nodename] = current_value;
  return saveYamlConfig();
}


void setup()
{
  #if defined ARDUINO_M5Stack_Core_ESP32
    M5.begin();
    if( M5.BtnA.isPressed() ) {
      SD.remove( config_file );
      Serial.println("Deleted config file");
      while( M5.BtnA.isPressed() ) { M5.update(); } // wait for release
      ESP.restart();
    }
  #elif defined CORE_TEENSY
    Serial.begin(115200);
    SD.begin( BUILTIN_SDCARD );
  #else
    #error "please implement"
  #endif



  _load_config:
  config_loaded = loadYamlConfig();

  if( !config_loaded ) {
    Serial.printf("Ceating config file %s\n", config_file );
    if( !writeTestYaml( SD, config_file ) ) {
      Serial.println("Could not create config file, aborting");
      while(1) delay_fn(1);
    }
    // write succeeded, reload config
    goto _load_config;
  }

  Serial.printf("Current config value: [%s] = %s\n", nodename, current_value ? "true" : "false" );
}



void loop()
{
  #if defined ARDUINO_M5Stack_Core_ESP32
  M5.update();

  if( M5.BtnB.wasPressed() ) {
    if( !toggleYamlProperty() ) {
      Serial.println("Failed to save property");
    }
  }
  #endif
}

