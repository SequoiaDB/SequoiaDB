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

   Source File Name = ixm.hpp

   Descriptive Name = Index Management Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   Index Record ID (IRID) and Index Control Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/07/2014  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXM_COMMON_HPP_
#define IXM_COMMON_HPP_

#include "core.hpp"
#include "../bson/bson.h"

using namespace std ;
using namespace bson ;

#define IXM_NAME_FIELD              IXM_FIELD_NAME_NAME
#define IXM_KEY_FIELD               IXM_FIELD_NAME_KEY
#define IXM_V_FIELD                 IXM_FIELD_NAME_V
#define IXM_UNIQUE_FIELD            IXM_FIELD_NAME_UNIQUE
#define IXM_ENFORCED_FIELD          IXM_FIELD_NAME_ENFORCED
#define IXM_DROPDUP_FIELD           IXM_FIELD_NAME_DROPDUPS
#define IXM_2DRANGE_FIELD           IXM_FIELD_NAME_2DRANGE

namespace engine
{
   /*
      _ixmHashValue define
   */
   union _ixmHashValue
   {
      struct
      {
         UINT32   hash1 ;
         UINT32   hash2 ;
      } columns ;
      UINT64 hash ;
   } ;
   typedef _ixmHashValue ixmHashValue ;

   /*
      Make object's array field hash value
   */
   void ixmMakeHashValue( const BSONObj &obj, const BSONObj &key,
                          ixmHashValue &hashValue ) ;

   void ixmMakeHashValue( const BSONElement &eleArray,
                          ixmHashValue &hashValue ) ;

}

#endif /* IXM_COMMON_HPP_ */

