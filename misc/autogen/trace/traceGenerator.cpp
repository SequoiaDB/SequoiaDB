#include "traceGenerator.hpp"

static const char *_suffix[] = {
   TRACE_SUFFIX_LIST
} ;

static const char *_filterDir[] = {
   TRACE_FILTER_DIR
} ;

#define SUFFIX_SIZE (sizeof( _suffix ) / sizeof( const char * ))
#define FILTER_SIZE (sizeof( _filterDir ) / sizeof( const char * ))

#if defined (GENERAL_TRACE_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( traceGenerator, GENERAL_TRACE_FILE ) ;
#endif

traceGenerator::traceGenerator() : _isFinish( false ),
                                   _funcNum( 0 )
{
   _seqNum = 0 ;
}

traceGenerator::~traceGenerator()
{
}

int traceGenerator::init()
{
   int rc = 0 ;

   _seqNum = 0 ;
   _funcNum = 0 ;
   _isFinish = false ;
   _traceList.clear() ;
   
   rc = _getTraceList() ;

   return rc ;
}

bool traceGenerator::hasNext()
{
   return !_isFinish ;
}

int traceGenerator::outputFile( int id, fileOutStream &fout,
                                string &outputPath )
{
   int rc = 0 ;

   if ( id < _traceList.size() )
   {
      rc = _genTraceFile( id, fout, outputPath ) ;
   }
   else
   {
      rc = _genTraceFuncListFile( fout, outputPath ) ;

      _isFinish = true ;
   }

   return rc ;
}

int traceGenerator::_genTraceFile( int id, fileOutStream &fout,
                                   string &outputPath )
{
   int rc = 0 ;
   unsigned int one = 1 ;
   vector<_traceFuncInfo>::iterator it ;
   string headerDesc ;

   outputPath = TRACE_FILE_PATH ;
   outputPath += _traceList[id].name + TRACE_FILE_EXTNAME ;

   const char *tmp = outputPath.c_str() ;

   _buildStatement( headerDesc ) ;

   fout << std::left << headerDesc << "\n"
        << "#ifndef " << _traceList[id].name
        << "TRACE_H__\n"
           "#define " << _traceList[id].name
        << "TRACE_H__\n"
           "// Component: " << _traceList[id].name << "\n" ;

   it = _traceList[id].funcList.begin() ;
   for ( ; it != _traceList[id].funcList.end(); ++it )
   {
      unsigned int componentID = 0 ;
      unsigned long long uniqueID = 0 ;
      struct _traceFuncInfo funcInfo = *it ;
      char hexBuffer[512] = {0} ;

      componentID = one << id ;
      uniqueID = ((unsigned long long)componentID) << 32 | _seqNum ;

      utilSnprintf( hexBuffer, sizeof( hexBuffer ), "#define %-50s 0x%llxL",
                    funcInfo.alias.c_str(), uniqueID ) ;

      fout << hexBuffer << "\n" ;

      ++_seqNum ;
   }

   fout << "#endif\n" ;

   return rc ;
}

int traceGenerator::_genTraceFuncListFile( fileOutStream &fout,
                                           string &outputPath )
{
   int rc = 0 ;
   bool isFirst = true ;
   vector<_traceInfo>::iterator it ;
   string headerDesc ;

   outputPath = TRACE_FUNC_LIST_FILE_PATH ;

   _buildStatement( headerDesc ) ;

   fout << std::left << headerDesc << "\n" ;
   fout << "#include \"core.hpp\"\n"
           "static const CHAR *_pTraceFunctionList[] = {\n" ;

   it = _traceList.begin() ;
   for ( ; it != _traceList.end(); ++it )
   {
      struct _traceInfo traceInfo = *it ;
      vector<_traceFuncInfo>::iterator funcItem ;

      funcItem = traceInfo.funcList.begin() ;
      for ( ; funcItem != traceInfo.funcList.end(); ++funcItem )
      {
         struct _traceFuncInfo funcInfo = *funcItem ;

         if( isFirst )
         {
            fout << "    " ;
            isFirst = false ;
         }
         else
         {
            fout << ",   " ;
         }

         fout << "\"" << funcInfo.func << "\"\n" ;
      }
   }

   fout << "} ;\n"
           "const UINT32 _pTraceFunctionListNum = " << _funcNum << " ;\n"
           "const UINT32 pdGetTraceFunctionListNum()\n"
           "{\n"
           "  return " << _funcNum << ";\n"
           "}\n"
           "const CHAR *pdGetTraceFunction ( UINT64 id )\n"
           "{\n"
           "   UINT32 funcID = (UINT32)(id & 0xFFFFFFFF) ;\n"
           "   if ( funcID >= _pTraceFunctionListNum )\n"
           "      return NULL ;\n"
           "   return _pTraceFunctionList[funcID] ;\n"
           "}\n" ;

   return rc ;
}

int traceGenerator::_getTraceList()
{
   int rc = 0 ;
   string content ;
   string path = utilGetRealPath2( TRACE_MODULE_LIST_PATH ) ;

   rc = utilGetFileContent( path.c_str(), content ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "Failed to get file content: path = "
                           << path << endl ;
      goto error ;
   }

   rc = _parseTraceComponents( content ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "Failed to parse trace components list: path = "
                           << path << endl ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

int traceGenerator::_parseTraceComponents( const string &content )
{
   int rc = 0 ;
   size_t pos = -1 ;
   size_t begin = 0 ;
   size_t finish = 0 ;
   vector<struct _traceInfo>::iterator it ;

   pos = content.find( "_pdTraceComponentDir", pos + 1 ) ;
   if ( pos == string::npos )
   {
      rc = 1 ;
      goto error ;
   }

   pos = content.find( "{", pos + 1 ) ;
   if ( pos == string::npos )
   {
      rc = 1 ;
      goto error ;
   }

   begin = pos ;

   finish = content.find( "}", begin + 1 ) ;
   if ( finish == string::npos )
   {
      rc = 1 ;
      goto error ;
   }

   do
   {
      size_t end = 0 ;
      const char *tmp = content.c_str() + begin + 1 ;
      struct _traceInfo traceInfo ;

      begin = content.find( "\"", begin + 1 ) ;
      if ( begin == string::npos || begin >= finish )
      {
         break ;
      }

      end = content.find( "\"", begin + 1 ) ;
      if ( end == string::npos )
      {
         rc = 1 ;
         goto error ;
      }

      if ( end >= finish )
      {
         break ;
      }

      if ( begin + 1 < end )
      {
         traceInfo.name = content.substr( begin + 1, end - begin - 1 ) ;

         it = std::find( _traceList.begin(), _traceList.end(), traceInfo.name ) ;
         if ( it != _traceList.end() )
         {
            rc = 1 ;
            printLog( PD_ERROR ) << "Error: trace component is exist: name = "
                                 << traceInfo.name << endl ;
            goto error ;
         }

         rc = _getTraceFuncList( traceInfo.name, traceInfo.funcList ) ;
         if ( rc )
         {
            goto error ;
         }

         _traceList.push_back( traceInfo ) ;
      }

      begin = end ;

   } while( true ) ;

done:
   return rc ;
error:
   goto done ;
}

int traceGenerator::_getTraceFuncList( const string &module,
                                       vector<_traceFuncInfo> &funcList )
{
   int rc = 0 ;
   vector<string> fileList ;
   vector<string>::iterator it ;
   string path = ENGINE_PATH + module ;
   string modulePath = utilGetRealPath2( path.c_str() ) ;

   utilGetPathListBySuffix( modulePath.c_str(),
                            _suffix, SUFFIX_SIZE,
                            _filterDir, FILTER_SIZE,
                            fileList ) ;

   it = fileList.begin() ;
   for( ; it != fileList.end(); ++it )
   {
      const string &filePath = *it ;

      rc = _parseTraceFunc( filePath, funcList ) ;
      if ( rc )
      {
         printLog( PD_ERROR ) << "Failed to parse trace file: path = "
                              << filePath << endl ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

int traceGenerator::_checkTraceFunctionExist( const string &content,
                                              size_t offset,
                                              const string &path,
                                              struct _traceFuncInfo &funcInfo )
{
   int rc = 0 ;
   size_t pos = -1 ;
   size_t funcPos = -1 ;

   pos = content.find( "(", offset ) ;
   funcPos = content.find( funcInfo.func, offset ) ;
   if ( funcPos == string::npos || funcPos > pos )
   {
      printLog( PD_WARNING ) << "Warning: invalid trace function"
                             << ", path = " << path
                             << ", name = " << funcInfo.alias
                             << ", function = " << funcInfo.func << endl ;
   }

   return rc ;
}

int traceGenerator::_checkTraceEntryAndExit( const string &content,
                                             size_t offset,
                                             const string &path,
                                             struct _traceFuncInfo &funcInfo )
{
   int rc = 0 ;
   size_t pos = -1 ;
   size_t entryPos = -1 ;
   size_t exitPos = -1 ;
   string entryName ;
   string exitName ;

   pos = content.find( TRACE_DEFINE_NAME, offset ) ;

   entryPos = content.find( TRACE_ENTRY_NAME, offset ) ;
   if ( entryPos != string::npos )
   {
      const char *p = NULL ;
      const char *pAlias = NULL ;
      const char *start = content.c_str() ;

      p = start + entryPos + TRACE_ENTRY_NAME_LEN ;

      while ( *p != 0 && *p != '(' )
      {
         ++p ;
      }

      ++p ;

      while ( *p != 0 && ( *p == ' ' || *p == '\t' ) )
      {
         ++p ;
      }

      // get ID
      pAlias = p ;

      // skip everything until space or ,
      while ( *p != 0 && *p != ' ' && *p != '\t' && *p != ')' )
      {
         ++p ;
      }

      entryName = string( pAlias, p - pAlias ) ;
   }

   exitPos  = content.find( TRACE_EXIT_NAME , offset ) ;
   if ( exitPos != string::npos )
   {
      char isRc = ')' ;
      const char *p = NULL ;
      const char *pAlias = NULL ;
      const char *start = content.c_str() ;

      p = start + exitPos + TRACE_EXIT_NAME_LEN ;

      if ( p[0] == 'R' && p[1] == 'C' )
      {
         p += 2 ;
         isRc = ',' ;
      }

      while ( *p != 0 && *p != '(' )
      {
         ++p ;
      }

      ++p ;

      while ( *p != 0 && ( *p == ' ' || *p == '\t' ) )
      {
         ++p ;
      }

      // get ID
      pAlias = p ;

      // skip everything until space or ,
      while ( *p != 0 && *p != ' ' && *p != '\t' && *p != isRc )
      {
         ++p ;
      }

      exitName = string( pAlias, p - pAlias ) ;
   }

   if ( entryPos < pos && funcInfo.alias == entryName )
   {
      if ( exitPos == string::npos ||
           exitPos >= pos ||
           funcInfo.alias != exitName )
      {
         printLog( PD_WARNING ) << "Warning: " TRACE_EXIT_NAME" not found"
                                << ", path = " << path
                                << ", name = " << funcInfo.alias
                                << ", func = " << funcInfo.func << endl ;
      }
   }
   else if ( exitPos < pos && funcInfo.alias == exitName )
   {
      if ( entryPos == string::npos ||
           entryPos >= pos ||
           funcInfo.alias != entryName )
      {
         printLog( PD_WARNING ) << "Warning: " TRACE_ENTRY_NAME" not found"
                                << ", path = " << path
                                << ", name = " << funcInfo.alias
                                << ", func = " << funcInfo.func << endl ;
      }
   }
   else if ( entryPos != string::npos && entryPos < pos &&
             exitPos != string::npos && exitPos < pos &&
             entryPos > exitPos )
   {
      printLog( PD_WARNING ) << "Warning: invalid " TRACE_ENTRY_NAME" and " TRACE_EXIT_NAME
                             << ", path = " << path
                             << ", name = " << funcInfo.alias
                             << ", func = " << funcInfo.func << endl ;
   }

   return rc ;
}

int traceGenerator::_parseTraceFunc( const string &path,
                                     vector<_traceFuncInfo> &funcList )
{
   int rc = 0 ;
   size_t pos = -1 ;
   size_t funcPos = -1 ;
   const char *start = NULL ;
   string content ;
   vector<struct _traceInfo>::iterator it ;

   rc = utilGetFileContent( path.c_str(), content ) ;
   if ( rc )
   {
      goto error ;
   }

   start = content.c_str() ;

   while( true )
   {
      bool skipCheck = false ;
      size_t entryPos = -1 ;
      size_t exitPos = -1 ;
      const char *p        = NULL ;
      const char *pAlias   = NULL ;
      const char *pFunc    = NULL ;
      _traceFuncInfo funcInfo ;

      pos = content.find( TRACE_DEFINE_NAME, pos + 1 ) ;
      if ( pos == string::npos )
      {
         break ;
      }

      p = start + pos + TRACE_DEFINE_NAME_LEN ;

      while ( *p != 0 && *p != '(' )
      {
         ++p ;
      }

      ++p ;

      while ( *p != 0 && ( *p == ' ' || *p == '\t' ) )
      {
         ++p ;
      }

      // get ID
      pAlias = p ;

      // skip everything until space or ,
      while ( *p != 0 && *p != ' ' && *p != '\t' && *p != ',' )
      {
         ++p ;
      }

      funcInfo.alias = string( pAlias, p - pAlias ) ;

      ++p ;

      // skip everything until "
      while ( *p != 0 && *p != '"' )
      {
         ++p ;
      }

      ++p ;

      pFunc = p ;

      // skip everything until "
      while ( *p != 0 && *p != '"' )
      {
         ++p ;
      }

      funcInfo.func = string( pFunc, p - pFunc ) ;

      if ( funcInfo.func.at( 0 ) == '$' )
      {
         skipCheck = true ;
         funcInfo.func = string( pFunc + 1, p - pFunc - 1 ) ;
      }

      //check trace empty or not
      if ( funcInfo.alias.length() == 0 || funcInfo.func.length() == 0 )
      {
         printLog( PD_WARNING ) << "Warning: invalid trace info"
                                << ", path = " << path
                                << ", name = " << funcInfo.alias
                                << ", function = " << funcInfo.func << endl ;
      }

      if ( false == skipCheck )
      {
         if ( funcInfo.func.find( " " ) != string::npos )
         {
            printLog( PD_WARNING ) << "Warning: invalid trace function"
                                   << ", path = " << path
                                   << ", name = " << funcInfo.alias
                                   << ", function = " << funcInfo.func << endl ;
         }
         else if ( funcInfo.func.find( ")" ) != string::npos )
         {
            printLog( PD_WARNING ) << "Warning: invalid trace function"
                                   << ", path = " << path
                                   << ", name = " << funcInfo.alias
                                   << ", function = " << funcInfo.func << endl ;
         }
         else if ( funcInfo.func.find( "(" ) != string::npos )
         {
            printLog( PD_WARNING ) << "Warning: invalid trace function"
                                   << ", path = " << path
                                   << ", name = " << funcInfo.alias
                                   << ", function = " << funcInfo.func << endl ;
         }

         //check trace function exist or not
         rc = _checkTraceFunctionExist( content, p - start, path, funcInfo ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      //check trace entry and exist function exist or not
      rc = _checkTraceEntryAndExit( content, p - start, path, funcInfo ) ;
      if ( rc )
      {
         goto error ;
      }

      it = std::find( _traceList.begin(), _traceList.end(), funcInfo ) ;
      if ( it != _traceList.end() )
      {
         printLog( PD_ERROR ) << "Error: trace name or function is exist"
                              << ", path = " << path
                              << ", name = " << funcInfo.alias
                              << ", function = " << funcInfo.func << endl ;
         rc = 1 ;
         goto error ;
      }

      funcList.push_back( funcInfo ) ;

      ++_funcNum ;

      pos = p - start ;
   }

done:
   return rc ;
error:
   goto done ;
}

int traceGenerator::_buildStatement( string &headerDesc )
{
   int rc = 0 ;
   char buff[2048] = { 0 } ;

   rc = (int)utilSnprintf( buff, 2048, GLOBAL_LICENSE2,
                           utilGetCurrentYear() ) ;
   if ( rc < 0 )
   {
      rc = 1 ;
      goto error ;
   }

   headerDesc = "" ;
   headerDesc += buff ;
   headerDesc += "\n\n" ;
   headerDesc += TRACE_FILE_STATEMENT ;

done:
   return rc ;
error:
   goto done ;
}
