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

   Source File Name = coordShardKicker.cpp

   Descriptive Name = Runtime Coord Operator

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   general operations on coordniator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordKeyKicker.hpp"
#include "pmdEDU.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordKeyKicker implement
   */
   _coordKeyKicker::_coordKeyKicker()
   {
      _pResource = NULL ;
   }

   _coordKeyKicker::~_coordKeyKicker()
   {
   }

   void _coordKeyKicker::bind( coordResource *pResource,
                               const CoordCataInfoPtr &cataPtr )
   {
      _pResource = pResource ;
      _cataPtr = cataPtr ;
   }

   BOOLEAN _coordKeyKicker::_isUpdateReplace( const BSONObj &updator )
   {
      //INT32 rc = SDB_OK ;
      BSONObjIterator iter( updator ) ;
      while ( iter.more() )
      {
         BSONElement beTmp = iter.next() ;
         if ( 0 == ossStrcmp( beTmp.fieldName(),
                              CMD_ADMIN_PREFIX FIELD_OP_VALUE_REPLACE ) )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   // eUpdate: one element of updator is sharding key
   // matcher: condition
   // if any element of matcher not equal eUpdate, that means sharding key change
   BOOLEAN _coordKeyKicker::_isShardingKeyChange( const BSONElement &eUpdate,
                                                  const BSONObj &matcher )
   {
      BOOLEAN change = FALSE ;
      BSONElement eMatcher = matcher.getField( "$and" ) ;

      if( Array == eMatcher.type() )
      {
         //update({$set:{a:1,c:2},{$and:[{a:1},{b:2}]},{},{KeepShardingKey:true})
         BSONObjIterator ele( eMatcher.embeddedObject() ) ;
         while ( ele.more() )
         {
            BSONElement eArray = ele.next() ;
            // eArray will be "0":{a:1} "1":{b:2}
            if( Object != eArray.type() )
            {
               change = TRUE ;
               goto done ;
            }
            change = _isShardingKeyChange( eUpdate, eArray.embeddedObject() ) ;
            // operator is $and so if one element of matcher equal eUpdate,then return FALSE
            if( FALSE == change )
            {
               goto done ;
            }
         }
      }

      // if $and matcher failure, continue to process other element
      change = FALSE ;

      eMatcher = matcher.getField( eUpdate.fieldName() ) ;
      if( EOO == eMatcher.type() )
      {
         // shardingKey field eUpdate not exist in matcher,set change TRUE
         change = TRUE ;
         goto done ;
      }

      // if the format of the shard key nested field is not consistent
      // with the format of matcher nested field , we not recoginze
      // update(({$set:{"a.b":10},c:1},{a:{b:10}},{},{KeepShardingKey:true}) errno -178
      // update(({$set:{a:{b:10}},c:1},{a:{b:10}},{},{KeepShardingKey:true}) update success
      if( Object == eMatcher.type() )
      {
         BSONObj obj = eMatcher.embeddedObject() ;

         // eMatcher "a":{$et:2}, obj {$et:2}
         if( 1 == obj.nFields() &&
             0 == ossStrcmp( obj.firstElement().fieldName(), "$et" ) )
         {
            // set eMatcher = $et:2
            eMatcher = obj.firstElement();
         }
      }

      // compare update element with matcher element, exclude field name
      if( 0 != eUpdate.woCompare( eMatcher, FALSE ) )
      {
         // eUpdate exist in matcher, but not equal with eMatcher
         change =  TRUE ;
         goto done ;
      }

   done:
       return change ;
   }


   UINT32 _coordKeyKicker::_addKeys( const BSONObj &objKey )
   {
      UINT32 count = 0 ;
      BSONObjIterator itr( objKey ) ;
      while( itr.more() )
      {
         BSONElement e = itr.next() ;
         if ( _setKeys.count( e.fieldName() ) > 0 )
         {
            continue ;
         }
         ++count ;
         _setKeys.insert( e.fieldName() ) ;
      }
      return count ;
   }

   /// kick sharding key and auto-increment key of a cl
   /// we kick sharding key anyway, and when doing replace,
   /// if new obj has no auto-increment key, we keep the old key.
   INT32 _coordKeyKicker::_kickKey( const CoordCataInfoPtr &cataInfo,
                                    const BSONObj &updator,
                                    BSONObj &newUpdator,
                                    const BSONObj &matcher,
                                    BOOLEAN &shardingKeyChanged,
                                    BOOLEAN &hasKeepAutoInc,
                                    BOOLEAN ignoreAutoInc )
   {
      INT32 rc = SDB_OK ;
      UINT32 skSiteID = cataInfo->getShardingKeySiteID() ;
      ossPoolSet< strContainner > doneFields ;

      if ( skSiteID > 0 )
      {
         /// if is the same sharding key
         SiteIDSet::iterator it = _skSiteIDs.find( skSiteID );
         if ( it != _skSiteIDs.end() && ignoreAutoInc )
         {
            newUpdator = updator ;
            shardingKeyChanged = it->second ;
            hasKeepAutoInc = FALSE ;
            goto done ;
         }
      }

      try
      {
         BSONObjBuilder bobNewUpdator( updator.objsize() ) ;
         BSONObj boShardingKey ;
         const clsAutoIncSet &autoIncSet = cataInfo->getAutoIncSet() ;
         BSONObj subObj ;

         BOOLEAN isReplace = _isUpdateReplace( updator ) ;
         cataInfo->getShardingKey( boShardingKey ) ;

         BSONObjIterator iter( updator ) ;
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next() ;
            if ( beTmp.type() != Object )
            {
               rc = SDB_INVALIDARG;
               PD_LOG( PDERROR, "updator's element must be an Object type:"
                       "updator=%s", updator.toString().c_str() ) ;
               goto error ;
            }

            subObj = beTmp.embeddedObject() ;
            //if replace. leave the keep
            if ( isReplace &&
                 0 == ossStrcmp( beTmp.fieldName(),
                                 CMD_ADMIN_PREFIX FIELD_OP_VALUE_KEEP ) )
            {
               _addKeys( subObj ) ;
               continue ;
            }

            BSONObjBuilder subBuilder( bobNewUpdator.subobjStart(
                                       beTmp.fieldName() ) ) ;
            BSONObjIterator iterField( subObj ) ;
            while( iterField.more() )
            {
               BSONElement beField = iterField.next() ;
               const CHAR *pField = beField.fieldName() ;

               if ( _isKey( pField, boShardingKey ) )
               {
                  if( 0 == ossStrcmp( beTmp.fieldName(),
                                      CMD_ADMIN_PREFIX FIELD_OP_VALUE_REPLACE ) )
                  {
                     // rule is replace, shardingKeyChanged will be juded in later flow
                     if( 0 == _setKeys.count( pField ) &&
                         !_isShardingKeyChange( beField, matcher ) )
                     {
                        _setKeys.insert( pField ) ;
                     }
                  }
                  else if( 0 == ossStrcmp( beTmp.fieldName(),
                                           CMD_ADMIN_PREFIX FIELD_OP_VALUE_SET ) )
                  {
                     if( FALSE == shardingKeyChanged &&
                         _isShardingKeyChange( beField, matcher ) )
                     {
                        shardingKeyChanged = TRUE ;
                     }
                  }
                  else
                  {
                     // other rule like $unset $inc, then set shardingKeyChanged TRUE
                     shardingKeyChanged = TRUE ;
                  }
               }
               else if ( 0 == ossStrcmp( beTmp.fieldName(),
                                         CMD_ADMIN_PREFIX CMD_VALUE_NAME_RENAME ) )
               {
                  // If $rename target value is shardingKey, do nothing
                  if ( _isKey( beField.valuestrsafe(), boShardingKey ) )
                  {
                     shardingKeyChanged = TRUE ;
                  }
                  else
                  {
                     subBuilder.append( beField ) ;
                  }
               }
               else
               {
                  subBuilder.append( beField ) ;
               }

               if ( !ignoreAutoInc && isReplace &&
                    NULL != autoIncSet.findItem( pField ) )
               {
                  doneFields.insert( pField ) ;
               }
            } // while( iterField.more() )

            subBuilder.done() ;
         } // while ( iter.more() )

         if ( isReplace )
         {
            //generate new $keep by combining boUpdator.$keep & boShardingKey.
            UINT32 count = _addKeys( boShardingKey ) ;
            if ( count > 0 )
            {
               // exist boShardingKey field not equal boReplace and boKeeper
               shardingKeyChanged = TRUE ;
            }

            if ( !ignoreAutoInc )
            {
               clsAutoIncIterator autoIncIt( autoIncSet );
               while ( autoIncIt.more() )
               {
                  const clsAutoIncItem *pItem = autoIncIt.next();
                  if ( doneFields.count( pItem->fieldName() ) == 0 )
                  {
                     _setKeys.insert( pItem->fieldName() );
                     hasKeepAutoInc = TRUE;
                  }
               }
            }

            if ( !_setKeys.empty() )
            {
               BSONObjBuilder keepBuilder( bobNewUpdator.subobjStart(
                                           CMD_ADMIN_PREFIX FIELD_OP_VALUE_KEEP ) ) ;
               SET_KEEPKEY::iterator itKey = _setKeys.begin() ;
               while( itKey != _setKeys.end() )
               {
                  keepBuilder.append( (*itKey)._pStr, (INT32)1 ) ;
                  ++itKey ;
               }
               keepBuilder.done() ;
            }
         } // if ( isReplace )
         newUpdator = bobNewUpdator.obj() ;

         if ( skSiteID > 0 )
         {
            _skSiteIDs.insert(
                       pair< UINT32, BOOLEAN >( skSiteID, shardingKeyChanged ) ) ;
         }

      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR,"Failed to kick sharding key from the record,"
                  "occured unexpected error: %s", e.what() ) ;

         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   /// kick sharding key and auto-increment key of cl
   INT32 _coordKeyKicker::kickKey( const BSONObj &updator,
                                   BSONObj &newUpdator,
                                   BOOLEAN &isChanged,
                                   pmdEDUCB *cb,
                                   const BSONObj &matcher,
                                   BOOLEAN keepShardingKey )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN shardingKeyChanged = FALSE ;
      BOOLEAN hasKeepAutoInc = FALSE ;

      if ( !_cataPtr.get() || (!_cataPtr->isSharded() &&
                               !_cataPtr->hasAutoIncrement()) )
      {
         newUpdator = updator ;
         goto done ;
      }

      /// clear
      _skSiteIDs.clear() ;
      _setKeys.clear() ;

      rc = _kickKey( _cataPtr, updator, newUpdator,
                     matcher, shardingKeyChanged, hasKeepAutoInc, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Kick sharding key failed, rc: %d", rc ) ;
         goto error ;
      }
      if ( keepShardingKey )
      {
         if ( shardingKeyChanged )
         {
            rc = SDB_UPDATE_SHARD_KEY ;
            PD_LOG( PDERROR, "Sharding key cannot be updated, rc: %d", rc ) ;
            goto error ;
         }
         // can't set isChanged to FALSE, in case it is TRUE
         // when passing to this function.
      }
      else
      {
         if ( shardingKeyChanged )
         {
            isChanged = TRUE ;
         }
      }
      if ( hasKeepAutoInc )
      {
         isChanged = TRUE ;
      }

      /// When is main collection, need to kick all sub-collection's
      /// sharding key
      if ( _cataPtr->isMainCL() )
      {
         CLS_SUBCL_LIST subCLLst ;
         CLS_SUBCL_LIST_IT iterCL ;
         BSONObj subUpdator = newUpdator ;

         rc = _cataPtr->getMatchSubCLs( matcher, subCLLst ) ;
         if ( SDB_CAT_NO_MATCH_CATALOG == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get matched sub collections failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         iterCL = subCLLst.begin() ;
         while( iterCL != subCLLst.end() )
         {
            rc = _kickShardingKey( *iterCL, subUpdator, newUpdator,
                                   matcher, isChanged, cb, keepShardingKey ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Kick sharding key for sub-collection[%s] "
                       "failed, rc: %d", (*iterCL).c_str(), rc ) ;
               goto error ;
            }
            subUpdator = newUpdator ;
            ++iterCL ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /// kick sharding key of subcl
   INT32 _coordKeyKicker::_kickShardingKey( const string &collectionName,
                                            const BSONObj &updator,
                                            BSONObj &newUpdator,
                                            const BSONObj &matcher,
                                            BOOLEAN &isChanged,
                                            pmdEDUCB *cb,
                                            BOOLEAN keepShardingKey )
   {
      INT32 rc = SDB_OK ;
      CoordCataInfoPtr cataPtr ;
      BOOLEAN shardingKeyChanged = FALSE ;
      BOOLEAN hasKeepAutoInc = FALSE ;

      rc = _pResource->getOrUpdateCataInfo( collectionName.c_str(),
                                            cataPtr,
                                            cb ) ;
      if ( SDB_CLS_COORD_NODE_CAT_VER_OLD == rc )
      {
         /// When the main-collection is old, ignored
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Update collection[%s]'s catalog info failed, "
                 "rc: %d", collectionName.c_str(), rc ) ;
         goto error ;
      }

      /// when subcl isn't partition cl
      if ( !cataPtr->isSharded() )
      {
         goto done ;
      }

      rc = _kickKey( cataPtr, updator, newUpdator,
                     matcher, shardingKeyChanged, hasKeepAutoInc, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( keepShardingKey )
      {
         if ( 1 != cataPtr->getGroupNum() && shardingKeyChanged )
         {
            // num >= 2, cl which has been splited, not allow sharding key
            rc = SDB_UPDATE_SHARD_KEY ;
            PD_LOG( PDERROR, "When the partition cl falls on two or "
                    "more groups, the update rule don't allow sharding"
                    " key. rc: %d", rc ) ;
            goto error ;
         }
         // can't set isChanged to FALSE, in case it is TRUE
         // when passing to this function.
      }
      else
      {
         if ( shardingKeyChanged )
         {
            isChanged = TRUE ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordKeyKicker::checkShardingKey( const BSONObj &updator,
                                            BOOLEAN &hasInclude,
                                            _pmdEDUCB *cb,
                                            const BSONObj &matcher )
   {
      INT32 rc = SDB_OK ;

      if ( !_cataPtr.get() || !_cataPtr->isSharded() )
      {
         goto done ;
      }

      /// clear
      _skSiteIDs.clear() ;
      _setKeys.clear() ;

      rc = _checkShardingKey( _cataPtr, updator, hasInclude ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Check sharding key failed, rc: %d", rc ) ;
         goto error ;
      }
      if ( hasInclude )
      {
         goto done ;
      }

      /// When is main collection, need to check all sub-collection's
      /// sharding key
      if ( _cataPtr->isMainCL() )
      {
         CLS_SUBCL_LIST subCLLst ;
         CLS_SUBCL_LIST_IT iterCL ;

         rc = _cataPtr->getMatchSubCLs( matcher, subCLLst ) ;
         if ( SDB_CAT_NO_MATCH_CATALOG == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get matched sub collections failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         iterCL = subCLLst.begin() ;
         while( iterCL != subCLLst.end() )
         {
            rc = _checkShardingKey( *iterCL, updator, hasInclude, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Check sharding key for sub-collection[%s] "
                       "failed, rc: %d", (*iterCL).c_str(), rc ) ;
               goto error ;
            }
            if ( hasInclude )
            {
               goto done ;
            }
            ++iterCL ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordKeyKicker::_checkShardingKey( const CoordCataInfoPtr &cataInfo,
                                             const BSONObj &updator,
                                             BOOLEAN &hasInclude )
   {
      INT32 rc = SDB_OK ;
      UINT32 skSiteID = cataInfo->getShardingKeySiteID() ;

      if ( skSiteID > 0 )
      {
         /// if is the same sharding key
         if ( _skSiteIDs.count( skSiteID ) > 0 )
         {
            goto done ;
         }
         _skSiteIDs.insert( pair< UINT32, BOOLEAN >( skSiteID, TRUE ) ) ;
      }

      try
      {
         BSONObjBuilder bobNewUpdator( updator.objsize() ) ;
         BSONObj boShardingKey ;
         BSONObj subObj ;

         BOOLEAN isReplace = _isUpdateReplace( updator ) ;
         cataInfo->getShardingKey( boShardingKey ) ;

         BSONObjIterator iter( updator ) ;
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next() ;
            if ( beTmp.type() != Object )
            {
               rc = SDB_INVALIDARG;
               PD_LOG( PDERROR, "updator's element must be an Object type:"
                       "updator=%s", updator.toString().c_str() ) ;
               goto error ;
            }

            subObj = beTmp.embeddedObject() ;
            //if replace. leave the keep
            if ( isReplace &&
                 0 == ossStrcmp( beTmp.fieldName(),
                                 CMD_ADMIN_PREFIX FIELD_OP_VALUE_KEEP ) )
            {
               _addKeys( subObj ) ;
               continue ;
            }

            BSONObjIterator iterField( subObj ) ;
            while( iterField.more() )
            {
               BSONElement beField = iterField.next() ;
               const CHAR *pField = beField.fieldName() ;
               if ( _isKey( pField, boShardingKey ) )
               {
                  hasInclude = TRUE ;
                  goto done ;
               }
            } // while( iterField.more() )
         } // while ( iter.more() )

         if ( isReplace && _addKeys( boShardingKey ) > 0 )
         {
            hasInclude = TRUE ;
            goto done ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR,"Failed to check the record is include sharding-key,"
                  "occured unexpected error: %s", e.what() ) ;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _coordKeyKicker::_checkShardingKey( const string &collectionName,
                                             const BSONObj &updator,
                                             BOOLEAN &hasInclude,
                                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CoordCataInfoPtr cataPtr ;

      rc = _pResource->getOrUpdateCataInfo( collectionName.c_str(),
                                            cataPtr,
                                            cb ) ;
      if ( SDB_CLS_COORD_NODE_CAT_VER_OLD == rc )
      {
         /// When the main-collection is old, ignored
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Update collection[%s]'s catalog info failed, "
                 "rc: %d", collectionName.c_str(), rc ) ;
         goto error ;
      }

      if ( !cataPtr->isSharded() )
      {
         goto done ;
      }

      rc = _checkShardingKey( cataPtr, updator, hasInclude ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordKeyKicker::_isKey( const CHAR *pField, BSONObj &boKey )
   {
      BSONObjIterator iterKey( boKey ) ;
      BOOLEAN isKey = FALSE ;
      const CHAR *pUpdateKey = NULL ;

      while( iterKey.more() )
      {
         BSONElement beKey = iterKey.next();
         const CHAR *pKey = beKey.fieldName();
         pUpdateKey = pField ;

         while( *pKey == *pUpdateKey && *pKey != '\0' )
         {
            ++pKey ;
            ++pUpdateKey ;
         }

         // shardingkey_fieldName == updator_fieldName
         /// key: { a:1 }  field : { a:1 } or { "a.b":1 }
         /// key: { "a.b":1 } field: { a:1 } or { "a.b":1 } or
         ///                         { "a.b.c":1 }
         if ( *pKey == *pUpdateKey ||
              ( '\0' == *pKey && '.' == *pUpdateKey ) ||
              ( '.' == *pKey && '\0' == *pUpdateKey ) )
         {
            isKey = TRUE ;
            break ;
         }
      }
      return isKey ;
   }

}

