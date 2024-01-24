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

   Source File Name = pmdOperator.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Operaotr Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for EDU Control
   Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          11/15/2022  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMDOPERATOR_HPP__
#define PMDOPERATOR_HPP__

#include "sdbInterface.hpp"

#if defined ( SDB_ENGINE )
#include "utilReplSizePlan.hpp"
#include "utilLocation.hpp"
#endif

namespace engine
{

   class _pmdOperator : public _IOperator
   {
   public:
      _pmdOperator()
      {
         _pMsg = NULL ;
#if defined ( SDB_ENGINE )
         _orgReplSize = 1 ;
         _replStrategy = SDB_CONSISTENCY_PRY_LOC_MAJOR ;
         _waitPlan.reset() ;
#endif // SDB_ENGINE
      }
      virtual ~_pmdOperator()
      {
         _pMsg = NULL ;
      }

   public:
      virtual const MsgHeader* getMsg() const
      {
         return _pMsg ;
      }
      virtual const MsgGlobalID& getGlobalID() const
      {
         return _globalID ;
      }
      virtual void updateGlobalID( const MsgGlobalID &globalID )
      {
         _globalID = globalID ;
         if ( _pMsg )
         {
            _pMsg->globalID = _globalID ;
         }
      }
      void setMsg( MsgHeader *pMsg )
      {
         if ( pMsg )
         {
            _pMsg = pMsg ;
            _globalID = _pMsg->globalID ;
         }
      }
      void clearMsg()
      {
         _pMsg = NULL ;
      }

#if defined ( SDB_ENGINE )

      void setOrgReplSize( INT16 replSize )
      {
         _orgReplSize = replSize ;
      }

      INT16 getOrgReplSize() const
      {
         return _orgReplSize ;
      }

      void setReplStrategy( SDB_CONSISTENCY_STRATEGY replStrategy )
      {
         _replStrategy = replStrategy ;
      }

      SDB_CONSISTENCY_STRATEGY getReplStrategy() const
      {
         return _replStrategy ;
      }

      void setWaitplan( UINT8 nodes,
                        const utilLocationInfo &info,
                        BOOLEAN isCriticalLoc = FALSE,
                        BOOLEAN remoteLocationConsistency = TRUE )
      {
         // In critical location mode, we just need to check primary location nodes
         if ( isCriticalLoc )
         {
            _waitPlan.reset() ;
            _waitPlan.primaryLocationNodes = info.primaryLocationNodes ;
         }
         else
         {
            switch ( _replStrategy )
            {
               case SDB_CONSISTENCY_NODE:
                  _waitPlan.setNodeReplSizePlan( nodes, info.affinitiveNodes ) ;
                  break ;
               case SDB_CONSISTENCY_LOC_MAJOR:
                  _waitPlan.setLocMajorReplSizePlan( nodes,
                                                     info.affinitiveLocations,
                                                     info.primaryLocationNodes,
                                                     remoteLocationConsistency ?
                                                     info.locations : info.affinitiveLocations,
                                                     info.affinitiveNodes ) ;
                  break ;
               case SDB_CONSISTENCY_PRY_LOC_MAJOR:
                  _waitPlan.setPryLocMajorReplSizePlan( nodes,
                                                        info.affinitiveLocations,
                                                        info.primaryLocationNodes,
                                                        remoteLocationConsistency ?
                                                        info.locations : info.affinitiveLocations,
                                                        info.affinitiveNodes ) ;
                  break ;
               default:
                  SDB_ASSERT( FALSE, "impossible" ) ;
                  _waitPlan.setNodeReplSizePlan( nodes, info.affinitiveNodes ) ;
            }
         }
      }

      utilReplSizePlan getWaitplan() const
      {
         return _waitPlan ;
      }
#endif // SDB_ENGINE

   private:
      MsgHeader*               _pMsg ;
      MsgGlobalID              _globalID ;
#if defined ( SDB_ENGINE )
      INT16                    _orgReplSize ;
      SDB_CONSISTENCY_STRATEGY _replStrategy ;
      utilReplSizePlan         _waitPlan ;
#endif // SDB_ENGINE
   } ;
   typedef _pmdOperator pmdOperator ;

}

#endif // PMDOPERATOR_HPP__