/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = utilFileStream.hpp

   Descriptive Name = File I/O stream

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_FILE_STREAM_HPP_
#define UTIL_FILE_STREAM_HPP_

#include "utilStream.hpp"
#include "ossFile.hpp"
#include <string>

namespace engine
{
   class utilFileStreamBase: public SDBObject
   {
   private:
      // disallow copy and assign
      utilFileStreamBase( const utilFileStreamBase& ) ;
      void operator=( const utilFileStreamBase& ) ;

   protected:
      utilFileStreamBase() ;
      
   public:
      virtual ~utilFileStreamBase() ;
      INT32 init( ossFile* file, BOOLEAN fileManaged ) ;
      OSS_INLINE ossFile* file() const { return _file ; }

   protected:
      // in stream only
      INT32 _read( CHAR* buf, INT64 bufLen, INT64& readSize ) ;

      // out stream only
      INT32 _write( const CHAR* buf, INT64 bufLen ) ;
      INT32 _flush() ;

      INT32 _close() ;

   protected:
      ossFile*    _file ;
      BOOLEAN     _fileManaged ;
      BOOLEAN     _inited ;
   } ;
   
   class utilFileInStream: public utilFileStreamBase, public utilInStream
   {
   public:
      utilFileInStream() ;
      ~utilFileInStream() ;

   public:
      INT32 read( CHAR* buf, INT64 bufLen, INT64& readSize ) ;
      INT32 close() ;
   } ;

   class utilFileOutStream: public utilFileStreamBase, public utilOutStream
   {
   public:
      utilFileOutStream() ;
      ~utilFileOutStream() ;

   public:
      INT32 write( const CHAR* buf, INT64 bufLen ) ;
      INT32 flush() ;
      INT32 close() ;
   } ;
}

#endif /* UTIL_FILE_STREAM_HPP_ */
