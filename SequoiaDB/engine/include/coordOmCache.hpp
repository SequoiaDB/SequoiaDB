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

   Source File Name = coordOmCache.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/13/2018  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_OM_CACHE_HPP__
#define COORD_OM_CACHE_HPP__

#include "omStrategyDef.hpp"
#include "ossLatch.hpp"
#include "ossEvent.hpp"

#include <map>
#include <set>

using namespace std ;

namespace engine
{
   class _pmdEDUCB ;

   /*
      Base Type Define
   */
   typedef set<INT64>                           SET_RULEID ;
   typedef SET_RULEID::iterator                 SET_RULEID_IT ;

   typedef multimap< string, INT64 >            MMAP_STR2ID ;
   typedef MMAP_STR2ID::iterator                MMAP_STR2ID_IT ;
   typedef MMAP_STR2ID::const_iterator          MMAP_STR2ID_CIT ;

   typedef map< INT64, omTaskStrategyInfoPtr >  MAP_RULE2STRATEGY ;
   typedef MAP_RULE2STRATEGY::iterator          MAP_RULE2STRATEGY_IT ;

   /*
      _coordTaskInfoItem define
   */
   class _coordTaskInfoItem : public SDBObject
   {
      public:
         _coordTaskInfoItem() ;
         ~_coordTaskInfoItem() ;

         INT32       init( INT64 taskID, const string &taskName ) ;

         UINT32      searchByUser( const string &userName,
                                   SET_RULEID &setID ) const ;

         UINT32      searchByIP( const string &ip,
                                 SET_RULEID &setID ) const ;

         BOOLEAN     getStrategyPtrByID( INT64 ruleID,
                                         omTaskStrategyInfoPtr &ptr ) ;

         omTaskStrategyInfoPtr   getDefaultStrategyPtr() ;

         void        insertStrategy( omTaskStrategyInfoPtr ptr ) ;

      protected:
         UINT32      _search( const MMAP_STR2ID &mapInfo,
                              const string &key,
                              SET_RULEID &setID ) const ;

      private:
         MMAP_STR2ID                _mapUserName ;
         MMAP_STR2ID                _mapIP ;
         MAP_RULE2STRATEGY          _mapStrategy ;
         omTaskStrategyInfoPtr      _defaultPtr ;
   } ;
   typedef _coordTaskInfoItem coordTaskInfoItem ;
   typedef map< string, coordTaskInfoItem* >       MAP_TASK_INFO ;
   typedef MAP_TASK_INFO::iterator                 MAP_TASK_INFO_IT ;

   /*
      _coordOmStrategyAgent define
   */
   class _coordOmStrategyAgent : public SDBObject
   {
      public:
         _coordOmStrategyAgent() ;
         ~_coordOmStrategyAgent() ;

         INT32             init() ;
         void              fini() ;

         BOOLEAN           isValid() const ;
         INT32             getLastVersion() const ;

         void              lock( OSS_LATCH_MODE mode ) ;
         void              unlock( OSS_LATCH_MODE mode ) ;

         void              clear() ;

         INT32             getTaskStrategy( const string &taskName,
                                            const string &userName,
                                            const string &ip,
                                            omTaskStrategyInfoPtr &ptr,
                                            BOOLEAN hasLocked ) ;

         INT32             update( _pmdEDUCB *cb,
                                   INT64 timeout = -1 ) ;

         INT32             waitChange( INT64 millisec ) ;

      protected:
         void              updateLastVersion( INT32 version ) ;

         INT32             insertTaskInfo( const vector<omTaskInfoPtr> &vecTaskInfo,
                                           BOOLEAN hasLocked ) ;

         INT32             insertStrategyInfo( const vector<omTaskStrategyInfoPtr> &vecStrategyInfo,
                                               BOOLEAN hasLocked ) ;

      protected:
         void              _mergeRuleID( const SET_RULEID &left,
                                         const SET_RULEID &right,
                                         SET_RULEID &result ) ;

         BOOLEAN           _findTaskItem( const string &name,
                                          coordTaskInfoItem **ppItem ) ;

         void              _clear( BOOLEAN hasLocked, BOOLEAN needNotify ) ;

      private:
         INT32                         _lastVersion ;
         omTaskStrategyInfoPtr         _defaultPtr ;
         MAP_TASK_INFO                 _mapTaskInfo ;
         ossSpinSLatch                 _latch ;
         ossAutoEvent                  _changeEvent ;

   } ;
   typedef _coordOmStrategyAgent coordOmStrategyAgent ;

}

#endif //COORD_OM_CACHE_HPP__
