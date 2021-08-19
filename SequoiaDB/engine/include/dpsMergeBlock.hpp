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

   Source File Name = dpsMergeBlock.hpp

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

#ifndef DPSMERGEBLOCK_H_
#define DPSMERGEBLOCK_H_

#include "core.hpp"
#include "oss.hpp"
#include "dpsLogDef.hpp"
#include "dpsLogRecord.hpp"
#include "dpsPageMeta.hpp"
#include "dms.hpp"
#include "sdbInterface.hpp"

namespace engine
{
   class _dpsLogPage ;
   class _dpsReplicaLogMgr ;
   class _dpsMergeInfo ;

   /*
      _dpsMergeBlock define
   */
   class _dpsMergeBlock : public SDBObject
   {
      friend class _dpsReplicaLogMgr ;
      friend class _dpsMergeInfo ;

      private:
         dpsLogRecord _record ;
         dpsPageMeta _pageMeta ;
         BOOLEAN _isRow;

      public:
         _dpsMergeBlock();

         ~_dpsMergeBlock();

      public:
         OSS_INLINE BOOLEAN isRow()
         {
            return _isRow;
         }

         OSS_INLINE void setRow( BOOLEAN isRow )
         {
            _isRow = isRow;
         }

         OSS_INLINE dpsLogRecord &record()
         {
            return _record ;
         }

         OSS_INLINE dpsPageMeta &pageMeta()
         {
            return _pageMeta ;
         }

      public:
         void clear();
   };

   typedef class _dpsMergeBlock dpsMergeBlock;

   /*
      _dpsMergeInfo define
   */
   class _dpsMergeInfo : public SDBObject
   {
      public:
         _dpsMergeInfo () ;
         _dpsMergeInfo ( dpsMergeBlock &block ) ;
         ~_dpsMergeInfo () ;

         dpsMergeBlock &getDummyBlock () ;
         dpsMergeBlock &getMergeBlock () ;
         BOOLEAN       hasDummy () ;
         void clear()
         {
            _mergeBlock.clear() ;
            _dummyBlock.clear() ;
            _hasDummy = FALSE ;
         }
         void setInfoEx( UINT32 csLID, UINT32 clLID, dmsExtentID extLID,
                         IExecutor *cb )
         {
            _csLID   = csLID ;
            _clLID   = clLID ;
            _extLID  = extLID ;
            _needNty = TRUE ;
            _pCB     = cb ;
         }
         void enableTrans()
         {
            _transEnabled = TRUE ;
         }
         void resetInfoEx()
         { 
            _needNty = FALSE ;
            _transEnabled = FALSE ;
            _pCB     = NULL ;
         }
         BOOLEAN isNeedNotify() const { return _needNty ; }
         BOOLEAN isTransEnabled() const { return _transEnabled ; }
         UINT32  getCSLID() const { return _csLID ; }
         UINT32  getCLLID() const { return _clLID ; }
         dmsExtentID getExtentLID() const { return _extLID ; }
         IExecutor* getEDUCB() const { return _pCB ; }

      private:
         dpsMergeBlock        _mergeBlock ;
         dpsMergeBlock        _dummyBlock ;
         dpsMergeBlock        &_refer ;
         BOOLEAN              _hasDummy ;

         UINT32               _csLID ;
         UINT32               _clLID ;
         dmsExtentID          _extLID ;
         BOOLEAN              _needNty ;
         BOOLEAN              _transEnabled ;
         IExecutor            *_pCB ;

   } ;

   typedef class _dpsMergeInfo dpsMergeInfo ;

}

#endif
