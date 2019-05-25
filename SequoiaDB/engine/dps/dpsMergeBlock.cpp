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
      _extLID  = DMS_INVALID_EXTENT ;
      _needNty = FALSE ;
      _transEnabled = FALSE ;
   }

   _dpsMergeInfo::_dpsMergeInfo( dpsMergeBlock &block )
   :_refer(block),
    _hasDummy(FALSE)
   {
      _csLID   = ~0 ;
      _clLID   = ~0 ;
      _extLID  = DMS_INVALID_EXTENT ;
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

