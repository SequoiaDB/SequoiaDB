/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilCompressionStream.hpp

   Descriptive Name = Compression stream

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
#ifndef UTIL_COMPRESSION_STREAM_HPP_
#define UTIL_COMPRESSION_STREAM_HPP_

#include "utilStream.hpp"
#include "utilCompression.hpp"

namespace engine
{
   
   class utilCompressionInStream: public utilInStream
   {
   private:
      utilCompressionInStream( const utilCompressionInStream& ) ;
      void operator=( const utilCompressionInStream& ) ;

   protected:
      utilCompressionInStream()
         : _upstream( NULL )
      {}

   public:
      virtual ~utilCompressionInStream() {}
      virtual INT32 init( utilInStream& upstream,
                          INT32 bufSize = UTIL_STREAM_DEFAULT_BUFFER_SIZE ) = 0 ;

   protected:
      utilInStream*  _upstream ;
   } ;

   class utilCompressionOutStream: public utilOutStream
   {
   private:
      utilCompressionOutStream( const utilCompressionOutStream& ) ;
      void operator=( const utilCompressionOutStream& ) ;

   protected:
      utilCompressionOutStream()
         : _downstream( NULL )
      {}

   public:
      virtual ~utilCompressionOutStream() {}
      virtual INT32 init( utilOutStream& downstream,
                          UTIL_COMPRESSION_LEVEL level = UTIL_COMP_BALANCE,
                          INT32 bufSize = UTIL_STREAM_DEFAULT_BUFFER_SIZE ) = 0 ;

      virtual INT32 finish() = 0 ;

   protected:
      utilOutStream* _downstream ;
   } ;
}

#endif /* UTIL_COMPRESSION_STREAM_HPP_ */

