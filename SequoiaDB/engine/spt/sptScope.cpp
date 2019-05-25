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

   Source File Name = sptScope.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptScope.hpp"
#include "pd.hpp"
#include "sptObjDesc.hpp"
#include "ossUtil.hpp"
#include "sptCommon.hpp"
#include <algorithm>
using namespace std ;
namespace engine
{

   /*
      _sptResultVal implement
   */
   _sptResultVal::_sptResultVal()
   {
   }

   _sptResultVal::~_sptResultVal()
   {
   }

   BOOLEAN _sptResultVal::hasError() const
   {
      return _errStr.empty() ? FALSE : TRUE ;
   }

   const CHAR* _sptResultVal::getErrrInfo() const
   {
      return _errStr.c_str() ;
   }

   void _sptResultVal::setError( const  string &err )
   {
      _errStr = err ;
   }

   /*
      _sptScope implement
   */
   _sptScope::_sptScope()
   {
      _loadMask = 0 ;
   }

   _sptScope::~_sptScope()
   {

   }

   INT32 _sptScope::getLastError() const
   {
      return sdbGetErrno() ;
   }

   const CHAR* _sptScope::getLastErrMsg() const
   {
      return sdbGetErrMsg() ;
   }

   bson::BSONObj _sptScope::getLastErrObj() const
   {
      const CHAR *pObjData = sdbGetErrorObj() ;

      if ( pObjData )
      {
         try
         {
            bson::BSONObj obj( pObjData ) ;
            return obj ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      return bson::BSONObj() ;
   }

   INT32 _sptScope::loadUsrDefObj( _sptObjDesc *desc )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != desc, "desc can not be NULL" ) ;
      SDB_ASSERT( NULL != desc->getJSClassName(),
                  "obj name can not be empty" ) ;
      rc = _loadUsrDefObj( desc ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to load object defined by user:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptScope::pushJSFileNameToStack( const string &filename )
   {
      _fileNameStack.push_back( filename ) ;
   }

   void _sptScope::popJSFileNameFromStack()
   {
      _fileNameStack.pop_back() ;
   }

   INT32 _sptScope::getStackSize()
   {
      return _fileNameStack.size() ;
   }

   void _sptScope::addJSFileNameToList( const string &filename )
   {
      _fileNameList.push_back( filename ) ;
   }

   void _sptScope::clearJSFileNameList()
   {
      _fileNameList.clear() ;
   }

   BOOLEAN _sptScope::isJSFileNameExistInStack( const string &filename )
   {
      BOOLEAN isExist = FALSE ;
      if( find( _fileNameStack.begin(), _fileNameStack.end(), filename )
          != _fileNameStack.end() )
      {
         isExist = TRUE ;
      }
      return isExist ;
   }

   BOOLEAN _sptScope::isJSFileNameExistInList( const string &filename )
   {
      BOOLEAN isExist = FALSE ;
      if( find( _fileNameList.begin(), _fileNameList.end(), filename )
          != _fileNameList.end() )
      {
         isExist = TRUE ;
      }
      return isExist ;
   }
}
