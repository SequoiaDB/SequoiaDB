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

   Source File Name = pmdSessionBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/12/2020  LYB  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdSessionBase.hpp"
#include "pmdEDU.hpp"
#include "pmdEnv.hpp"
#include "pmd.hpp"
#include "dpsLogWrapper.hpp"
#include "netFrame.hpp"

namespace engine
{
   /*
      _pmdSessionBase implement
   */
   _pmdSessionBase::_pmdSessionBase() : _processor( NULL )
   {
   }

   _pmdSessionBase::~_pmdSessionBase()
   {
      _processor = NULL ;
   }

   IProcessor* _pmdSessionBase::getProcessor()
   {
      return _processor ;
   }

   void _pmdSessionBase::attachProcessor( _pmdProcessorBase *pProcessor )
   {
      SDB_ASSERT( NULL != pProcessor, "Processor can't be NULL" ) ;
      _processor = pProcessor ;
      _processor->_attachSession( this ) ;
   }

   void _pmdSessionBase::detachProcessor()
   {
      SDB_ASSERT( _processor, "Processor can't be NULL" ) ;
      _processor->_detachSession() ;
      _processor = NULL ;
   }

   IOperator* _pmdSessionBase::getOperator()
   {
      if ( eduCB() )
      {
         return eduCB()->getOperator() ;
      }
      return NULL ;
   }

   BOOLEAN _pmdSessionBase::checkPrivilegesForCmd( const CHAR *cmdName,
                                                   const CHAR *pQuery,
                                                   const CHAR *pSelector,
                                                   const CHAR *pOrderby,
                                                   const CHAR *pHint )
   {
      return SDB_OK ;
   }

   INT32 _pmdSessionBase::checkPrivilegesForActionsOnExact( const CHAR *pCollectionName,
                                                            const authActionSet &actions )
   {
      return SDB_OK;
   }

   INT32 _pmdSessionBase::checkPrivilegesForActionsOnCluster( const authActionSet &actions )
   {
      return SDB_OK;
   }

   INT32 _pmdSessionBase::checkPrivilegesForActionsOnResource(
      const boost::shared_ptr< authResource > &,
      const authActionSet &actions )
   {
      return SDB_OK;
   }

   BOOLEAN _pmdSessionBase::privilegeCheckEnabled()
   {
      return getClient()->privCheckEnabled() && pmdGetOptionCB()->privilegeCheckEnabled();
   }

   INT32 _pmdSessionBase::getACL( boost::shared_ptr< const authAccessControlList > &acl )
   {
      PD_LOG( PDERROR, "get ACL failed, not supported on this session type %d", sessionType() );
      acl.reset();
      return SDB_SYS;
   }

   /*
      _pmdProcessorBase implement
   */
   _pmdProcessorBase::_pmdProcessorBase() : _pSession( NULL )
   {
   }

   _pmdProcessorBase::~_pmdProcessorBase()
   {
      _pSession = NULL ;
   }

   void _pmdProcessorBase::_attachSession( pmdSessionBase *pSession )
   {
      SDB_ASSERT( pSession, "Session can't be NULL" ) ;
      _pSession = pSession ;
      _onAttach() ;
   }

   void _pmdProcessorBase::_detachSession()
   {
      SDB_ASSERT( _pSession, "Session can't be NULL" ) ;
      if ( NULL != eduCB() )
      {
         eduCB()->getMonAppCB()->setSvcTaskInfo( NULL ) ;
      }

      _onDetach() ;
      _pSession = NULL ;
   }

   _dpsLogWrapper* _pmdProcessorBase::getDPSCB()
   {
      if ( _pSession )
      {
         return _pSession->getDPSCB() ;
      }
      return NULL ;
   }

   _IClient* _pmdProcessorBase::getClient()
   {
      if ( _pSession )
      {
         return _pSession->getClient() ;
      }
      return NULL ;
   }

   _pmdEDUCB* _pmdProcessorBase::eduCB()
   {
      if ( _pSession )
      {
         return _pSession->eduCB() ;
      }
      return NULL ;
   }

   EDUID _pmdProcessorBase::eduID() const
   {
      if ( _pSession )
      {
         return _pSession->eduID() ;
      }
      return PMD_INVALID_EDUID ;
   }

}



