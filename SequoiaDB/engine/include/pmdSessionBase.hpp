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

   Source File Name = pmdSessionBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/12/2020  LYB  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_SESSION_BASE_HPP_
#define PMD_SESSION_BASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pmdOperationContext.hpp"
#include "sdbInterface.hpp"
#include "pmdIProcessor.hpp"
#include "pmdOperator.hpp"

namespace engine
{

   class _pmdEDUCB ;
   class _dpsLogWrapper ;
   class _pmdProcessorBase ;

   /*
      _pmdSessionBase define
   */
   class _pmdSessionBase : public _ISession
   {
   public:
      _pmdSessionBase() ;
      virtual ~_pmdSessionBase() ;

   public:
      virtual _pmdEDUCB*      eduCB () const = 0 ;
      virtual EDUID           eduID () const = 0 ;

      virtual BOOLEAN         isBusinessSession() const { return FALSE ; }
      virtual IProcessor*     getProcessor() ;
      virtual _dpsLogWrapper* getDPSCB() { return NULL ; }
      virtual void            attachProcessor( _pmdProcessorBase *pProcessor ) ;
      virtual void            detachProcessor() ;
      virtual IOperator*      getOperator() ;

      virtual INT32 checkPrivilegesForCmd( const CHAR *cmdName,
                                           const CHAR *pQuery,
                                           const CHAR *pSelector,
                                           const CHAR *pOrderby,
                                           const CHAR *pHint );

      virtual INT32 checkPrivilegesForActionsOnExact( const CHAR *pCollectionName,
                                                      const authActionSet &actions );

      virtual INT32 checkPrivilegesForActionsOnCluster( const authActionSet &actions );

      virtual INT32 checkPrivilegesForActionsOnResource( const boost::shared_ptr< authResource > &,
                                                         const authActionSet &actions );

      virtual BOOLEAN privilegeCheckEnabled();

      virtual INT32 getACL( boost::shared_ptr<const authAccessControlList> &acl );

      virtual IOperationContext *getOperationContext()
      {
         return &_operationContext ;
      }

   protected:
      _pmdProcessorBase *_processor ;
      pmdOperationContext _operationContext ;
   } ;

   typedef _pmdSessionBase pmdSessionBase ;

   class _pmdProcessorBase : public _IProcessor
   {
   friend class _pmdSessionBase ;
   public:
      _pmdProcessorBase() ;
      virtual ~_pmdProcessorBase() ;

      virtual ISession* getSession() { return _pSession ; }

   protected:
      virtual void      _attachSession( pmdSessionBase *pSession ) ;
      virtual void      _detachSession() ;

      _dpsLogWrapper*   getDPSCB() ;
      _IClient*         getClient() ;
      _pmdEDUCB*        eduCB() ;
      EDUID             eduID() const ;

   protected:
      pmdSessionBase*   _pSession ;
   } ;
   typedef _pmdProcessorBase pmdProcessorBase ;
}

#endif //PMD_SESSION_BASE_HPP_

