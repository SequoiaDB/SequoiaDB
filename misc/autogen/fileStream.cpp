#include "fileStream.hpp"

fileOutStream::fileOutStream() : _force( false ),
                                 _ss( "" )
{
}

fileOutStream::~fileOutStream()
{
}

void fileOutStream::setReplace()
{
   _force = true ;
}

bool fileOutStream::_isReplace( const char *path )
{
   string source ;

   utilGetFileContent( path, source ) ;

   return source.compare( _ss.str() ) != 0 ;
}

int fileOutStream::close( const char *path )
{
   int rc = 0 ;

   if ( _isReplace( path ) || _force )
   {
      utilPutFileContent( path, _ss.str().c_str() ) ;
   }
   else
   {
      rc = -1 ;
   }

   return rc ;
}

