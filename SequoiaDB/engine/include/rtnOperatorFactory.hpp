/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnOperatorFactory.hpp

   Descriptive Name = Factory for rtn operators.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_OPERATOR_FACTORY__
#define RTN_OPERATOR_FACTORY__

#include "rtnOperator.hpp"
#include "rtnQueryOptions.hpp"
#include "../bson/bsonobj.h"

namespace engine
{
   enum _rtnQueryType
   {
      RTN_QUERY_NORMAL = 1,
      RTN_QUERY_TEXT
   } ;
   typedef enum _rtnQueryType rtnQueryType ;

   class _rtnOperatorFactory : public SDBObject
   {
      public:
         _rtnOperatorFactory() ;
         ~_rtnOperatorFactory() ;

         INT32 create( const rtnQueryOptions &options,
                       rtnOperator **pOperator ) ;
         void release( rtnOperator **pOperator ) ;
      private:
         INT32 _getQueryType( const BSONObj &query, rtnQueryType &qType ) ;
   } ;
   typedef _rtnOperatorFactory rtnOperatorFactory ;

   rtnOperatorFactory* rtnGetOperatorFactory() ;
}

#endif /* RTN_OPERATOR_FACTORY__ */

