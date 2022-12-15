/*\
 *
 * YAMLDuino
 * Project Page: https://github.com/tobozo/YAMLDuino
 *
 * Copyright 2022 tobozo http://github.com/tobozo
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files ("YAMLDuino"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
\*/

#include "./i18n.hpp"

#if defined I18N_SUPPORT && defined HAS_ARDUINOJSON

#include <vector>


// L10N: Return localized string when given a path e.g. 'blah:some:property:count:1'
const char* l10n_t::gettext(const char* l10npath, char delimiter )
{
  if( !l10npath ) return "";
  const char delimStr[2] = { delimiter, 0 };
  char* l10npathCopy = strdup( l10npath );
  char *found = NULL;
  JsonVariantConst _docref = docref; // copy ref to language root node

  if( _docref.isNull()) goto _not_found; // uh-oh, language not loaded

  if( strchr( l10npath, delimiter ) == NULL ) { // no delimiter found, just a key
    if( _docref[l10npath].isNull() ) goto _not_found; // no property under this name
    _docref = _docref[l10npath]; // ini file style
    goto _success;
  }

  // walk through delimited properties
  found = strtok( l10npathCopy, delimStr );

  while( found != NULL ) {
    if( !_docref[found].isNull() ) { // map key->val
      _docref = _docref[found];
    } else {
      int idx = atoi( found );
      if( idx >= 0 && !_docref[idx].isNull() ) { // array index->val
        _docref = _docref[idx];
      } else {
        goto _not_found; // not even a number
      }
    }
    found = strtok( NULL, delimStr ); // get next token
  }

  if( _docref.isNull() ) goto _not_found; // no match

  _success:
    free( l10npathCopy );
    return _docref.as<const char*>();

  _not_found:
    free( l10npathCopy );
    return l10npath; // not found
}





void i18n_t::setFS( fs::FS *_fs )
{
  assert( _fs );
  fs = _fs;
}


void i18n_t::freel10n()
{
  if( l10n.docref != NULL ) l10n.docref = NULL;
  if( l10n.docptr != nullptr ) {
    delete l10n.docptr;
    l10n.docptr = nullptr;
  }
}


void i18n_t::clearLocale()
{
  memset( locale.language,  0, sizeof(i18n_locale_t::language) );
  memset( locale.country,   0, sizeof(i18n_locale_t::country) );
  memset( locale.variant,   0, sizeof(i18n_locale_t::variant) );
  memset( locale.delimiter, 0, sizeof(i18n_locale_t::delimiter) );
}


const char* i18n_t::gettext( const char* l10npath, char delimiter )
{
  return l10n.gettext( l10npath, delimiter );
}


const std::string i18n_t::getLocale()
{
  assert( locale.language[0] != 0 );
  std::string ret = "";
  ret += locale.language;
  if( locale.country[0] != 0 ) {
    ret += locale.delimiter;
    ret += locale.country;
  }
  if( locale.variant[0] != 0 ) {
    ret += locale.delimiter;
    ret += locale.variant;
  }
  return ret;
}


bool i18n_t::isValidISO( char* maybe_iso, size_t min, size_t max )
{
  if( !maybe_iso ) return false;
  size_t len = strlen( maybe_iso );
  return ( len>=min && len<=max );
}


// TODO: enforce validation with lowercase/uppercase check
bool i18n_t::isValidLocale(  char* maybe_locale )   { return isValidISO( maybe_locale,   2, sizeof(i18n_locale_t) );   }
bool i18n_t::isValidLang(    char* maybe_language ) { return isValidISO( maybe_language, 2, sizeof(i18n_locale_t::language) ); }
bool i18n_t::isValidVariant( char* maybe_variant )  { return isValidISO( maybe_variant,  2, sizeof(i18n_locale_t::variant) );  }
bool i18n_t::isValidCountry( char* maybe_country )  { return isValidISO( maybe_country,  2, sizeof(i18n_locale_t::country) );  }


#define goto_error(x) { if(x[0]!=0) YAML_LOG_e(x); goto _error; }

bool i18n_t::presetLocale( const char* localeStr )
{
  assert( localeStr );
  assert( strlen(localeStr) >= 2 && strlen(localeStr)<=32 );

  char* localeCopy;    // Copy of localeStr;
  char* localename;    // Pointer used for filepath deconstructor
  char *found = NULL;  // Pointer used for string search

  clearLocale(); // Clear any previously loaded locale
  localeCopy = strdup( localeStr ); // Allocate a copy of localeStr

  found = strrchr( localeCopy, '.' ); // Look for a dot: is locale a filename?
  if( found != NULL ) { // Locale string is a file name, deconstruct file extension
    this->extension = found+1; // Update file extension property
    *found = '\0'; // Truncate file extension, only keep the locale
  }

  found = strrchr( localeCopy, '/' ); // Look for a path separator: is locale prexifed with a path?
  if( found != NULL ) { // Locale string has a path separator, deconstruct [path][locale]
    localename = found+1; // Locale name starts after the last path separator
    if( !isValidLocale( localename ) ) goto_error(""); // Perform a naive locale length validation
    int pathlen = found-localeCopy+1; // Get the last path separator position
    this->path = std::string( localeCopy ).substr( 0, pathlen ); // Update path property
    snprintf( localeCopy, strlen( localename  )+1, "%s", localename ); // Update localeCopy with value trimmed from path
  } else { // Locale string has no path separator
    if( !isValidLocale( localeCopy ) ) goto_error(""); // Perform a naive locale length validation
  }

  found = NULL; // Reset search pointer as it'll now be used to find a locale delimiter

  for( int i=0;i<sizeof(i18n_t::delimiters);i++ ) { // Search for known locale delimiters in order to guess if locale is xx-XX, or xx_XX or xx
    found = strchr( localeCopy, delimiters[i] );
    if( found != NULL ) { // Found a known locale delimiter
      locale.delimiter[0] = delimiters[i]; // Update delimiter property
      break;
    }
  }

  if( found == NULL ) { // No delimiter found, assume it's a language-only locale of at least 2 chars
    if( !isValidLang( localeCopy ) ) goto_error("Bad ISO-639 language code"); // Perform a naive ISO-639 length validation
    memcpy( locale.language, localeCopy, strlen(localeCopy) );// Update language property
    goto _success;
  }

  // A delimiter was found! assume it's at least language+country, and at most language+country+variant
  found = strtok( localeCopy, locale.delimiter ); // split by delimiter, first element is language code
  if( !isValidLang( found ) ) goto_error("Bad ISO-639 language code"); // Perform a naive ISO-639 length validation
  memcpy( locale.language, found, strlen(found) );

  found = strtok( NULL, locale.delimiter ); // Get next delimited element, should be country code
  if( !isValidCountry( found ) ) goto_error("Bad ISO-3166 country code"); // Perform a naive ISO-639 length validation
  memcpy( locale.country, found, strlen(found) );

  found = strtok( NULL, locale.delimiter ); // Get next optional delimited element, should be country variant code
  if( found == NULL ) goto _success; // no variant provided
  if( !isValidVariant( found ) ) goto_error("Bad ISO-3166 variant code"); // Perform a naive ISO-639 length validation
  memcpy( locale.variant, found, strlen(found) );

  _success:
    YAML_LOG_d("Loaded locale: %s", localeCopy );
    free(localeCopy);
    return true;

  _error:
    YAML_LOG_e("[Error] Malformed locale, given='%s', filtered='%s'", localeStr, localeCopy );
    free(localeCopy);
    return false;
}


bool i18n_t::setLocale( const char* localeStr, const char* filePath )
{
  if( filePath == nullptr ) return presetLocale( localeStr ) ? loadLocale() : false; // no path provided, guess
  if( fs == nullptr || ! fs->exists( filePath ) ) return false; // a filePath was provided but filesystem or file doesn't exist
  if( ! presetLocale( localeStr ) ) return false; // localeStr is invalid
  fs::File localeFile = fs->open( filePath, "r" );
  if( ! localeFile ) return false; // file is invalid
  bool ret = loadLocaleStream( localeFile, localeFile.size()*2 ); // TODO: better size evaluation
  localeFile.close();
  return ret;
}


bool i18n_t::loadLocale()
{
  if( fs == nullptr ) {
    YAML_LOG_n("No filesystem attached, use setFS() e.g. i18n.setFS( &LiffleFS );");
    return false;
  }
  std::string localeStr = getLocale();
  std::string fileName = this->path + localeStr + "." + this->extension;
  if( ! fs->exists( fileName.c_str() ) ) return false; // this test is useless with SPIFFS
  fs::File localeFile = fs->open( fileName.c_str(), "r" );
  if( ! localeFile ) return false;
  bool ret = loadLocaleStream( localeFile, localeFile.size()*2 ); // TODO: better size evaluation
  localeFile.close();
  if( !ret ) return false;
  // perf+mem optimization: optionally reparent tree if root key is unique and value equals locale or language
  JsonVariantConst rootvar = *l10n.docptr; // alias the l10n set
  if( !rootvar[localeStr].isNull() && rootvar.size() == 1 ) l10n.docref = rootvar[localeStr]; // standard i18n files use the locale string as root key
  else if( !rootvar[locale.language].isNull()  && rootvar.size() == 1 ) l10n.docref = rootvar[locale.language]; // some only use the language
  else l10n.docref = *l10n.docptr; // some use flat translations
  return ! l10n.docref.isNull();
}


bool i18n_t::loadLocaleStream( Stream& stream, size_t size )
{
  freel10n();
  l10n.docptr = new DynamicJsonDocument( size );
  auto err = deserializeYml( *l10n.docptr, stream ); // deserialize yaml stream to JsonDocument
  if( err ) {
    YAML_LOG_n("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
    return false;
  }
  l10n.docref = *l10n.docptr;
  return true;
}



#endif // if defined I18N_SUPPORT
