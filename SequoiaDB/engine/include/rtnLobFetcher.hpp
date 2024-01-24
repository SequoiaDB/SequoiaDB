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

   Source File Name = rtnLobFetcher.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_LOBFETCHER_HPP_
#define RTN_LOBFETCHER_HPP_

#include "dmsLobDef.hpp"
#include "rtn.hpp"

namespace engine
{
   class _rtnLobFetcher : public SDBObject
   {
   public:
      _rtnLobFetcher() ;
      ~_rtnLobFetcher() ;

   public:
      INT32 init( const CHAR *fullName,
                  BOOLEAN onlyMetaPage ) ;

      INT32 fetch( _pmdEDUCB *cb,
                   _dmsLobInfoOnPage &piece,
                   _dpsMessageBlock *mb = NULL ) ;

      void  close( INT32 cause = SDB_DMS_EOC ) ;

      BOOLEAN hitEnd() const
      {
         return SDB_DMS_EOC == _lastErr ||
                SDB_DMS_NOTEXIST == _lastErr ;
      }

      std::pair< OID, UINT32 > toBeFetched() const
      {
         if ( _cursor )
         {
            dmsLobInfoOnPage info ;
            const CHAR *data = nullptr ;
            _cursor->getCurrentLobRecord( info, &data ) ;
            return { info._oid, info._sequence } ;
         }
         return { OID(), 0 } ;
      }

      _dmsStorageUnit *getSu()
      {
         return _su ;
      }

      _dmsMBContext *getMBContext()
      {
         return _mbContext ;
      }

      const CHAR* collectionName() const
      {
         return _fullName ;
      }

   private:
      void _fini() ;
   private:
      dmsStorageUnitID     _suID ;
      _dmsStorageUnit      *_su ;
      _dmsMBContext        *_mbContext ;
      BOOLEAN              _onlyMetaPage ;
      INT32                _lastErr ;
      CHAR                 _fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      std::unique_ptr<ILobCursor> _cursor ;
   } ;
   typedef class _rtnLobFetcher rtnLobFetcher ;
}

#endif

