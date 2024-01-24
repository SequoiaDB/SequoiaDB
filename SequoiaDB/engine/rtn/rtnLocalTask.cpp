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

   Source File Name = rtnLocalTask.cpp

   Descriptive Name = Local Task

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS Temporary Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2020  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnLocalTask.hpp"
#include "msgDef.hpp"

using namespace bson ;

namespace engine
{

   /*
      _rtnLTRename implement
   */
   _rtnLTRename::_rtnLTRename()
   {
      ossMemset( _from, 0, sizeof( _from ) ) ;
      ossMemset( _to, 0, sizeof( _to ) ) ;
      ossMemset( _time, 0, sizeof( _time ) ) ;
   }

   _rtnLTRename::~_rtnLTRename()
   {
   }

   void _rtnLTRename::setInfo( const CHAR *from, const CHAR *to )
   {
      ossStrncpy( _from, from, DMS_COLLECTION_FULL_NAME_SZ ) ;
      ossStrncpy( _to, to, DMS_COLLECTION_FULL_NAME_SZ ) ;

      ossTimestamp tm ;
      ossGetCurrentTime( tm ) ;

      /// make time
      ossTimestampToString( tm, _time ) ;
   }

   const CHAR* _rtnLTRename::getFrom() const
   {
      return _from ;
   }

   const CHAR* _rtnLTRename::getTo() const
   {
      return _to ;
   }

   INT32 _rtnLTRename::initFromBson( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjIterator itr( obj ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;

            if ( 0 == ossStrcmp( e.fieldName(), RTN_LT_FIELD_FROM ) )
            {
               if ( String != e.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                          RTN_LT_FIELD_FROM, obj.toString().c_str() ) ;
                  goto error ;
               }
               ossStrncpy( _from, e.valuestr(), DMS_COLLECTION_FULL_NAME_SZ ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), RTN_LT_FIELD_TO ) )
            {
               if ( String != e.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                          RTN_LT_FIELD_TO, obj.toString().c_str() ) ;
                  goto error ;
               }
               ossStrncpy( _to, e.valuestr(), DMS_COLLECTION_FULL_NAME_SZ ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), RTN_LT_FIELD_TIME ) )
            {
               if ( String != e.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                          RTN_LT_FIELD_TIME, obj.toString().c_str() ) ;
                  goto error ;
               }
               ossStrncpy( _time, e.valuestr(), OSS_TIMESTAMP_STRING_LEN ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( 0 == _from[0] || 0 == _to[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Not found field[%s] or field[%s] in obj[%s]",
                 RTN_LT_FIELD_FROM, RTN_LT_FIELD_TO,
                 obj.toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _rtnLTRename::muteXOn( const _rtnLocalTaskBase *pOther ) const
   {
      BOOLEAN muted = FALSE ;
      _rtnLTRename *pOtherRename = NULL ;

      /*
         RenameCS\RecycleCS\ReturnCS <-> RenameCS\RecycleCS\ReturnCS
         RenameCL\RecycleCL\ReturnCL <-> RenameCL\RecycleCL\ReturnCL
      */
      if ( _isSameLevel( pOther ) )
      {
         pOtherRename = ( _rtnLTRename* )pOther ;

         /*
            From  <->  From
            To    <->  To
            From  <->  To
            To    <->  From ignored by( == Reverse muteXOn From  <->  To )
         */
         if ( 0 == ossStrcmp( _from, pOtherRename->getFrom() ) ||
              0 == ossStrcmp( _to, pOtherRename->getTo() ) ||
              0 == ossStrcmp( _from, pOtherRename->getTo() ) )
         {
            muted = TRUE ;
         }
      }
      /*
         RenameCL\RecycleCL\ReturnCL  <->  RenameCS\RecycleCS\ReturnCS
         reverse is ignored by( == Reverse muteXOn )
      */
      else if ( RTN_LOCAL_TASK_RENAMECS == pOther->getTaskType() ||
                RTN_LOCAL_TASK_RECYCLECS == pOther->getTaskType() ||
                RTN_LOCAL_TASK_RETURNCS == pOther->getTaskType() )
      {
         const CHAR *pFromDot = ossStrchr( _from, '.' ) ;
         const CHAR *pToDot = ossStrchr( _to, '.' ) ;

         pOtherRename = ( _rtnLTRename* )pOther ;

         if ( pFromDot && pToDot )
         {
            UINT32 fromSize = pFromDot - _from ;
            UINT32 toSize = pToDot - _to ;
            const CHAR *otherFrom = pOtherRename->getFrom() ;
            const CHAR *otherTo = pOtherRename->getTo() ;

            /*
               From  <->  From
               To    <->  To
               From  <->  To
               To    <->  From
            */
            if ( ( 0 == ossStrncmp( _from, otherFrom, fromSize ) &&
                   0 == otherFrom[ fromSize ] ) ||
                 ( 0 == ossStrncmp( _to, otherTo, toSize ) &&
                   0 == otherTo[ toSize ] ) ||
                 ( 0 == ossStrncmp( _from, otherTo, fromSize ) &&
                   0 == otherTo[ fromSize ] ) ||
                 ( 0 == ossStrncmp( _to, otherFrom, toSize ) &&
                   0 == otherFrom[ toSize ] ) )
            {
               muted = TRUE ;
            }
         }
      }

      return muted ;
   }

   BOOLEAN _rtnLTRename::_isSameLevel( const _rtnLocalTaskBase *pOther ) const
   {
      BOOLEAN isSame = FALSE ;

      switch ( getTaskType() )
      {
         case RTN_LOCAL_TASK_RENAMECS :
         case RTN_LOCAL_TASK_RECYCLECS :
         case RTN_LOCAL_TASK_RETURNCS :
            isSame = ( RTN_LOCAL_TASK_RENAMECS == pOther->getTaskType() ||
                       RTN_LOCAL_TASK_RECYCLECS == pOther->getTaskType() ||
                       RTN_LOCAL_TASK_RETURNCS == pOther->getTaskType() ) ;
            break ;
         case RTN_LOCAL_TASK_RENAMECL :
         case RTN_LOCAL_TASK_RECYCLECL :
         case RTN_LOCAL_TASK_RETURNCL :
            isSame = ( RTN_LOCAL_TASK_RENAMECL == pOther->getTaskType() ||
                       RTN_LOCAL_TASK_RECYCLECL == pOther->getTaskType() ||
                       RTN_LOCAL_TASK_RETURNCL == pOther->getTaskType() ) ;
            break ;
         default :
            break ;
      }

      return isSame ;
   }

   INT32 _rtnLTRename::_toBson( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         builder.append( RTN_LT_FIELD_FROM, _from ) ;
         builder.append( RTN_LT_FIELD_TO, _to ) ;
         builder.append( RTN_LT_FIELD_TIME, _time ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON object for rename task, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _rtnLTRenameCS implement
   */
   RTN_IMPLEMENT_LT_AUTO_REGISTER( _rtnLTRenameCS ) ;
   _rtnLTRenameCS::_rtnLTRenameCS()
   {
   }

   _rtnLTRenameCS::~_rtnLTRenameCS()
   {
   }

   RTN_LOCAL_TASK_TYPE _rtnLTRenameCS::getTaskType() const
   {
      return RTN_LOCAL_TASK_RENAMECS ;
   }

   /*
      _rtnLTRenameCL implement
   */
   RTN_IMPLEMENT_LT_AUTO_REGISTER( _rtnLTRenameCL ) ;
   _rtnLTRenameCL::_rtnLTRenameCL()
   {
   }

   _rtnLTRenameCL::~_rtnLTRenameCL()
   {
   }

   RTN_LOCAL_TASK_TYPE _rtnLTRenameCL::getTaskType() const
   {
      return RTN_LOCAL_TASK_RENAMECL ;
   }

   /*
      _rtnLTRecycleBase implement
    */
   _rtnLTRecycleBase::_rtnLTRecycleBase()
   : _BASE()
   {
   }

   _rtnLTRecycleBase::~_rtnLTRecycleBase()
   {
   }

   void _rtnLTRecycleBase::setInfo( const CHAR *from,
                                    const CHAR *to,
                                    const utilRecycleItem &recycleItem )
   {
      _BASE::setInfo( from, to ) ;
      setRecycleItem( recycleItem ) ;
   }

   INT32 _rtnLTRecycleBase::initFromBson( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      rc = _BASE::initFromBson( obj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize rename task from BSON, "
                   "rc: %d", rc ) ;

      rc = _recycleItem.fromBSON( obj, FIELD_NAME_RECYCLE_ITEM ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item from BSON, "
                   "rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _rtnLTRecycleBase::_toBson( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      rc = _BASE::_toBson( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for rename task, "
                   "rc: %d", rc ) ;

      rc = _recycleItem.toBSON( builder, FIELD_NAME_RECYCLE_ITEM ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for recycle item, "
                   "rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _rtnLTRecycleCS implement
    */
   RTN_IMPLEMENT_LT_AUTO_REGISTER( _rtnLTRecycleCS ) ;

   /*
      _rtnLTRecycleCL implement
    */
   RTN_IMPLEMENT_LT_AUTO_REGISTER( _rtnLTRecycleCL ) ;

   /*
      _rtnLTReturnCS implement
    */
   RTN_IMPLEMENT_LT_AUTO_REGISTER( _rtnLTReturnCS ) ;

   /*
      _rtnLTReturnCL implement
    */
   RTN_IMPLEMENT_LT_AUTO_REGISTER( _rtnLTReturnCL ) ;

}
