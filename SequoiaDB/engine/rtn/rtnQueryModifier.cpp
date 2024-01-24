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

   Source File Name = rtnQueryModifier.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/3/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnQueryModifier.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{
   _rtnQueryModifier::_rtnQueryModifier()
   : _returnNew ( FALSE )
   {
   }

   INT32 _rtnQueryModifier::_parseModifyEle( const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( FIELD_NAME_RETURNNEW, ele.fieldName() ) )
      {
         if ( !ele.isBoolean() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Field[%s] in hint should be bool[%d]",
                    FIELD_NAME_RETURNNEW, rc ) ;
            goto error ;
         }
         _returnNew = ele.boolean() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnQueryModifier::_onInit()
   {
      INT32 rc = SDB_OK ;

      if ( RTN_MODIFY_UPDATE != _modifyOp )
      {
         goto done ;
      }

      if ( _opOption.isEmpty() )
      {
         PD_LOG ( PDERROR, "modifier can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         rc = _modifier.loadPattern ( _opOption,
                                      &_dollarList ) ;
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected for updator: "
                                   "%s", _opOption.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Invalid pattern is detected for update: %s: %s",
                  _opOption.toString().c_str(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
