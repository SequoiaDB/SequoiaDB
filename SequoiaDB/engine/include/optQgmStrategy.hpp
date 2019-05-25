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

   Source File Name = optQgmStrategy.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          25/04/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OPT_QGM_STRATEGY_HPP_
#define OPT_QGM_STRATEGY_HPP_

#include "qgmOptiTree.hpp"

namespace engine
{

   enum OPT_QGM_SS_RESULT
   {
      OPT_SS_PROCESSED  = 0,
      OPT_SS_ACCEPT     = 1,
      OPT_SS_TAKEOVER   = 2,
      OPT_SS_REFUSE     = 3
   };

   class _optQgmStrategyBase : public SDBObject
   {
      public:
         _optQgmStrategyBase () {}
         virtual ~_optQgmStrategyBase () {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) = 0 ;

         virtual const CHAR* strategyName() const = 0 ;

   };
   typedef _optQgmStrategyBase optQgmStrategyBase ;

   class _optQgmStrategyTable : public SDBObject
   {
      public:
         _optQgmStrategyTable () ;
         ~_optQgmStrategyTable () ;

         optQgmStrategyBase* getStrategy( INT32 oprUnitType,
                                          INT32 nodeType ) ;

         INT32    init () ;

      protected:
         void   _clear () ;
         OSS_INLINE INT32  _typeCheck ( INT32 oprUnitType, INT32 nodeType ) ;
         optQgmStrategyBase* _findFromMap( const std::string &typeName ) ;
         void                _addToMap ( const std::string &typeName,
                                         optQgmStrategyBase *pStrategy ) ;

      private:
         optQgmStrategyBase *_pStrategyTable[QGM_OPTI_NODE_MAX][QGM_OPTI_NODE_MAX] ;
         std::map< std::string, optQgmStrategyBase* > _nameToStrategyMap ;

   };
   typedef _optQgmStrategyTable optQgmStrategyTable ;

   OSS_INLINE INT32 _optQgmStrategyTable::_typeCheck( INT32 oprUnitType,
                                                  INT32 nodeType )
   {
      INT32 rc = SDB_OK ;

      if ( oprUnitType < 0 || oprUnitType >= QGM_OPTI_NODE_MAX )
      {
         PD_LOG( PDERROR, "oprUnitType[%d] error", oprUnitType ) ;
         rc = SDB_INVALIDARG ;
      }
      else if ( nodeType < 0 || nodeType >= QGM_OPTI_NODE_MAX )
      {
         PD_LOG( PDERROR, "nodeType[%d] error", nodeType ) ;
         rc = SDB_INVALIDARG ;
      }

      return rc ;
   }

   optQgmStrategyTable* getQgmStrategyTable() ;

}

#endif //OPT_QGM_STRATEGY_HPP_

