/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnContextDump.hpp

   Descriptive Name = RunTime Dump Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/26/2017  David Li  Split from rtnContext.hpp

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXT_DUMP_HPP_
#define RTN_CONTEXT_DUMP_HPP_

#include "rtnContext.hpp"
#include "rtnFetchBase.hpp"
#include "mthMatchRuntime.hpp"

namespace engine
{
   /*
      _rtnContextDump define
   */
   class _rtnContextDump : public _rtnContextBase,
                           public _mthMatchTreeHolder
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
      public:
         _rtnContextDump ( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextDump () ;

         INT32 open ( const BSONObj &selector, const BSONObj &matcher,
                      INT64 numToReturn = -1, INT64 numToSkip = 0 ) ;

         INT32 monAppend( const BSONObj &result ) ;

         void  setMonFetch( rtnFetchBase *pFetch, BOOLEAN ownned ) ;
         rtnFetchBase* getMonFetch() { return _pFetch ; }
         BOOLEAN isMonFetchOwnned() const { return _ownnedFetch ; }

         INT64 getNumToReturn() const { return _numToReturn ; }

      public:
         virtual std::string      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;
         virtual _dmsStorageUnit* getSU () { return NULL ; }

      protected:
         virtual INT32  _prepareData( _pmdEDUCB *cb ) ;
         virtual void   _toString( stringstream &ss ) ;

      private:
         SINT64                     _numToReturn ;
         SINT64                     _numToSkip ;

         rtnFetchBase               *_pFetch ;
         BOOLEAN                    _ownnedFetch ;

   } ;
   typedef _rtnContextDump rtnContextDump ;
}

#endif /* RTN_CONTEXT_DUMP_HPP_ */

