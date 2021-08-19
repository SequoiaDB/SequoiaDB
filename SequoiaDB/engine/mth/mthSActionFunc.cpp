/******************************************************************************

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

   Source File Name = mthSActionFunc.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthSActionFunc.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "mthSAction.hpp"
#include "mthElemMatchIterator.hpp"
#include "utilString.hpp"
#include "utilStr.hpp"
#include "../util/fromjson.hpp"


using namespace bson ;

namespace engine
{
   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHINCLUDEBUILD, "mthIncludeBuild" )
   INT32 mthIncludeBuild( const CHAR *fieldName,
                          const bson::BSONElement &e,
                          _mthSAction *action,
                          bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHINCLUDEBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      if ( !e.eoo() )
      {
         builder.append( e ) ;
      }
      PD_TRACE_EXITRC( SDB__MTHINCLUDEBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHINCLUDEGET, "mthIncludeGet" )
   INT32 mthIncludeGet( const CHAR *fieldName,
                        const bson::BSONElement &in,
                        _mthSAction *action,
                        bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHINCLUDEGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      out = in ;
      PD_TRACE_EXITRC( SDB__MTHINCLUDEGET, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDEFAULTBUILD, "mthDefaultBuild" )
   INT32 mthDefaultBuild( const CHAR *fieldName,
                          const bson::BSONElement &e,
                          _mthSAction *action,
                          bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDEFAULTBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      if ( e.eoo() )
      {
         builder.appendAs( action->getValue(), fieldName ) ;
      }
      else
      {
         builder.append( e ) ;
      }
      PD_TRACE_EXITRC( SDB__MTHDEFAULTBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDEFAULTGET, "mthDefaultGet" )
   INT32 mthDefaultGet( const CHAR *fieldName,
                        const bson::BSONElement &in,
                        _mthSAction *action,
                        bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDEFAULTGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      if ( !in.eoo() )
      {
         out = in ;
         goto done ;
      }

      if ( action->getObj().isEmpty() )
      {
         bson::BSONObjBuilder builder ;
         builder.appendAs( action->getValue(), fieldName ) ;
         bson::BSONObj obj = builder.obj() ;
         action->setObj( obj ) ;
         action->setValue( obj.getField( fieldName ) ) ;
      }

      out = action->getValue() ;
   done:
      PD_TRACE_EXITRC( SDB__MTHDEFAULTGET, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSLICEBUILD, "mthSliceBuild" )
   INT32 mthSliceBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSLICEBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      BSONObj args = action->getArg() ;

      INT32 begin = args.getIntField( "arg1" ) ;
      INT32 limit = args.getIntField( "arg2" ) ;
      rc = mthSlice( fieldName, e, begin, limit, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthSlice failed:rc=%d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHSLICEBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSLICEGET, "mthSliceGet" )
   INT32 mthSliceGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSLICEGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      BSONObj args = action->getArg() ;
      INT32 begin = args.getIntField( "arg1" ) ;
      INT32 limit = args.getIntField( "arg2" ) ;
      rc = mthSlice( fieldName, in, begin, limit, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthSlice failed:rc=%d", rc ) ;

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSLICEGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHBUILDN, "mthElemMatchBuildN" )
   static INT32 mthElemMatchBuildN( const CHAR *fieldName,
                                    const bson::BSONElement &e,
                                    _mthSAction *action,
                                    bson::BSONObjBuilder &builder,
                                    INT32 n )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHBUILDN ) ;

      PD_CHECK( NULL != action->getMatchTree(), SDB_SYS, error, PDERROR,
                "Failed to get match tree" ) ;

      if ( Array == e.type() )
      {
         BSONArrayBuilder arrayBuilder( builder.subarrayStart( fieldName ) ) ;
         _mthElemMatchIterator i( e.embeddedObject(),
                                  action->getMatchTree(),
                                  n ) ;
         do
         {
            BSONElement next ;
            rc = i.next( next ) ;
            if ( SDB_OK == rc )
            {
               arrayBuilder.append( next ) ;
            }
            else if ( SDB_DMS_EOC == rc )
            {
               arrayBuilder.doneFast() ;
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG( PDERROR, "failed to get next element:%d", rc ) ;
               goto error ;
            }
         } while ( TRUE ) ;
      }
      else if ( Object == e.type() )
      {
         _mthElemMatchIterator i( e.embeddedObject(), action->getMatchTree(),
                                  n, FALSE ) ;
         do
         {
            BSONElement next ;
            rc = i.next( next ) ;
            if ( SDB_OK == rc )
            {
               builder.append( fieldName, next.wrap() ) ;
            }
            else if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG( PDERROR, "failed to get next element:%d", rc ) ;
               goto error ;
            }
         } while ( TRUE ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHBUILDN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHGETN, "mthElemMatchGetN" )
   static INT32 mthElemMatchGetN( const CHAR *fieldName,
                                  const bson::BSONElement &in,
                                  _mthSAction *action,
                                  bson::BSONElement &out,
                                  INT32 n )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHGETN ) ;

      PD_CHECK( NULL != action->getMatchTree(), SDB_SYS, error, PDERROR,
                "Failed to get match tree" ) ;

      if ( Array == in.type() )
      {
         BSONObjBuilder objBuilder ;
         BSONArrayBuilder arrayBuilder( objBuilder.subarrayStart( fieldName ) ) ;
         _mthElemMatchIterator i( in.embeddedObject(),
                                  action->getMatchTree(),
                                  n ) ;
         do
         {
            BSONElement next ;
            rc = i.next( next ) ;
            if ( SDB_OK == rc )
            {
               arrayBuilder.append( next ) ;
            }
            else if ( SDB_DMS_EOC == rc )
            {
               arrayBuilder.doneFast() ;
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG( PDERROR, "failed to get next element:%d", rc ) ;
               goto error ;
            }
         } while ( TRUE ) ;

         action->setObj( objBuilder.obj() ) ;
         out = action->getObj().getField( fieldName ) ;
      }
      else
      {
         out = BSONElement() ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHGETN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHBUILD, "mthElemMatchBuild" )
   INT32 mthElemMatchBuild( const CHAR *fieldName,
                            const bson::BSONElement &e,
                            _mthSAction *action,
                            bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHBUILD ) ;
      rc = mthElemMatchBuildN( fieldName, e, action, builder, -1 ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHGET, "mthElemMatchGet" )
   INT32 mthElemMatchGet( const CHAR *fieldName,
                          const bson::BSONElement &in,
                          _mthSAction *action,
                          bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHGET ) ;
      rc = mthElemMatchGetN( fieldName, in, action, out, -1 ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHGET, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHONEBUILD, "mthElemMatchOneBuild" )
   INT32 mthElemMatchOneBuild( const CHAR *fieldName,
                               const bson::BSONElement &e,
                               _mthSAction *action,
                               bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHONEBUILD ) ;
      rc = mthElemMatchBuildN( fieldName, e, action, builder, 1 ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHONEBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHONEGET, "mthElemMatchOneGet" )
   INT32 mthElemMatchOneGet( const CHAR *fieldName,
                             const bson::BSONElement &in,
                             _mthSAction *action,
                             bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHONEGET ) ;
      rc = mthElemMatchGetN( fieldName, in, action, out, 1 ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHONEGET, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHABSBUILD, "mthAbsBuild" )
   INT32 mthAbsBuild( const CHAR *fieldName,
                      const bson::BSONElement &e,
                      _mthSAction *action,
                      bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHABSBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;

      rc = mthAbs( fieldName, e, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthAbs failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, abs(%lld), rc = %d",
                 fieldName, e.numberLong(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHABSBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHABSGET, "mthAbsGet" )
   INT32 mthAbsGet( const CHAR *fieldName,
                    const bson::BSONElement &in,
                    _mthSAction *action,
                    bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHABSGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      rc = mthAbs( fieldName, in, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthAbs failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, abs(%lld), rc = %d",
                 fieldName, in.numberLong(), rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHABSGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCEILINGBUILD, "mthCeilingBuild" )
   INT32 mthCeilingBuild( const CHAR *fieldName,
                          const bson::BSONElement &e,
                          _mthSAction *action,
                          bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCEILINGBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      rc = mthCeiling( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthCeiling failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHCEILINGBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCEILINGGET, "mthCeilingGet" )
   INT32 mthCeilingGet( const CHAR *fieldName,
                        const bson::BSONElement &in,
                        _mthSAction *action,
                        bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCEILINGGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      rc = mthCeiling( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthCeiling failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHCEILINGGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHFLOORBUILD, "mthFloorBuild" )
   INT32 mthFloorBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHFLOORBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      rc = mthFloor( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthFloor failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHFLOORBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHFLOORGET, "mthFloorGet" )
   INT32 mthFloorGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHFLOORGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      rc = mthFloor( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthFloor failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHFLOORGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMODBUILD, "mthModBuild" )
   INT32 mthModBuild( const CHAR *fieldName,
                      const bson::BSONElement &e,
                      _mthSAction *action,
                      bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMODBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;

      rc = mthMod( fieldName, e, arg, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMod failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHMODBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

    ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMODGET, "mthModGet" )
   INT32 mthModGet( const CHAR *fieldName,
                    const bson::BSONElement &in,
                    _mthSAction *action,
                    bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMODBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      const BSONObj &arg = action->getArg() ;
      BSONElement argEle = arg.getField( "arg1" ) ;

      rc = mthMod( fieldName, in, argEle, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMod failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHMODBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCASTBUILD, "mthCastBuild" )
   INT32 mthCastBuild( const CHAR *fieldName,
                       const bson::BSONElement &e,
                       _mthSAction *action,
                       bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCASTBUILD ) ;
      BSONElement arg ;
      BSONType type = EOO ;

      arg = action->getArg().getField( "arg1" ) ;
      if ( !arg.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid arg element:%s",
                 arg.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }

      type = ( BSONType )( arg.numberInt() ) ;
      rc = mthCast( fieldName, e, type, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthCast failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHCASTBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCASTGET, "mthCastGet" )
   INT32 mthCastGet( const CHAR *fieldName,
                     const bson::BSONElement &in,
                     _mthSAction *action,
                     bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCASTGET ) ;
      BSONElement arg ;
      BSONType type = EOO ;
      BSONObjBuilder builder ;

      if ( in.eoo() )
      {
         goto done ;
      }

      arg = action->getArg().getField( "arg1" ) ;
      if ( !arg.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid arg element:%s",
                 arg.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }

      type = ( BSONType )( arg.numberInt() ) ;
      if ( in.type() == type )
      {
         out = in ;
         goto done ;
      }
      else
      {
         rc = mthCast( fieldName, in, type, builder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to cast element[%s] to"
                    " type[%d]", in.toString().c_str(), type ) ;
            goto error ;

         }

         action->setObj( builder.obj() ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHCASTGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRBUILD, "mthSubStrBuild" )
   INT32 mthSubStrBuild( const CHAR *fieldName,
                         const bson::BSONElement &e,
                         _mthSAction *action,
                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;

      begin = action->getArg().getIntField( "arg1" ) ;
      limit = action->getArg().getIntField( "arg2" ) ;
      rc = mthSubStr( fieldName, e, begin, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSubStr failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBSTRBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRGET, "mthSubStrGet" )
   INT32 mthSubStrGet( const CHAR *fieldName,
                       const bson::BSONElement &in,
                       _mthSAction *action,
                       bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;

      begin = action->getArg().getIntField( "arg1" ) ;
      limit = action->getArg().getIntField( "arg2" ) ;
      rc = mthSubStr( fieldName, in, begin, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSubStr failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBSTRGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENBUILD, "mthStrLenBuild" )
   INT32 mthStrLenBuild( const CHAR *fieldName,
                         const bson::BSONElement &e,
                         _mthSAction *action,
                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSTRLENBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      rc = mthStrLen( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthStrLen failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENGET, "mthStrLenGet" )
   INT32 mthStrLenGet( const CHAR *fieldName,
                       const bson::BSONElement &in,
                       _mthSAction *action,
                       bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSTRLENGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      if ( in.eoo() )
      {
         goto done ;
      }

      rc = mthStrLen( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthStrLen failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLOWERBUILD, "mthLowerBuild" )
   INT32 mthLowerBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLOWERBUILD ) ;
      if ( e.eoo() )
      {
         goto done ;
      }

      rc = mthLower( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthLower failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHLOWERBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLOWERGET, "mthLowerGet" )
   INT32 mthLowerGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLOWERGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      if ( in.eoo() )
      {
         goto done ;
      }

      rc = mthLower( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthLower failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHLOWERGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHUPPERBUILD, "mthUpperBuild" )
   INT32 mthUpperBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHUPPERBUILD ) ;
      if ( e.eoo() )
      {
         goto done ;
      }

      rc = mthUpper( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthUpper failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHUPPERBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHUPPERGET, "mthUpperGet" )
   INT32 mthUpperGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHUPPERGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      if ( in.eoo() )
      {
         goto done ;
      }

      rc = mthUpper( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthUpper failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHUPPERGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLRTRIMBUILD, "mthLRTrimBuild" )
   static INT32 mthLRTrimBuild( const CHAR *fieldName,
                                const bson::BSONElement &e,
                                _mthSAction *action,
                                INT8 lr,
                                bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLRTRIMBUILD ) ;

      rc = mthTrim( fieldName, e, lr, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthTrim failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHLRTRIMBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLRTRIMGET, "mthLRTrimGet" )
   static INT32 mthLRTrimGet( const CHAR *fieldName,
                              const bson::BSONElement &in,
                              _mthSAction *action,
                              INT8 lr,
                              bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLRTRIMGET ) ;
      BSONObjBuilder builder ;

      rc = mthTrim( fieldName, in, lr, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthTrim failed:rc=%d", rc ) ;
         goto error ;
      }

      action->setObj( builder.obj() ) ;
      out = action->getObj().getField( fieldName ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHLRTRIMGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTRIMBUILD, "mthTrimBuild" )
   INT32 mthTrimBuild( const CHAR *fieldName,
                       const bson::BSONElement &e,
                       _mthSAction *action,
                       bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHTRIMBUILD ) ;
      rc = mthLRTrimBuild( fieldName, e, action, 0, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHTRIMBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTRIMGET, "mthTrimGet" )
   INT32 mthTrimGet( const CHAR *fieldName,
                     const bson::BSONElement &in,
                     _mthSAction *action,
                     bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHTRIMGET ) ;
      rc = mthLRTrimGet( fieldName, in, action, 0, out ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHTRIMGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLTRIMBUILD, "mthLTrimBuild" )
   INT32 mthLTrimBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLTRIMBUILD ) ;
      rc = mthLRTrimBuild( fieldName, e, action, -1, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHLTRIMBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLTRIMGET, "mthLTrimGet" )
   INT32 mthLTrimGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLTRIMGET ) ;
      rc = mthLRTrimGet( fieldName, in, action, -1, out ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHLTRIMGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRTRIMBUILD, "mthRTrimBuild" )
   INT32 mthRTrimBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHRTRIMBUILD ) ;
      rc = mthLRTrimBuild( fieldName, e, action, 1, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHRTRIMBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRTRIMGET, "mthRTrimGet" )
   INT32 mthRTrimGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHRTRIMGET ) ;
      rc = mthLRTrimGet( fieldName, in, action, 1, out ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHRTRIMGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHADDBUILD, "mthAddBuild" )
   INT32 mthAddBuild( const CHAR *fieldName,
                      const bson::BSONElement &e,
                      _mthSAction *action,
                      bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHADDBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthAdd( fieldName, e, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthAdd failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld + %lld), rc = %d",
                 fieldName, e.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHADDBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHADDGET, "mthAddGet" )
   INT32 mthAddGet( const CHAR *fieldName,
                    const bson::BSONElement &in,
                    _mthSAction *action,
                    bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHADDGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthAdd( fieldName, in, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthAdd failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld + %lld), rc = %d",
                 fieldName, in.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHADDGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBTRACTBUILD, "mthSubtractBuild" )
   INT32 mthSubtractBuild( const CHAR *fieldName,
                           const bson::BSONElement &e,
                           _mthSAction *action,
                           bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHSUBTRACTBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthSub( fieldName, e, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSub failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld - %lld), rc = %d",
                 fieldName, e.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBTRACTBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBTRACTGET, "mthSubtractGet" )
   INT32 mthSubtractGet( const CHAR *fieldName,
                         const bson::BSONElement &in,
                         _mthSAction *action,
                         bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHSUBTRACTGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthSub( fieldName, in, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSub failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld - %lld), rc = %d",
                 fieldName, in.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBTRACTGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMULTIPLYBUILD, "mthMultiplyBuild" )
   INT32 mthMultiplyBuild( const CHAR *fieldName,
                           const bson::BSONElement &e,
                           _mthSAction *action,
                           bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHMULTIPLYBUILD ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthMultiply( fieldName, e, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMultiply failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld * %lld), rc = %d",
                 fieldName, e.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHMULTIPLYBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMULTIPLYGET, "mthMultiplyGet" )
   INT32 mthMultiplyGet( const CHAR *fieldName,
                         const bson::BSONElement &in,
                         _mthSAction *action,
                         bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHMULTIPLYGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthMultiply( fieldName, in, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMultiply failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld * %lld), rc = %d",
                 fieldName, in.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHMULTIPLYGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDIVIDEBUILD, "mthDivideBuild" )
   INT32 mthDivideBuild( const CHAR *fieldName,
                         const bson::BSONElement &e,
                         _mthSAction *action,
                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHDIVIDEBUILD ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthDivide( fieldName, e, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthDivide failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld / %lld), rc = %d",
                 fieldName, e.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHDIVIDEBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDIVIDEGET, "mthDivideGet" )
   INT32 mthDivideGet( const CHAR *fieldName,
                       const bson::BSONElement &in,
                       _mthSAction *action,
                       bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHDIVIDEGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthDivide( fieldName, in, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthDivide failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld / %lld), rc = %d",
                 fieldName, in.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHDIVIDEGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSIZEBUILD, "mthSizeBuild" )
   INT32 mthSizeBuild( const CHAR *fieldName, const bson::BSONElement &e,
                       _mthSAction *action, bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSIZEBUILD ) ;

      rc = mthSize( fieldName, e, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthSize failed:rc=%d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHSIZEBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSIZEGET, "mthSizeGet" )
   INT32 mthSizeGet( const CHAR *fieldName, const bson::BSONElement &in,
                     _mthSAction *action, bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSIZEGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      rc = mthSize( fieldName, in, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthSize failed:rc=%d", rc ) ;

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSIZEGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTYPEBUILD, "mthTypeBuild" )
   INT32 mthTypeBuild( const CHAR *fieldName, const bson::BSONElement &e,
                       _mthSAction *action, bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHTYPEBUILD ) ;
      INT32 resultType = 1 ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      resultType = arg.numberInt() ;

      rc = mthType( fieldName, resultType, e, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthType failed:rc=%d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHTYPEBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTYPEGET, "mthTypeGet" )
   INT32 mthTypeGet( const CHAR *fieldName, const bson::BSONElement &in,
                     _mthSAction *action, bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHTYPEGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      INT32 resultType = 1 ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      resultType = arg.numberInt() ;

      rc = mthType( fieldName, resultType, in, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthType failed:rc=%d", rc ) ;

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHTYPEGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}


