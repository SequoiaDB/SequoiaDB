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

   Source File Name = coordShardKicker.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_SHARD_KICKER_HPP__
#define COORD_SHARD_KICKER_HPP__

#include "coordResource.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      _coordShardKicker define
   */
   class _coordShardKicker : public SDBObject
   {
      struct strContainner
      {
         const CHAR *_pStr ;
         strContainner( const CHAR *str )
         {
            _pStr = str ;
         }
         strContainner()
         {
            _pStr = NULL ;
         }
         bool operator<( const strContainner &right ) const
         {
            if ( !right._pStr )
            {
               return false ;
            }
            else if ( !_pStr )
            {
               return true ;
            }
            return ossStrcmp( _pStr, right._pStr ) < 0 ? true : false ;
         }
      } ;

      typedef set< strContainner >              SET_SHARDINGKEY ;

   public:
      _coordShardKicker() ;
      ~_coordShardKicker() ;

      void     bind( coordResource *pResource,
                     const CoordCataInfoPtr &cataPtr ) ;

   public:
      INT32    kickShardingKey( const BSONObj &updator,
                                BSONObj &newUpdator,
                                BOOLEAN &isChange,
                                _pmdEDUCB *cb,
                                const BSONObj &matcher = BSONObj(),
                                BOOLEAN keepShardingKey = FALSE ) ;

      INT32    checkShardingKey( const BSONObj &updator,
                                 BOOLEAN &hasInclude,
                                 _pmdEDUCB *cb,
                                 const BSONObj &matcher = BSONObj() ) ;

   protected:
      BOOLEAN     _isUpdateReplace( const BSONObj &updator ) ;
      UINT32      _addKeys( const BSONObj &objKey ) ;

      INT32       _kickShardingKey( const CoordCataInfoPtr &cataInfo,
                                    const BSONObj &updator,
                                    BSONObj &newUpdator,
                                    BOOLEAN &hasShardingKey ) ;

      INT32       _kickShardingKey( const string &collectionName,
                                    const BSONObj &updator,
                                    BSONObj &newUpdator,
                                    BOOLEAN &isChange,
                                    _pmdEDUCB *cb,
                                    BOOLEAN keepShardingKey ) ;

      INT32       _checkShardingKey( const CoordCataInfoPtr &cataInfo,
                                     const BSONObj &updator,
                                     BOOLEAN &hasInclude ) ;

      INT32       _checkShardingKey( const string &collectionName,
                                     const BSONObj &updator,
                                     BOOLEAN &hasInclude,
                                     _pmdEDUCB *cb ) ;

   private:
      map< UINT32, BOOLEAN >     _skSiteIDs ;
      SET_SHARDINGKEY            _setKeys ;

      coordResource              *_pResource ;
      CoordCataInfoPtr           _cataPtr ;

   } ;
   typedef _coordShardKicker coordShardKicker ;

}

#endif //COORD_SHARD_KICKER_HPP__

