
#ifndef INIREADER_HPP__
#define INIREADER_HPP__

#include <string>

#ifdef linux
#include <string.h>
#endif

#include <map>
#include "core.h"
using namespace std ;

class iniReader
{
private:
   map<string,string> _config ;
   string _configName ;
public:
   INT32 init( string configName ) ;
   INT32 get( string key, string &value ) ;
} ;

#endif