#pragma once
/*
 *
 * ESP32-yaml
 * Project Page: https://github.com/tobozo/esp32-yaml
 *
 * Copyright 2022 tobozo http://github.com/tobozo
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files ("ESP32-yaml"), to deal in the Software without
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
 */

// Emit logs in arduino style at software level rather than firmware level

#ifdef ESP32
  #include <esp32-hal-log.h>
  #define LOG_PRINTF log_printf // aliasing this for non-esp32 compat
#else
  // declare macros and functions needed by the logger
  #define ARDUHAL_SHORT_LOG_FORMAT(letter, format)  ## letter format "\r\n"
  #define LOG_PRINTF printf
  const char * pathToFileName(const char * path)
  {
    size_t i = 0, pos = 0;
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
#endif


namespace YAML
{
  // maximum size of log string
  #define LOG_MAXLENGTH 215
  // the default logging function
  static void _LOG(const char* path, int line, int loglevel, const char* fmr, ...);
  // the pointer to the logging function (can be overloaded with a custom logger)
  __attribute__((unused)) static void (*LOG)(const char* path, int line, int loglevel, const char* fmr, ...) = _LOG;
  typedef void (*YAML_LOGGER_t)(const char* path, int line, int loglevel, const char* fmr, ...);
  // the logging function setter
  __attribute__((unused)) static void setLoggerFunc( YAML_LOGGER_t fn )
  {
    LOG = fn;
  }
  // supported log levels
  enum LogLevel_t
  {
    LogLevelVerbose, // err+warn+info+debug+verbose
    LogLevelDebug,   // err+warn+info+debug
    LogLevelInfo,    // err+warn+info
    LogLevelWarning, // err+warn
    LogLevelError    // err
  };

  // default log level
  static LogLevel_t LOG_LEVEL = LogLevelWarning;


  /**
  * @brief default logging function
  *
  * @param path full path to the source file emitting the log message
  * @param line line number in the source file emitting the log message
  * @param loglevel log level of the message
  * @param fmr format string
  * @param mixed ... args for the format string
  * @return void
  */
  void _LOG(const char* path, int line, int loglevel, const char* fmr, ...)
  {
    using namespace YAML;
    if (loglevel >= LOG_LEVEL) {
      char log_buffer[LOG_MAXLENGTH+1] = {0};
      va_list arg;
      va_start(arg, fmr);
      vsnprintf(log_buffer, LOG_MAXLENGTH, fmr, arg);
      va_end(arg);
      if( log_buffer[0] != '\0' ) {
        switch( loglevel ) {
          case LogLevelVerbose: LOG_PRINTF(ARDUHAL_SHORT_LOG_FORMAT(D, "[V][%s:%d] %s"), pathToFileName(path), line, log_buffer); break;
          case LogLevelDebug:   LOG_PRINTF(ARDUHAL_SHORT_LOG_FORMAT(D, "[D][%s:%d] %s"), pathToFileName(path), line, log_buffer); break;
          case LogLevelInfo:    LOG_PRINTF(ARDUHAL_SHORT_LOG_FORMAT(I, "[I][%s:%d] %s"), pathToFileName(path), line, log_buffer); break;
          case LogLevelWarning: LOG_PRINTF(ARDUHAL_SHORT_LOG_FORMAT(W, "[W][%s:%d] %s"), pathToFileName(path), line, log_buffer); break;
          case LogLevelError:   LOG_PRINTF(ARDUHAL_SHORT_LOG_FORMAT(E, "[E][%s:%d] %s"), pathToFileName(path), line, log_buffer); break;
        }
      }
    }
  }

};


// log functions turned to macros to allow gathering of file name, log level, etc
#define YAML_LOG_v(format, ...) YAML::LOG(__FILE__, __LINE__, YAML::LogLevelVerbose, format, ##__VA_ARGS__)
#define YAML_LOG_d(format, ...) YAML::LOG(__FILE__, __LINE__, YAML::LogLevelDebug,   format, ##__VA_ARGS__)
#define YAML_LOG_i(format, ...) YAML::LOG(__FILE__, __LINE__, YAML::LogLevelInfo,    format, ##__VA_ARGS__)
#define YAML_LOG_w(format, ...) YAML::LOG(__FILE__, __LINE__, YAML::LogLevelWarning, format, ##__VA_ARGS__)
#define YAML_LOG_e(format, ...) YAML::LOG(__FILE__, __LINE__, YAML::LogLevelError,   format, ##__VA_ARGS__)

