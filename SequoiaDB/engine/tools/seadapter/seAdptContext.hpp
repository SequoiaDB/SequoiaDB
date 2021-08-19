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
#include "utilESFetcher.hpp"
#include "utilPooledObject.hpp"
#include "rtnSimpleCondParser.hpp"
#include "seAdptIdxMetaMgr.hpp"

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

      // Initialize the rebuilder with the original message.
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

   // Context for operations of adapter.
   // It maintains the buffer.
   class _seAdptContextBase : public utilPooledObject
   {
   public:
      _seAdptContextBase() ;
      virtual ~_seAdptContextBase() {}

      virtual INT32 open( const CHAR *clName,
                          UINT16 indexID,
                          const BSONObj &matcher,
                          const BSONObj &selector,
                          const BSONObj &orderBy,
                          const BSONObj &hint,
                          utilCommObjBuff &objBuff,
                          pmdEDUCB *eduCB ) = 0 ;
      // Prepare the selector, matcher, order by and hint in objBuff.
      virtual INT32 getMore( INT32 returnNum, utilCommObjBuff &objBuff ) = 0 ;

      BOOLEAN eof() const
      {
         return _hitEnd ;
      }

   protected:
      BOOLEAN _hitEnd ;
   } ;
   typedef _seAdptContextBase seAdptContextBase ;

   // Context for query modify.
   class _seAdptContextQuery : public _seAdptContextBase
   {
   public:
      _seAdptContextQuery() ;
      virtual ~_seAdptContextQuery() ;

      INT32 open( const CHAR *clName,
                  UINT16 indexID,
                  const BSONObj &matcher,
                  const BSONObj &selector,
                  const BSONObj &orderBy,
                  const BSONObj &hint,
                  utilCommObjBuff &objBuff,
                  _pmdEDUCB *eduCB ) ;

      // Prepare the selector, matcher, order by and hint in objBuff.
      INT32 getMore( INT32 returnNum, utilCommObjBuff &objBuff ) ;

   private:
      INT32 _prepareSearch( const BSONObj &queryCond ) ;
      INT32 _getMore( utilCommObjBuff &result ) ;
      INT32 _fetchAll( const BSONObj &queryCond,
                       utilCommObjBuff &result, UINT32 limitNum ) ;

      INT32 _buildInCond( utilCommObjBuff &objBuff,
                          BSONObj &condition ) ;
   private:
      seIdxMetaContext       *_imContext ;
      seAdptQueryRebuilder    _queryRebuilder ;
      rtnSimpleCondParseTree  _condTree ;
      utilESFetcher          *_esFetcher ;
   } ;
   typedef _seAdptContextQuery seAdptContextQuery ;
}

#endif /* SEADPT_CONTEXT__ */

