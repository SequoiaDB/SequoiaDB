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

   Source File Name = utilCommBuff.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/11/2018  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilCommBuff.hpp"
#include "utilBuffBlock.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"

namespace engine
{
   _utilBuffMonitor::_utilBuffMonitor()
   : _totalUsed( 0 )
   {
   }

   _utilBuffMonitor::~_utilBuffMonitor()
   {
   }

   INT32 _utilBuffMonitor::init( UINT64 limit )
   {
      _limit = limit ;
      return SDB_OK ;
   }

   _utilCommBuff::_utilCommBuff( utilCommBuffMode mode, UINT32 initSize,
                                 UINT64 limit )
   : _mode( mode ),
     _initSize( initSize ),
     _limit( limit ),
     _monitor( NULL ),
     _workBlockIdx( -1 ),
     _totalSize( 0 ),
     _activeBlocksSize( 0 )
   {
   }

   _utilCommBuff::_utilCommBuff( utilCommBuffMode mode, UINT32 initSize,
                                 utilBuffMonitor *monitor )
   : _mode( mode ),
     _initSize( initSize ),
     _limit( 0 ),
     _monitor( monitor ),
     _workBlockIdx( -1 ),
     _totalSize( 0 ),
     _activeBlocksSize( 0 )
   {
   }

   _utilCommBuff::~_utilCommBuff()
   {
      utilBuffBlock *block = NULL ;
      BLOCK_ARRAY_ITR itr( _blocks ) ;
      while( itr.next( block ) )
      {
         if ( block )
         {
            if ( _monitor )
            {
               _monitor->onRelease( block->capacity() ) ;
            }
            SDB_OSS_DEL block ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMMBUFF_ENSURESPACE, "_utilCommBuff::ensureSpace" )
   INT32 _utilCommBuff::ensureSpace( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMMBUFF_ENSURESPACE ) ;

      if ( -1 == _workBlockIdx )
      {
         // If request size is greater than initial size, use the request size
         // for allocation.
         UINT32 blockSize = size > _initSize ?  size : _initSize ;
         blockSize = (UINT32)ossNextPowerOf2( blockSize ) ;

         rc = _allocBlock( blockSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate buffer block of size[%u] "
                                   "failed[%d]", blockSize, rc ) ;
      }
      else if ( size > _blocks[ _workBlockIdx ]->freeSize() )
      {
         SDB_ASSERT( _workBlockIdx >= 0 && _blocks.size() > 0,
                     "Block array is invalid" ) ;
         BOOLEAN needExpand = TRUE ;
         if ( UTIL_BUFF_MODE_SCATTER == _mode )
         {
            // Check if any idle block which can hold the object. All the skipped
            // ones need to be counted into _activeBlockSize.
            UINT32 nextIdx = 0 ;
            UINT64 skipSize = _blocks[ _workBlockIdx ]->capacity() ;
            for ( nextIdx = (UINT32)(_workBlockIdx + 1);
                  nextIdx < _blocks.size(); ++nextIdx )
            {
               if ( size <= _blocks[ nextIdx ]->capacity() )
               {
                  _workBlockIdx = nextIdx ;
                  needExpand = FALSE ;
                  break ;
               }
               else
               {
                  skipSize += _blocks[ nextIdx ]->capacity() ;
               }
            }
            if ( !needExpand )
            {
               _activeBlocksSize += skipSize ;
            }
         }

         if ( needExpand )
         {
            rc = _expandBuff( size ) ;
            if ( rc )
            {
               if ( SDB_HIT_HIGH_WATERMARK != rc )
               {
                  PD_LOG( PDERROR, "Extend sort buffer failed[%d]", rc ) ;
               }
               goto error ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCOMMBUFF_ENSURESPACE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMMBUFF_APPEND, "_utilCommBuff::append" )
   INT32 _utilCommBuff::append( const CHAR *data, UINT32 size, UINT64 *offset )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMMBUFF_APPEND ) ;
      UINT64 offsetInBlk = 0 ;

      SDB_ASSERT( data, "Data is NULL" ) ;
      SDB_ASSERT( size > 0, "Data size is invalid" ) ;

      rc = ensureSpace( size ) ;
      if ( rc )
      {
         if ( SDB_HIT_HIGH_WATERMARK != rc )
         {
            PD_LOG( PDERROR, "Ensure space of size[%u] in buffer failed[%d]",
                    rc ) ;
         }
         goto error ;
      }

      offsetInBlk = _blocks[ _workBlockIdx ]->length() ;
      rc = _blocks[ _workBlockIdx ]->append( data, size ) ;
      PD_RC_CHECK( rc, PDERROR, "Append data to sort buffer failed[%d]", rc ) ;

      if ( offset )
      {
         *offset = ( UTIL_BUFF_MODE_SERIAL == _mode ) ?
                   offsetInBlk : ( _activeBlocksSize + offsetInBlk ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCOMMBUFF_APPEND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMMBUFF_SWITCHMODE, "_utilCommBuff::switchMode" )
   void _utilCommBuff::switchMode( utilCommBuffMode newMode )
   {
      PD_TRACE_ENTRY( SDB__UTILCOMMBUFF_SWITCHMODE ) ;

      if ( UTIL_BUFF_MODE_SCATTER == _mode &&
           UTIL_BUFF_MODE_SERIAL == newMode &&
           _blocks.size() > 1 )
      {
         utilBuffBlock *block = NULL ;
         utilBuffBlock *largestBlock = NULL ;
         BLOCK_ARRAY_ITR itr( _blocks ) ;

         itr.next( largestBlock ) ;

         while ( itr.next( block ) )
         {
            utilBuffBlock *blockToRelease = NULL ;
            if ( !block )
            {
               break ;
            }

            // Release the smaller block.
            blockToRelease = block->capacity() > largestBlock->capacity() ?
                             largestBlock : block ;
            if ( blockToRelease == largestBlock )
            {
               largestBlock = block ;
            }
            if ( _monitor )
            {
               _monitor->onRelease( blockToRelease->capacity() ) ;
            }

            SDB_OSS_DEL blockToRelease ;
         }
         _blocks.clear( TRUE ) ;
         _blocks.append( largestBlock ) ;
         _workBlockIdx = 0 ;
         _totalSize = largestBlock->capacity() ;
         _activeBlocksSize = 0 ;
      }

      _mode = newMode ;
      PD_TRACE_EXIT( SDB__UTILCOMMBUFF_SWITCHMODE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMMBUFF_CLEAR, "_utilCommBuff::clear" )
   void _utilCommBuff::clear()
   {
      PD_TRACE_ENTRY( SDB__UTILCOMMBUFF_CLEAR ) ;
      utilBuffBlock *block = NULL ;
      BLOCK_ARRAY_ITR itr( _blocks ) ;

      if ( _blocks.size() > 0 )
      {
         while ( itr.next( block ) )
         {
            if ( block )
            {
               if ( _monitor )
               {
                  _monitor->onRelease( block->capacity() ) ;
               }
               _totalSize -= block->capacity() ;
               SDB_OSS_DEL block ;
            }
         }

         _blocks.clear( TRUE ) ;
         _workBlockIdx = -1 ;
         _activeBlocksSize = 0 ;
      }

      PD_TRACE_EXIT( SDB__UTILCOMMBUFF_CLEAR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMMBUFF_RESET, "_utilCommBuff::reset" )
   void _utilCommBuff::reset()
   {
      PD_TRACE_ENTRY( SDB__UTILCOMMBUFF_RESET ) ;
      utilBuffBlock *block = NULL ;
      BLOCK_ARRAY_ITR itr( _blocks ) ;

      if ( 0 == _blocks.size() )
      {
         goto done ;
      }

      while ( itr.next( block ) )
      {
         if ( block )
         {
            block->reset() ;
         }
      }

      _workBlockIdx = 0 ;
      _activeBlocksSize = 0 ;

   done:
      PD_TRACE_EXIT( SDB__UTILCOMMBUFF_RESET ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMMBUFF_ACQUIREALL, "_utilCommBuff::acquireAll" )
   INT32 _utilCommBuff::acquireAll( BOOLEAN resetBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMMBUFF_ACQUIREALL ) ;
      UINT64 extendSize =
            _monitor ? _monitor->freeSpace() : ( _limit - _totalSize ) ;

      if ( ( -1 == _workBlockIdx ) || UTIL_BUFF_MODE_SCATTER == _mode )
      {
         rc = _allocBlock( extendSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate buffer block of size[%llu] "
                                   "failed[%d]", extendSize, rc ) ;
      }
      else
      {
         rc = _blocks[ _workBlockIdx ]->resize( _totalSize + extendSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Resize buffer block from size[%zd] to "
                                   "[%llu] failed[%d]",
                      _blocks[ _workBlockIdx ] ->capacity(), _limit, rc ) ;
         _totalSize += extendSize ;
      }

      if ( resetBuff )
      {
         reset() ;
      }
   done:
      PD_TRACE_EXITRC( SDB__UTILCOMMBUFF_ACQUIREALL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMMBUFF_OFFSET2ADDR, "_utilCommBuff::offset2Addr" )
   CHAR *_utilCommBuff::offset2Addr( UINT64 offset )
   {
      PD_TRACE_ENTRY( SDB__UTILCOMMBUFF_OFFSET2ADDR ) ;
      utilBuffBlock *block = NULL ;
      BLOCK_ARRAY_ITR itr( _blocks ) ;

      while ( itr.next( block ) )
      {
         if ( block )
         {
            if ( offset < block->capacity() )
            {
               break ;
            }
            offset -= block->capacity() ;
            block = NULL ;
         }
      }

      PD_TRACE_EXIT( SDB__UTILCOMMBUFF_OFFSET2ADDR ) ;
      return block ? (block->offset2Addr( offset )) : NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMMBUFF__EXPANDBUFF, "_utilCommBuff::_expandBuff" )
   INT32 _utilCommBuff::_expandBuff( UINT32 minExpandSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMMBUFF__EXPANDBUFF ) ;
      UINT64 expandSize = 0 ;
      UINT64 newTotalSize = 0 ;

      expandSize = _totalSize < _reservedSpace() ?
                   _totalSize : _reservedSpace() ;
      if ( 0 == expandSize )
      {
         rc = SDB_HIT_HIGH_WATERMARK ;
         goto error ;
      }

      if ( minExpandSize > expandSize )
      {
         UINT64 expectSize = ossNextPowerOf2( minExpandSize ) ;
         if ( expectSize > _reservedSpace() )
         {
            rc = SDB_HIT_HIGH_WATERMARK ;
            goto error ;
         }
         else
         {
            expandSize = expectSize ;
         }
      }

      newTotalSize = _totalSize + expandSize ;
      if ( UTIL_BUFF_MODE_SERIAL == _mode )
      {
         rc = _blocks[ _workBlockIdx ]->resize( newTotalSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Resize buffer block from size[%zd] to "
                                   "size[%lld] failed[%d]",
                      _blocks[ _workBlockIdx ]->capacity(), newTotalSize, rc ) ;
         if ( _monitor )
         {
            _monitor->onAllocate( expandSize ) ;
         }
         _totalSize = newTotalSize ;
      }
      else
      {
         rc = _allocBlock( expandSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate buffer block of size[%llu] "
                                   "failed[%d]", expandSize, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCOMMBUFF__EXPANDBUFF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMMBUFF__ALLOCBLOCK, "_utilCommBuff::_allocBlock" )
   INT32 _utilCommBuff::_allocBlock( UINT64 size, utilBuffBlock **block )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMMBUFF__ALLOCBLOCK ) ;
      utilBuffBlock *newBlock = NULL ;

      if ( _reservedSpace() < size )
      {
         rc = SDB_HIT_HIGH_WATERMARK ;
         goto error ;
      }

      newBlock = SDB_OSS_NEW utilBuffBlock( size ) ;
      if ( !newBlock )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory for buffer block failed. Requested "
                          "size: %u", sizeof( utilBuffBlock ) ) ;
         goto error ;
      }

      rc = newBlock->init() ;
      PD_RC_CHECK( rc, PDERROR, "Initialize buffer block failed[%d]", rc ) ;

      rc = _blocks.append( newBlock ) ;
      PD_RC_CHECK( rc, PDERROR, "Insert block into list failed[%d]", rc ) ;

      // If the current working block is valid, add its size to active block
      // size before switch to the new working block.
      if ( -1 != _workBlockIdx )
      {
         SDB_ASSERT( UTIL_BUFF_MODE_SCATTER == _mode, "Mode is wrong" ) ;
         _activeBlocksSize = _totalSize ;
      }
      _workBlockIdx = _blocks.size() - 1 ;
      _totalSize += size ;

      if ( _monitor )
      {
         _monitor->onAllocate( size ) ;
      }

      if ( block )
      {
         *block = newBlock ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILCOMMBUFF__ALLOCBLOCK, rc ) ;
      return rc ;
   error:
      SAFE_OSS_DELETE( newBlock ) ;
      goto done ;
   }
}

