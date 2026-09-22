/**********************************************************************************************************************
 * Class    : CLogger
 * 
 * Abstract : This class is used to manage displaying logging messages in an application.  This class implements the 
 *            following features
 *               (1) allows setting the severity of the message to be displayed.  The threshold value can be set in
 *                   the constructor or via the accessor function 'setLevel'
 *               (2) allows the severity of the message to be set by specifying the level as the second argument to 
 *                   the 'msgOut' function  (display if threshold_level <= message_level)
 *               (3) allows defining one or more output function, i.e. places where the logging message will be sent to
 *                   these function have the signature 'void (int, char*)'
 *               (4) allows specifing where the output message will be sent by specifying the output location as the 
 *                   first argument to 'msgOut' function
 *               (5) the function 'outMsg' can take any number of arguments that will be subsituted into the final 
 *                   displayed message
 *               (6) this class is implemented using a singleton pattern -- thus an application has one and only one
 *                   logger.
 *               (7) provides a simple command line output function ('cmdOut') 
 * 
 * History  : created May 2019 (gkhuber)
 *            Jun 2023 (gkhuber) added a 'NOTICE' level to the severity enumeration.  The idea is to have a severity 
 *                               level that will always be shown regardless of the threshold.  
 *            Jul 2025 (gkhuber) (a) added a 'SUCCESS' level to the severity enumeration so that successful opeation 
 *                               could also be logged via this mechanism
 *                               (b) modified the signature of the output function to accept an integer argument that 
 *                               is the message severity.  This is used to generate the correct message prefix.
 *                               (c) modifies the 'cmdOut' function to accept this argument and display the corrrect
 *                               prefix
 *                               (d) creaated a cmdColoOut output function that prints to the console with the 
 *                               correct prefix and also print the messages in color based on the severity of the 
 *                               message
 *            
 *********************************************************************************************************************/

#ifndef _logger_h_
#define _logger_h_


#include <stdint.h>
#include <cstdint>
#include <map>
#include <functional>
#include <version>

#if _MSVC_LANG == 202002L             // test for compiler supporting c++20
    #include <format>
    #include <utility>
#endif



typedef void (*fnct)(int, char*);          //std::function<void(int, char*)>    fnct;
typedef std::map<int, fnct>           mapType;

//predefined output identiferis
void cmdOut(int, char*);
void cmdColorOut(int, char*);

class CLogger
{
public:
  // set success level to be very large, that way it will always be printed out, regardless of output setting.
  enum level: std::int8_t{INFO = 1, DEBUG=2, WARNING=3, ERR=4, FATAL=5, NOTICE=6, SUCCESS=20};
  enum outLoc:  std::int8_t { CONSOLE = 0, FILE = 1};

  void setLevel(int l){m_level = l;}

  static CLogger* getInstance();
  static void     delInstance();


  void     outMsg(int, int, const char*, ...); 

#if _MSVC_LANG == 202002L             // test for compiler supporting c++20 
  template<class... Args>
  void outMsg2(int nWhich, int level, std::format_string<Args...> fmt, Args&&... args) 
  {
    mapType::iterator iter = m_mapCallbacks.find(nWhich);
    if ((iter != m_mapCallbacks.end()) && ((*iter).second)) 
    {
      if (m_level <= level) 
      {
        // 1. Create a char buffer (std::vformat_to works naturally with char)
        std::string buf;

        // 2. Use vformat_to and make_format_args to handle the runtime format string
        std::vformat_to(std::back_inserter(buf), fmt.get(), std::make_format_args(args...));

        // 3. Pass the raw pointer (char*) to your callback function
        (*iter).second(level, const_cast<char*>(buf.c_str()));
      }
    }
  }
#endif
  
  void     regOutDevice(int, fnct);

private:
    CLogger();
    ~CLogger();

    int             m_level;
    static CLogger* m_pThis;
#pragma warning(push)
#pragma warning(disable: 4251)
    mapType         m_mapCallbacks;  
#pragma warning(pop)
};

#endif



