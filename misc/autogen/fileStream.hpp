#ifndef FILE_STREAM_HPP
#define FILE_STREAM_HPP

#include "util.h"
#include <string>
#include <iomanip>
#include <sstream>
using namespace std;

class fileOutStream
{
public:
   fileOutStream() ;
   ~fileOutStream() ;

   template <typename T>
   ostringstream& operator << (T a)
   {
      _ss << a ;
      return _ss ;
   }

   void setReplace() ;
   int close( const char *path ) ;

private:
   bool _isReplace( const char *path ) ;

private:
   bool           _force ;
   ostringstream  _ss ;
} ;

#endif