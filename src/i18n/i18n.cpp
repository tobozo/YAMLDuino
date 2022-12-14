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

#include <vector>

// count occurences of a char in a string
size_t strchr_count(const char* s, char c)
{
  size_t r = 0;
  for (; *s; ++s)
      r += *s == c;
  return r;
}


// return localized string when given a path e.g. 'blah:some:property:count:1'
const char* l10n_t::gettext(const char* l10npath, char delim )
{
  if( !l10npath ) return "";
  const char delimStr[2] = {delim,0};
  char* l10npath_cstr = strdup( l10npath );
  char *found = NULL;
  size_t depth = 0;
  JsonVariantConst _variant = variant; // copy ref to language root node

  if( _variant.isNull()) goto _not_found; // uh-oh, language not loaded

  depth = strchr_count( l10npath, delim ); // how many delimiters found?

  if( depth == 0 ) {
    if( _variant[l10npath].isNull() ) goto _not_found; // no property under this name
    _variant = _variant[l10npath]; // ini file style
    goto _success;
  }

  // walk through delimited properties

  found = strtok( l10npath_cstr, delimStr );

  while( found != NULL ) {
    if( !_variant[found].isNull() ) { // map key->val
      _variant = _variant[found];
    } else {
      int idx = atoi( found );
      if( idx >= 0 && !_variant[idx].isNull() ) { // array index->val
        _variant = _variant[idx];
      } else {
        goto _not_found; // not even a number
      }
    }
    found = strtok( NULL, delimStr );
  }

  if( _variant.isNull() ) goto _not_found; // no match

  _success:
    free( l10npath_cstr );
    return _variant.as<const char*>();

  _not_found:
    free( l10npath_cstr );
    return l10npath; // not found
}



void i18n_t::setFS( fs::FS *_fs )
{
  assert( _fs );
  fs = _fs;
}


void i18n_t::setPath( const char* path )
{
  localesPath = path;
  if( localesPath[localesPath.size()-1]!='/' ) {
    localesPath +="/";
  }
}


void i18n_t::setExtension( const char* ext )
{
  assert( ext );
  fileExtension = ext;
}


const char* i18n_t::gettext( const char* l10npath )
{
  return l10n.gettext( l10npath );
}


const char* i18n_t::gettext( String l10npath )
{
  return gettext( l10npath.c_str() );
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


bool i18n_t::setLocale( i18n_locale_t l )
{
  locale = l; return loadLocale();
}


bool i18n_t::setLocale( const char* localeStr )
{
  assert( localeStr );
  size_t len = strlen( localeStr );
  assert( len >= 2 && len <=sizeof(i18n_locale_t) );

  char* localeCopy = strdup( localeStr );

  l10n.variant = NULL;
  memset( locale.language, 0, sizeof(locale.language) );
  memset( locale.country,  0, sizeof(locale.country) );
  memset( locale.variant,  0, sizeof(locale.variant) );

  char delimiters[3] = {'-', '_'}; // guess delimiter
  char *found = NULL;
  size_t tokenlen = 0;

  for( int i=0;i<sizeof(delimiters);i++ ) {
    found = strchr( localeCopy, delimiters[i] );
    if( found ) {
      locale.delimiter[0] = delimiters[i];
      break;
    }
  }

  if( found == NULL ) { // No delimiter found, assume it's a language-only locale
    if( len >= sizeof( locale.language ) ) goto _error; // lame ISO-639 validation
    memcpy( locale.language, localeCopy, len );
    goto _success;
  }

  // a delimiter was found, assume it's at least language+country, and at most language+country+variant
  found = strtok( localeCopy, locale.delimiter );
  if( found == NULL ) goto _error; // this should not happen as the delimiter was found earlier
  tokenlen = strlen( found );
  if( tokenlen !=2 && tokenlen != 3 ) goto _error; // bad language value, should be 2 or three letters
  memcpy( locale.language, found, strlen(found) );

  found = strtok( NULL, locale.delimiter );
  if( found == NULL ) goto _error; // missing country value ?
  tokenlen = strlen( found );
  if( tokenlen !=2 && tokenlen != 3 ) goto _error; // bad country value, should be 2 or three letters
  memcpy( locale.country, found, strlen(found) );

  found = strtok( NULL, locale.delimiter );
  if( found == NULL ) goto _success; // no variant provided
  tokenlen = strlen( found );
  if( tokenlen >= sizeof( locale.variant ) ) goto _error; // bad variant value, should be 2 to 15 letters
  memcpy( locale.variant, found, strlen(found) );

  _success:
    free(localeCopy);
    return loadLocale();

  _error:
    printf("Error, malformed locale: %s", localeStr );
    free(localeCopy);
    return false;
}


bool i18n_t::loadLocale()
{
  if( fs == nullptr ) {
    log_n("No filesystem attached, use setFS() e.g. i18n.setFS( &LiffleFS );");
    return false;
  }
  std::string localeStr = getLocale();
  std::string fileName = localesPath + localeStr + "." + fileExtension;
  if( ! fs->exists( fileName.c_str() ) ) return false; // this test is useless with SPIFFS
  fs::File localeFile = fs->open( fileName.c_str() );
  if( ! localeFile ) return false;
  size_t localeSize = localeFile.size();

  if( l10n.doc != nullptr ) delete l10n.doc;
  l10n.doc = new DynamicJsonDocument( localeSize );
  JsonVariantConst rootvar = *l10n.doc;

  auto err = deserializeYml( *l10n.doc, localeFile ); // deserialize yaml stream to JsonDocument

  localeFile.close();

  if( err ) {
    YAML_LOG_n("Unable to deserialize demo YAML to JsonObject: %s", err.c_str() );
    return false;
  }

  // standard i18n files use the locale string as root key
  if( !rootvar[localeStr].isNull() ) l10n.variant = rootvar[localeStr];
  // some only use the language
  else if( !rootvar[locale.language].isNull() ) l10n.variant = rootvar[locale.language];
  // some just don't care
  else l10n.variant = rootvar;

  return true;
}
