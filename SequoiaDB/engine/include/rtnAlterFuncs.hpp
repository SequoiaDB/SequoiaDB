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

   Source File Name = rtnAlterFuncs.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_ALTERFUNCS_HPP_
#define RTN_ALTERFUNCS_HPP_

#include "rtnAlterDef.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _dpsLogWrapper ;

   INT32 rtnCreateIDIndex( const CHAR *name,
                           const bson::BSONObj &pubArgs,
                           const bson::BSONObj &args,
                           _pmdEDUCB *cb,
                           _dpsLogWrapper *dpsCB ) ;

   INT32 rtnCreateIDIndexVerify( const bson::BSONObj &args ) ;

   INT32 rtnDropIDIndex( const CHAR *name,
                         const bson::BSONObj &pubArgs,
                         const bson::BSONObj &args,
                         _pmdEDUCB *cb,
                         _dpsLogWrapper *dpsCB ) ;

   INT32 rtnDropIDIndexVerify( const bson::BSONObj &args )
   {
      return SDB_OK ;
   }
}

#endif

