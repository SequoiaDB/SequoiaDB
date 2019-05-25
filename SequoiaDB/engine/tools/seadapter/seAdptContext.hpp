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

   Source File Name = seAdptContext.hpp

   Descriptive Name = Context on search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_CONTEXT__
#define SEADPT_CONTEXT__

#include "pmdEDU.hpp"
#include "utilCommObjBuff.hpp"
#include "utilESClt.hpp"
#include "rtnSimpleCondParser.hpp"

using namespace engine ;

namespace seadapter
{
   enum _seadptQueryRebldType
   {
      SE_QUERY_REBLD_QUERY = 1,
      SE_QUERY_REBLD_SEL,
      SE_QUERY_REBLD_ORD,
      SE_QUERY_REBLD_HINT,
   } ;

   typedef std::map<_seadptQueryRebldType, const BSONObj*> REBUILD_ITEM_MAP ;
   typedef const std::map<_seadptQueryRebldType, const BSONObj*>::iterator REBUILD_ITEM_MAP_ITR ;

   /* Query rebuilder.
   */
   class _seAdptQueryRebuilder : public SDBObject
   {
   public:
      _seAdptQueryRebuilder() ;
      ~_seAdptQueryRebuilder() ;

      INT32 init( const BSONObj &matcher,
                  const BSONObj &selector,
                  const BSONObj &orderBy,
                  const BSONObj &hint ) ;
      INT32 rebuild( REBUILD_ITEM_MAP &rebuildItems,
                     utilCommObjBuff &objBuff ) ;

   private:
      BSONObj _query ;
      BSONObj _selector ;
      BSONObj _orderBy ;
      BSONObj _hint ;
   } ;
   typedef _seAdptQueryRebuilder seAdptQueryRebuilder ;

   class _seAdptContextBase : public SDBObject
   {
   public:
      _seAdptContextBase( const string &indexName,
                          const string &typeName,
                          utilESClt *seClt ) ;
      virtual ~_seAdptContextBase() ;

      virtual INT32 open( const BSONObj &matcher,
                          const BSONObj &selector,
                          const BSONObj &orderBy,
                          const BSONObj &hint,
                          utilCommObjBuff &objBuff,
                          pmdEDUCB *eduCB ) = 0 ;
      virtual INT32 getMore( INT32 returnNum, utilCommObjBuff &objBuff ) = 0 ;

   protected:
      string _indexName ;
      string _type ;
      utilESClt *_esClt ;
      string _scrollID ;
      utilCommObjBuff _objBuff ;
   } ;
   typedef _seAdptContextBase seAdptContextBase ;

   class _seAdptContextQuery : public _seAdptContextBase
   {
   public:
      _seAdptContextQuery( const string &indexName,
                           const string &typeName,
                           utilESClt *seClt ) ;
      virtual ~_seAdptContextQuery() ;

      INT32 open( const BSONObj &matcher,
                  const BSONObj &selector,
                  const BSONObj &orderBy,
                  const BSONObj &hint,
                  utilCommObjBuff &objBuff,
                  _pmdEDUCB *eduCB ) ;

      INT32 getMore( INT32 returnNum, utilCommObjBuff &objBuff ) ;

   private:
      INT32 _fetchFirstBatch( const string &queryCond, utilCommObjBuff &result ) ;
      INT32 _fetchNextBatch( utilCommObjBuff &result ) ;
      INT32 _fetchAll( const string &queryCond,
                       utilCommObjBuff &result, UINT32 limitNum ) ;

      INT32 _buildInCond( utilCommObjBuff &objBuff,
                          BSONObj &condition ) ;
   private:
      seAdptQueryRebuilder _queryRebuilder ;
      rtnSimpleCondParseTree _condTree ;
   } ;
   typedef _seAdptContextQuery seAdptContextQuery ;
}

#endif /* SEADPT_CONTEXT__ */

