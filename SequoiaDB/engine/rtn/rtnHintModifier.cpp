/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = rtnHintModifier.cpp

   Descriptive Name = Hint modifier

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/06/2022  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnHintModifier.hpp"
#include "../bson/bson.hpp"
#include "rtnTrace.hpp"
#include "ossUtil.hpp"
#include "msgDef.hpp"
#include "rtn.hpp"

using namespace bson ;

namespace engine
{
   _rtnHintModifier::_rtnHintModifier()
   : _modifyOp( RTN_MODIFY_INVALID ),
     _updateShardingKey( FALSE )
   {
   }

   _rtnHintModifier::~_rtnHintModifier()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNHINTMODIFIER_INIT, "_rtnHintModifier::init" )
   INT32 _rtnHintModifier::init( const BSONObj &hint, BOOLEAN copy )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNHINTMODIFIER_INIT ) ;

      try
      {
         BSONObjIterator iter( hint );
         _hint = copy ? hint.copy() : hint ;

         while ( iter.more() )
         {
            BSONElement elem = iter.next() ;
            if ( 0 == ossStrcmp( elem.fieldName(), FIELD_NAME_MODIFY ) )
            {
               // $Modify
               BSONObj modify = elem.Obj() ;
               BSONObjIterator modifyItr( modify ) ;
               while ( modifyItr.more() )
               {
                  BSONElement modifyEle = modifyItr.next() ;
                  if ( 0 == ossStrcmp( FIELD_NAME_OP, modifyEle.fieldName() ) )
                  {
                     if ( String != modifyEle.type() )
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG( PDERROR, "Field[%s] in hint should be a "
                                         "string[%d]", FIELD_NAME_OP, rc ) ;
                        goto error ;
                     }
                     if ( 0 == ossStrcasecmp( FIELD_OP_VALUE_UPDATE,
                                              modifyEle.valuestrsafe() ) )
                     {
                        _modifyOp = RTN_MODIFY_UPDATE ;
                     }
                     else if ( 0 == ossStrcasecmp( FIELD_OP_VALUE_REMOVE,
                                                   modifyEle.valuestrsafe() ) )
                     {
                        _modifyOp = RTN_MODIFY_REMOVE ;
                     }
                     else
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG( PDERROR, "Value[%s] for field[%s] is "
                                "invalid[%d]",
                                modifyEle.valuestrsafe(), FIELD_NAME_OP, rc ) ;
                        goto error ;
                     }
                  }
                  else if ( 0 == ossStrcmp( FIELD_NAME_OP_UPDATE,
                                            modifyEle.fieldName() ) )
                  {
                     if ( Object != modifyEle.type() )
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG( PDERROR, "Field[%s] in hint should be an "
                                "object[%d]", FIELD_NAME_OP_UPDATE, rc ) ;
                        goto error ;
                     }

                     _opOption = modifyEle.Obj() ;
                     if ( _opOption.isEmpty() )
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG( PDERROR, "Field[%s] in the hint can not be "
                                "empty[%d]", FIELD_NAME_OP_UPDATE, rc  ) ;
                     }
                  }
                  else if ( 0 == ossStrcmp( FIELD_NAME_OP_REMOVE,
                                            modifyEle.fieldName() ) )
                  {
                     if ( ( Bool != modifyEle.type() ) ||
                          ( TRUE != modifyEle.boolean() ) )
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG( PDERROR, "Field[%s] in hint is invalid[%d]",
                                FIELD_NAME_OP_REMOVE, rc ) ;
                        goto error ;
                     }
                  }
                  else if ( 0 == ossStrcmp( FIELD_NAME_UPDATE_SHARDING_KEY,
                                            modifyEle.fieldName() ) )
                  {
                     // Field 'UpdateShardingKey' is used internally to indicate
                     // that the updator is trying to change the sharding key.
                     // It's added to the hint by the coodinator, and used by
                     // the data node.
                     if ( Bool != modifyEle.type() )
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG( PDERROR, "Field[%s] in hint is invalid[%d]",
                                FIELD_NAME_UPDATE_SHARDING_KEY, rc ) ;
                        goto error ;
                     }
                     _updateShardingKey = modifyEle.boolean() ;
                  }
                  else
                  {
                     rc = _parseModifyEle( modifyEle ) ;
                     PD_RC_CHECK( rc, PDERROR, "Parse modify element[%s] in "
                                  "hint failed[%d]",
                                  modifyEle.toString().c_str(), rc ) ;
                  }
               }
            }
         }

         rc = _onInit() ;
         PD_RC_CHECK( rc, PDERROR, "Hint modifier init post action failed[%d]",
                      rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNHINTMODIFIER_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnHintModifier::hint( BSONObj &hint )
   {
      INT32 rc = SDB_OK ;

      try
      {
         if ( !_updateShardingKey )
         {
            // Nothing changed, just return the original hint.
            hint = _hint ;
            goto done ;
         }

         BSONObjBuilder hintBuilder ;
         BSONObjIterator itr( _hint ) ;
         BSONObjBuilder modifyBuilder(
            hintBuilder.subobjStart( FIELD_NAME_MODIFY ) ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_MODIFY ) )
            {
               // Other hints except the $Modify element.
               hintBuilder.append( ele ) ;
            }
            else
            {
               BSONObj modify = ele.Obj() ;
               modifyBuilder.appendElements( modify ) ;
               modifyBuilder.appendBool( FIELD_NAME_UPDATE_SHARDING_KEY,
                                         _updateShardingKey ) ;
               modifyBuilder.doneFast() ;
            }
         }

         _hint = hintBuilder.obj() ;
         hint = _hint ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
