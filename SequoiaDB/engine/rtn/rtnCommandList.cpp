/*******************************************************************************


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

   Source File Name = rtnCommandList.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/09/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/


#include "rtnCommandList.hpp"

using namespace bson ;

namespace engine
{

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCollections)

   BOOLEAN _rtnListCollections::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCollectionsInner)

   BOOLEAN _rtnListCollectionsInner::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCollectionspaces)

   BOOLEAN _rtnListCollectionspaces::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCollectionspacesInner)
   /*
      _rtnListCollectionspacesInner implement
   */
   BOOLEAN _rtnListCollectionspacesInner::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListContexts)
   /*
      _rtnListContexts implement
   */
   BOOLEAN _rtnListContexts::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListContextsInner)
   /*
      _rtnListContextsInner implement
   */
   BOOLEAN _rtnListContextsInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListContextsCurrent)
   /*
      _rtnListContextsCurrent implement
   */
   BOOLEAN _rtnListContextsCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListContextsCurrentInner)
   /*
      _rtnListContextsCurrentInner implement
   */
   BOOLEAN _rtnListContextsCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSessions)
   /*
      _rtnListSessions implement
   */

   BOOLEAN _rtnListSessions::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSessionsInner)

   /*
      _rtnListSessionsInner implement
   */
   BOOLEAN _rtnListSessionsInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSessionsCurrent)
   /*
      _rtnListSessionsCurrent implement
   */

   BOOLEAN _rtnListSessionsCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSessionsCurrentInner)
   /*
      _rtnListSessionsCurrentInner implement
   */
   BOOLEAN _rtnListSessionsCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListStorageUnits)
   /*
      _rtnListStorageUnits define
   */
   BOOLEAN _rtnListStorageUnits::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListStorageUnitsInner)
   /*
      _rtnListStorageUnitsInner implement
   */
   BOOLEAN _rtnListStorageUnitsInner::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListBackups)
   /*
      _rtnListBackups implement
   */
   BOOLEAN _rtnListBackups::_isCurrent() const
   {
      return FALSE ;
   }

   BSONObj _rtnListBackups::_getOptObj() const
   {
      try
      {
         BSONObj hintObj( _hintBuff ) ;
         return hintObj ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }
      return BSONObj() ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListBackupsInner)
   /*
      _rtnListBackupsInner implement
   */
   BOOLEAN _rtnListBackupsInner::_isCurrent() const
   {
      return FALSE ;
   }

   BSONObj _rtnListBackupsInner::_getOptObj() const
   {
      BSONObj obj ;
      try
      {
         BSONObjBuilder builder ;
         builder.appendBool( FIELD_NAME_DETAIL, TRUE ) ;
         obj = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }
      return obj ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListTrans)
   /*
      _rtnListTrans implement
   */
   BOOLEAN _rtnListTrans::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListTransInner)
   /*
      _rtnListTransInner implement
   */
   BOOLEAN _rtnListTransInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListTransCurrent)
   /*
      _rtnListTransCurrent implement
   */
   BOOLEAN _rtnListTransCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListTransCurrentInner)
   /*
      _rtnListTransCurrentInner implement
   */
   BOOLEAN _rtnListTransCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSvcTasks)
   /*
      _rtnListSvcTasks implement
   */
   BOOLEAN _rtnListSvcTasks::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSvcTasksInner)
   /*
      _rtnListSvcTasksInner implement
   */

   BOOLEAN _rtnListSvcTasksInner::_isCurrent() const
   {
      return FALSE ;
   }
}


