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

   Source File Name = ixmExtent.cpp

   Descriptive Name = Index Manager Extent

   When/how to use: this program may be used on binary and text-formatted
   versions of Index Manager component. This file contains functions for index
   extent implmenetation. This include B tree insert/update/delete.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "ixmExtent.hpp"
#include "dmsStorageIndex.hpp"
#include "pd.hpp"
#include "monCB.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsDump.hpp"
#include "pdTrace.hpp"
#include "ixmTrace.hpp"
#include "pdSecure.hpp"

using namespace bson ;

namespace engine
{

   // used in index cursor
   // currentKey is the key from current disk location that trying to be matched
   // prevKey is the key examed from previous run
   // keepFieldsNum is the number of fields from prevKey that should match the
   // currentKey (for example if the prevKey is {c1:1, c2:1}, and keepFieldsNum
   // = 1, that means we want to match c1:1 key for the current location.
   // Depends on if we have skipToNext set, if we do that means we want to skip
   // c1:1 and match whatever the next (for example c1:1.1); otherwise we want
   // to continue match the elements from matchEle )
   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMEXT_KEYCMP, "_ixmExtent::keyCmp" )
   INT32 _ixmExtent::keyCmp ( const BSONObj &currentKey,
                              const BSONObj &prevKey,
                              INT32 keepFieldsNum, BOOLEAN skipToNext,
                              const VEC_ELE_CMP &matchEle,
                              const VEC_BOOLEAN &matchInclusive,
                              const Ordering &o, INT32 direction )
   {
      PD_TRACE_ENTRY ( SDB_IXMEXT_KEYCMP );
      BSONObjIterator ll ( currentKey ) ;
      BSONObjIterator rr ( prevKey ) ;
      VEC_ELE_CMP::const_iterator eleItr = matchEle.begin() ;
      VEC_BOOLEAN ::const_iterator incItr = matchInclusive.begin() ;
      UINT32 mask = 1 ;
      INT32 retCode = 0 ;
      // match keepFieldsNum fields
      for ( INT32 i = 0 ; i < keepFieldsNum; ++i, mask<<=1 )
      {
         BSONElement curEle = ll.next() ;
         BSONElement prevEle = rr.next() ;
         // skip those fields since we don't want to match them from
         // startstopkey iterator
         ++eleItr ;
         ++incItr ;
         INT32 result = curEle.woCompare ( prevEle, FALSE ) ;
         if ( o.descending ( mask ))
            result = -result ;
         if ( result )
         {
            retCode = result ;
            goto done ;
         }
      }
      // if all the keepFieldsNum fields got matched, let's see if we want to
      // simply skip to next key, if so we don't need to match all other
      // elements
      // if that happen, the return value should be -direction, since we want to
      // return -1 if searching forward, otherwise return 1
      if ( skipToNext )
      {
         retCode = -direction ;
         goto done ;
      }
      // if all keepFieldsNum fields got matched, and we want to further match
      // startstopkey iterator, let's move on
      for ( ; ll.more(); mask<<=1 )
      {
         // curEle is always get from current key
         BSONElement curEle = ll.next() ;
         // now let's get the expected element from startstopkey iterator
         BSONElement prevEle = **eleItr ;
         ++eleItr ;
         INT32 result = curEle.woCompare ( prevEle, FALSE ) ;
         if ( o.descending ( mask ))
            result = -result ;
         if ( result )
         {
            retCode = result ;
            goto done ;
         }
         // when getting here, that means the key matches expectation, then
         // let's see if we want inclusive predicate. If not we need to return
         // the negative of direction ( -1 for forward scan, otherwise 1 )
         if ( !*incItr )
         {
            retCode = -direction ;
            goto done ;
         }
         // when get here, it means key match AND inclusive
         ++incItr ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_IXMEXT_KEYCMP, retCode );
      return retCode ;
   }

}


