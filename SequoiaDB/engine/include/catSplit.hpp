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

   Source File Name = catSplit.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/07/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_SPLIT_HPP__
#define CAT_SPLIT_HPP__

#include "core.hpp"
#include "pd.hpp"
#include "oss.hpp"
#include "ossErr.h"
#include "../bson/bson.h"
#include "catDef.hpp"
#include "pmd.hpp"
#include "clsTask.hpp"
#include "catLevelLock.hpp"

using namespace bson ;

namespace engine
{

   INT32 catSplitPrepare ( const BSONObj &splitInfo, pmdEDUCB *cb,
                           UINT32 &returnGroupID, INT32 &returnVersion ) ;

   INT32 catSplitReady ( const BSONObj &splitInfo, UINT64 taskID,
                         pmdEDUCB *cb, INT16 w,
                         UINT32 &returnGroupID, INT32 &returnVersion ) ;

   INT32 catSplitStart ( UINT64 taskID, pmdEDUCB *cb, INT16 w ) ;

   INT32 catSplitChgMeta ( const BSONObj &splitInfo, UINT64 taskID,
                           pmdEDUCB * cb, INT16 w ) ;

   INT32 catSplitCleanup ( UINT64 taskID, pmdEDUCB *cb, INT16 w ) ;

   INT32 catSplitFinish ( UINT64 taskID, pmdEDUCB *cb, INT16 w ) ;

   INT32 catSplitCancel ( const BSONObj &splitInfo, pmdEDUCB *cb,
                          UINT64 &taskID, INT16 w, UINT32 &returnGroupID ) ;

}


#endif //CAT_SPLIT_HPP__

