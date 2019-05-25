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

   Source File Name = qgmPlHashJoin.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMPLHASHJOIN_HPP_
#define QGMPLHASHJOIN_HPP_

#include "qgmPlJoin.hpp"
#include "qgmHashTable.hpp"

namespace engine
{
   enum QGM_HJ_FETCH_STATE
   {
      QGM_HJ_FETCH_STATE_BUILD = 0,
      QGM_HJ_FETCH_STATE_PROBE,
      QGM_HJ_FETCH_STATE_FIND,
   } ;

   class _qgmOptiNLJoin ;

   class _qgmPlHashJoin : public _qgmPlJoin
   {
   public:
      _qgmPlHashJoin( INT32 type ) ;
      virtual ~_qgmPlHashJoin() ;

   public:
      virtual string toString()const ;

      INT32 init( _qgmOptiNLJoin *opti ) ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next ) ;

      INT32 _buildHashTbl() ;

   private:
      _qgmHashTable _hashTbl ;
      _qgmPlan *_build ;
      _qgmPlan *_probe ;
      const qgmField *_buildAlias ;
      const qgmField *_probeAlias ;
      std::string _buildKey ;
      std::string _probeKey ;
      QGM_HT_CONTEXT _hashContext ;
      _qgmFetchOut _buildF ;
      _qgmFetchOut _probeF ;
      BSONElement _probeEle ;
      QGM_HJ_FETCH_STATE _state ;
      BOOLEAN _hitBuildEnd ;
   } ;
}

#endif

