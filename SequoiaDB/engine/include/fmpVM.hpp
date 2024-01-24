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

   Source File Name = fmpVM.hpp

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

#ifndef FMPVM_HPP_
#define FMPVM_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "fmpDef.hpp"
#include "../bson/bson.h"

using namespace bson ;

class _fmpVM : public SDBObject
{
public:
   _fmpVM() ;
   virtual ~_fmpVM() ;

public:
   virtual INT32 init( const BSONObj &param ) ;

//   virtual INT32 compile( const BSONElement &func, const CHAR * ) = 0 ;

   virtual INT32 eval( const BSONObj &func,
                       BSONObj &res ) = 0 ;

   virtual INT32 fetch( BSONObj &res ) = 0 ;

   virtual INT32 initGlobalDB( const CHAR *pHostName,
                               const CHAR *pSvcname,
                               const CHAR *pUser,
                               const CHAR *pPasswd,
                               BSONObj &res ) = 0 ;

   OSS_INLINE BOOLEAN ok() const { return _ok ;}

protected:
   OSS_INLINE void _setContext( SINT64 contextID )
   {
      _contextID = contextID ;
      return ;
   }
   OSS_INLINE SINT64 _getContext(){return _contextID ;}

   OSS_INLINE const BSONObj &_getParam() const
   {
      return _param ;
   }

   OSS_INLINE void _setOK( BOOLEAN isOK )
   {
      _ok = isOK ;
      return ;
   }

private:
   BSONObj _param ;
   SINT64 _contextID ;
   BOOLEAN _ok ;
} ;

#endif

