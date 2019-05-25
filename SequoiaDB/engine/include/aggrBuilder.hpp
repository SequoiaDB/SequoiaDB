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

   Source File Name = aggrBuilder.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2013  JHL  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef AGGRBUILDER_HPP__
#define AGGRBUILDER_HPP__

#include "aggrParser.hpp"
#include "pmdEDU.hpp"
#include "qgmPtrTable.hpp"
#include "qgmOptiTree.hpp"
#include "qgmParamTable.hpp"
#include "qgmPlanContainer.hpp"
#include "sdbInterface.hpp"
#include <map>
#include <vector>

using namespace std ;
using namespace bson ;

namespace engine
{
   #define AGGR_PARSER_BEGIN  void aggrBuilder::addParser(){
   #define AGGR_PARSER_END    }
   #define AGGR_PARSER_ADD( parserName, parserClass ) {\
               aggrParser *pObj = SDB_OSS_NEW parserClass();\
               _parserMap.insert( AGGR_PARSER_MAP::value_type(parserName, pObj));}

   typedef std::map< std::string, aggrParser* > AGGR_PARSER_MAP;

   /*
      aggrBuilder define
   */
   class aggrBuilder : public _IControlBlock
   {
   public:
      aggrBuilder();
      ~aggrBuilder();

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_AGGR ; }
      virtual const CHAR* cbName() const { return "AGGRCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;

      INT32 build( const BSONObj &objs, INT32 objNum,
                   const CHAR *pCLName, _pmdEDUCB *cb,
                   SINT64 &contextID  ) ;

   private:
      INT32 buildTree( const BSONObj &objs,
                       INT32 objNum,
                       _qgmOptiTreeNode *&root,
                       _qgmPtrTable * pPtrTable,
                       _qgmParamTable *pParamTable,
                       const CHAR *pCollectionName );

      INT32 createContext( _qgmPlanContainer *container,
                           _pmdEDUCB *cb, SINT64 &contextID );

      void addParser();

   private:
      AGGR_PARSER_MAP         _parserMap ;
   } ;

   class _aggrCmdBase
   {
      public:
         _aggrCmdBase() {}
         virtual ~_aggrCmdBase() {}

      public:
         INT32    appendObj( const BSONObj &obj,
                             CHAR *&pOutputBuffer,
                             INT32 &bufferSize,
                             INT32 &bufUsed,
                             INT32 &buffObjNum ) ;

         INT32    openContext( const CHAR *pObjBuff,
                               INT32 objNum,
                               const CHAR *pInnerCmd,
                               const BSONObj &selector,
                               _pmdEDUCB *cb,
                               SINT64 &contextID ) ;

         INT32    parseUserAggr( const BSONObj &hint,
                                 vector< BSONObj > &vecObj ) ;

         INT32    parseMatcher( const BSONObj &query,
                                BSONObj &nodesMatcher,
                                BSONObj &newMatcher,
                                BOOLEAN ignoreNodeParam = FALSE,
                                BOOLEAN ignoreCtrlParam = FALSE ) ;

   } ;
   typedef _aggrCmdBase aggrCmdBase ;

   /*
      get global aggr cb
   */
   aggrBuilder* sdbGetAggrCB () ;

}

#endif // AGGRBUILDER_HPP__
