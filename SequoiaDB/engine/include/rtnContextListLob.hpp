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
      virtual std::string      name() const { return "LIST_LOB" ; } ;
      virtual RTN_CONTEXT_TYPE getType() const { return RTN_CONTEXT_LIST_LOB ; }
      virtual _dmsStorageUnit*  getSU () ;

   public:
      INT32 open( const BSONObj &condition,
                  _pmdEDUCB *cb ) ;

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
   } ;
   typedef class _rtnContextListLob rtnContextListLob ;
}

#endif

