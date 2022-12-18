#include <LittleFS.h>
#include <YAMLDuino.h>

#include <vector>
static std::vector<String> i18nFiles;

const char* extension = "yml";
const char* path = "/lang";

i18n_t i18n( &LittleFS ); // attach LittleFS to i18n loader

void setup()
{
  Serial.begin(115200);
  LittleFS.begin();

  Serial.println( "Hello i18n test");

  // scan the lang folder and store filenames in an array

  File dir =  LittleFS.open( path );

  if( !dir ) {
    Serial.println("Error, can't access filesystem, halting");
    while(1) vTaskDelay(1);
  }

  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) { // no more files
      break;
    }
    if (entry.isDirectory()) continue;

    String fileName = entry.path();
    if( fileName.endsWith(  extension ) ) {
      i18nFiles.push_back( fileName );
    }
    entry.close();
  }

  dir.close();
}


void loop()
{
  int randLang = rand()%(i18nFiles.size());

  int free_heap_before_load = HEAP_AVAILABLE();

  if(! i18n.setLocale(i18nFiles[randLang].c_str()) ) {

    YAML_LOG_n( "Error loading locale %s, halting\n", i18nFiles[randLang].c_str());

    while(1) vTaskDelay(1);
  }

  int free_heap_after_load = HEAP_AVAILABLE();

  YAML_LOG_n( "[%d-%d] Locale file %s loaded\n", free_heap_before_load, free_heap_after_load, i18nFiles[randLang].c_str());

  Serial.println( i18n.gettext("activerecord:errors:messages:record_invalid" ) ); // "La validation a échoué : %{errors}"
  Serial.println( i18n.gettext("date:abbr_day_names:2" ) ); // "mar"
  Serial.println( i18n.gettext("time:pm" ) ); // "pm", last element
  delay( 1000 );
}
