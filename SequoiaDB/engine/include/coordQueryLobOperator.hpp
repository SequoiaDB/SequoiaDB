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

   Source File Name = coordQueryLobOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/14/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_QUERY_LOB_OPERATOR_HPP__
#define COORD_QUERY_LOB_OPERATOR_HPP__

#include "coordQueryOperator.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordQueryLobOperator define
   */
   class _coordQueryLobOperator : public _coordQueryOperator
   {
      public:
         _coordQueryLobOperator() ;
         virtual ~_coordQueryLobOperator() ;

      public:
         virtual INT32 doOpOnCL( coordCataSel &cataSel,
                                 const BSONObj &objMatch,
                                 coordSendMsgIn &inMsg,
                                 coordSendOptions &options,
                                 pmdEDUCB *cb,
                                 coordProcessResult &result ) ;
   } ;

   typedef _coordQueryLobOperator coordQueryLobOperator ;
}

#endif //COORD_QUERY_LOB_OPERATOR_HPP__

