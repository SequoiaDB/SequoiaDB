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

   Source File Name = dmsCompress.hpp

   Descriptive Name =

   When/how to use: str util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/06/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMSCOMPRESS_HPP__
#define DMSCOMPRESS_HPP__

#include "core.hpp"
#include "ossRWMutex.hpp"
#include "utilCompressor.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   /*
    * One collection has one compressor entry, in which the compressor and
    * dictionary addresses are stored.
    * Compressor and dictionary values:
    * (1) No compression is configured for the collection, both are NULL.
    * (2) Dictionary compression is configured, both are valid.
    * (3) Otherwise( snappy ), compressor is valid while dictionary is NULL.
    */
   class _dmsCompressorEntry
   {
      friend class _dmsCompressorGuard ;
   public:
      _dmsCompressorEntry() ;

      void setCompressor( utilCompressor *compressor ) ;
      void setDictionary( const utilDictHandle dictionary ) ;

      OSS_INLINE utilCompressor* getCompressor() { return _compressor ; }
      OSS_INLINE const utilDictHandle getDictionary() { return _dictionary ; }

      /*
       * Whether the compressor is ready. Only then it's true the compression/
       * decompression can be done.
       */
      OSS_INLINE BOOLEAN ready() { return ( NULL != _compressor ) ; }

      void reset() ;

   private:
      utilCompressor *_compressor ;    /* Global compressor address */
      utilDictHandle _dictionary ;     /* For dictionary compression */
      ossRWMutex _lock ;
   } ;
   typedef _dmsCompressorEntry dmsCompressorEntry ;

   /* Concurrence protection for collection compressor entry. */
   class _dmsCompressorGuard
   {
   public:
      _dmsCompressorGuard( _dmsCompressorEntry *compEntry, OSS_LATCH_MODE mode )
         : _lock( &compEntry->_lock),
           _mode( mode )
      {
         if ( SHARED == _mode )
         {
            _lock->lock_r() ;
         }
         else if ( EXCLUSIVE == _mode )
         {
            _lock->lock_w() ;
         }
      }

      ~_dmsCompressorGuard()
      {
         release() ;
      }

      void release()
      {
         if ( SHARED == _mode )
         {
            _lock->release_r() ;
         }
         else if ( EXCLUSIVE == _mode )
         {
            _lock->release_w() ;
         }
         _mode = -1 ;
      }

   private:
      ossRWMutex *_lock ;
      INT32 _mode ;
   } ;
   typedef _dmsCompressorGuard dmsCompressorGuard ;

   /*
      ppData: output data pointer, not need release
   */
   INT32 dmsCompress ( _pmdEDUCB *cb, _dmsCompressorEntry *compressorEntry,
                       const CHAR *pInputData, INT32 inputSize,
                       const CHAR **ppData, INT32 *pDataSize,
                       UINT8 &ratio ) ;

   INT32 dmsCompress ( _pmdEDUCB *cb, _dmsCompressorEntry *compressorEntry,
                       const BSONObj &obj, const CHAR* pOIDPtr, INT32 oidLen,
                       const CHAR **ppData, INT32 *pDataSize, UINT8 &ratio ) ;

   /*
      ppData: output data pointer, not need release
   */
   INT32 dmsUncompress ( _pmdEDUCB *cb, _dmsCompressorEntry *compressorEntry,
                         const CHAR *pInputData, INT32 inputSize,
                         const CHAR **ppData, INT32 *pDataSize ) ;
}

#endif // DMSCOMPRESS_HPP__

