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

   Source File Name = rtnContextListLob.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/19/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_CONTEXTLISTLOB_HPP_
#define RTN_CONTEXTLISTLOB_HPP_

#include "rtnContext.hpp"
#include "rtnLobFetcher.hpp"

namespace engine
{
   class _rtnContextListLob : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public:
      _rtnContextListLob( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextListLob() ;

   public:
      virtual const CHAR*      name() const { return "LIST_LOB" ; } ;
      virtual RTN_CONTEXT_TYPE getType() const { return RTN_CONTEXT_LIST_LOB ; }
      virtual _dmsStorageUnit*  getSU () ;

   public:
      INT32 open( const BSONObj &query, const BSONObj &selector,
                  const BSONObj &hint, INT64 skip,
                  INT64 returnNum, _pmdEDUCB *cb ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _getMetaInfo( _pmdEDUCB *cb, BSONObj &obj ) ;
      INT32 _getSequenceInfo( _pmdEDUCB *cb, BSONObj &obj ) ;
      INT32 _reallocate( UINT32 len ) ;
   private:
      _rtnLobFetcher _fetcher ;
      CHAR *_buf ;
      UINT32 _bufLen ;
      std::string _fullName ;
      BOOLEAN _fetchLobHead ;
      BSONObj _query ;
      BSONObj _selector ;
      BSONObj _hint ;
      INT64 _skip ;
      INT64 _returnNum ;

      _mthSelector _selectorParser ;
      _mthMatchTree _matchTree ;
   } ;
   typedef class _rtnContextListLob rtnContextListLob ;
}

#endif

