/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = sdbCheckDumpMem.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/
#include "oss.hpp"
#include "ossIO.hpp"
#include "ossMem.h"
#include "ossVer.h"
#include "ossUtil.hpp"
#include "utilOptions.hpp"
#include "filenames.hpp"

#include <iostream>
#include <stdio.h>
#include <string>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

using namespace std ;
using namespace engine ;

namespace fs = boost::filesystem ;

namespace memcheck
{
   #define _TYPE(T) utilOptType(T)

   #define MEMCHECK_OPTION_HELP              "help"
   #define MEMCHECK_OPTION_VERSION           "version"
   #define MEMCHECK_OPTION_CORE_FILE         "corefile"
   #define MEMCHECK_OPTION_OUTPUT            "output"

   #define MEMCHECK_EXPLAIN_HELP             "print help information"
   #define MEMCHECK_EXPLAIN_VERSION          "print version"
   #define MEMCHECK_EXPLAIN_CORE_FILE        "core file"
   #define MEMCHECK_EXPLAIN_OUTPUT           "output file"

   #define MEMCHECK_GENERAL_OPTIONS \
      (MEMCHECK_OPTION_HELP",h",             /* no arg */     MEMCHECK_EXPLAIN_HELP) \
      (MEMCHECK_OPTION_VERSION",V",          /* no arg */     MEMCHECK_EXPLAIN_VERSION) \
      (MEMCHECK_EXPLAIN_CORE_FILE",f",      _TYPE(string),    MEMCHECK_EXPLAIN_CORE_FILE) \
      (MEMCHECK_OPTION_OUTPUT",o",          _TYPE(string),    MEMCHECK_EXPLAIN_OUTPUT) \


   class Options: public utilOptions
   {
   public:
      Options();
      ~Options();
      INT32 parse(INT32 argc, CHAR* argv[]);
      void printHelpInfo();
      BOOLEAN hasHelp();
      BOOLEAN hasVersion();

      OSS_INLINE const string& coreFile() const { return _coreFile; }
      OSS_INLINE const string& outputFile() const { return _outputFile; }

   private:
      INT32 setOptions();

   private:
      BOOLEAN        _parsed;
      string         _coreFile;
      string         _outputFile;
   } ;

   Options::Options()
   {
      _parsed = FALSE ;
   }

   Options::~Options()
   {
   }

   INT32 Options::parse(INT32 argc, CHAR* argv[])
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(!_parsed, "can't parse again");

      addOptions("General Options")
         MEMCHECK_GENERAL_OPTIONS
      ;

      rc = utilOptions::parse(argc, argv);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _parsed = TRUE;

      if (has(MEMCHECK_OPTION_HELP) ||
          has(MEMCHECK_OPTION_VERSION))
      {
         goto done;
      }

      rc = setOptions();
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
   
   void Options::printHelpInfo()
   {
      SDB_ASSERT(_parsed, "must be parsed");
      print();
   }
   
   BOOLEAN Options::hasHelp()
   {
      return has(MEMCHECK_OPTION_HELP);
   }

   BOOLEAN Options::hasVersion()
   {
      return has(MEMCHECK_OPTION_VERSION);
   }
   
   INT32 Options::setOptions()
   {
      INT32 rc = SDB_OK;

      if (has(MEMCHECK_EXPLAIN_CORE_FILE))
      {
         _coreFile = get<string>(MEMCHECK_EXPLAIN_CORE_FILE);
      }

      if (_coreFile.empty())
      {
         std::cerr << "Core file is not configured" << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if (has(MEMCHECK_OPTION_OUTPUT))
      {
         _outputFile = get<string>(MEMCHECK_OPTION_OUTPUT);
      }

   done:
      return rc;
   error:
      goto done;
   }
   
   struct _memHeader
   {
      UINT32               _eye1 ;
      UINT32               _freed ;
      UINT64               _size ;
      UINT32               _debug ;
      UINT32               _file ;
      UINT32               _line ;
      UINT32               _eye2 ;

      _memHeader ()
      {
         SDB_ASSERT( 32 == sizeof(_memHeader), "Size must be 32Byte" ) ;
      }
   } ;
   typedef _memHeader memHeader ;

   #define TEST_MEM_HEADSZ                sizeof(memHeader)
   #define TEST_BUFFSIZE                  (1024*1024*1024)

   string g_corefile ;
   string g_outputfile ;

   OSSFILE g_pCoreFile ;
   BOOLEAN g_openCoreFile  = FALSE ;
   UINT64  g_readPos       = 0 ;
   CHAR*   g_pBuff         = NULL ;
   INT64   g_dataLen       = 0 ;
   OSSFILE g_pOutFile ;
   BOOLEAN g_openOutFile   = FALSE ;
   CHAR    g_textBuff[ 1024 ] = {0} ;

   map< UINT64, UINT32 > g_memMap ;
   UINT64  g_totalMemSize  = 0 ;
   UINT64  g_totalMemNum   = 0 ;
   UINT32  g_errMemNum     = 0 ;

   INT32 init()
   {
      INT32 rc = SDB_OK ;

      rc = ossOpen( g_corefile.c_str(), OSS_READONLY, OSS_RU, g_pCoreFile ) ;
      if ( rc )
      {
         std::cerr << "Open file[" << g_corefile << "] failed, rc: " << rc
                   << std::endl ;
         goto error ;
      }
      g_openCoreFile = TRUE ;

      g_pBuff = new ( std::nothrow ) CHAR[TEST_BUFFSIZE] ;
      if( !g_pBuff )
      {
         std::cerr << "Alloc memory failed" << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }

      if ( !g_outputfile.empty() )
      {
         rc = ossOpen( g_outputfile.c_str(), OSS_CREATE|OSS_READWRITE,
                       OSS_RWXU|OSS_RWXG, g_pOutFile ) ;
         if ( rc )
         {
            std::cerr << "Open file[" << g_outputfile << "] failed, rc: " << rc
                      << std::endl ;
            goto error ;
         }
         g_openOutFile = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 fini()
   {
      if ( g_openCoreFile )
      {
         ossClose( g_pCoreFile ) ;
         g_openCoreFile = FALSE ;
      }
      if ( g_openOutFile )
      {
         ossClose( g_pOutFile ) ;
         g_openOutFile = FALSE ;
      }
      if ( g_pBuff )
      {
         delete [] g_pBuff ;
         g_pBuff = NULL ;
      }
 
      return SDB_OK ;
   }

   INT32 getNextBlock()
   {
      INT32 rc = SDB_OK ;
      INT64 readLen = 0 ;
      INT64 needRead = TEST_BUFFSIZE ;

      if ( !g_openCoreFile )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      rc = ossSeek( &g_pCoreFile, (INT64)g_readPos, OSS_SEEK_SET ) ;
      if ( rc )
      {
         std::cerr << "seek to " << g_readPos << "failed" << std::endl ;
         goto error ;
      }
      g_dataLen = 0 ;

      while ( TRUE )
      {
         readLen = 0 ;
         rc = ossRead( &g_pCoreFile, g_pBuff+g_dataLen, needRead-g_dataLen,
                       &readLen ) ;
         g_dataLen += readLen ;
         if ( SDB_INTERRUPT == rc )
         {
            continue ;
         }
         else if ( SDB_OK == rc )
         {
            break ;
         }
         else if ( SDB_EOF == rc )
         {
            break ;
         }
         else
         {
            std::cerr << "read failed, rc: " << rc << std::endl ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void addMem( UINT64 key )
   {
      map< UINT64, UINT32 >::iterator it = g_memMap.find( key ) ;
      if ( it == g_memMap.end() )
      {
         g_memMap[ key ] = 1 ;
      }
      else
      {
         (it->second)++ ;
      }
   }

   string getFileName( UINT32 fileCode )
   {
      map<UINT32, string>::const_iterator it ;
      it = filenamesMap.find( fileCode ) ;
      if (it != filenamesMap.end() )
      {
         return it->second ;
      }
      else
      {
         return "" ;
      }
   }

   void printMemInfo( CHAR *pointer, BOOLEAN isError )
   {
      memHeader *pHeader = (memHeader*)pointer ;
      if ( FALSE == isError )
      {
         ossSnprintf( g_textBuff, sizeof(g_textBuff)-1,
                      "%p    %10ld    %25s(%10u)    %6u\n",
                      pointer, pHeader->_size,
                      getFileName(pHeader->_file).c_str(), pHeader->_file,
                      pHeader->_line ) ;
      }
      else
      {
         ossSnprintf( g_textBuff, sizeof(g_textBuff)-1,
                      "%p    %10ld    %25s(%10u)    %6u    ****(has error)\n",
                      pointer, pHeader->_size,
                      getFileName(pHeader->_file).c_str(), pHeader->_file,
                      pHeader->_line ) ;
      }

      if ( g_openOutFile )
      {
         INT32 rc = SDB_OK ;
         INT64 writeLen = 0 ;
         INT64 toWrite = ossStrlen( g_textBuff ) ;
         const CHAR *writePtr = g_textBuff ;

         while ( TRUE )
         {
            writeLen = 0 ;
            rc = ossWrite( &g_pOutFile, writePtr, toWrite, &writeLen ) ;
            toWrite -= writeLen ;
            writePtr += writeLen ;
            if ( SDB_INTERRUPT == rc )
            {
               continue ;
            }
            break ;
         }
      }
      else
      {
         ossPrintf ( "%s", g_textBuff ) ;
      }
   }

   INT32 doMemCheck()
   {
      INT64 pos = 0 ;
      BOOLEAN hasError = FALSE ;

      while ( pos < g_dataLen &&
              pos < (INT64)(TEST_BUFFSIZE - TEST_MEM_HEADSZ) )
      {
         hasError = FALSE ;
         memHeader *pHeader = ( memHeader* )( g_pBuff + pos ) ;
         if ( pHeader->_eye1 == SDB_MEMHEAD_EYECATCHER1 &&
              pHeader->_eye2 == SDB_MEMHEAD_EYECATCHER2 &&
              pHeader->_freed == 0 )
         {
            ++g_totalMemNum ;
            UINT64 key = ossPack32To64( pHeader->_file, pHeader->_line ) ;
            addMem( key ) ;
            if ( pHeader->_size > 1024*1024*1024 )
            {
               hasError = TRUE ;
               pos += 1 ;
               ++g_errMemNum ;
            }
            else
            {
               pos += ( pHeader->_size + TEST_MEM_HEADSZ ) ;
               g_totalMemSize += pHeader->_size ;
            }

            printMemInfo( (CHAR*)pHeader, hasError ) ;
         }
         else
         {
            pos += 1 ;
         }
      }

      g_readPos += pos ;

      return SDB_OK ;
   }

   void printResult()
   {
      UINT32 file = 0 ;
      UINT32 line = 0 ;
      INT64 writeLen = 0 ;

      ossSnprintf( g_textBuff, sizeof(g_textBuff)-1,
                   "\n\nStat Info:\n"
                   "                      File           :   Line ----      Count\n\n") ;
      if ( g_openOutFile )
      {
         ossWrite( &g_pOutFile, g_textBuff, ossStrlen(g_textBuff), &writeLen ) ;
      }
      else
      {
         ossPrintf ( "%s", g_textBuff ) ;
      }

      multimap<UINT32, UINT64> countFileMap ;
      map< UINT64, UINT32 >::iterator fileCountIt = g_memMap.begin() ;
      while ( fileCountIt != g_memMap.end() )
      {
         countFileMap.insert(
            multimap<UINT32, UINT64>::value_type(
               fileCountIt->second, fileCountIt->first ) ) ;
         ++fileCountIt ;
      }

      multimap<UINT32, UINT64>::reverse_iterator it = countFileMap.rbegin() ;
      while ( it != countFileMap.rend() )
      {
         ossUnpack32From64( it->second, file, line ) ;

         ossSnprintf( g_textBuff, sizeof(g_textBuff)-1, "%25s(%10u): %6u ---- %10u\n",
                      getFileName(file).c_str(), file, line, it->first ) ;
         
         if ( g_openOutFile )
         {
            ossWrite( &g_pOutFile, g_textBuff, ossStrlen(g_textBuff), &writeLen ) ;
         }
         else
         {
            ossPrintf ( "%s", g_textBuff ) ;
         }

         ++it ;
      }

      ossSnprintf( g_textBuff, sizeof(g_textBuff)-1, "\n\nTotalNum: %llu\n"
                   "TotalSize: %llu\nErrorNum: %u\nStatNum: %u\n",
                   g_totalMemNum, g_totalMemSize, g_errMemNum,
                   g_memMap.size() ) ;
      if ( g_openOutFile )
      {
         ossWrite( &g_pOutFile, g_textBuff, ossStrlen(g_textBuff), &writeLen ) ;
      }
      else
      {
         ossPrintf ( "%s", g_textBuff ) ;
      }
   }

   INT32 mastMain( int argc, char **argv )
   {
      INT32 rc = SDB_OK ;
      CHAR    titleStr[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      Options options ;

      rc = options.parse(argc, argv);
      if (SDB_OK != rc)
      {
         goto error ;
      }

      if (options.hasHelp())
      {
         options.printHelpInfo();
         goto done;
      }

      if (options.hasVersion())
      {
         ossPrintVersion("sdbmemcheck");
         goto done;
      }

      g_corefile = options.coreFile();
      g_outputfile = options.outputFile();

      rc = init() ;
      if ( rc )
      {
         goto error ;
      }

      ossSnprintf( titleStr, OSS_MAX_PATHSIZE,
                   "Memory Info List:\n"
                   "  Address                Size                        File                  Line\n\n" ) ;
      if ( g_openOutFile )
      {
         ossWriteN( &g_pOutFile, titleStr, ossStrlen( titleStr ) ) ;
      }
      else
      {
         ossPrintf( "%s", titleStr ) ;
      }

      while ( SDB_OK == getNextBlock() )
      {
         rc = doMemCheck() ;
         if ( rc )
         {
            goto error ;
         }
      }

      printResult() ;

   done:
      fini() ;
      return rc ;
   error:
      goto done ;
   }

}

int main ( int argc, char **argv )
{
   return memcheck::mastMain( argc, argv ) ;
}

