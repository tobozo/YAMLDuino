#include <LittleFS.h>
#include <ArduinoJson.h>
#define YAML_DISABLE_CJSON // not needed here
#include <YAMLDuino.h>
#include <i18n/i18n.hpp>


void setup()
{
  Serial.begin(115200);

  Serial.printf( "[%d] %s\n", ESP.getFreeHeap(), "Hello i18n test");

  LittleFS.begin();

  i18n.setFS( &LittleFS );

  if(! i18n.setLocale("fr-FR") ) {

    Serial.printf( "[%d] %s\n", ESP.getFreeHeap(), "Error loading locale, halting");

    while(1) vTaskDelay(1);
  }

  Serial.printf( "[%d] %s\n", ESP.getFreeHeap(), "Locale loaded");
}


void loop()
{
  Serial.printf( "[%d] %s\n", ESP.getFreeHeap(), i18n.gettext("activerecord:errors:messages:record_invalid" ) ); // "La validation a échoué : %{errors}"
  Serial.printf( "[%d] %s\n", ESP.getFreeHeap(), i18n.gettext("date:abbr_day_names:2" ) ); // "mar"
  Serial.printf( "[%d] %s\n", ESP.getFreeHeap(), i18n.gettext("time:pm" ) ); // "pm", last element
  delay(1000);
}
