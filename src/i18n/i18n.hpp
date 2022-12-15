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


#if !defined WIO_TERMINAL
  #define I18N_SUPPORT
#endif

#include "../ArduinoYaml.hpp"

#if defined I18N_SUPPORT && defined HAS_ARDUINOJSON

#include <FS.h>

// Localization set, serialized as ArduinoJSON document
struct l10n_t
{
  DynamicJsonDocument *docptr {nullptr};
  JsonVariantConst docref {NULL};
  const char* gettext(const char* l10npath, char delimiter );
};


// Deconstructed locale
struct i18n_locale_t
{
  char language[5];  // ISO-639 language code
  char country[5];   // ISO-3166 country code
  char variant[5];   // ISO-3166 variant code
  char delimiter[2]; // lang-country-variant delimiter, either stroke or underscore
};


// I18N setlocale() and gettext()
struct i18n_t
{

public:
  i18n_t() { };
  i18n_t( fs::FS *_fs) { setFS( _fs); };
  ~i18n_t() { clearLocale(); freel10n(); };

  // Use filePath if filename differs from locale e.g. setLocale("en-UD", "/path/to/arbitrary_non_locale_filename_yml")
  // 'localeStr' must be valid, it can be "xx_XX" or "/path/to/xx_XX.yml"
  bool setLocale( const char* localeStr, const char* filePath=nullptr );
  const char* gettext( const char* l10npath, char delimiter=':' );
  void setFS( fs::FS *_fs ); // set filesystem

private:
  fs::FS *fs = nullptr;            // Filesystem
  i18n_locale_t locale; // Deconstructed locale
  l10n_t l10n;       // Localization set, serialized as JsonDocument
  std::string path = "/lang/";     // Deconstructed path where the l10n files can be found, with trailing slash
  std::string extension = "yml";   // Deconstructed file extension (yml, yaml, json)
  constexpr static const char delimiters[2] = {'-', '_'}; // Supported locale delimiters

  const std::string getLocale(); // reconstruct locale

  bool presetLocale( const char* localeStr );
  void clearLocale();
  void freel10n();

  bool loadLocale();
  bool loadLocaleStream( Stream& stream, size_t size );

  // Note: validation is not made on *values*. Only string length is checked for overflow protection
  bool isValidISO( char* maybe_iso, size_t min, size_t max );
  bool isValidLocale( char* maybe_locale );
  bool isValidLang( char* maybe_iso3166 );
  bool isValidVariant( char* maybe_iso3166 );
  bool isValidCountry( char* maybe_iso639 );

};


#endif // defined WIO_TERMINAL
