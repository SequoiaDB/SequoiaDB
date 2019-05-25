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

   Source File Name = dpsLogPage.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSLOGPAGE_HPP_
#define DPSLOGPAGE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "dpsMessageBlock.hpp"
#include "ossRWMutex.hpp"
#include "dpsLogDef.hpp"
#include <string>

using namespace std ;

namespace engine
{
   const INT8 DPS_LSN_START_FROM_HEAD = 0;
   const INT8 DPS_LSN_NOT_START_FROM_HEAD = -1;

   class _dpsLogPage : public SDBObject
   {
      private:
         ossRWMutex _mtx;
         _dpsMessageBlock *_mb;
         UINT32 _pageNumber;
         INT8 _startPage;
         DPS_LSN _beginLSN ;

      public:
         _dpsLogPage();

         _dpsLogPage( UINT32 size );

         ~_dpsLogPage();

         string toString() const ;

      public:
         OSS_INLINE UINT32 getLastSize() const
         {
            return _mb->idleSize();
         }

         OSS_INLINE UINT32 getBufSize() const
         {
            return _mb->size();
         }

         OSS_INLINE UINT32 getLength() const
         {
            return _mb->length();
         }

         OSS_INLINE void clear()
         {
            _startPage = DPS_LSN_START_FROM_HEAD ;
            _mb->clear();
            return ;
         }

         OSS_INLINE void setStartFlag( INT8 flag )
         {
            _startPage = flag;
            return;
         }

         OSS_INLINE INT8 getStartFlag() const
         {
            return _startPage;
         }

         OSS_INLINE void setLsn( const DPS_LSN &lsn , UINT32 offset )
         {
            ossMemcpy( _mb->offset( offset ), ( CHAR * )&lsn,
                       sizeof(DPS_LSN) );
            return;
         }

         OSS_INLINE DPS_LSN getLsn( UINT32 offset )
         {
            return *( (DPS_LSN *)_mb->offset( offset ) );
         }

         OSS_INLINE void setBeginLSN ( const DPS_LSN &lsn )
         {
            _beginLSN = lsn  ;
         }
         OSS_INLINE DPS_LSN getBeginLSN ()
         {
            return _beginLSN ;
         }

         OSS_INLINE _dpsMessageBlock *mb()
         {
            return _mb;
         }

         OSS_INLINE void setNumber( UINT32 number )
         {
            _pageNumber = number;
            return;
         }

         OSS_INLINE UINT32 getNumber()
         {
            return _pageNumber;
         }

         OSS_INLINE void lockShared()
         {
            _mtx.lock_r();
            return ;
         }

         OSS_INLINE void unlockShared()
         {
            _mtx.release_r() ;
            return ;
         }

         OSS_INLINE void lock()
         {
            _mtx.lock_w() ;
            return ;
         }

         OSS_INLINE void unlock()
         {
            _mtx.release_w() ;
            return ;
         }

      public:

         INT32 fill( UINT32 offset, const CHAR *src, UINT32 len );

         INT32 allocate( UINT32 len, UINT64 &offset );

         INT32 allocate( UINT32 len );
   };

   typedef class _dpsLogPage dpsLogPage;
}
#endif

