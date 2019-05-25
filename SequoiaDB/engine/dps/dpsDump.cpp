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

   Source File Name = dpsDump.cpp

   Descriptive Name = Data Protection Service Log File

   When/how to use: this program may be used on binary and text-formatted
   versions of data protection component. This file contains code logic for
   DPS transaction log file basic operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/12/2013  XJH  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsDump.hpp"
#include "dpsDef.hpp"
#include "ossUtil.hpp"
#include "dpsLogFile.hpp"

namespace engine
{

   UINT32 _dpsDump::dumpLogFileHead( CHAR *inBuf, UINT32 inSize,
                                     CHAR *outBuf, UINT32 outSize,
                                     UINT32 options )
   {
      SDB_ASSERT ( inBuf, "inbuf can't be NULL" ) ;
      SDB_ASSERT ( outBuf, "outbuf can't be NULL" ) ;
      SDB_ASSERT ( DPS_LOG_HEAD_LEN == inSize,
                   "insize must be DPS_LOG_HEAD_LEN" ) ;

      UINT32 len           = 0 ;
      UINT32 hexDumpOption = 0 ;
      if ( DPS_DMP_OPT_HEX & options )
      {
         hexDumpOption |= OSS_HEXDUMP_INCLUDE_ADDR ;
         if ( !(DPS_DMP_OPT_HEX_WITH_ASCII & options ) )
         {
            hexDumpOption |= OSS_HEXDUMP_RAW_HEX_ONLY ;
         }
         ossHexDumpBuffer ( (void*)inBuf, inSize, outBuf, outSize, NULL,
                            hexDumpOption ) ;
         len = ossStrlen ( outBuf ) ;
         outBuf [ len ] = '\n' ;
         ++len ;
      }
      if ( DPS_DMP_OPT_FORMATTED & options )
      {
         dpsLogHeader *logHead = (dpsLogHeader*)inBuf ;
         /* dump output looks like
          *  Head    : SDBLOGHD
          *  FirstLSN: 0x00000000123456789(4886718345)
          *  LogID   : 10
          */
         len += ossSnprintf ( outBuf + len, outSize - len,
                              OSS_NEWLINE
                              " Head   : %c%c%c%c%c%c%c%c"OSS_NEWLINE,
                              logHead->_eyeCatcher[0],
                              logHead->_eyeCatcher[1],
                              logHead->_eyeCatcher[2],
                              logHead->_eyeCatcher[3],
                              logHead->_eyeCatcher[4],
                              logHead->_eyeCatcher[5],
                              logHead->_eyeCatcher[6],
                              logHead->_eyeCatcher[7] ) ;
         if ( ossMemcmp ( DPS_LOG_HEADER_EYECATCHER, logHead->_eyeCatcher,
                          DPS_LOG_HEADER_EYECATCHER_LEN ) != 0 )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Invalid Eye Catcher"OSS_NEWLINE ) ;
            goto exit ;
         }
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " FirstLSN: 0x%016lx(%lld)"OSS_NEWLINE,
                              logHead->_firstLSN.offset,
                              logHead->_firstLSN.offset ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " LogID  : %d"OSS_NEWLINE,
                              logHead->_logID ) ;
      }

   exit :
      return len ;
   }

}

