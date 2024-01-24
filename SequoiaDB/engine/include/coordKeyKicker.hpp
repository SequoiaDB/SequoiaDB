/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = coordKeyKicker.hpp

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

#ifndef COORD_KEY_KICKER_HPP__
#define COORD_KEY_KICKER_HPP__

#include "coordResource.hpp"
#include "../bson/bson.h"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordKeyKicker define
   */
   class _coordKeyKicker : public SDBObject
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
         BOOLEAN operator<( const strContainner &right ) const
         {
            if ( !right._pStr )
            {
               return FALSE ;
            }
            else if ( !_pStr )
            {
               return TRUE ;
            }
            return ossStrcmp( _pStr, right._pStr ) < 0 ? TRUE : FALSE ;
         }
      } ;

      typedef ossPoolSet< strContainner >  SET_KEEPKEY ;

   public:
      _coordKeyKicker() ;
      ~_coordKeyKicker() ;

      void     bind( coordResource *pResource,
                     const CoordCataInfoPtr &cataPtr ) ;

   public:
      INT32    kickKey( const BSONObj &updator,
                        BSONObj &newUpdator,
                        BOOLEAN &isChanged,
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

      INT32       _kickKey( const CoordCataInfoPtr &cataInfo,
                            const BSONObj &updator,
                            BSONObj &newUpdator,
                            const BSONObj &matcher,
                            BOOLEAN &shardingKeyChanged,
                            BOOLEAN &hasKeepAutoInc,
                            BOOLEAN ignoreAutoInc = FALSE ) ;

      INT32       _kickShardingKey( const string &collectionName,
                                    const BSONObj &updator,
                                    BSONObj &newUpdator,
                                    const BSONObj &matcher,
                                    BOOLEAN &isChanged,
                                    _pmdEDUCB *cb,
                                    BOOLEAN keepShardingKey ) ;

      INT32       _checkShardingKey( const CoordCataInfoPtr &cataInfo,
                                     const BSONObj &updator,
                                     BOOLEAN &hasInclude ) ;

      INT32       _checkShardingKey( const string &collectionName,
                                     const BSONObj &updator,
                                     BOOLEAN &hasInclude,
                                     _pmdEDUCB *cb ) ;

      BOOLEAN     _isKey( const CHAR *pField, BSONObj &boKey ) ;

      BOOLEAN     _isShardingKeyChange( const BSONElement &beField,
                                        const BSONObj &matcher ) ;

   private:
      typedef ossPoolMap< UINT32, BOOLEAN > SiteIDSet ;
      SiteIDSet                  _skSiteIDs ;
      SET_KEEPKEY                _setKeys ;

      coordResource              *_pResource ;
      CoordCataInfoPtr           _cataPtr ;

   } ;
   typedef _coordKeyKicker coordKeyKicker ;

}

#endif //COORD_KEY_KICKER_HPP__

