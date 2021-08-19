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

   Source File Name = utilStream.hpp

   Descriptive Name = Stream API

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/5/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_STREAM_HPP_
#define UTIL_STREAM_HPP_

#include "ossTypes.hpp"
#include "oss.hpp"

namespace engine
{
   /* interface */
   class utilInStream: public SDBObject
   {
   private:
      // disallow copy and assign
      utilInStream( const utilInStream& ) ;
      void operator=( const utilInStream& ) ;
   protected:
      utilInStream() {}
   public:
      virtual ~utilInStream() {}
      virtual INT32 read( CHAR* buf, INT64 bufLen, INT64& readSize ) = 0 ;
      virtual INT32 close() = 0 ;
   } ;

   /* interface */
   class utilOutStream: public SDBObject
   {
   private:
      // disallow copy and assign
      utilOutStream( const utilOutStream& ) ;
      void operator=( const utilOutStream& ) ;
   protected:
      utilOutStream() {}
   public:
      virtual ~utilOutStream() {}
      virtual INT32 write( const CHAR* buf, INT64 bufLen ) = 0 ;
      virtual INT32 flush() = 0 ;
      virtual INT32 close() = 0 ;
   } ;

   #define UTIL_STREAM_DEFAULT_BUFFER_SIZE (64 * 1024)

   class utilStreamInterrupt
   {
   public:
      utilStreamInterrupt() {}
      virtual ~utilStreamInterrupt() {}
      virtual BOOLEAN isInterrupted() = 0 ;
   } ;

   class utilStream: public SDBObject
   {
   public:
      utilStream() ;
      ~utilStream() ;

   public:
      INT32 init( INT32 bufSize = UTIL_STREAM_DEFAULT_BUFFER_SIZE ) ;
      INT32 copy( utilInStream& in, utilOutStream& out,
                  INT64* streamSize = NULL, utilStreamInterrupt* si = NULL ) ;
      INT32 copy( utilInStream& in, utilOutStream&out, INT64 size,
                  INT64* streamSize = NULL, utilStreamInterrupt* si = NULL ) ;

   private:
      CHAR* _buf ;
      INT32 _bufSize ;
   } ;
}

#endif /* UTIL_STREAM_HPP_ */
