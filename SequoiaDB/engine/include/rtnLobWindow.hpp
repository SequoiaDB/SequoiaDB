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

   Source File Name = rtnLobWindow.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_LOBWINDOW_HPP_
#define RTN_LOBWINDOW_HPP_

#include "dmsLobDef.hpp"
#include "rtnLobTuple.hpp"

namespace engine
{
   class _rtnLobWindow : public SDBObject
   {
   public:
      _rtnLobWindow() ;
      virtual ~_rtnLobWindow() ;

   public:
      INT32 init( INT32 pageSize, BOOLEAN mergeMeta, BOOLEAN metaPageDataCached ) ;

      BOOLEAN continuous( INT64 offset ) const ;

      UINT32 getLobSequence() const ;

      INT32 prepare4Write( INT64 offset, UINT32 len, const CHAR *data ) ;

      BOOLEAN getNextWriteSequences( RTN_LOB_TUPLES &tuples ) ;

      void cacheLastDataOrClearCache() ;

      BOOLEAN getCachedData( _rtnLobTuple &tuple ) ;

      BOOLEAN getMetaPageData( _rtnLobTuple &tuple ) ;

      INT32 prepare4Read( INT64 lobLen,
                          INT64 offset,
                          UINT32 len,
                          RTN_LOB_TUPLES &tuples ) ;
   private:
      BOOLEAN _getNextWriteSequence( _rtnLobTuple &tuple ) ;
      BOOLEAN _hasData() const ;
      UINT32  _getCurDataPageSize() const ;

   private:
      INT32          _pageSize ;
      UINT32         _logarithmic ;
      BOOLEAN        _mergeMeta ;
      BOOLEAN        _metaPageDataCached ;

      INT64          _curOffset ;
      CHAR*          _pool ;
      INT32          _cachedSz ;
      INT32          _metaSize ;

      /// reuse rtnLobPiece to keep write data.
      _rtnLobTuple   _writeData ;
   } ;
   typedef class _rtnLobWindow rtnLobWindow ;
}

#endif

