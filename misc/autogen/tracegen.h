#ifndef TRACEGEN_H
#define TRACEGEN_H

#include <string>
#include "core.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include "filenamegen.h"

#define TRACEFILENAMEPATH SOURCEPATH "include/pdTrace.h"
#define TRACEFILENAMEPATH1 SOURCEPATH "pd/pdFunctionList.cpp"
#define TRACEINCLUDEPATH SOURCEPATH "include/"
#define TRACEINCLUDESUFFIX "Trace.h"
#define TRACEEYECATCHER "PD_TRACE_DECLARE_FUNCTION"
#define TRACEEYECATCHERLEN 25
class TraceGen
{
public :
   static void genList ();
private :
   static void _extractFromFile ( std::ofstream *fout,
                                  std::ofstream &fout1,
                                  const CHAR *pFileName,
                                  INT32 compid ) ;
   static void _genList ( const CHAR *pPath,
                          std::ofstream *fout,
                          std::ofstream &fout1,
                          INT32 compid ) ;
} ;

const INT32 _pdTraceComponentNum = 28 ;
const CHAR *pdGetTraceComponent ( UINT32 id ) ;

#endif
