#include "logger.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>

CLogger* CLogger::m_pThis = nullptr;


CLogger*  CLogger::getInstance()
{
  if (nullptr == m_pThis)
    m_pThis = new CLogger;

  return m_pThis;
}



void CLogger::delInstance()
{
  delete m_pThis;
}


void CLogger::regOutDevice(int nWhich, fnct callback)
{
  std::pair<mapType::iterator, bool>   retPair;
  retPair = m_mapCallbacks.insert(std::pair<int, fnct>(nWhich, callback));
  if (!retPair.second)
  {
    outMsg(0, CLogger::level::ERR, "failed to insert outdevice #%d", nWhich);
  }
}



void CLogger::outMsg(int nWhich, int level, const char* fmt, ...)
{
  mapType::iterator  iter = m_mapCallbacks.find(nWhich);

  if ((iter != m_mapCallbacks.end()) && ((*iter).second)) // did we find the entry, and is it a valid callback.
  {
    if(m_level <= level)      // only do work if level of message is greater than or equal to report level
    {
      va_list     args, args_copy;
      va_start(args, fmt);

      // make a copy of the argument list so we can reuse it...
      va_copy(args_copy, args);

      // do a trial vsnprintf to get the needed length
      int len = vsnprintf(nullptr, 0, fmt, args_copy);
      va_end(args_copy);

      // now that we know how big of a buffer we need, allocate it
      char* line = new char[len + 1];
      memset((void*)line, '\0', len + 1);

      vsnprintf(line, len + 1, fmt, args);

      // shoot message out to device...
      (*iter).second(level, line);

      va_end(args);
      delete[] line;
    }
  }
}

//#if _MSVC_LANG ==202002L
//template<class... Args>
//void CLogger::outMsg2(int nWhich, int level, std::format_string<Args...> fmt, Args&&... args)
//{
//  mapType::iterator  iter = m_mapCallbacks.find(nWhich);
//
//  if ((iter != m_mapCallbacks.end()) && ((*iter).second)) // did we find the entry, and is it a valid callback.
//  {
//    if (m_level <= level)          // only do work if level of message is greater than or equal to report level
//    {
//      std::vector<uint8_t> buf;
//      std::format_to(std::back_inserter<buf>, fmt, std::format<Args>(args)...);
//      //std::cout << std::format(fmt, std::format<Args>(args)...) << "\n";
//
//      (*iter).second(level, buf);
//    }
//  }
//}
//#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// privae functions
CLogger::CLogger() 
{

}


CLogger::~CLogger()
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// predefined output functions
void cmdOut(int level, char* msg)
{
  switch (level)
  {
    case CLogger::level::INFO:
      printf("%s", "[INFO   ]:");
      break;
    case CLogger::level::DEBUG:
      printf("%s", "[DEBUG  ]:");
      break;
    case CLogger::level::WARNING:
      printf("%s", "[WARNING]:");
      break;
    case CLogger::level::ERR:
      printf("%s", "[ERROR  ]:");
      break;
    case CLogger::level::FATAL:
      printf("%s", "[FATAL  ]:");
      break;  
    case CLogger::level::NOTICE:
      printf("%s", "[NOTICE ]:");
      break;
    case CLogger::level::SUCCESS:
      printf("%s", "[SUCCESS]:");
      break;
  }
  printf("%s\n", msg);
}


/**********************************************************************************************************************
 * Function: 
 *
 * Abstract: Adds the prefixes [INFO   ],....[SUCCESS] to the output message and prints the messages in color.  For
 *           fatal and error messages the string is printed in red ('\33[31m'), for warnings messages the string is 
 *           printed in yellow ('\033[33m'), for success messages the string is printed in green ('\033[32m').  For all 
 *           other strings, they are printed in white ('\33[37m').
 *           see https://superuser.com/questions/413073/windows-console-with-ansi-colors-handling
 *
 * Input   : level -- [in] integer, the severity of the message being displayed, this is used to set the prefix
 *           msg -- [in] pointer to a null-terminated string, the message to be displayed
 * 
 * Returns :
 *
 * Written : July 2025 (gkhuber) 
 *********************************************************************************************************************/
void cmdColorOut(int level, char* msg)
{
  switch (level)
  {
  case CLogger::level::INFO:
    printf("%s", "\033[36m[INFO   ]:");               // print info level in cyan
    break;
  case CLogger::level::DEBUG:
    printf("%s", "[DEBUG  ]:");                       // print debug level in gray
    break;
  case CLogger::level::WARNING:
    printf("%s", "\033[33m[WARNING]:");               // print warning level in yellow
    break;
  case CLogger::level::ERR:
    printf("%s", "\033[31m[ERROR  ]:");               // print error level in red
    break;
  case CLogger::level::FATAL:
    printf("%s", "\033[91m[FATAL  ]:");               // print fatal level in strong red
    break;
  case CLogger::level::NOTICE:
    printf("%s", "[NOTICE ]:");
    break;
  case CLogger::level::SUCCESS:
    printf("%s", "\033[32m[SUCCESS]:");               // print success in green
    break;
  }

  printf("%s\033[0m\n", msg);




}

//void setupConsole()
//{
//#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
//  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
//  DWORD dwMode = 0;
//  GetConsoleMode(hOut, &dwMode);
//  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
//  SetConsoleMode(hOut, dwMode);
//
//  // References:
//  //SetConsoleMode() and ENABLE_VIRTUAL_TERMINAL_PROCESSING?
//  //https://stackoverflow.com/questions/38772468/setconsolemode-and-enable-virtual-terminal-processing
//
//  // Windows console with ANSI colors handling
//  // https://superuser.com/questions/413073/windows-console-with-ansi-colors-handling
//#endif

//}
