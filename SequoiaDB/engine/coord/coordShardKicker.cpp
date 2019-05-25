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

#include "coordShardKicker.hpp"
#include "pmdEDU.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordShardKicker implement
   */
   _coordShardKicker::_coordShardKicker()
   {
      _pResource = NULL ;
   }

   _coordShardKicker::~_coordShardKicker()
   {
   }

   void _coordShardKicker::bind( coordResource *pResource,
                                 const CoordCataInfoPtr &cataPtr )
   {
      _pResource = pResource ;
      _cataPtr = cataPtr ;
   }

   BOOLEAN _coordShardKicker::_isUpdateReplace( const BSONObj &updator )
   {
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

   UINT32 _coordShardKicker::_addKeys( const BSONObj &objKey )
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

   INT32 _coordShardKicker::_kickShardingKey( const CoordCataInfoPtr &cataInfo,
                                              const BSONObj &updator,
                                              BSONObj &newUpdator,
                                              BOOLEAN &hasShardingKey )
   {
      INT32 rc = SDB_OK ;
      UINT32 skSiteID = cataInfo->getShardingKeySiteID() ;

      if ( skSiteID > 0 )
      {
         map< UINT32, BOOLEAN >::iterator it = _skSiteIDs.find( skSiteID );
         if ( it != _skSiteIDs.end() )
         {
            newUpdator = updator ;
            hasShardingKey = it->second ;
            goto done ;
         }
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
               BSONObjIterator iterKey( boShardingKey ) ;
               BOOLEAN isKey = FALSE ;
               while( iterKey.more() )
               {
                  BSONElement beKey = iterKey.next();
                  const CHAR *pKey = beKey.fieldName();
                  const CHAR *pField = beField.fieldName();
                  while( *pKey == *pField && *pKey != '\0' )
                  {
                     ++pKey ;
                     ++pField ;
                  }

                  if ( *pKey == *pField ||
                       ( '\0' == *pKey && '.' == *pField ) ||
                       ( '.' == *pKey && '\0' == *pField ) )
                  {
                     isKey = TRUE ;
                     break ;
                  }
               }
               if ( isKey )
               {
                  hasShardingKey = TRUE;
               }
               else
               {
                  subBuilder.append( beField ) ;
               }
            } // while( iterField.more() )

            subBuilder.done() ;
         } // while ( iter.more() )

         if ( isReplace )
         {
            UINT32 count = _addKeys( boShardingKey ) ;
            if ( count > 0 )
            {
               hasShardingKey = TRUE ;
            }

            if ( !_setKeys.empty() )
            {
               BSONObjBuilder keepBuilder( bobNewUpdator.subobjStart(
                                           CMD_ADMIN_PREFIX FIELD_OP_VALUE_KEEP ) ) ;
               SET_SHARDINGKEY::iterator itKey = _setKeys.begin() ;
               while( itKey != _setKeys.end() )
               {
                  keepBuilder.append( itKey->_pStr, (INT32)1 ) ;
                  ++itKey ;
               }
               keepBuilder.done() ;
            }
         } // if ( isReplace )
         newUpdator = bobNewUpdator.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR,"Failed to kick sharding key from the record,"
                  "occured unexpected error: %s", e.what() ) ;

         goto error;
      }

      if ( skSiteID > 0 )
      {
         _skSiteIDs.insert(
                    pair< UINT32, BOOLEAN >( skSiteID, hasShardingKey ) ) ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _coordShardKicker::kickShardingKey( const BSONObj &updator,
                                             BSONObj &newUpdator,
                                             BOOLEAN &isChanged,
                                             pmdEDUCB *cb,
                                             const BSONObj &matcher,
                                             BOOLEAN keepShardingKey )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasShardingKey = FALSE ;

      if ( !_cataPtr.get() || !_cataPtr->isSharded() )
      {
         newUpdator = updator ;
         goto done ;
      }

      _skSiteIDs.clear() ;
      _setKeys.clear() ;

      rc = _kickShardingKey( _cataPtr, updator, newUpdator, hasShardingKey ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Kick sharding key failed, rc: %d", rc ) ;
         goto error ;
      }
      if ( keepShardingKey )
      {
         if ( hasShardingKey && 1 != _cataPtr->getGroupNum() )
         {
            rc = SDB_UPDATE_SHARD_KEY ;
            PD_LOG( PDERROR, "When the partition cl falls on two or more groups, "
                    "or it is a main cl, the update rule don't allow sharding key. "
                    "rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         if ( hasShardingKey )
         {
            isChanged = TRUE ;
         }
      }


      if ( _cataPtr->isMainCL() )
      {
         vector< string > subCLLst ;
         vector< string >::iterator iterCL ;
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
                                   isChanged, cb, keepShardingKey ) ;
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

   INT32 _coordShardKicker::_kickShardingKey( const string &collectionName,
                                              const BSONObj &updator,
                                              BSONObj &newUpdator,
                                              BOOLEAN &isChanged,
                                              pmdEDUCB *cb,
                                              BOOLEAN keepShardingKey )
   {
      INT32 rc = SDB_OK ;
      CoordCataInfoPtr cataPtr ;
      BOOLEAN hasShardingKey = FALSE ;

      rc = _pResource->getOrUpdateCataInfo( collectionName.c_str(),
                                            cataPtr,
                                            cb ) ;
      if ( SDB_CLS_COORD_NODE_CAT_VER_OLD == rc )
      {
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

      rc = _kickShardingKey( cataPtr, updator, newUpdator,
                             hasShardingKey ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( keepShardingKey )
      {
         if ( 1 != cataPtr->getGroupNum() && hasShardingKey )
         {
            rc = SDB_UPDATE_SHARD_KEY ;
            PD_LOG( PDERROR, "When the partition cl falls on two or "
                    "more groups, the update rule don't allow sharding"
                    " key. rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         if ( hasShardingKey )
         {
            isChanged = TRUE ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordShardKicker::checkShardingKey( const BSONObj &updator,
                                              BOOLEAN &hasInclude,
                                              _pmdEDUCB *cb,
                                              const BSONObj &matcher )
   {
      INT32 rc = SDB_OK ;

      if ( !_cataPtr.get() || !_cataPtr->isSharded() )
      {
         goto done ;
      }

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

      if ( _cataPtr->isMainCL() )
      {
         vector< string > subCLLst ;
         vector< string >::iterator iterCL ;

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

   INT32 _coordShardKicker::_checkShardingKey( const CoordCataInfoPtr &cataInfo,
                                               const BSONObj &updator,
                                               BOOLEAN &hasInclude )
   {
      INT32 rc = SDB_OK ;
      UINT32 skSiteID = cataInfo->getShardingKeySiteID() ;

      if ( skSiteID > 0 )
      {
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
               BSONObjIterator iterKey( boShardingKey ) ;
               BOOLEAN isKey = FALSE ;
               while( iterKey.more() )
               {
                  BSONElement beKey = iterKey.next() ;
                  const CHAR *pKey = beKey.fieldName() ;
                  const CHAR *pField = beField.fieldName() ;
                  while( *pKey == *pField && *pKey != '\0' )
                  {
                     ++pKey ;
                     ++pField ;
                  }

                  if ( *pKey == *pField ||
                       ( '\0' == *pKey && '.' == *pField ) ||
                       ( '.' == *pKey && '\0' == *pField ) )
                  {
                     isKey = TRUE ;
                     break ;
                  }
               }
               if ( isKey )
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

   INT32 _coordShardKicker::_checkShardingKey( const string &collectionName,
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

}

