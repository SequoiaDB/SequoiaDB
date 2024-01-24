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

   Source File Name = authAccessControlList.cpp

   Descriptive Name = 

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "authAccessControlList.hpp"
#include "authDef.hpp"

namespace engine
{
   using namespace bson;
   using boost::shared_ptr;
   INT32 authAccessControlList::addPrivilege( const authPrivilege &p )
   {
      INT32 rc = SDB_OK;
      const boost::shared_ptr< authResource > &res = p.getResource();
      const boost::shared_ptr< authActionSet > &actions = p.getActionSet();

      try
      {
         DATA_TYPE::iterator it = _data.find( res );
         if ( it == _data.end() )
         {
            std::pair< DATA_TYPE::iterator, BOOLEAN > result =
               _data.insert( std::make_pair( res, actions ) );
            if ( !result.second )
            {
               rc = SDB_SYS;
               PD_LOG( PDERROR, "Failed to insert privilege into ACL" );
               goto error;
            }
         }
         else
         {
            it->second->addAllActionsFromSet( *actions );
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 authAccessControlList::addPrivilege( const bson::BSONObj &privObj )
   {
      INT32 rc = SDB_OK;
      BSONObj resObj = privObj.getObjectField( AUTH_FIELD_NAME_RESOURCE );
      BSONObj actionsObj = privObj.getObjectField( AUTH_FIELD_NAME_ACTIONS );

      shared_ptr< authResource > res = authResource::fromBson( resObj );
      shared_ptr< authActionSet > actionSet = authActionSet::fromBson( actionsObj );
      if ( !res || !actionSet )
      {
         rc = SDB_SYS;
         PD_RC_CHECK( rc, PDERROR, "Failed to convert bson to resource or actionSet, rc: %d", rc );
      }

      {
         authPrivilege p( res, actionSet );
         rc = addPrivilege( p );
         PD_RC_CHECK( rc, PDERROR, "Failed to add privilege to ACL, rc: %d", rc );
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 authAccessControlList::removePrivilege( const authPrivilege &p )
   {
      INT32 rc = SDB_OK;
      const boost::shared_ptr< authResource > &res = p.getResource();
      const boost::shared_ptr< authActionSet > &actions = p.getActionSet();

      try
      {
         DATA_TYPE::iterator it = _data.find( res );
         if ( it != _data.end() )
         {
            it->second->removeActionsFromSet( *actions );
            if ( it->second->empty() )
            {
               _data.erase( it );
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 authAccessControlList::removePrivilege( const bson::BSONObj &privObj )
   {
      INT32 rc = SDB_OK;
      BSONObj resObj = privObj.getObjectField( AUTH_FIELD_NAME_RESOURCE );
      BSONObj actionsObj = privObj.getObjectField( AUTH_FIELD_NAME_ACTIONS );

      shared_ptr< authResource > res = authResource::fromBson( resObj );
      shared_ptr< authActionSet > actionSet = authActionSet::fromBson( actionsObj );
      if ( !res || !actionSet )
      {
         rc = SDB_SYS;
         PD_RC_CHECK( rc, PDERROR, "Failed to convert bson to resource or actionSet, rc: %d", rc );
      }

      {
         authPrivilege p( res, actionSet );
         rc = removePrivilege( p );
         PD_RC_CHECK( rc, PDERROR, "Failed to add privilege to ACL, rc: %d", rc );
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN authAccessControlList::isAuthorizedForPrivilege( const authPrivilege &p ) const
   {
      const boost::shared_ptr< authResource > &res = p.getResource();
      const boost::shared_ptr< authActionSet > &actions = p.getActionSet();

      return isAuthorizedForActionsOnResource( *res, *actions );
   }

   BOOLEAN authAccessControlList::isAuthorizedForActionsOnResource(
      const authResource &res,
      const authActionSet &actions ) const
   {
      authActionSet unmet( actions );
      for ( DATA_TYPE::const_iterator it = _data.begin(); it != _data.end(); ++it )
      {
         if ( res.isIncluded( *it->first ) )
         {
            unmet.removeActionsFromSet(*it->second);
            if ( unmet.empty() )
            {
               return TRUE;
            }
         }
      }
      return unmet.empty();
   }

   void authAccessControlList::toBSONArray( bson::BSONArrayBuilder &builder ) const
   {
      for ( DATA_TYPE::const_iterator it = _data.begin(); it != _data.end(); ++it )
      {
         BSONObjBuilder objBuilder( builder.subobjStart() );
         BSONObjBuilder resBuilder( objBuilder.subobjStart( AUTH_FIELD_NAME_RESOURCE ) );
         it->first->toBSONObj( resBuilder );
         BSONArrayBuilder setBuilder( objBuilder.subarrayStart( AUTH_FIELD_NAME_ACTIONS ) );
         it->second->toBSONArray( setBuilder );
         objBuilder.doneFast();
      }
      builder.doneFast();
   }
} // namespace engine