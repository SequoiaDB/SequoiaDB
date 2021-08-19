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

   Source File Name = monClass.hpp

   Descriptive Name = monitor Service Header

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
#ifndef MONCLASS_HPP_
#define MONCLASS_HPP_

#include "monLatch.hpp"
#include "dpsTransLockDef.hpp"
#include "utilPooledObject.hpp"
#include "ossTypes.h"
#include "monLatch.hpp"
#include "ossAtomic.hpp"
#include "ossUtil.hpp"
#include "utilList.hpp"
#include "msg.h"
#include "pd.hpp"
#include "../bson/bson.hpp"
#include <boost/intrusive/list.hpp>
#include <iterator>
#include "monLatch.hpp"

using namespace bson ;

namespace engine
{

class _monAppCB ;

#define MONQUERY_SET_NAME(edu, n)\
  if (edu->getMonQueryCB()) \
  { \
     edu->getMonQueryCB()->name.assign(n); \
  }\

#define MONQUERY_SET_QUERY_TEXT(edu, n)\
  if (edu->getMonQueryCB() && \
      edu->getMonQueryCB()->dataLvl == MON_DATA_LVL_DETAIL )\
  {\
     edu->getMonQueryCB()->queryText.assign(n); \
  }\

typedef enum
{
   MON_CLASS_QUERY = 0,  // _monClassQuery
   MON_CLASS_LATCH,      // _monClassLatch
   MON_CLASS_LOCK,       // _monClassLock
   MON_CLASS_MAX
} MON_CLASS_TYPE ;

// Status for _monClass
#define MON_CLASS_STATUS_NORMAL   0x00
#define MON_CLASS_STATUS_PEND_DEL 0x01
#define MON_CLASS_STATUS_PEND_ARC 0x02

// monitor data capture level
typedef enum
{
   MON_DATA_LVL_NONE = 0,
   MON_DATA_LVL_BASIC,
   MON_DATA_LVL_DETAIL
} MON_DATA_LEVEL ;

struct _monClassBaseData
{
} ;

// Data structure used when constructing a monClassLatch
struct _monClassLatchData : public _monClassBaseData
{
   UINT32 waiterTID ;
   UINT32 ownerTID ;
   MON_LATCH_IDENTIFIER latchID ;
   void* latchAddr ;
   OSS_LATCH_MODE latchMode ;
   UINT32 lastSOwner ;
   UINT32 numOwner ;

   _monClassLatchData()
      : waiterTID( 0 ), ownerTID( 0 ), latchID( MON_LATCH_ID_MAX ),
        latchAddr( NULL ), latchMode( SHARED ), lastSOwner( 0 ),
        numOwner( 0 )
   {}
} ;

typedef _monClassLatchData monClassLatchData ;

typedef boost::intrusive::list_base_hook< > BaseHook ;

/**
 * _monClass - Parent class for all monitor class
 *
 * It is up to the user to define the synchronization strategy for each
 * _monClass. For example, if a metric can be concurrently updated
 * by more than one thread, then the user might want to
 * define that metric as atomic
 */
class _monClass : public BaseHook, public utilPooledObject
{
   ossTimestamp    _endTS ;      /**! end timestamp for this object */
   ossTick  _createTSTick ;      /**! create tick for this object */
   UINT16         _status ;      /**! object status */

protected:
   MON_CLASS_TYPE _type ;      /**! object type */

public:
   //TODO retrieve this directly from container
   MON_DATA_LEVEL    dataLvl ;      /**! monitor data collection level */

   _monClass() ;
   virtual ~_monClass() ;

   /**
    * getType - return the type of the object
    */
   MON_CLASS_TYPE getType () const { return _type; }

   /**
    * setPendingDelete - mark an object to be pending delete
    */
   void setPendingDelete() { _status |= MON_CLASS_STATUS_PEND_DEL ; }
   BOOLEAN isPendingDelete() const
   {
      return ( _status & MON_CLASS_STATUS_PEND_DEL ) ? TRUE : FALSE ;
   }

   /**
    * setPendingArchive - mark an object to be pending archive
    */
   void setPendingArchive() { _status |= MON_CLASS_STATUS_PEND_ARC ; }
   BOOLEAN isPendingArchive() const
   {
      return ( _status & MON_CLASS_STATUS_PEND_ARC ) ? TRUE : FALSE ;
   }

   /**
    * getStatus() - get the current status
    */
   UINT16 getStatus() const { return _status ; }

   /**
    * Get the create timestamp
    */
   ossTimestamp getCreateTS() const
   {
      ossTimestamp ts ;
      _createTSTick.convertToTimestamp( ts ) ;
      return ts ;
   }

   /**
    * Get the end timestamp
    */
   ossTimestamp &getEndTS() { return _endTS ; }

   /**
    * Get the create timestamp tick
    */
   ossTick getCreateTSTick() const { return _createTSTick ; }

   /**
    * Format object into a BSON
    */
   virtual void dump( BSONObj &obj ) = 0 ;

   /**
    * Reset metrics
    */
   virtual void reset() = 0 ;
} ;

typedef _monClass monClass ;

/**
 * Template to ensure all subclasses implement the getType function
 */
template<class T>
class monClassTemplate : public _monClass
{
protected:
   monClassTemplate() {}

public:
   ~monClassTemplate()
   {
      T::getType();
   }
} ;

/**
 * archive query information based on response time
 */
BOOLEAN monArchiveQuery ( _monClass *obj ) ;

/**
 * archive latch information based on wait time
 */
BOOLEAN monArchiveLatch ( _monClass *obj ) ;

BOOLEAN monArchiveLock ( _monClass *obj ) ;

/**
 * Default function pointer to indicate no archive
 */
BOOLEAN monNoArchive ( _monClass *obj ) ;

typedef BOOLEAN (*archiveFunc)(_monClass *) ;

typedef enum
{
   MON_CLASS_ACTIVE_LIST,
   MON_CLASS_ARCHIVED_LIST
} MON_CLASS_LIST_TYPE ;

/*
 * Structure to help compute the delta from monAppCB
 */
struct _monClassQueryTmpData
{
   UINT32            dataRead ;
   UINT32           indexRead ;
   UINT32           dataWrite ;
   UINT32           indexWrite ;

   _monClassQueryTmpData()
      : dataRead(0), indexRead(0), dataWrite(0), indexWrite(0)
   {}

   _monClassQueryTmpData& operator=(const _monAppCB& cb) ;

   void diff(_monAppCB &cb) ;
} ;
typedef _monClassQueryTmpData monClassQueryTmpData ;

/**
 * Capture metrics about a query
 */
class _monClassQuery : public monClassTemplate<_monClassQuery>
{
public:
   UINT32                 tid ;  /**! TID of the EDU */
   BSONObj         clientInfo ;  /**! Information of Client which connects to engine */
   UINT32           clientTID ;  /**! OS TID of Client which connects to engine */
   ossPoolString   clientHost ;  /**! Client IP */
   ossPoolString         name ;  /**! The target object name of this query */
   SINT64        accessPlanID ;  /**! Access plan ID used by the query */
   UINT32              opCode ;  /**! Message opCode */
   UINT32           sessionID ;  /**! EDU Session ID */
   ossTickDelta  responseTime ;  /**! Response time of the query */
   ossTickDelta latchWaitTime ;  /**! Time spent on latch wait */
   ossTickDelta  lockWaitTime ;  /**! Time spent on lock wait */
   UINT32            dataRead ;  /**! Total data read (record)*/
   UINT32           indexRead ;  /**! Total index read (record)*/
   UINT32             lobRead ;  /**! Total LOB read */
   UINT32           dataWrite ;  /**! Total data write (record) */
   UINT32          indexWrite ;  /**! Total index write (record) */
   UINT32            lobWrite ;  /**! Total LOB write */
   UINT32        rowsReturned ;  /**! Total number of rows returned */
   UINT32          numMsgSent ;  /**! Total # of msgs sent to remote nodes */
   std::set<UINT32>     nodes ;  /**! Node ID where messages were sent to */
   MsgRouteID      relatedNID ;  /**! coordinator node node ID */
   UINT32          relatedTID ;  /**! coordinator node edu TID */
   BOOLEAN    anchorToContext ;  /**! Whether this obj anchored to a context */
   ossPoolString    queryText ;  /**! Full query text */
   ossTickDelta remoteNodesResponseTime ; /*! Time spent waiting remote nodes */
   ossTickDelta msgSentTime ;    /**! Time spent sending msgs to remote nodes */

   _monClassQuery ()
     :  clientTID( 0 ),
        accessPlanID( -1 ),
        opCode( 0 ),
        sessionID( 0 ),
        dataRead( 0 ),
        indexRead( 0 ),
        lobRead( 0 ),
        dataWrite( 0 ),
        indexWrite( 0 ),
        lobWrite( 0 ),
        rowsReturned( 0 ),
        numMsgSent( 0 ),
        relatedTID( 0 ),
        anchorToContext( FALSE )
   {
      _type = MON_CLASS_QUERY ;
      relatedNID.value = 0 ;
   }
   static MON_CLASS_TYPE getType () { return MON_CLASS_QUERY ; }

   //TODO: to be implemented
   virtual void dump( BSONObj &obj ) {}

   //TODO: to be implemented
   virtual void reset() {}

   void startLatchTimer() { _latchWaitTimer.sample() ; }

   void stopLatchTimer()
   {
      ossTick tick ;
      tick.sample() ;
      latchWaitTime += (tick - _latchWaitTimer) ;
   }

   void incMetrics( monClassQueryTmpData &tmpData )
   {
      dataRead += tmpData.dataRead ;
      dataWrite += tmpData.dataWrite ;
      indexRead += tmpData.indexRead ;
      indexWrite += tmpData.indexWrite ;
   }

private:
   ossTick _latchWaitTimer ;
} ;

typedef _monClassQuery monClassQuery ;

class _monClassLatch : public monClassTemplate<_monClassLatch>
{
public:
   _monClassLatch ()
      : xOwnerTID( 0 ), waiterTID( 0 ), latchID( MON_LATCH_ID_MAX ),
        latchAddr( NULL ), numOwner( 0 ), lastSOwner( 0 )
   {
      _type = MON_CLASS_LATCH ;
   }

   _monClassLatch (_monClassBaseData *data)
   {
      if ( data )
      {
         _monClassLatchData *latchData = (_monClassLatchData *) data ;
         xOwnerTID = latchData->ownerTID ;
         waiterTID = latchData->waiterTID ;
         latchID = latchData->latchID ;
         latchAddr = latchData->latchAddr ;
         latchMode = latchData->latchMode ;
         numOwner = latchData->numOwner ;
         lastSOwner = latchData->lastSOwner ;
      }
      _type = MON_CLASS_LATCH ;
   }
   static MON_CLASS_TYPE getType () { return MON_CLASS_LATCH ; }

   virtual void dump( BSONObj &obj ) {}

   virtual void reset() {}

   ossTickDelta waitTime ;
   UINT32 xOwnerTID ;
   UINT32 waiterTID ;
   MON_LATCH_IDENTIFIER latchID ;
   void *latchAddr ;
   OSS_LATCH_MODE latchMode ;
   UINT32 numOwner ;
   UINT32 lastSOwner ;
} ;

typedef _monClassLatch monClassLatch ;

class _monClassLock : public monClassTemplate<_monClassLock>
{
public:
   _monClassLock ()
      : xOwnerTID( 0 ), waiterTID( 0 ), numOwner( 0 ),
        lockMode( DPS_TRANSLOCK_MAX )
   {
      _type = MON_CLASS_LOCK ;
      waiterTID = 0 ;
   }

   static MON_CLASS_TYPE getType () { return MON_CLASS_LOCK ; }

   virtual void dump( BSONObj &obj ) {}

   virtual void reset() {}

   ossTickDelta waitTime ;
   UINT32 xOwnerTID ;
   UINT32 waiterTID ;
   UINT32 numOwner ;
   dpsTransLockId lockID ;
   DPS_TRANSLOCK_TYPE lockMode ;
} ;

typedef _monClassLock monClassLock ;

UINT32 monGetTID() ;

/**
 * _monClassContainer defines
 *
 * It is a container for a _monClass type. Contains an active list and
 * archived list.
 * See below for latch protocol for these two lists.
 */
class _monClassContainer : public SDBObject
{
   friend class _monMonitorManager ;

   typedef boost::intrusive::list< _monClass, boost::intrusive::base_hook<BaseHook> > MONCLASS_LIST ;
   typedef _utilPartitionList< monClass, MONCLASS_LIST, monGetTID > MON_PARTITION_LIST ;

public:
   class iterator
   {
      friend class _monClassContainer ;
      _monClassContainer* _container ; // Pointer to monClassContainer
      MON_CLASS_LIST_TYPE _listType ; // which list this itr plan to scan
      MON_CLASS_LIST_TYPE _curListType ;  // current type of the list it is scanning
      MON_PARTITION_LIST::iterator _itr1 ; // active list iterator
      MONCLASS_LIST::iterator _itr2 ; // archive list iterator

      iterator( MON_PARTITION_LIST::iterator it )
      {
         _curListType = MON_CLASS_ACTIVE_LIST ;
         _itr1 = it ;
      }

      iterator( MONCLASS_LIST::iterator it )
      {
         _curListType = MON_CLASS_ARCHIVED_LIST ;
         _itr2 = it ;
      }

      // copy constructor
      iterator( const _monClassContainer::iterator &obj )
      {
         (*this) = obj ;
      }

   public:
      iterator() {}
      ~iterator()
      {
      }
      // By calling the constructor, the list type and the target container
      // pointer has to be passed in to initialize the member inside the object
      // Also the constructor decides which list to traverse and which
      // iterator to be used based on the list type.
      // If the list type is active list. _itr1 is used and _curlist is set to
      // active list to indicate which list we are traversing
      // If the list type is archived list. First check whether there is
      // pending archive, if so _curlist is set to active list first
      // and _itr1 is uesed to traverse first, after we done the active list
      // we will switch to archived list in ++ operation
      iterator( MON_CLASS_LIST_TYPE type, _monClassContainer* classContainer )
         : _container( classContainer ), _listType( type )
      {
         if ( _listType == MON_CLASS_ARCHIVED_LIST )
         {
            if ( _container->getNumPendingArchive() > 0 )
            {
               _itr1 = _container->_activeList.begin() ;
               // Since scan is paralle to delete, it is possible that
               // when we get the itr, it already hit the end of active list
               // if that happens, we need to switch to archived list
               if ( _itr1 != _container->_activeList.end() )
               {
                  _curListType = MON_CLASS_ACTIVE_LIST ;
               }
               else
               {
                  _curListType = MON_CLASS_ARCHIVED_LIST ;
                  _itr2 = _container-> _archivedList.begin() ;
               }
            }
            else
            {
               _curListType = MON_CLASS_ARCHIVED_LIST ;
               _itr2 = _container->_archivedList.begin() ;
               _itr1 = _container->_activeList.end() ;
            }
         }
         else
         {
            _curListType = MON_CLASS_ACTIVE_LIST ;
            _itr1 = _container->_activeList.begin() ;
            _itr2 = _container->_archivedList.end() ;
         }
      }

      // increment the iterator of current list it is traversing
      // if we hit the end of active list and is scan archived list
      // switch to the begin of _archivedList
      iterator& operator++ () // Pre-increment
      {
         if ( _curListType == MON_CLASS_ACTIVE_LIST )
         {
            SDB_ASSERT( _itr1 != _container->_activeList.end(),
                        "end of active list has already been reached" ) ;

            // Move to scan archive list if the end of active list is reached
            if ( ++_itr1 == _container->_activeList.end() &&
                _listType == MON_CLASS_ARCHIVED_LIST )
            {
               _curListType = MON_CLASS_ARCHIVED_LIST ;
               _itr2 = _container-> _archivedList.begin() ;
            }
         }
         else
         {
            SDB_ASSERT( _itr2 != _container->_archivedList.end(),
                        "end of archived list has already been reached" ) ;
            ++_itr2 ;
         }
         return *this;
      }

      iterator operator++ (int) // Post-increment
      {
         iterator tmp( *this ) ;
         ++( *this ) ;
         return tmp ;
      }

      BOOLEAN operator == (const iterator& rhs) const
      {
         if ( _curListType == rhs._curListType )
         {
            if ( _curListType == MON_CLASS_ACTIVE_LIST )
            {
               return _itr1 == rhs._itr1 ;
            }
            else
            {
               return _itr2 == rhs._itr2 ;
            }
         }
         return FALSE ;
      }

      BOOLEAN operator != (const iterator& rhs) const
      {
         return operator==( rhs ) ? FALSE : TRUE ;
      }

      MONCLASS_LIST::reference operator* () const
      {
         return ( _curListType == MON_CLASS_ACTIVE_LIST ) ? *_itr1 : *_itr2 ;
      }

      MONCLASS_LIST::pointer operator-> () const
      {
         return ( _curListType == MON_CLASS_ACTIVE_LIST ) ? &(*_itr1) : &(*_itr2) ;
      }
   } ;

   INT32 cleanup()
   {

      setMaxArchivedListLen( 0 ) ;
      setMonitorLvl( MON_DATA_LVL_NONE ) ;

      MON_PARTITION_LIST::iterator it = _activeList.begin() ;

      while ( it != _activeList.end() )
      {
         monClass &obj = *it ;
         it = _activeList.erase(it) ;
         SDB_OSS_DEL &obj ;
      }

      MONCLASS_LIST::iterator it2 = _archivedList.begin() ;

      while ( it2 != _archivedList.end() )
      {
         monClass &obj = *it2 ;
         it2 = _archivedList.erase(it2) ;
         SDB_OSS_DEL &obj ;
      }
      return SDB_OK ;
   }

   ~_monClassContainer ()
   {
      cleanup() ;
   }

private:
   _monClassContainer (MON_CLASS_TYPE type) ;

   // disable assignment/copy
   _monClassContainer& operator= (const _monClassContainer& other) ;
   _monClassContainer( const _monClassContainer& other ) ;

   // a list of all the active objects
   MON_PARTITION_LIST _activeList ;

   // a list of all the archived objects
   MONCLASS_LIST _archivedList ;

   // Latch protecting the archived list. If active list and archived list must
   // be accessed at the same time, get the archive latch before begin scanning
   // the active list.
   ossSpinSLatch _archiveListLatch ;

   /**< number of elements in the archived list, protected by archivedListLatch */
   UINT32 _archivedListMaxLen ;
   /**< maximum number of elements allowed in the archived list */

   /**< Number of pending archive objects in the active list */
   ossAtomic32 _numPendingArchive ;
   /**< Number of pending delete objects in the active list */
   ossAtomic32 _numPendingDelete ;

   /**< The current data collection level for this class */
   MON_DATA_LEVEL _curCollectionLvl ; 
   /**< The minimum collection level when monClass objects will get created */
   MON_DATA_LEVEL _minOperationalLvl ;

   archiveFunc _doArchive ;

   /*
    * Process pending archive/delete objects from the active list
    */
   void _processPendingObj() ;

   /*
    * Remove archived objects when the number of obj exceeds the capacity
    */
   void _removeArchivedObj() ;

public:

   UINT32 getArchivedListLen() const { return _archivedList.size() ; }

   UINT32 getActiveListLen() { return _activeList.size() ; }

   void setMaxArchivedListLen( UINT32 size ) { _archivedListMaxLen = size ; }
   UINT32 getMaxArchivedListLen() const { return _archivedListMaxLen ; }

   UINT32 getNumPendingArchive() { return _numPendingArchive.fetch() ; }

   UINT32 getNumPendingDelete() { return _numPendingDelete.fetch() ; }

   void getArchiveLatch ( OSS_LATCH_MODE latchMode )
   {
      latchMode == EXCLUSIVE ? _archiveListLatch.get()
                             : _archiveListLatch.get_shared() ;
   }

   void releaseArchiveLatch ( OSS_LATCH_MODE latchMode )
   {
      latchMode == EXCLUSIVE ? _archiveListLatch.release()
                             : _archiveListLatch.release_shared() ;
   }

   void setMonitorLvl( MON_DATA_LEVEL mode )
   {
      _curCollectionLvl = mode ;
   }
   MON_DATA_LEVEL getCollectionLvl() { return _curCollectionLvl ; }

   BOOLEAN isOperational()
   {
      return ( _curCollectionLvl >= _minOperationalLvl )? TRUE: FALSE ;
   }

   /**
    * Add an object to the active list
    * @return The object added to the container
    */
   template<class T>
   T* add ()
   {
      T* obj = NULL ;
      if ( isOperational() )
      {
         obj = SDB_OSS_NEW T() ;
         if ( obj )
         {
            obj->dataLvl = getCollectionLvl() ;
            _activeList.add( obj ) ;
         }
         else
         {
            PD_LOG( PDERROR, "failed to allocate new monitor cb" ) ;
         }
      }
      return obj ;
   }

   /**
    * Add an object to the active list
    * @return The object added to the container
    */
   template<class T>
   T* add (_monClassBaseData *data)
   {
      T* obj = NULL ;
      if ( isOperational() )
      {
         obj = SDB_OSS_NEW T(data) ;
         if ( obj )
         {
            obj->dataLvl = getCollectionLvl() ;
            _activeList.add( obj ) ;
         }
         else
         {
            PD_LOG( PDERROR, "failed to allocate new monitor cb" ) ;
         }
      }
      return obj ;
   }

   /**
    * Remove an object from the active list. The object will get archived based
    * on the archive rule.
    * @param obj the object to be added
    */
   void remove ( _monClass *obj ) ;

   /**
    * Return iterator to the first node of the list

    * Dependency: proper latches will been take care by iteraor
    * @param listType the type of list to read
    */
   iterator begin( MON_CLASS_LIST_TYPE listType )
   {
      SDB_ASSERT ( listType == MON_CLASS_ARCHIVED_LIST ||
                   listType == MON_CLASS_ACTIVE_LIST,
                   "Invalid monitor class list type" ) ;
      switch ( listType )
      {
         case ( MON_CLASS_ARCHIVED_LIST ):
            return iterator( MON_CLASS_ARCHIVED_LIST, this ) ;
         case ( MON_CLASS_ACTIVE_LIST ):
         default:
            return iterator( MON_CLASS_ACTIVE_LIST, this ) ;
      }
   }

   iterator end( MON_CLASS_LIST_TYPE listType )
   {
      SDB_ASSERT ( listType == MON_CLASS_ARCHIVED_LIST ||
                   listType == MON_CLASS_ACTIVE_LIST,
                   "Invalid monitor class list type" ) ;

      if ( listType == MON_CLASS_ARCHIVED_LIST )
      {
         return iterator( _archivedList.end() ) ;
      }
      else
      {
         return iterator( _activeList.end() ) ;
      }
   }
} ;

typedef _monClassContainer monClassContainer ;

class _monClassReadScanner : public utilPooledObject
{
private:
   typedef _monClassContainer::iterator IT ;

   monClassContainer *_container ; /**< container for the type of monClass */
   IT _itr ;                       /**< iterator pointing to current node */
   MON_CLASS_LIST_TYPE _listType ; /**< Type of list to scan (active/archive) */
   BOOLEAN _initCalled ;           /**< Whether scan initialization has done */
   BOOLEAN _endReached ;         /**< Whether the scan has read all the nodes */
   BOOLEAN _hasArchiveLatch ;      /**< Whether the archive latch was acquired */

   void initScan() ;

public:
   _monClassReadScanner( monClassContainer *container,
                         MON_CLASS_LIST_TYPE listType )
     : _container( container ),
       _listType( listType ),
       _initCalled( FALSE ),
       _endReached( FALSE ),
       _hasArchiveLatch( FALSE )
   {
   }

   ~_monClassReadScanner()
   {
      if ( _hasArchiveLatch )
      {
         _container->releaseArchiveLatch( SHARED ) ;
      }
   }

   /**
    * Return the next node
    */
   _monClass* getNext() ;
} ;

typedef _monClassReadScanner monClassReadScanner ;

} // namespace engine

#endif //MONCLASS_HPP_
