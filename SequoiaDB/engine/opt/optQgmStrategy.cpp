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

   Source File Name = optQgmStrategy.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          25/04/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "optQgmStrategy.hpp"
#include "optQgmCommStrategy.hpp"
#include "optQgmSpecStrategy.hpp"

namespace engine
{

#define ON_STRATEGY(oprUnitType, nodeType, strategyClass) \
   do { \
      optQgmStrategyBase *pStrategy = NULL ; \
      std::string classTypeName = typeid( strategyClass ).name() ; \
      rc = _typeCheck( oprUnitType, nodeType ) ; \
      if ( SDB_OK != rc ) \
      { \
         goto error ; \
      } \
      if ( _pStrategyTable[oprUnitType][nodeType] ) \
      { \
         PD_LOG( PDERROR, "The point[%d,%d] strategy conflict", oprUnitType, \
                 nodeType ) ; \
         rc = SDB_SYS ; \
         goto error ;\
      } \
      pStrategy = _findFromMap( classTypeName ) ; \
      if ( !pStrategy ) \
      { \
         pStrategy = SDB_OSS_NEW strategyClass() ; \
         if ( !pStrategy ) \
         { \
            rc = SDB_OOM ; \
            goto error ; \
         } \
         _addToMap( classTypeName, pStrategy ) ; \
      } \
      _pStrategyTable[oprUnitType][nodeType] = pStrategy ; \
   } while ( 0 )

#define ON_STRATEGY_RANGE(oprUnitType, nodeStartType, nodeEndType, strategyClass) \
   do { \
      if ( nodeEndType < nodeStartType ) \
      { \
         PD_LOG( PDERROR, "nodeEndType[%d] must >= nodeStartType[%d]", \
                 nodeEndType, nodeStartType ) ; \
         rc = SDB_INVALIDARG ; \
         goto error ; \
      } \
      UINT32 index = nodeStartType ; \
      while ( index < nodeEndType ) \
      { \
         ON_STRATEGY( oprUnitType, index, strategyClass ) ; \
         ++index ; \
      } \
   } while ( 0 )


   optQgmStrategyTable* getQgmStrategyTable ()
   {
      static optQgmStrategyTable s_QgmStrategyTable ;
      return &s_QgmStrategyTable ;
   }

   _optQgmStrategyTable::_optQgmStrategyTable()
   {
      _clear () ;
   }

   _optQgmStrategyTable::~_optQgmStrategyTable()
   {
      _clear () ;

      std::map< std::string, optQgmStrategyBase* >::iterator it =
         _nameToStrategyMap.begin() ;
      while ( it != _nameToStrategyMap.end() )
      {
         SDB_OSS_DEL it->second ;
         ++it ;
      }
      _nameToStrategyMap.clear () ;
   }

   void _optQgmStrategyTable::_clear ()
   {
      UINT32 i = 0 ;
      UINT32 j = 0 ;

      for ( ; i < QGM_OPTI_NODE_MAX ; ++i )
      {
         for ( j = 0 ; j < QGM_OPTI_NODE_MAX ; ++j )
         {
            _pStrategyTable[i][j] = NULL ;
         }
      }
   }

   optQgmStrategyBase* _optQgmStrategyTable::getStrategy( INT32 oprUnitType,
                                                          INT32 nodeType )
   {
      optQgmStrategyBase* pStrategy = NULL ;
      INT32 rc = _typeCheck( oprUnitType, nodeType ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      pStrategy = _pStrategyTable[oprUnitType][nodeType] ;

   done:
      return pStrategy ;
   error:
      goto done ;
   }

   optQgmStrategyBase* _optQgmStrategyTable::_findFromMap( const  string & typeName )
   {
      std::map< std::string, optQgmStrategyBase* >::iterator it =
         _nameToStrategyMap.find( typeName ) ;
      if ( it == _nameToStrategyMap.end() )
      {
         return NULL ;
      }
      return it->second ;
   }

   void _optQgmStrategyTable::_addToMap( const  string & typeName,
                                         optQgmStrategyBase * pStrategy )
   {
      _nameToStrategyMap[typeName] = pStrategy ;
   }

   INT32 _optQgmStrategyTable::init ()
   {
      INT32 rc = SDB_OK ;


      ON_STRATEGY( QGM_OPTI_TYPE_SORT, QGM_OPTI_TYPE_SELECT, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_SORT, QGM_OPTI_TYPE_SORT, optQgmAcceptSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_SORT, QGM_OPTI_TYPE_FILTER, optQgmSortFilterSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_SORT, QGM_OPTI_TYPE_AGGR, optQgmSortAggrSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_SORT, QGM_OPTI_TYPE_SCAN, optQgmAcceptSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_SORT, QGM_OPTI_TYPE_JOIN, optQgmSortJoinSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_SORT, QGM_OPTI_TYPE_MTHMCHSEL, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_SORT, QGM_OPTI_TYPE_MTHMCHSCAN, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_SORT, QGM_OPTI_TYPE_MTHMCHFILTER, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_SORT, QGM_OPTI_TYPE_SPLIT, optQgmRefuseSty ) ;

      ON_STRATEGY( QGM_OPTI_TYPE_FILTER, QGM_OPTI_TYPE_SELECT, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_FILTER, QGM_OPTI_TYPE_SORT, optQgmFilterSortSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_FILTER, QGM_OPTI_TYPE_FILTER, optQgmFilterFilterSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_FILTER, QGM_OPTI_TYPE_AGGR, optQgmFilterAggrSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_FILTER, QGM_OPTI_TYPE_SCAN, optQgmFilterScanSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_FILTER, QGM_OPTI_TYPE_JOIN, optQgmFilterJoinSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_FILTER, QGM_OPTI_TYPE_MTHMCHSEL, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_FILTER, QGM_OPTI_TYPE_MTHMCHSCAN, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_FILTER, QGM_OPTI_TYPE_MTHMCHFILTER, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_FILTER, QGM_OPTI_TYPE_SPLIT, optQgmRefuseSty ) ;

      ON_STRATEGY_RANGE( QGM_OPTI_TYPE_AGGR, QGM_OPTI_TYPE_SELECT,
                         QGM_OPTI_TYPE_JOIN, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_AGGR, QGM_OPTI_TYPE_MTHMCHSEL, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_AGGR, QGM_OPTI_TYPE_MTHMCHSCAN, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_AGGR, QGM_OPTI_TYPE_MTHMCHFILTER, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_AGGR, QGM_OPTI_TYPE_SPLIT, optQgmRefuseSty ) ;


      ON_STRATEGY_RANGE( QGM_OPTI_TYPE_JOIN, QGM_OPTI_TYPE_SELECT,
                         QGM_OPTI_TYPE_JOIN, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_JOIN, QGM_OPTI_TYPE_MTHMCHSEL, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_JOIN, QGM_OPTI_TYPE_MTHMCHSCAN, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_JOIN, QGM_OPTI_TYPE_MTHMCHFILTER, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_JOIN, QGM_OPTI_TYPE_SPLIT, optQgmRefuseSty ) ;

      ON_STRATEGY_RANGE( QGM_OPTI_TYPE_JOIN_CONDITION, QGM_OPTI_TYPE_SELECT,
                         QGM_OPTI_TYPE_SPLIT, optQgmRefuseSty ) ;

      ON_STRATEGY_RANGE( QGM_OPTI_TYPE_INSERT, QGM_OPTI_TYPE_SELECT,
                         QGM_OPTI_TYPE_JOIN, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_INSERT, QGM_OPTI_TYPE_MTHMCHSCAN, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_INSERT, QGM_OPTI_TYPE_MTHMCHFILTER, optQgmRefuseSty ) ;
      ON_STRATEGY( QGM_OPTI_TYPE_INSERT, QGM_OPTI_TYPE_SPLIT, optQgmRefuseSty ) ;


   done:
      return rc ;
   error:
      goto done ;
   }

}


