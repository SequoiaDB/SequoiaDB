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

   Source File Name = ixm.cpp

   Descriptive Name = Index Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of Index Manager component. This file contains functions for
   Index Manager Control Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/07/2014  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "ixm_common.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "ixmTrace.hpp"

using namespace bson;

namespace engine
{

   /*
      Tool functions
   */
   void ixmMakeHashValue( const BSONObj &obj, const BSONObj &key,
                          ixmHashValue & hashValue )
   {
      hashValue.hash = 0 ;
      try
      {
         BSONObjIterator itr( key ) ;
         while ( itr.more() )
         {
            BSONElement orderEle = itr.next() ;
            const CHAR* name = orderEle.fieldName() ;
            BSONElement arrEle = obj.getFieldDottedOrArray( name ) ;
            if ( Array == arrEle.type() )
            {
               hashValue.columns.hash1 = ossHash( arrEle.value(),
                                                  arrEle.valuesize() ) ;
               hashValue.columns.hash2 = ossHash( arrEle.value(),
                                                  arrEle.valuesize(), 3 ) ;
               break ;
            }
         }
      }
      catch( std::exception & e )
      {
         PD_LOG( PDWARNING, "Make hash value occurred exception: %s",
                 e.what() ) ;
      }
   }

   void ixmMakeHashValue( const BSONElement &eleArray,
                          ixmHashValue &hashValue )
   {
      hashValue.columns.hash1 = ossHash( eleArray.value(),
                                         eleArray.valuesize() ) ;
      hashValue.columns.hash2 = ossHash( eleArray.value(),
                                         eleArray.valuesize(), 3 ) ;
   }

}

