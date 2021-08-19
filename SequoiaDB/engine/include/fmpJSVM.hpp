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

   Source File Name = fmpJSVM.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef FMPJSVM_HPP_
#define FMPJSVM_HPP_

#include "fmpVM.hpp"
#include <string>

namespace engine
{
   class _sptContainer ;
   class _sptScope ;
   class _sptSPVal ;
}
namespace sdbclient
{
   class _sdbCursor ;
}

class _fmpJSVM : public _fmpVM
{
public:
   _fmpJSVM() ;
   virtual ~_fmpJSVM() ;

public:
   virtual INT32 eval( const BSONObj &func,
                       BSONObj &res ) ;

   virtual INT32 fetch( BSONObj &res ) ;

   virtual INT32 initGlobalDB( const CHAR *pHostName,
                               const CHAR *pSvcname,
                               const CHAR *pUser,
                               const CHAR *pPasswd,
                               BSONObj &res ) ;

  // virtual INT32 compile( const BSONElement &func, const CHAR *name ) ;

private:
   INT32 _transCode2Str( const BSONElement &ele, std::string &str ) ;

   INT32 _getValType( const engine::_sptSPVal *pVal ) const ;

private:
   engine::_sptContainer   *_engine ;
   engine::_sptScope       *_scope ;
   std::string             _cmd ;
   sdbclient::_sdbCursor*  _cursor ;
} ;

#endif

