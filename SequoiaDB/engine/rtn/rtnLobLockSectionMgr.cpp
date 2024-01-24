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

   Source File Name = rtnLobLockSectionMgr.cpp

   Descriptive Name = lob lock section management

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/07/2019  LinYouBin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLobLockSectionMgr.hpp"
#include "msgDef.h"
#include "pd.hpp"

using namespace bson ;

namespace engine
{
   _rtnLockSection::_rtnLockSection( INT64 begin, INT64 end, LOB_LOCK_MODE mode,
                                     INT64 ownerID )
   {
      _section.setBegin( begin ) ;
      _section.setEnd( end ) ;
      _mode = mode ;
      _ownerSet.insert( ownerID ) ;
   }

   _rtnLockSection::~_rtnLockSection()
   {
      _ownerSet.clear() ;
   }

   INT64 _rtnLockSection::begin() const
   {
      return _section.begin() ;
   }

   INT64 _rtnLockSection::end() const
   {
      return _section.end() ;
   }

   BOOLEAN _rtnLockSection::isShared() const
   {
      return _mode == LOB_LOCK_MODE_SHARE ;
   }

   LOB_LOCK_MODE _rtnLockSection::getMode() const
   {
      return _mode ;
   }

   BOOLEAN _rtnLockSection::isContain( INT64 ownerID ) const
   {
      return  _ownerSet.find( ownerID ) != _ownerSet.end() ;
   }

   void _rtnLockSection::setBegin( INT64 begin )
   {
      _section.setBegin( begin ) ;
   }

   void _rtnLockSection::setEnd( INT64 end )
   {
      _section.setEnd( end ) ;
   }

   OWNER_SET_TYPE* _rtnLockSection::getOwnerSet()
   {
      return &_ownerSet ;
   }

   string _rtnLockSection::toString()
   {
      stringstream ss ;
      ss << "Begin:" << _section.begin() << ", "
         << "End:" << _section.end() << ", "
         << "Mode:" << _mode << ", "
         << "Owners:" ;

      ss << "[" ;
      for ( OWNER_SET_TYPE_ITERATOR iter = _ownerSet.begin();
            iter != _ownerSet.end(); ++iter )
      {
         if ( iter != _ownerSet.begin() )
         {
            ss << ", " ;
         }
         ss << *iter ;
      }

      ss << "]" ;

      return ss.str() ;
   }

   void _rtnLockSection::addOwnerID( INT64 ownerID )
   {
      _ownerSet.insert( ownerID ) ;
   }

   _rtnLobLockSectionMgr::_rtnLobLockSectionMgr( const bson::OID& oid )
   {
      _oid = oid ;
   }

   _rtnLobLockSectionMgr::~_rtnLobLockSectionMgr()
   {
      _sectionMap.clear() ;
   }

   BOOLEAN _rtnLobLockSectionMgr::_isSameOwner( OWNER_SET_TYPE *left,
                                                OWNER_SET_TYPE *right )
   {
      if ( left->size() != right->size() )
      {
         return FALSE ;
      }

      OWNER_SET_TYPE_ITERATOR iterLeft = left->begin() ;
      OWNER_SET_TYPE_ITERATOR iterRight = right->begin() ;
      // elements is sorted by set
      while ( iterLeft != left->end() )
      {
         if ( *iterLeft != *iterRight )
         {
            return FALSE ;
         }

         ++iterLeft ;
         ++iterRight ;
      }

      return TRUE ;
   }

   INT32 _rtnLobLockSectionMgr::_lockSectionInsideIter( INT64 newOwnerID,
                                       _rtnLockSection &newSection,
                                       LOCKSECTION_MAP_ITERATOR &iter,
                                       LOCKSECTION_MAP_TYPE &addedSectionMap )
   {
      INT32 rc = SDB_OK ;

      // iter: [10, 20, S, [oldOwners])
      // new:  [12, 18, S, [newOwner])
      if ( _isSameOwner( newSection.getOwnerSet(), iter->second.getOwnerSet()) )
      {
         SDB_ASSERT( newSection.getMode() == iter->second.getMode(),
                     "mode should be same!" ) ;
         ++iter ;
         // result: [10, 20, S, [oldOwners] )
         // addition: null
         goto done ;
      }

      if ( !newSection.isShared() || !iter->second.isShared() )
      {
         rc = SDB_LOB_LOCK_CONFLICTED ;
         PD_LOG( PDERROR, "Lob is conflicted:owned:[%s], expect:[%s]",
                 iter->second.toString().c_str(),
                 newSection.toString().c_str() ) ;
         goto error ;
      }

      if ( iter->second.isContain( newOwnerID ) )
      {
         // iter: [10, 20, S, [oldOwners])
         // new:  [12, 18, S, [newOwner])
         // result: [10, 20, S, [oldOwners])
         // addition: null
         ++iter ;
         goto done ;
      }

      if ( newSection.begin() == iter->second.begin() )
      {
         if ( newSection.end() == iter->second.end() )
         {
            // iter: [10, 20, S, [oldOwners])
            // new:  [10, 20, S, [newOwner])
            // result: [10, 20, S, [oldOwners + newOwner])
            // addition: [10, 20, S, [newOwner])
            addedSectionMap[ newSection.begin() ] = newSection ;
            iter->second.addOwnerID( newOwnerID ) ;
            ++iter ;
         }
         else
         {
            // iter: [10, 20, S, [oldOwners])
            // new:  [10, 18, S, [newOwner])
            // result: [10, 18, S, [oldOwners + newOwner]) + [18, 20, S, [oldOwners])
            // addition: [10, 20, S, [newOwner])
            addedSectionMap[ newSection.begin() ] = newSection ;

            _rtnLockSection rightPart = iter->second ;
            rightPart.setBegin( newSection.end() ) ;

            iter->second.setEnd( newSection.end() ) ;
            iter->second.addOwnerID( newOwnerID ) ;

            _sectionMap[ rightPart.begin() ] = rightPart ;
            ++iter ;
         }
      }
      else
      {
         if ( newSection.end() == iter->second.end() )
         {
            // iter: [10, 20, S, [oldOwners])
            // new:  [12, 20, S, [newOwner])
            // result: [10, 12, S, [oldOwners]) + [12, 20, S, [oldOwners + newOwner])
            // addition: [10, 20, S, [newOwner])
            addedSectionMap[ newSection.begin() ] = newSection ;

            _rtnLockSection rightPart = iter->second ;
            rightPart.setBegin( newSection.begin() ) ;
            rightPart.addOwnerID( newOwnerID ) ;

            iter->second.setEnd( newSection.begin() ) ;

            _sectionMap[ rightPart.begin() ] = rightPart ;
            ++iter ;
         }
         else
         {
            // iter: [10, 20, S, [oldOwners])
            // new:  [12, 18, S, [newOwner])
            // result: [10, 12, S, [oldOwners]) + [12, 18, S, [oldOwners + newOwner])
            //         + [18, 20, S, [oldOwners])
            // addition: [10, 20, S, [newOwner])
            addedSectionMap[ newSection.begin() ] =  newSection ;

            _rtnLockSection middle = iter->second ;
            middle.setBegin( newSection.begin() ) ;
            middle.setEnd( newSection.end() ) ;
            middle.addOwnerID( newOwnerID ) ;

            _rtnLockSection rightPart = iter->second ;
            rightPart.setBegin( newSection.end() ) ;

            iter->second.setEnd( newSection.begin() ) ;

            _sectionMap[ middle.begin() ] = middle ;
            _sectionMap[ rightPart.begin() ] = rightPart ;
            ++iter ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLobLockSectionMgr::_lockSectionOnIterLeft(
                                       INT64 newOwnerID,
                                       _rtnLockSection &newSection,
                                       LOCKSECTION_MAP_ITERATOR &iter,
                                       LOCKSECTION_MAP_TYPE &addedSectionMap )
   {
      INT32 rc = SDB_OK ;

      if ( _isSameOwner( newSection.getOwnerSet(), iter->second.getOwnerSet()) )
      {
         SDB_ASSERT( newSection.getMode() == iter->second.getMode(),
                     "mode should be same!" ) ;
         // iter: [10, 20, S, [oldOwners])
         // new:  [0, 15, S, [newOwner])
         // result: [0, 20, S, [newOwner] )
         // addition: [0, 10, S, [newOwner])
         _rtnLockSection additionSection( newSection.begin(),
                                          iter->second.begin(),
                                          newSection.getMode(),
                                          newOwnerID ) ;
         addedSectionMap[ additionSection.begin() ] = additionSection ;

         newSection.setEnd( iter->second.end() ) ;
         _sectionMap[ newSection.begin() ] = newSection ;
         _sectionMap.erase( iter++ ) ;
         goto done ;
      }

      if ( !newSection.isShared() || !iter->second.isShared() )
      {
         rc = SDB_LOB_LOCK_CONFLICTED ;
         PD_LOG( PDERROR, "Lob is conflicted:owned:[%s], expect:[%s]",
                 iter->second.toString().c_str(), newSection.toString().c_str() ) ;
         goto error ;
      }

      if ( iter->second.isContain( newOwnerID ) )
      {
         // iter: [10, 20, S, [oldOwners])
         // new:  [0, 15, S, [newOwner])
         // result: [0, 10, S, [newOwner]) + [10, 20, S, [oldOwners])
         // addition: [0, 10, S, [newOwner])
         _rtnLockSection additionSection( newSection.begin(),
                                          iter->second.begin(),
                                          newSection.getMode(),
                                          newOwnerID ) ;
         addedSectionMap[ additionSection.begin() ] = additionSection ;
         _sectionMap[ additionSection.begin() ] = additionSection ;
         ++iter ;
         goto done ;
      }

      if ( newSection.end() == iter->second.end() )
      {
         // iter:   [10, 20, S, [oldOwners])
         // new:    [0, 20, S, [newOwner])
         // result: [0, 10, S, [newOwner]) + [10, 20, S, [newOwner, oldOwners])
         // addition: [0, 20, S, [newOwner])
         addedSectionMap[ newSection.begin() ] = newSection ; /* addition */

         iter->second.addOwnerID( newOwnerID ) ;

         newSection.setEnd( iter->second.begin() ) ;
         _sectionMap[ newSection.begin() ] = newSection ;
         ++iter ;
      }
      else
      {
         // iter:   [10, 20, S, [oldOwners])
         // new:    [0, 15, S, [newOwner])
         // result: [0, 10, S, [newOwner]) + [10, 15, S, [newOwner, oldOwners])
         //         + [15, 20, S, [oldOwners])
         // addition: [0, 15, S, [newOwner])
         addedSectionMap[ newSection.begin() ] = newSection ; /* addition */

         _rtnLockSection leftPart = newSection ;
         leftPart.setEnd( iter->second.begin() ) ;

         _rtnLockSection rightPart = iter->second ;
         rightPart.setBegin( newSection.end() ) ;

         iter->second.setEnd( rightPart.begin() ) ;
         iter->second.addOwnerID( newOwnerID ) ;

         _sectionMap[ leftPart.begin() ] = leftPart ;
         _sectionMap[rightPart.begin() ] = rightPart ;
         ++iter ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLobLockSectionMgr::_lockSection( INT64 begin, INT64 end,
                                       LOB_LOCK_MODE mode,
                                       INT64 newOwnerID,
                                       LOCKSECTION_MAP_TYPE &addedSectionMap )
   {
      INT32 rc = SDB_OK ;
      LOCKSECTION_MAP_ITERATOR iter ;
      _rtnLockSection newSection( begin, end, mode, newOwnerID ) ;

      if ( _sectionMap.size() == 0 )
      {
         _sectionMap[ newSection.begin() ] = newSection ;
         goto done ;
      }

      iter = _sectionMap.lower_bound( newSection.begin() ) ;
      if ( iter != _sectionMap.begin() )
      {
         // previou section may overlap the newSection
         // _sectionMap: [1,5) [10, 15)
         // newSection: [4, 20)
         // lower_bound: [10, 15), but we should deal with [1, 5) too
         --iter ;
      }

      while ( iter != _sectionMap.end() )
      {
         // iter: [10, 20)
         // new:  [20, 30)
         if ( newSection.begin() >=  iter->second.end() )
         {
            ++iter ;
            continue ;
         }

         if ( newSection.end() <= iter->second.begin() )
         {
            addedSectionMap[ newSection.begin() ] = newSection ;
            if ( newSection.end() < iter->second.begin() )
            {
               // iter: [10, 20)
               // new:  [0, 8)
               _sectionMap[ newSection.begin() ] = newSection ;
            }
            else
            {
               // iter: [10, 20)
               // new:  [0, 10)
               if ( _isSameOwner( newSection.getOwnerSet(),
                                  iter->second.getOwnerSet()) )
               {
                  SDB_ASSERT( newSection.getMode() == iter->second.getMode(),
                              "mode should be same!" ) ;
                  newSection.setEnd( iter->second.end() ) ;
                  _sectionMap[ newSection.begin() ] = newSection ;
                  _sectionMap.erase( iter++ ) ;
               }
               else
               {
                  _sectionMap[ newSection.begin() ] = newSection ;
               }
            }

            goto done ;
         }

         if ( newSection.begin() < iter->second.begin() )
         {
            if ( newSection.end() <= iter->second.end() )
            {
               // iter: [10, 20)
               // new:  [0, 15)
               rc = _lockSectionOnIterLeft( newOwnerID, newSection, iter,
                                            addedSectionMap ) ;
               goto done ;
            }

            // iter: [10, 20)
            // new:  [0, 30)
            // leftPart:  [0, 20)
            // rightPart: [20, 30)
            _rtnLockSection leftPart( newSection.begin(), iter->second.end(),
                                      mode, newOwnerID ) ;
            //[20,30]
            newSection.setBegin( iter->second.end() ) ;

            rc = _lockSectionOnIterLeft( newOwnerID, leftPart, iter,
                                         addedSectionMap ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         else
         {
            if ( newSection.end() <= iter->second.end() )
            {
               // iter: [10, 20)
               // new:  [12, 18) or [12, 20)
               rc = _lockSectionInsideIter( newOwnerID, newSection, iter,
                                            addedSectionMap ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }

               goto done ;
            }

            // iter: [10, 20)
            // new:  [12, 30)
            // leftPart: [12, 20)
            // rightPart:[20, 30)
            _rtnLockSection leftPart( newSection.begin(), iter->second.end(),
                                      mode, newOwnerID ) ;
            newSection.setBegin( iter->second.end() ) ;

            rc = _lockSectionInsideIter( newOwnerID, leftPart, iter,
                                         addedSectionMap ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
      }

      addedSectionMap[ newSection.begin() ] = newSection ;
      _sectionMap[ newSection.begin() ] = newSection ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnLobLockSectionMgr::_rollbackSectionInsideIter( INT64 ownerID,
                                              _rtnLockSection &section,
                                              LOCKSECTION_MAP_ITERATOR &iter )
   {
      if ( !iter->second.isContain( ownerID) )
      {
         ++iter ;
         goto done ;
      }

      if ( section.begin() == iter->second.begin() )
      {
         _rollbackSectionOnIterLeft( ownerID, section, iter ) ;
         goto done ;
      }

      if ( section.end() == iter->second.end() )
      {
         // iter:     [10, 20, S, [oldOwners])
         // section:  [12, 20, S, [newOwner])
         OWNER_SET_TYPE *ownerSet = iter->second.getOwnerSet() ;
         if ( ownerSet->size() == 1 )
         {
            // iter: [10, 12, S, [oldOwners])
            iter->second.setEnd( section.begin() ) ;
            ++iter ;
         }
         else
         {
            // rightPart: [12, 20, S, [oldOwners- ownerID])
            _rtnLockSection rightPart = iter->second ;
            rightPart.setBegin( section.begin() ) ;
            rightPart.getOwnerSet()->erase( ownerID ) ;
            _sectionMap[ rightPart.begin() ] = rightPart ;

            // iter: [10, 12, S, [oldOwners])
            iter->second.setEnd( section.begin() ) ;
            ++iter ;
         }
      }
      else
      {
         // iter:    [10, 20, S, [oldOwners])
         // section: [12, 18, S, [newOwner])

         // middle [12, 20, S, [oldOwners - newOwner])
         _rtnLockSection middle = iter->second ;
         middle.setBegin( section.begin() ) ;
         middle.setEnd( section.end() ) ;

         // right [18, 20, S, [oldOwners])
         _rtnLockSection rightPart = iter->second ;
         rightPart.setBegin( section.end() ) ;

         // left [10, 12, S, [oldOwners])
         iter->second.setEnd( section.begin() ) ;

         if ( middle.getOwnerSet()->size() > 1 )
         {
            middle.getOwnerSet()->erase( ownerID ) ;
            _sectionMap[ middle.begin() ] = middle ;
         }

         _sectionMap[ rightPart.begin() ] = rightPart ;
         ++iter ;
      }

   done:
      return ;
   }

   void _rtnLobLockSectionMgr::_rollbackSectionOnIterLeft( INT64 ownerID,
                                              _rtnLockSection &section,
                                              LOCKSECTION_MAP_ITERATOR &iter )
   {
      if ( !iter->second.isContain( ownerID) )
      {
         ++iter ;
         goto done ;
      }

      if ( section.end() == iter->second.end() )
      {
         // iter:     [10, 20, S, [oldOwners])
         // section:  [0, 20, S, [newOwner])
         OWNER_SET_TYPE *ownerSet = iter->second.getOwnerSet() ;
         if ( ownerSet->size() == 1 )
         {
            _sectionMap.erase( iter++ ) ;
         }
         else
         {
            ownerSet->erase( ownerID ) ;
            ++iter ;
         }
      }
      else
      {
         // iter:    [10, 20, S, [oldOwners])
         // section: [0, 15, S, [newOwner])
         OWNER_SET_TYPE *ownerSet = iter->second.getOwnerSet() ;
         _rtnLockSection rightPart = iter->second ;
         rightPart.setBegin( section.end() ) ;
         _sectionMap[ rightPart.begin() ] = rightPart ;

         if ( ownerSet->size() == 1 )
         {
            //result: [15, 20, S, [owners])
            _sectionMap.erase( iter++ ) ;
         }
         else
         {
            //result: [10, 15, S, [oldOwners-newOwner]) + [15, 20, S, [oldOwners])
            iter->second.setEnd( section.end() ) ;
            ownerSet->erase( ownerID ) ;
            ++iter ;
         }
      }

   done:
      return ;
   }

   void _rtnLobLockSectionMgr::_rollbackSection( INT64 ownerID,
                                                 _rtnLockSection &section )
   {
      LOCKSECTION_MAP_ITERATOR iter ;

      if ( _sectionMap.size() == 0 )
      {
         // no need to rollback
         goto done ;
      }

      iter = _sectionMap.lower_bound( section.begin() ) ;
      if ( iter != _sectionMap.begin() )
      {
         // previou section may overlap the newSection
         // _sectionMap: [1,5) [10, 15)
         // section: [4, 20)
         // lower_bound: [10, 15), but we should deal with [1, 5) too
         --iter ;
      }

      while ( iter != _sectionMap.end() )
      {
         if ( section.begin() >= iter->second.end() )
         {
            iter++ ;
            continue ;
         }

         if ( section.end() <= iter->second.begin() )
         {
            goto done ;
         }

         if ( section.begin() <= iter->second.begin() )
         {
            if ( section.end() <= iter->second.end() )
            {
               // iter: [10, 20)
               // section:  [0, 15)
               _rollbackSectionOnIterLeft( ownerID, section, iter ) ;
               goto done ;
            }

            // iter: [10, 20)
            // section:  [0, 30)
            // leftPart:  [0, 20)
            // rightPart: [20, 30)
            _rtnLockSection leftPart( section.begin(), iter->second.end(),
                                      section.getMode(), ownerID ) ;
            //[20,30]
            section.setBegin( iter->second.end() ) ;
            _rollbackSectionOnIterLeft( ownerID, leftPart, iter ) ;
         }
         else
         {
            if ( section.end() <= iter->second.end() )
            {
               // iter: [10, 20)
               // new:  [12, 18) or [12, 20)
               _rollbackSectionInsideIter( ownerID, section, iter ) ;
               goto done ;
            }

            // iter: [10, 20)
            // new:  [12, 30)
            // leftPart: [12, 20)
            // rightPart:[20, 30)
            _rtnLockSection leftPart( section.begin(), iter->second.end(),
                                      section.getMode(), ownerID ) ;
            section.setBegin( iter->second.end() ) ;

            _rollbackSectionInsideIter( ownerID, leftPart, iter ) ;
         }
      }

   done:
      return ;
   }

   void _rtnLobLockSectionMgr::_rollbackSections( INT64 ownerID,
                                          LOCKSECTION_MAP_TYPE &rollbackMap )
   {
      for ( LOCKSECTION_MAP_ITERATOR iterRollback = rollbackMap.begin();
            iterRollback != rollbackMap.end(); ++iterRollback )
      {
         _rtnLockSection &section = iterRollback->second ;

         OWNER_SET_TYPE *ownerSet = section.getOwnerSet() ;
         SDB_ASSERT( ownerSet->size() == 1, "size must be 1") ;
         SDB_ASSERT( ownerSet->find(ownerID) != ownerSet->end(),
                     "owner id must be same") ;

         _rollbackSection( ownerID, section ) ;
      }

      rollbackMap.clear() ;
   }

   INT32 _rtnLobLockSectionMgr::lockSection( INT64 offset, INT64 length,
                                             LOB_LOCK_MODE mode,
                                             INT64 newOwnerID )
   {
      INT32 rc = SDB_OK ;
      LOCKSECTION_MAP_TYPE addedSectionMap ;

      try
      {
         rc = _lockSection( offset, offset + length, mode, newOwnerID,
                            addedSectionMap ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to add section[offset:%lld,"
                    "length:%lld,lockMode:%d,contextID:%lld],rc=%d",
                    offset, length, mode, newOwnerID, rc ) ;

            _rollbackSections( newOwnerID, addedSectionMap ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to add section, occur unexpected"
                 "error:%s", e.what() ) ;
         rc = SDB_OP_INCOMPLETE ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _rtnLobLockSectionMgr::isTotalContain( INT64 offset, INT64 length,
                                                  LOB_LOCK_MODE mode,
                                                  INT64 ownerID )
   {
      if ( _sectionMap.size() == 0 )
      {
         return FALSE ;
      }

      _rtnLockSection newSection( offset, offset + length, mode, ownerID ) ;
      LOCKSECTION_MAP_ITERATOR iter =
                               _sectionMap.lower_bound( newSection.begin() ) ;
      if ( iter != _sectionMap.begin() )
      {
         --iter ;
      }

      for ( ; iter != _sectionMap.end() ; ++iter )
      {
         if ( newSection.begin() >= iter->second.end() )
         {
            // iter [10, 20)
            // new  [20, 30)
            continue ;
         }

         if ( newSection.begin() < iter->second.begin() )
         {
            // iter [10, 20)
            // new  [0,30) or [0,5) or [0, 18)
            return FALSE ;
         }

         if ( newSection.end() <= iter->second.end() )
         {
            // iter [10, 20)
            // new  [12, 18)
            if ( iter->second.isContain(ownerID) )
            {
               SDB_ASSERT( iter->second.getMode() == mode, "mode must be same" ) ;
               return TRUE ;
            }

            return FALSE ;
         }

         // iter [10, 20)
         // new  [12, 30)
         if ( !iter->second.isContain(ownerID) )
         {
            return FALSE ;
         }

         // continue to check rest part [20, 30)
         SDB_ASSERT( iter->second.getMode() == mode, "mode must be same" ) ;
         newSection.setBegin( iter->second.end() ) ;
      }

      return FALSE ;
   }

   void _rtnLobLockSectionMgr::removeSectionByOwnerID( INT64 ownerID )
   {
      LOCKSECTION_MAP_ITERATOR iter = _sectionMap.begin() ;
      while ( iter != _sectionMap.end() )
      {
         OWNER_SET_TYPE *ownerSet = iter->second.getOwnerSet() ;
         OWNER_SET_TYPE_ITERATOR ownerIter = ownerSet->find( ownerID ) ;
         if ( ownerIter != ownerSet->end() )
         {
            ownerSet->erase( ownerIter ) ;
            if ( ownerSet->size() == 0 )
            {
               _sectionMap.erase( iter++ ) ;
               continue ;
            }
         }

         ++iter ;
      }
   }

   BOOLEAN _rtnLobLockSectionMgr::isEmpty() const
   {
      return _sectionMap.empty() ? TRUE : FALSE ;
   }

   string _rtnLobLockSectionMgr::toString()
   {
      stringstream ss ;
      ss << "Oid:" << _oid.toString() << ", "
         << FIELD_NAME_LOB_LOCK_SECTIONS << ":" ;

      ss << "[" ;
      for ( LOCKSECTION_MAP_ITERATOR iter = _sectionMap.begin();
            iter != _sectionMap.end(); ++iter )
      {
         if ( iter != _sectionMap.begin() )
         {
            ss << ", " ;
         }

         ss << iter->second.toString() ;
      }

      ss << "]" ;

      return ss.str() ;
   }

   INT32 _rtnLobLockSectionMgr::toBSONBuilder( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONArrayBuilder subArray(
                       builder.subarrayStart( FIELD_NAME_LOB_LOCK_SECTIONS ) ) ;
         for ( LOCKSECTION_MAP_ITERATOR iter = _sectionMap.begin();
               iter != _sectionMap.end(); ++iter )
         {
            OWNER_SET_TYPE *ownerSet = NULL ;
            BSONObjBuilder sectionBuilder( subArray.subobjStart() ) ;
            sectionBuilder.append( FIELD_NAME_LOB_SECTION_BEGIN,
                                   iter->second.begin() ) ;
            sectionBuilder.append( FIELD_NAME_LOB_SECTION_END,
                                   iter->second.end() ) ;
            sectionBuilder.append( FIELD_NAME_LOB_LOCK_TYPE,
                                   iter->second.isShared() ? "S" : "X" ) ;

            BSONArrayBuilder contextArray (
                        sectionBuilder.subarrayStart( FIELD_NAME_CONTEXTS ) ) ;
            ownerSet = iter->second.getOwnerSet() ;
            for ( OWNER_SET_TYPE_ITERATOR setIter = ownerSet->begin();
                  setIter != ownerSet->end(); ++setIter )
            {
               contextArray.append( *setIter ) ;
            }
            contextArray.done() ;

            sectionBuilder.done() ;
         }

         subArray.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to build _rtnLobLockSectionMgr, "
                 "occur unexpected error:%s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}


