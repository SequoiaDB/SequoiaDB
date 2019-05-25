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

   Source File Name = sdbIPersistence.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Event Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for events that
   used as inter-EDU communications.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/01/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_PERSISTENCE_HPP__
#define SDB_I_PERSISTENCE_HPP__

#include "sdbInterface.hpp"

namespace engine
{

   /*
      _IDataSyncBase define
   */
   class _IDataSyncBase
   {
      public:
         _IDataSyncBase() {}
         virtual ~_IDataSyncBase() {}

      public:
         virtual BOOLEAN      isClosed() const = 0 ;
         virtual BOOLEAN      canSync( BOOLEAN &force ) const = 0 ;

         virtual INT32        sync( BOOLEAN force,
                                    BOOLEAN sync,
                                    IExecutor* cb ) = 0 ;

         virtual void         lock() = 0 ;
         virtual void         unlock() = 0 ;
   } ;
   typedef _IDataSyncBase IDataSyncBase ;

   /*
      _IDataSyncManager define
   */
   class _IDataSyncManager
   {
      public:
         _IDataSyncManager() {}
         virtual ~_IDataSyncManager() {}

      public:
         virtual void         registerSync( IDataSyncBase *pSyncUnit ) = 0 ;
         virtual void         unregSync( IDataSyncBase *pSyncUnit ) = 0 ;
         virtual void         notifyChange() = 0 ;
   } ;
   typedef _IDataSyncManager IDataSyncManager ;

}

#endif // SDB_I_PERSISTENCE_HPP__

