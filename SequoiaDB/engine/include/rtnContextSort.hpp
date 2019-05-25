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

   Source File Name = rtnContextSort.hpp

   Descriptive Name = RunTime Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNCONTEXTSORT_HPP_
#define RTNCONTEXTSORT_HPP_

#include "rtnContext.hpp"
#include "rtnSorting.hpp"

namespace engine
{
   class _rtnContextSort : public _rtnContextBase,
                           public _rtnSubContextHolder
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public:
      _rtnContextSort( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextSort() ;

   public:
      virtual std::string      name() const ;
      virtual RTN_CONTEXT_TYPE getType() const ;
      virtual _dmsStorageUnit*  getSU () { return NULL ; }

      OSS_INLINE virtual optAccessPlanRuntime * getPlanRuntime ()
      {
         return &_planRuntime ;
      }

      OSS_INLINE virtual const optAccessPlanRuntime * getPlanRuntime () const
      {
         return &_planRuntime ;
      }

      OSS_INLINE const rtnContext * getSubContext () const
      {
         return _getSubContext() ;
      }

      OSS_INLINE BOOLEAN isInMemorySort () const
      {
         return _sorting.isInMemorySort() ;
      }

      OSS_INLINE const rtnReturnOptions & getReturnOptions () const
      {
         return _returnOptions ;
      }

      INT32 open( const BSONObj &orderBy,
                  rtnContext *context,
                  _pmdEDUCB *cb,
                  SINT64 numToSkip = 0,
                  SINT64 numToReturn = -1 ) ;

      virtual void setQueryActivity ( BOOLEAN hitEnd ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _sortData( _pmdEDUCB *cb );
      INT32 _rebuildSrcContext( const BSONObj &orderBy,
                                rtnContext *srcContext ) ;

   private:
      BSONObj _orderby ;
      _ixmIndexKeyGen _keyGen ;
      BOOLEAN _dataSorted ;
      _rtnSorting _sorting ;
      SINT64 _numToSkip ;
      SINT64 _numToReturn ;
      rtnReturnOptions _returnOptions ;
      optAccessPlanRuntime _planRuntime ;
   } ;
   typedef class _rtnContextSort rtnContextSort ;
}

#endif

