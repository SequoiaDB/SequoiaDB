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

   Source File Name = sqlUtil.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sqlUtil.hpp"
#include "../bson/bson.h"
#include "pdTrace.hpp"
#include "sqlTrace.hpp"

using namespace bson ;

namespace engine
{
   static void dump( const SQL_CONTAINER &c,
                          INT32 indent )
   {
      for ( SQL_CONTAINER::const_iterator itr = c.begin();
            itr != c.end();
            itr++ )
      {
         cout << string( indent * 4, ' ') << "|--(value:" <<
                 string( itr->value.begin(), itr->value.end())
                 << " id:"<< itr->value.id().to_long()<< ")" << endl ;
         dump( itr->children, indent + 1) ;
      }
   }


   void sqlDumpAst( const SQL_CONTAINER &c )
   {
      dump( c, 0 ) ;
   }
}
