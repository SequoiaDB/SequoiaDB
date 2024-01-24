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

   Source File Name = utilCommBuff.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/11/2018  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_OBJBUFF_HPP__
#define UTIL_OBJBUFF_HPP__

#include "utilArray.hpp"
#include "utilPooledObject.hpp"

namespace engine
{
   class _utilBuffBlock ;

   /**
    * @brief Buffer monitor is used to monitor space usage in buffer. It can be
    * used among multiple common buffers. In that case, they share the limit
    * defined by the monitor.
    */
   class _utilBuffMonitor : public SDBObject
   {
   public:
      _utilBuffMonitor() ;

      virtual ~_utilBuffMonitor() ;

      INT32 init( UINT64 limit ) ;

      UINT64 limit() const
      {
         return _limit ;
      }

      UINT64 freeSpace() const
      {
         return _limit - _totalUsed ;
      }

      void onAllocate( UINT64 size )
      {
         _totalUsed += size ;
      }

      void onRelease( UINT64 size )
      {
         _totalUsed -= size ;
      }

   private:
      UINT64 _limit ;
      UINT64 _totalUsed ;
   } ;
   typedef _utilBuffMonitor utilBuffMonitor ;

   /**
    * @brief A buffer can manage its memory in different ways. The space can
    * be a large block of continous memory, or several piceses of seperated
    * blocks. All depends on the user's demand.
    */
   enum _utilCommBuffMode
   {
      UTIL_BUFF_MODE_SERIAL,  // Buffer contains continous memory.
      UTIL_BUFF_MODE_SCATTER  // Memory blocks in buffer may seperate.
   } ;
   typedef _utilCommBuffMode utilCommBuffMode ;

   /**
    * @brief Common memory buffer. User can push their data into the buffer,
    * and retrieve them later.
    */
   class _utilCommBuff : public utilPooledObject
   {
      typedef _utilArray<_utilBuffBlock *> BLOCK_ARRAY ;
      typedef _utilArray<_utilBuffBlock *>::iterator BLOCK_ARRAY_ITR ;

   public:
      /**
       * @param mode Continous or seperated memory.
       * @param initSize Initial size of block on the very first allocation.
       * @param limit Space limit for the buffer.
       */
      _utilCommBuff( utilCommBuffMode mode, UINT32 initSize, UINT64 limit ) ;

      /**
       * @param mode Continous or seperated memory.
       * @param initSize Initial size of block on the very first allocation.
       * @param monitor Space monitor for the buffer.
       */
      _utilCommBuff( utilCommBuffMode mode, UINT32 initSize,
                     utilBuffMonitor *monitor ) ;

      ~_utilCommBuff() ;

      /**
       * @brief Prepare continous space of size. New block may be allocated.
       * @param size Requested size.
       */
      INT32 ensureSpace( UINT32 size ) ;

      /**
       * @brief Append data to the buffer.
       * @param[in] data Data to append.
       * @param[in] size Size of the data.
       * @param[out] offset Offset of the data in the buffer. User can
       * retrieve the data from the buffer later by this offset.
       */
      INT32 append( const CHAR *data, UINT32 size, UINT64 *offset = NULL ) ;

      // INT32 write( const CHAR *data, UINT64 offset, UINT32 len ) ;
      // INT32 read( CHAR *buff, UINT64 offset, UINT32 len ) ;

      /**
       * @brief Switch from one buffer mode to another.
       * @param newMode Target mode.
       * @note If current mode is scatter, and new mode is serial, all the
       * buffer blocks, except the largest one, will be freed.
       */
      void switchMode( utilCommBuffMode newMode ) ;

      /**
       * @brief Clear the buffer for reuse. All blocks except the largest one
       * will be freed. No impact in serial mode.
       */
      void clear() ;

      /**
       * @brief Reset the buffer. All blocks which have been allocated will be
       * freed.
       */
      void reset() ;

      /**
       * @brief Try to aquire buffer up to the limit. If it can not be done,
       * nothing will be changed.
       * @return
       */
      INT32 acquireAll( BOOLEAN resetBuff ) ;

      /**
       * @brief Convert an offset in the buffer to pointer.
       * @param offset Offset in buffer
       */
      CHAR *offset2Addr( UINT64 offset ) ;

      UINT64 capacity() const
      {
         return _totalSize ;
      }

   protected:
      UINT64 _reservedSpace() const
      {
         if ( _monitor )
         {
            return _monitor->freeSpace() ;
         }
         else
         {
            return _limit - _totalSize ;
         }
      }

      INT32 _expandBuff( UINT32 minExpandSize ) ;
      INT32 _allocBlock( UINT64 size, _utilBuffBlock **block = NULL ) ;

   protected:
      utilCommBuffMode _mode ;
      UINT32 _initSize ;
      UINT64 _limit ;
      utilBuffMonitor *_monitor ;

      // Three kinds of blocks:
      // Working block - The block currently used for writting.
      // Active block - Blocks before the working block in the array. It's used
      //                for fast calculation of offset for new insert data.
      // Idle block - Blocks after the working block in the array. After reset,
      //              idle blocks may appear.
      BLOCK_ARRAY _blocks ;
      INT32 _workBlockIdx ;
      UINT64 _totalSize ;   // Total block size in the buffer.
      UINT64 _activeBlocksSize ;
   } ;
   typedef _utilCommBuff utilCommBuff ;
}

#endif /* UTIL_OBJBUFF_HPP__ */

