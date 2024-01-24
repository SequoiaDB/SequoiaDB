/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

#ifdef _LINUX
#include <elf.h>

#ifdef OSS_ARCH_64

   typedef Elf64_Ehdr Elf_Ehdr ;
   typedef Elf64_Phdr Elf_Phdr ;

#else
   typedef Elf32_Ehdr Elf_Ehdr ;
   typedef Elf32_Phdr Elf_Phdr ;

#endif // OSS_ARCH_64

#endif //_LINUX

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
   #define MEMCHECK_OPTION_DETAIL            "detail"

   #define MEMCHECK_EXPLAIN_HELP             "print help information"
   #define MEMCHECK_EXPLAIN_VERSION          "print version"
   #define MEMCHECK_EXPLAIN_CORE_FILE        "core file"
   #define MEMCHECK_EXPLAIN_OUTPUT           "output file"
   #define MEMCHECK_EXPLAIN_DETAIL           "show detial items"

   #define MEMCHECK_GENERAL_OPTIONS \
      (MEMCHECK_OPTION_HELP",h",             /* no arg */     MEMCHECK_EXPLAIN_HELP) \
      (MEMCHECK_OPTION_VERSION",V",          /* no arg */     MEMCHECK_EXPLAIN_VERSION) \
      (MEMCHECK_EXPLAIN_CORE_FILE",f",      _TYPE(string),    MEMCHECK_EXPLAIN_CORE_FILE) \
      (MEMCHECK_OPTION_OUTPUT",o",          _TYPE(string),    MEMCHECK_EXPLAIN_OUTPUT) \
      (MEMCHECK_OPTION_DETAIL",d",           /* no arg */     MEMCHECK_EXPLAIN_DETAIL) \


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
      OSS_INLINE BOOLEAN       isDetail() const { return _detail ; }

   private:
      INT32 setOptions();

   private:
      BOOLEAN        _parsed;
      string         _coreFile;
      string         _outputFile;
      BOOLEAN        _detail ;
   } ;

   Options::Options()
   {
      _parsed = FALSE ;
      _detail = FALSE ;
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

      if (has(MEMCHECK_OPTION_DETAIL))
      {
         _detail = TRUE ;
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

   // global define
   string g_corefile ;
   string g_outputfile ;
   BOOLEAN g_outDetail = FALSE ;

   OSSFILE g_pCoreFile ;
   BOOLEAN g_openCoreFile  = FALSE ;
   UINT64  g_readPos       = 0 ;
   CHAR*   g_pBuff         = NULL ;
   INT64   g_dataLen       = 0 ;
   OSSFILE g_pOutFile ;
   BOOLEAN g_openOutFile   = FALSE ;
   CHAR    g_textBuff[ 1024 ] = { 0 } ;
   CHAR    g_textAddr[ 64 ] = { 0 } ;

   map< UINT64, UINT64 > g_elfHdlrMap ;
   BOOLEAN               g_elfParsed = FALSE ;
   UINT64                g_elfPhnum = 0 ;
   UINT64                g_elfParsedPhnum = 0 ;
   UINT64                g_elfPhoff = 0 ;
   UINT64                g_elfPhsize = 0 ;

   typedef struct _memItem
   {
      UINT32      _count ;
      UINT64      _size ;

      _memItem( UINT32 count = 0, UINT64 size = 0 )
      {
         _count = count ;
         _size  = size ;
      }

      bool operator< ( const _memItem &rhs ) const
      {
         return _count < rhs._count ? true : false ;
      }
   } memItem ;

   map< UINT64, memItem > g_memMap ;
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

      // buff
      g_pBuff = new ( std::nothrow ) CHAR[TEST_BUFFSIZE] ;
      if( !g_pBuff )
      {
         std::cerr << "Alloc memory failed" << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }

      // output
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

   void addMem( UINT64 key, UINT64 size )
   {
      map< UINT64, memItem >::iterator it = g_memMap.find( key ) ;
      if ( it == g_memMap.end() )
      {
         g_memMap[ key ] = memItem( 1, size ) ;
      }
      else
      {
         ++(it->second)._count ;
         (it->second)._size += size ;
      }
   }

#ifdef _LINUX

   INT32 doParseElfInfo()
   {
      INT32 rc = SDB_OK ;
      INT64 pos = 0 ;

      /// elf header
      if ( 0 == g_readPos )
      {
         Elf_Ehdr *pElfHeader = (Elf_Ehdr*)( g_pBuff + pos ) ;

         /// check magic
         if ( pElfHeader->e_ident[ EI_MAG0 ] != ELFMAG0 ||
              pElfHeader->e_ident[ EI_MAG1 ] != ELFMAG1 ||
              pElfHeader->e_ident[ EI_MAG2 ] != ELFMAG2 ||
              pElfHeader->e_ident[ EI_MAG3 ] != ELFMAG3 )
         {
            ossPrintf( "Invalid elf file\n" ) ;
            rc = SDB_INVALID_FILE_TYPE ;
            goto error ;
         }
         /// check type
         else if ( pElfHeader->e_type != ET_CORE )
         {
            ossPrintf( "Invalid corefile\n" ) ;
            rc = SDB_INVALID_FILE_TYPE ;
            goto error ;
         }
         else if ( pElfHeader->e_ehsize < sizeof( Elf_Ehdr ) )
         {
            ossPrintf( "Invalid ehsize\n" ) ;
            rc = SDB_INVALID_FILE_TYPE ;
            goto error ;
         }
         else if ( pElfHeader->e_phoff < pElfHeader->e_ehsize )
         {
            ossPrintf( "Invalid phoff\n" ) ;
            rc = SDB_INVALID_FILE_TYPE ;
            goto error ;
         }

         g_elfPhoff = pElfHeader->e_phoff ;
         g_elfPhnum = pElfHeader->e_phnum ;
         g_elfPhsize= pElfHeader->e_phentsize ;

         if ( g_elfPhsize < sizeof( Elf_Phdr ) )
         {
            ossPrintf( "Invalid phsize\n" ) ;
            rc = SDB_INVALID_FILE_TYPE ;
            goto error ;
         }

         /*
         ossPrintf( "Pasrse ELF header succeed:" OSS_NEWLINE
                    "  Elf Phoff : %llu" OSS_NEWLINE
                    "  Elf Phnum : %llu" OSS_NEWLINE
                    "  Elf Phsize: %llu" OSS_NEWLINE,
                    g_elfPhoff,
                    g_elfPhnum,
                    g_elfPhsize ) ; */

         pos += pElfHeader->e_ehsize ;
         g_readPos += pos ;
         g_dataLen -= pos ;
         pos = 0 ;
      }

      if ( g_elfParsedPhnum >= g_elfPhnum )
      {
         g_elfParsed = TRUE ;
         // ossPrintf( "Parse ELF Header table succeed" OSS_NEWLINE ) ;
         goto done ;
      }
      else if ( g_readPos + g_dataLen < g_elfPhoff + sizeof( Elf_Phdr ) )
      {
         g_readPos = g_elfPhoff ;
         goto done ;
      }
      else if ( g_readPos < g_elfPhoff )
      {
         pos = g_elfPhoff - g_readPos ;
      }

      while ( pos < g_dataLen &&
              pos < ( INT64 )(TEST_BUFFSIZE - sizeof( Elf_Phdr ) ) )
      {
         Elf_Phdr *pPhdr = ( Elf_Phdr* )( g_pBuff + pos ) ;

         /*
         ossPrintf( "PHDR Item (%llu)" OSS_NEWLINE
                    "offset: %lx, vaddr: %lx, paddr: %lx, filesz: %lx, memsz: %lx" OSS_NEWLINE,
                    g_elfParsedPhnum + 1,
                    pPhdr->p_offset, pPhdr->p_vaddr,
                    pPhdr->p_paddr, pPhdr->p_filesz,
                    pPhdr->p_memsz ) ; */

         g_elfHdlrMap[ pPhdr->p_vaddr ] = pPhdr->p_paddr ;
         ++g_elfParsedPhnum ;
         pos += g_elfPhsize ;

         if ( g_elfParsedPhnum >= g_elfPhnum )
         {
            g_elfParsed = TRUE ;
            // ossPrintf( "Parse ELF Header table succeed" OSS_NEWLINE ) ;
            break ;
         }
      }

      g_readPos += pos ;
      g_dataLen -= pos ;

   done:
      return rc ;
   error:
      goto done ;
   }

#else

   INT32 doParseElfInfo()
   {
      g_elfParsed = TRUE ;
      return SDB_OK ;
   }

#endif // _LINUX

   const CHAR* getAddrByOffset( UINT64 offset )
   {
      map< UINT64, UINT64 >::iterator it = g_elfHdlrMap.upper_bound( offset ) ;
      if ( it != g_elfHdlrMap.end() && --it != g_elfHdlrMap.end() )
      {
         CHAR *ptr = NULL ;
         *(ossValuePtr*)( &ptr ) =
            ( ossValuePtr )( it->second + offset - it->first ) ;
         ossSnprintf( g_textAddr, sizeof( g_textAddr ),
                      "%p", ptr ) ;
         return g_textAddr ;
      }
      return "-" ;
   }

   void printMemInfo( CHAR *pointer, UINT64 offset, BOOLEAN isError )
   {
      memHeader *pHeader = (memHeader*)pointer ;
      if ( FALSE == isError )
      {
         ossSnprintf( g_textBuff, sizeof(g_textBuff)-1,
                      "%14x %18s %10ld    %30s(%10u)    %6u\n",
                      offset,
                      getAddrByOffset( offset ),
                      pHeader->_size,
                      autoGetFileName(pHeader->_file).c_str(),
                      pHeader->_file,
                      pHeader->_line ) ;
      }
      else
      {
         ossSnprintf( g_textBuff, sizeof(g_textBuff)-1,
                      "%14x %18s %10ld    %30s(%10u)    %6u    ****(has error)\n",
                      offset,
                      getAddrByOffset( offset ),
                      pHeader->_size,
                      autoGetFileName(pHeader->_file).c_str(),
                      pHeader->_file,
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
            INT64 tmpPos = g_readPos + pos ;
            ++g_totalMemNum ;
            UINT64 key = ossPack32To64( pHeader->_file, pHeader->_line ) ;
            addMem( key, pHeader->_size ) ;

            if ( !ossMemVerify( (void*)( (CHAR*)pHeader + TEST_MEM_HEADSZ ) ) )
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

            if ( hasError || g_outDetail )
            {
               printMemInfo( (CHAR*)pHeader, tmpPos, hasError ) ;
            }
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
                   "                        File              :   Line ----      Count           Size\n\n" ) ;
      if ( g_openOutFile )
      {
         ossWrite( &g_pOutFile, g_textBuff, ossStrlen(g_textBuff), &writeLen ) ;
      }
      else
      {
         ossPrintf ( "%s", g_textBuff ) ;
      }

      // order by count
      multimap<memItem, UINT64> countFileMap ;
      map< UINT64, memItem >::iterator fileCountIt = g_memMap.begin() ;
      while ( fileCountIt != g_memMap.end() )
      {
         countFileMap.insert(
            multimap<memItem, UINT64>::value_type(
               fileCountIt->second, fileCountIt->first ) ) ;
         ++fileCountIt ;
      }

      multimap<memItem, UINT64>::reverse_iterator it = countFileMap.rbegin() ;
      while ( it != countFileMap.rend() )
      {
         ossUnpack32From64( it->second, file, line ) ;

         ossSnprintf( g_textBuff, sizeof(g_textBuff)-1,
                      "%30s(%10u): %6u ---- %10u %14ld\n",
                      autoGetFileName(file).c_str(), file, line,
                      it->first._count, it->first._size ) ;
         
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
      g_outDetail = options.isDetail() ;

      rc = init() ;
      if ( rc )
      {
         goto error ;
      }

      /// parse elf file
      while ( !g_elfParsed && SDB_OK == getNextBlock() )
      {
         rc = doParseElfInfo() ;
         if ( rc )
         {
            goto error ;
         }
      }

      /// print title
      ossSnprintf( titleStr, OSS_MAX_PATHSIZE,
                   "Memory Info List:\n"
                   "   Offset(Hex)            Address       Size                                          File      Line\n\n" ) ;
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

