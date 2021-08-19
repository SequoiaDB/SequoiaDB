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

   Source File Name = utilBuffBlock.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/11/2018  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_BUFFBLOCK_HPP__
#define UTIL_BUFFBLOCK_HPP__

#include "oss.hpp"

namespace engine
{
/**
 * @brief A continuous block of memory in the sort area.
 */
   class _utilBuffBlock : public SDBObject
   {
   public:
      _utilBuffBlock( UINT64 size ) ;

      virtual ~_utilBuffBlock() ;

      INT32 init() ;

      /**
       * @brief Insert data into the block in append mode.
       * @param[in] data - Data to insert.
       * @param[in] len - Data length.
       */
      INT32 append( const CHAR *data, UINT32 len ) ;

      /**
       * @brief Get address by offset of the block.
       * @param[in] offset - Offset in the block.
       * @return A valid pointer if offset is valid. Otherwise NULL will be
       * returned.
       * @note User can directly read from or write to any valid offset in the
       * block. In this case, the user should make sure not to corrupt the data
       * in the block by themselves.
       */
      CHAR *offset2Addr( UINT64 offset ) const
      {
         return ( offset < _size ) ? ( _buff + offset ) : NULL ;
      }

      UINT64 capacity() const
      {
         return _size ;
      }

      /**
       * @brief Total size of data in the block.
       */
      UINT64 length() const
      {
         return _writePos ;
      }

      UINT64 freeSize() const
      {
         return _size - _writePos ;
      }

      void reset()
      {
         _writePos = 0 ;
      }

      INT32 resize( UINT64 newSize ) ;

   private:
      CHAR *_buff ;
      UINT64 _size ;
      UINT64 _writePos ;
   };

   typedef _utilBuffBlock utilBuffBlock ;
}

#endif /* UTIL_BUFFBLOCK_HPP__ */

