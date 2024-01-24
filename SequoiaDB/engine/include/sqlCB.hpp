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

   Source File Name = sqlCB.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SQLCB_HPP_
#define SQLCB_HPP_

#include "sqlGrammar.hpp"
#include "qgmBuilder.hpp"
#include "sdbInterface.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _qgmPlanContainer ;
   class _rtnSQLFunc ;

   class _sqlCB : public _IControlBlock
   {
   public:
      _sqlCB() ;
      virtual ~_sqlCB() ;

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_SQL ; }
      virtual const CHAR* cbName() const { return "SQLCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;

   public:
      INT32 exec( const CHAR *sql, _pmdEDUCB *cb,
                  SINT64 &contextID,
                  BOOLEAN &needRollback,
                  BSONObjBuilder *pBuilder = NULL ) ;

      INT32 getFunc( const CHAR *name,
                     UINT32 paramNum,
                     _rtnSQLFunc *&func ) ;

   private:
      INT32 _createContext( _qgmPlanContainer *container,
                            _pmdEDUCB *cb, SINT64 &contextID ) ;

   private:
      SQL_GRAMMAR _grammar ;
   } ;

   typedef class _sqlCB SQL_CB ;

   /*
      get global sql cb
   */
   SQL_CB* sdbGetSQLCB () ;

}

#endif // SQLCB_HPP_

