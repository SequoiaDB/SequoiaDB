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

   Source File Name = dmsIndexCursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_INDEX_CURSOR_HPP_
#define SDB_DMS_INDEX_CURSOR_HPP_

#include "interface/ICursor.hpp"
#include "dms.hpp"

namespace engine
{

   /*
      _dmsIndexCursor defined
    */
   class _dmsIndexCursor : public IIndexCursor
   {
   public:
      _dmsIndexCursor() = default ;
      virtual ~_dmsIndexCursor() = default ;
      _dmsIndexCursor( const _dmsIndexCursor & ) = delete ;
      _dmsIndexCursor &operator =( const _dmsIndexCursor & ) = delete ;

      virtual BOOLEAN isOpened() const
      {
         return _isOpened ;
      }

      virtual BOOLEAN isClosed() const
      {
         return _isClosed ;
      }

      virtual BOOLEAN isForward() const
      {
         return _isForward ;
      }

      virtual BOOLEAN isBackward() const
      {
         return !_isForward ;
      }

      virtual BOOLEAN isEOF() const
      {
         return _isEOF ;
      }

      virtual INT32 getCurrentKeyString( keystring::keyString &key ) ;
      virtual INT32 getCurrentRecordID( dmsRecordID &recordID ) ;
      virtual INT32 getCurrentRecord( dmsRecordData &data ) ;

   protected:
      keystring::keyString _currentKey ;
      dmsRecordID _currentRecordID ;
      dmsRecordData _currentRecordData ;
      BOOLEAN _isOpened = FALSE ;
      BOOLEAN _isClosed = FALSE ;
      BOOLEAN _isForward = TRUE ;
      BOOLEAN _isEOF = FALSE ;
   } ;

   typedef class _dmsIndexCursor dmsIndexCursor ;

}

#endif // SDB_DMS_INDEX_CURSOR_HPP_