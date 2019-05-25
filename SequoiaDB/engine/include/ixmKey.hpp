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

   Source File Name = ixmKey.hpp

   Descriptive Name = Index Management Key Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   index keys. One index key may refer existing information in file, or contains
   its own buffer. So we have two classes for each purpose.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMKEY_HPP_
#define IXMKEY_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.h"
#include "pd.hpp"
#include <string>
#include <vector>
using namespace bson ;
using namespace std ;
namespace engine
{

   class _ixmKey : public SDBObject
   {
      void operator=(const _ixmKey&) ;
   protected:
      enum { IsBSON = 0xff } ;
      const UINT8 *_keyData ;
      BSONObj _bson() const
      {
         SDB_ASSERT ( !isCompactFormat(),
                      "Key shouldn't be compacted when calling _bson" ) ;
         return BSONObj((const CHAR *)_keyData + 1) ;
      }
   private:
      INT32 _compareHybrid ( const _ixmKey &r, const Ordering &order) const ;
   public:

      _ixmKey() { _keyData = NULL ; }
      _ixmKey ( const _ixmKey &r )
      {
         this->_keyData = r._keyData ;
      }
      _ixmKey ( const CHAR *keyData )
      {
         this->_keyData = (UINT8*)keyData ;
      }
      void assign(const _ixmKey &r )
      {
         this->_keyData = r._keyData ;
      }

      INT32 woCompare ( const _ixmKey &right, const Ordering &o ) const ;
      BOOLEAN woEqual ( const _ixmKey &right ) const ;
      BSONObj toBson() const ;
      std::string toString(BOOLEAN isArray = FALSE, BOOLEAN full=FALSE) const
      {
         return toBson().toString(isArray, full);
      }

      BOOLEAN isValid ()
      {
         return toBson().isValid () ;
      }

      BOOLEAN isUndefined() const ;

      OSS_INLINE const CHAR *data() const
      {
         return (const CHAR *) _keyData ;
      }

      INT32 dataSize() const ;
      OSS_INLINE BOOLEAN isValid() const
      {
         return (_keyData != NULL) ;
      }
      OSS_INLINE BOOLEAN isCompactFormat() const
      {
         return *_keyData != IsBSON ;
      }
   } ;
   typedef class _ixmKey ixmKey ;

   class _ixmKeyOwned : public _ixmKey
   {
      void operator=(const _ixmKeyOwned&);
   public:
      _ixmKeyOwned ( const BSONObj &obj ) ;
      _ixmKeyOwned ( const _ixmKey &r ) ;
      _ixmKeyOwned () {_keyData = NULL; }
   private:
      StackBufBuilder _b ;
      void _traditional ( const BSONObj &obj ) ;
   } ;
   typedef class _ixmKeyOwned ixmKeyOwned ;
}

#endif
