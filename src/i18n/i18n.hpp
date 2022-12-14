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

#pragma once

#include <FS.h>
#include <ArduinoJson.h>
#include "../ArduinoYaml.hpp"

#if !defined HAS_ARDUINOJSON
  #error "i18n feature requires ArduinoJson"
#endif

// localized strings holder
struct l10n_t
{
  DynamicJsonDocument *doc = nullptr;
  JsonVariantConst variant = NULL;
  const char* gettext(const char* l10npath, char delim=':' );
};


// locale name holder
struct i18n_locale_t
{
  char language[4] {0}; // ISO-639
  char country[4] {0};  // ISO-3166
  char variant[16] {0}; // ISO-3166
  char delimiter[2] {0};
  i18n_locale_t() = default;
};


// setlocale and gettext
struct i18n_t
{
private:
  fs::FS *fs { nullptr };
  i18n_locale_t locale;
  l10n_t l10n;
  std::string localesPath = "/lang/"; // locales folder where the l10n files reside
  std::string fileExtension = "yml";  // l10n file extension, .yml .yaml or .json, must be yaml-parsable!
  bool loadLocale();

public:
  bool setLocale( i18n_locale_t l );
  bool setLocale( const char* localeStr );

  const char* gettext( const char* l10npath );
  const char* gettext( String l10npath );

  const std::string getLocale();

  void setFS( fs::FS *_fs );            // set filesystem
  void setPath( const char* path );     // set folder name
  void setExtension( const char* ext ); // set extension (yml, yaml, json)
};


static i18n_t i18n;
