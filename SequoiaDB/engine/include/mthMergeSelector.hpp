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

   Source File Name = mthMergeSelector.hpp

   Descriptive Name = Method Merge Selector Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for selecting
   operation, which is generating a new BSON record from a given record, based
   on the given selecting rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTHMERGESELECTOR_HPP_
#define MTHMERGESELECTOR_HPP_
#include "core.hpp"
#include "oss.hpp"
#include <vector>
#include "ossUtil.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"
using namespace bson ;
namespace engine
{
   enum _MergeSelectorSide
   {
      MERGE_SELECTOR_SIDE_OUTER,
      MERGE_SELECTOR_SIDE_INNER,
      MERGE_SELECTOR_SIDE_UNKNOWN
   } ;
   class _MergeSelectorElement : public SDBObject
   {
   public :
      _MergeSelectorSide _side ;
      const CHAR *_pSourceName ;
      const CHAR *_pTargetName ;
      _MergeSelectorElement ()
      {
         _pSourceName = NULL ;
         _pTargetName = NULL ;
         _side = MERGE_SELECTOR_SIDE_UNKNOWN ;
      }
      INT32 init ( const CHAR *pSourceName,
                   const CHAR *pTargetName,
                   const CHAR *pOuterAlias,
                   const CHAR *pInnerAlias ) ;
   } ;
   typedef _MergeSelectorElement MergeSelectorElement ;
   class _mthMergeSelector : public SDBObject
   {
   private :
      const CHAR *_pOuterAlias ;
      const CHAR *_pInnerAlias ;
      BSONObj _selectorPattern ;
      BOOLEAN _initialized ;
      vector<MergeSelectorElement> _selectorElements ;
      INT32 _addSelector ( const BSONElement &ele ) ;

      INT32 _buildNewObj (
                           BSONObjBuilder &b,
                           const BSONObj &obj,
                           const CHAR *pSourceName,
                           const CHAR *pTargetName ) ;
   public :
      _mthMergeSelector ()
      {
         _initialized = FALSE ;
      }
      ~_mthMergeSelector()
      {
         _selectorElements.clear() ;
      }
      INT32 loadPattern ( const BSONObj &selectorPattern,
                          const CHAR *pOuterAlias,
                          const CHAR *pInnerAlias ) ;
      INT32 select ( const BSONObj &outer,
                     const BSONObj &inner,
                     BSONObj &target ) ;
      OSS_INLINE BOOLEAN isInitialized () { return _initialized ; }
      BSONObj getPattern ()
      {
         return _selectorPattern ;
      }
   } ;
   typedef _mthMergeSelector mthMergeSelector ;
}

#endif
