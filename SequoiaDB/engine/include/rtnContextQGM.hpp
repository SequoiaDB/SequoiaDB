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

   Source File Name = rtnContextQGM.hpp

   Descriptive Name = RunTime QGM Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.hpp

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXT_QGM_HPP_
#define RTN_CONTEXT_QGM_HPP_

#include "rtnContext.hpp"

namespace engine
{
   class _qgmPlanContainer ;
   
   /*
      _rtnContextQGM define
   */
   class _rtnContextQGM : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
      public:
         _rtnContextQGM ( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextQGM () ;

      public:
         virtual const CHAR*      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;
         virtual _dmsStorageUnit* getSU () { return NULL ; }

         INT32 open( _qgmPlanContainer *accPlan ) ;

      protected:
         virtual INT32   _prepareData( _pmdEDUCB *cb ) ;

      private:
         _qgmPlanContainer          *_accPlan ;

   } ;
   typedef _rtnContextQGM rtnContextQGM ;

   class _qgmPlan ;

   class _rtnContextQgmSort : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public:
      _rtnContextQgmSort( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextQgmSort() ;

   public:
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const ;
      virtual _dmsStorageUnit* getSU () { return NULL ; }

      INT32 open( _qgmPlan *qp ) ;

   protected:
      virtual INT32   _prepareData( _pmdEDUCB *cb ) ;

   private:
      _qgmPlan *_qp ;
   } ;
   typedef class _rtnContextQgmSort rtnContextQgmSort ;
}

#endif /* RTN_CONTEXT_QGM_HPP_ */

