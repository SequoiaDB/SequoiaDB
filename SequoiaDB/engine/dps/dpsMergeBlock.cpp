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

   Source File Name = dpsMergeBlock.cpp

   Descriptive Name = Data Protection Services Merge Block

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for merging blocks

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft
          12/05/2012  TW  remove std

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "dpsMergeBlock.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "dpsLogPage.hpp"
#include "dpsReplicaLogMgr.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"

namespace engine
{
   _dpsMergeBlock::_dpsMergeBlock():_isRow(FALSE)
   {
  //    clear() ;
   }

   _dpsMergeBlock::~_dpsMergeBlock()
   {
   }


   PD_TRACE_DECLARE_FUNCTION ( SDB__DPSMGBLK_CLEAR, "_dpsMergeBlock::clear" )
   void _dpsMergeBlock::clear()
   {
      PD_TRACE_ENTRY ( SDB__DPSMGBLK_CLEAR );
      _isRow = FALSE;
      _record.clear() ;
      _pageMeta.clear() ;
      PD_TRACE_EXIT ( SDB__DPSMGBLK_CLEAR );
   }

   _dpsMergeInfo::_dpsMergeInfo ()
   :_refer(_mergeBlock),
    _hasDummy(FALSE)
   {
      _csLID   = ~0 ;
      _clLID   = ~0 ;
      _extID  = DMS_INVALID_EXTENT ;
      _extOffset = DMS_INVALID_OFFSET ;
      _needNty = FALSE ;
      _transEnabled = FALSE ;
   }

   _dpsMergeInfo::_dpsMergeInfo( dpsMergeBlock &block )
   :_refer(block),
    _hasDummy(FALSE)
   {
      _csLID   = ~0 ;
      _clLID   = ~0 ;
      _extID  = DMS_INVALID_EXTENT ;
      _extOffset = DMS_INVALID_OFFSET ;
      _needNty = FALSE ;
      _transEnabled = FALSE ;
   }

   _dpsMergeInfo::~_dpsMergeInfo ()
   {
   }

   dpsMergeBlock &_dpsMergeInfo::getDummyBlock ()
   {
      return _dummyBlock ;
   }

   dpsMergeBlock &_dpsMergeInfo::getMergeBlock ()
   {
      return _refer ;
   }

   BOOLEAN _dpsMergeInfo::hasDummy ()
   {
      return _dummyBlock.pageMeta().valid() ? TRUE : FALSE ;
   }

}

