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

   Source File Name = omStrategyMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/18/2016  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_STRATEGY_MGR_HPP_
#define OM_STRATEGY_MGR_HPP_

#include "pmd.hpp"
#include "pmdEDU.hpp"
#include "dmsCB.hpp"
#include "rtnCB.hpp"
#include "ossLatch.hpp"
#include "omStrategyDef.hpp"
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{
   /*
      _omStrategyMgr define
   */
   class _omStrategyMgr : public SDBObject
   {
      public:
         _omStrategyMgr() ;
         _omStrategyMgr( const _omStrategyMgr &others ) ;
         ~_omStrategyMgr() ;

         INT32 init( pmdEDUCB *cb ) ;

         INT32 queryTasks( SINT64 &contextID,
                           pmdEDUCB *cb ) ;

         INT32 insertTask( omTaskStrategyInfo &strategyInfo,
                           pmdEDUCB *cb ) ;

         INT32 updateTaskNiceById( INT32 newNice,
                                   INT64 id,
                                   pmdEDUCB *cb ) ;

         INT32 addTaskIpsById( const set< string > &ips,
                               INT64 id,
                               pmdEDUCB *cb ) ;

         INT32 delTaskIpsById( const set< string > &ips,
                               INT64 id,
                               pmdEDUCB *cb ) ;

         INT32 delAllTaskIpsById( INT64 id, pmdEDUCB *cb ) ;

         INT32 delTaskById( INT64 id, pmdEDUCB *cb ) ;

      private:

         INT32 getFieldMaxValue( const CHAR *pFieldName,
                                 INT64 &value,
                                 INT64 defaultVal,
                                 pmdEDUCB *cb ) ;

         INT32 incRecordRuleID( INT64 beginID, pmdEDUCB *cb ) ;

         INT32 decRecordRuleID( INT64 beginID, pmdEDUCB *cb ) ;

         INT64 incRuleID() ;

         INT64 decRuleID() ;

         INT32 checkTaskStrategyInfo( omTaskStrategyInfo &strategyInfo,
                                      pmdEDUCB * cb ) ;

         INT32 getTaskID( const string &taskName,
                          INT64 &taskID,
                          pmdEDUCB *cb ) ;

         INT32 getATaskStrategyRecord( BSONObj &recordObj,
                                       const BSONObj &selector,
                                       const BSONObj &matcher,
                                       const BSONObj &orderBy,
                                       pmdEDUCB *cb ) ;

      private:
         ossSpinXLatch                 m_mutex ;
         INT64                         m_curTaskID ;
         INT64                         m_curRuleID ;
         pmdKRCB *                     m_pKrCB ;
         SDB_DMSCB *                   m_pDmsCB ;
         SDB_RTNCB *                   m_pRtnCB ;
   } ;
   typedef _omStrategyMgr omStrategyMgr ;

   omStrategyMgr* omGetStrategyMgr() ;

}

#endif // OM_STRATEGY_MGR_HPP_

