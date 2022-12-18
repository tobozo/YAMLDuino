/*\
 *
 * @file
 * @version 1.0
 * @author tobozo <tobozo@users.noreply.github.com>
 * @section DESCRIPTION
 *
 * YAML <=> JSON converter and l10n parser
 *
 * YAMLDuino
 * Project Page: https://github.com/tobozo/YAMLDuino
 *
 * @section LICENSE
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

// Emit logs in arduino style at software level rather than firmware level


// log functions turned to macros to allow gathering of file name, log level, etc
#define YAML_LOG_v(format, ...) YAML::logger::LOG(__FILE__, __LINE__, YAML::LogLevelVerbose, format, ##__VA_ARGS__)
#define YAML_LOG_d(format, ...) YAML::logger::LOG(__FILE__, __LINE__, YAML::LogLevelDebug,   format, ##__VA_ARGS__)
#define YAML_LOG_i(format, ...) YAML::logger::LOG(__FILE__, __LINE__, YAML::LogLevelInfo,    format, ##__VA_ARGS__)
#define YAML_LOG_w(format, ...) YAML::logger::LOG(__FILE__, __LINE__, YAML::LogLevelWarning, format, ##__VA_ARGS__)
#define YAML_LOG_e(format, ...) YAML::logger::LOG(__FILE__, __LINE__, YAML::LogLevelError,   format, ##__VA_ARGS__)
#define YAML_LOG_n(format, ...) YAML::logger::LOG(__FILE__, __LINE__, YAML::LogLevelNone,    format, ##__VA_ARGS__)


#if defined ESP32
  #include "Esp.h" // bring esp32-arduino specifics to scope
  #define LOG_PRINTF log_printf // built-in esp32
  #define HEAP_AVAILABLE() ESP.getFreeHeap()
  #define YAML_DEFAULT_LOG_LEVEL (LogLevel_t)ARDUHAL_LOG_LEVEL
  #define YAML_PATHNAME pathToFileName
  // uncomment this if your espresssif32 package complains
  //#pragma GCC diagnostic ignored "-Wunused-variable"
  //#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#elif defined ESP8266
  #include "Esp.h" // bring esp8266-arduino specifics to scope
  #define HEAP_AVAILABLE() ESP.getFreeHeap()
  #define LOG_PRINTF Serial.printf
#elif defined ARDUINO_ARCH_RP2040
  #include <Arduino.h>
  #define LOG_PRINTF Serial.printf
  #define HEAP_AVAILABLE() rp2040.getFreeHeap()
#elif defined CORE_TEENSY
  #include <stdarg.h>
  #include <Arduino.h>
  #define LOG_PRINTF Serial.printf
  extern unsigned long _heap_start;
  extern unsigned long _heap_end;
  extern char *__brkval;
  static int getFreeRam()
  {
    return (char *)&_heap_end - __brkval;
  }
  #define HEAP_AVAILABLE() getFreeRam()
#elif defined ARDUINO_ARCH_SAMD
  #include <stdarg.h>
  #include <Arduino.h>
  #define LOG_PRINTF Serial.printf
  #ifdef __arm__
    // should use uinstd.h to define sbrk but Due causes a conflict
    extern "C" char* sbrk(int incr);
  #else  // __ARM__
    extern char *__brkval;
  #endif  // __arm__
  static int getFreeRam()
  {
    char top;
    #ifdef __arm__
      return &top - reinterpret_cast<char*>(sbrk(0));
    #elif (ARDUINO > 103 && ARDUINO != 151)
      return &top - __brkval;
    #else  // __arm__
      return __brkval ? &top - __brkval : &top - __malloc_heap_start;
    #endif  // __arm__
  }
  #define HEAP_AVAILABLE() getFreeRam()
#else
  #include <stdarg.h>
  #include <Arduino.h>
  #define LOG_PRINTF Serial.printf
  static int getFreeRam()
  {
    // implement your own
    return 0;
  }
  #define HEAP_AVAILABLE() getFreeRam()

#endif

#if !defined YAML_PATHNAME
  #define YAML_PATHNAME _pathToFileName
  static const char * _pathToFileName(const char * path)
  {
    int i = 0, pos = 0;
    char * p = (char *)path;
    while(*p){
      i++;
      if(*p == '/' || *p == '\\'){
        pos = i;
      }
      p++;
    }
    return path+pos;
  }
  #include <stdarg.h>
  #include <stdio.h>
#endif

#if !defined LOG_PRINTF
  #define LOG_PRINTF printf
#endif



#if !defined YAML_DEFAULT_LOG_LEVEL
  #define YAML_DEFAULT_LOG_LEVEL LogLevelWarning
#endif



namespace YAML
{

  // supported log levels, inspired from esp32 arduhal
  enum LogLevel_t
  {
    LogLevelNone,    // no logging
    LogLevelError,   // err
    LogLevelWarning, // err+warn
    LogLevelInfo,    // err+warn+info
    LogLevelDebug,   // err+warn+info+debug
    LogLevelVerbose  // err+warn+info+debug+verbose
  };


  namespace logger
  {

    // maximum size of log string
    #define LOG_MAXLENGTH 215
    #define YAML_LOGGER_attr __attribute__((unused)) static

    // logger function signature
    typedef void (*YAML_LOGGER_t)(const char* path, int line, int loglevel, const char* fmr, ...);

    // log levels names
    YAML_LOGGER_attr const char* levelNames[6] = {"None","Error","Warning","Info","Debug","Verbose"};
    // the default logging function
    YAML_LOGGER_attr void _LOG(const char* path, int line, int loglevel, const char* fmr, ...);
    // the pointer to the logging function (can be overloaded with a custom logger)
    YAML_LOGGER_attr void (*LOG)(const char* path, int line, int loglevel, const char* fmr, ...) = _LOG;
    // log level setter
    YAML_LOGGER_attr void setLogLevel( LogLevel_t level );
    // the logging function setter
    YAML_LOGGER_attr void setLoggerFunc( YAML_LOGGER_t fn );
    // default log level
    YAML_LOGGER_attr LogLevel_t _LOG_LEVEL = YAML_DEFAULT_LOG_LEVEL;
    // log level getter (int)
    YAML_LOGGER_attr LogLevel_t logLevelInt();
    // log level getter (string)
    YAML_LOGGER_attr const char* logLevelStr();

    LogLevel_t logLevelInt()
    {
      return _LOG_LEVEL;
    }

    const char* logLevelStr()
    {
      return levelNames[_LOG_LEVEL];
    }

    void setLogLevel( LogLevel_t level )
    {
      YAML::logger::_LOG_LEVEL = level;
      YAML_LOG_n("New log level: %d", level );
    }

    void setLoggerFunc( YAML_LOGGER_t fn )
    {
      LOG = fn;
    }

    void _LOG(const char* path, int line, int loglevel, const char* fmr, ...)
    {
      if( loglevel <= YAML::logger::_LOG_LEVEL ) {
        using namespace YAML;
        char log_buffer[LOG_MAXLENGTH+1] = {0};
        va_list arg;
        va_start(arg, fmr);
        vsnprintf(log_buffer, LOG_MAXLENGTH, fmr, arg);
        va_end(arg);
        if( log_buffer[0] != '\0' ) {
          switch( loglevel ) {
            case LogLevelVerbose: LOG_PRINTF("[V][%d][%s:%d] %s\r\n", HEAP_AVAILABLE(), YAML_PATHNAME(path), line, log_buffer); break;
            case LogLevelDebug:   LOG_PRINTF("[D][%d][%s:%d] %s\r\n", HEAP_AVAILABLE(), YAML_PATHNAME(path), line, log_buffer); break;
            case LogLevelInfo:    LOG_PRINTF("[I][%d][%s:%d] %s\r\n", HEAP_AVAILABLE(), YAML_PATHNAME(path), line, log_buffer); break;
            case LogLevelWarning: LOG_PRINTF("[W][%d][%s:%d] %s\r\n", HEAP_AVAILABLE(), YAML_PATHNAME(path), line, log_buffer); break;
            case LogLevelError:   LOG_PRINTF("[E][%d][%s:%d] %s\r\n", HEAP_AVAILABLE(), YAML_PATHNAME(path), line, log_buffer); break;
            case LogLevelNone:    LOG_PRINTF("[N][%d][%s:%d] %s\r\n", HEAP_AVAILABLE(), YAML_PATHNAME(path), line, log_buffer); break;
          }
        }
      }
    }

  };

};
