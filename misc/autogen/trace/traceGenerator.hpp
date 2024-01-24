#ifndef TRACE_GENERATOR_HPP
#define TRACE_GENERATOR_HPP

#include "../generateInterface.hpp"
#include <algorithm>

/* =============== statement =============== */
#define TRACE_FILE_STATEMENT "/* This list file is automatically generated, \
you shoud NOT modify this file anyway!*/"

#define TRACE_SUFFIX_LIST \
   "hpp",   \
   "cpp",   \
   "h",     \
   "c",     \

#define TRACE_FILTER_DIR   ".svn"

#define TRACE_MODULE_LIST_PATH      ENGINE_PATH"pd/pdComponents.cpp"
#define TRACE_FUNC_LIST_FILE_PATH   ENGINE_PATH"pd/pdFunctionList.cpp"

#define TRACE_DEFINE_NAME           "PD_TRACE_DECLARE_FUNCTION"
#define TRACE_DEFINE_NAME_LEN       (sizeof(TRACE_DEFINE_NAME)-1)

#define TRACE_ENTRY_NAME            "PD_TRACE_ENTRY"
#define TRACE_ENTRY_NAME_LEN        (sizeof(TRACE_ENTRY_NAME)-1)

#define TRACE_EXIT_NAME             "PD_TRACE_EXIT"
#define TRACE_EXIT_NAME_LEN         (sizeof(TRACE_EXIT_NAME)-1)

#define TRACE_FILE_PATH             ENGINE_PATH"include/"
#define TRACE_FILE_EXTNAME          "Trace.h"

struct _traceFuncInfo
{
   string alias ;
   string func ;

   bool operator == (const string &v)
   {
      return (this->alias == v) ;
   }
} ;

struct _traceInfo
{
   string name ;
   vector<struct _traceFuncInfo> funcList ;

   bool operator == (const string &v)
   {
      return (this->name == v) ;
   }

   bool operator == (const struct _traceFuncInfo &v)
   {
      vector<struct _traceFuncInfo>::iterator it ;

      it = std::find( this->funcList.begin(), this->funcList.end(), v.alias ) ;

      return ( it != this->funcList.end() ) ;
   }
} ;

class traceGenerator : public generateBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   traceGenerator() ;
   ~traceGenerator() ;
   int init() ;
   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout,
                   string &outputPath ) ;
   const char* name() { return "trace" ; }

private:
   int _genTraceFile( int id, fileOutStream &fout, string &outputPath ) ;
   int _genTraceFuncListFile( fileOutStream &fout, string &outputPath ) ;

   int _getTraceList() ;
   int _parseTraceComponents( const string &content ) ;
   int _getTraceFuncList( const string &module,
                          vector<_traceFuncInfo> &funcList ) ;

   int _checkTraceFunctionExist( const string &content,
                                 size_t offset,
                                 const string &path,
                                 struct _traceFuncInfo &funcInfo ) ;
   int _checkTraceEntryAndExit( const string &content,
                                size_t offset,
                                const string &path,
                                struct _traceFuncInfo &funcInfo ) ;

   int _parseTraceFunc( const string &path, vector<_traceFuncInfo> &funcList ) ;
   int _buildStatement( string &headerDesc ) ;

private:
   bool _isFinish ;
   int  _funcNum ;
   int  _seqNum ;
   vector<struct _traceInfo> _traceList ;
} ;

#endif
