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

   Source File Name = monMgr.hpp

   Descriptive Name = Monitor Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/24/2019  CW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MONMGR_HPP_
#define MONMGR_HPP_

#include <vector>
#include "monClass.hpp"

namespace engine
{

#define MON_GROUP_MASK_DEFAULT 0
#define MON_GROUP_QUERY_BASIC  0x0000001
#define MON_GROUP_QUERY_DETAIL 0x00000002
#define MON_GROUP_LATCH_BASIC  0x00000004
#define MON_GROUP_LATCH_DETAIL 0x00000008
#define MON_GROUP_LOCK_BASIC   0x00000010
#define MON_GROUP_LOCK_DETAIL  0x00000020

/**
 * A monitor manager responsible for memory management and metric management
 * of the monitor classes
 */
class _monMonitorManager
{
   // disable assignment/copy
   _monMonitorManager& operator= (const _monMonitorManager&) ;
   _monMonitorManager(_monMonitorManager&) ;

public:
   _monMonitorManager() ;

   ~_monMonitorManager() ;

   /**
    * Register a new monitor object
    *
    * @param classType the type of monitor object getting registered
    * @return T a pointer to the new monitor object
    */
   template<class T>
   T* registerMonitorObject()
   {
      //TODO need to verify T is a subclass of monClass
      MON_CLASS_TYPE classType = T::getType() ;
      T* ptr = _monClass[classType]->add<T>();
      return ptr ;
   }

   /**
    * Register a new monitor object
    *
    * @param classType the type of monitor object getting registered
    * @return T a pointer to the new monitor object
    */
   template<class T>
   T* registerMonitorObject(_monClassBaseData *data)
   {
      //TODO need to verify T is a subclass of monClass
      MON_CLASS_TYPE classType = T::getType() ;
      T* ptr = _monClass[classType]->add<T>(data);

      return ptr ;
   }
   /**
    * Remove a monitor object.
    *
    * This will lookup the object type and remove the object from the
    * appropriate container
    *
    * @param obj the object to be removed
    */
   void removeMonitorObject(monClass* obj)
   {
      SDB_ASSERT ( NULL != obj, "removing a NULL monitor object" ) ;
      MON_CLASS_TYPE classType = obj->getType();
      _monClass[classType]->remove(obj);
   }

   /**
    * Update the monitor status using a mask
    *
    * @param mask the monitor mask
    */
   void setMonitorStatus( UINT64 mask )
   {
      if ( mask & MON_GROUP_QUERY_DETAIL )
      {
         setMonitorLvl( MON_CLASS_QUERY, MON_DATA_LVL_DETAIL ) ;
      }
      else if ( mask & MON_GROUP_QUERY_BASIC )
      {
         setMonitorLvl( MON_CLASS_QUERY, MON_DATA_LVL_BASIC ) ;
      }
      else
      {
         setMonitorLvl( MON_CLASS_QUERY, MON_DATA_LVL_NONE ) ;
      }

      if ( mask & MON_GROUP_LATCH_DETAIL )
      {
         setMonitorLvl( MON_CLASS_LATCH, MON_DATA_LVL_DETAIL ) ;
      }
      else if ( mask & MON_GROUP_LATCH_BASIC )
      {
         setMonitorLvl( MON_CLASS_LATCH, MON_DATA_LVL_BASIC ) ;
      }
      else
      {
         setMonitorLvl( MON_CLASS_LATCH, MON_DATA_LVL_NONE ) ;
      }

      if ( mask & MON_GROUP_LOCK_DETAIL )
      {
         setMonitorLvl( MON_CLASS_LOCK, MON_DATA_LVL_DETAIL ) ;
      }
      else if ( mask & MON_GROUP_LOCK_BASIC )
      {
         setMonitorLvl( MON_CLASS_LOCK, MON_DATA_LVL_BASIC ) ;
      }
      else
      {
         setMonitorLvl( MON_CLASS_LOCK, MON_DATA_LVL_NONE ) ;
      }
   }

   /**
    * Update the monitor data collection lvl of a class container
    *
    * @param classType the class type to update
    * @param mode      the new collection level
    */
   void setMonitorLvl( MON_CLASS_TYPE classType, MON_DATA_LEVEL mode )
   {
      _monClass[classType]->setMonitorLvl( mode ) ;
   }

   MON_DATA_LEVEL getCollectionLvl( MON_CLASS_TYPE classType )
   {
      return _monClass[classType]->getCollectionLvl() ;
   }

   BOOLEAN isOperational( MON_CLASS_TYPE classType )
   {
      return _monClass[classType]->isOperational() ;
   }

   /**
    * Update the history event size of all class container
    *
    * @param size      the new monitor history event size
    */
   void setHistEventSize( UINT32 size )
   {
      for ( INT32 i = 0; i < MON_CLASS_MAX; i ++ )
      {
         _monClass[i]->setMaxArchivedListLen( size ) ;
      }
   }

   /**
    * return the list of the target monClass
    * @param cachedMonClassList the vector to populate
    * @param classType the specific monClass to read
    * @param listType the type of list to read
    */
   template <class T> void dumpList ( ossPoolVector<T> & cachedMonClassList, MON_CLASS_TYPE classType,
                                                    MON_CLASS_LIST_TYPE listType)
   {
      _monClass[classType]->dumpList(cachedMonClassList, listType) ;
   }

   /**
    * Cleanup/Relocate each container's pending archive and pending delete objects
    */
   void relocate() ;

   INT32 fini() ;

private:
   ossPoolVector<monClassContainer*> _monClass;
} ;

typedef _monMonitorManager monMonitorManager ;

extern monMonitorManager *g_monMgrPtr;
}
#endif
