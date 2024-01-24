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
   // _keyData contains raw data, which may or may not be regular BSON
   // there is 1 byte indicator to show whether if it's BSON object
   // _ixmKey(BSONObj) is converting from regular BSON to key
   // toBson() is converting from key to BSON

   // _ixmKey class doesn't own the buffer, that means the _keyData is pointing
   // to some place that allocated by other threads/functions
   // If user want to use owned-buffered key, they should use _ixmKeyOwned
   class _ixmKey : public SDBObject
   {
      // disable = operator
      void operator=(const _ixmKey&) ;
   protected:
      enum { IsBSON = 0xff } ;
      // starting of raw data
      const UINT8 *_keyData ;
      // this function should only be used internally, when we know it's not
      // compact format
      BSONObj _bson() const
      {
         SDB_ASSERT ( !isCompactFormat(),
                      "Key shouldn't be compacted when calling _bson" ) ;
         // first byte is reserved for isCompact
         return BSONObj((const CHAR *)_keyData + 1) ;
      }

      // helper for hash key values
      UINT32 _hashCompact() const ;
      UINT32 _hashBSON() const ;
   private:
      INT32 _compareHybrid ( const _ixmKey &r, const Ordering &order) const ;
   public:

      // since _ixmKey itself never convert a real BSON into buffer, so it's
      // always refering from existing buffer or keys
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

      // well ordered compare
      INT32 woCompare ( const _ixmKey &right, const Ordering &o ) const ;
      // well ordered equao
      BOOLEAN woEqual ( const _ixmKey &right ) const ;
      // to BSON object
      void _toBson(BSONObjBuilder &b, BSONObjIterator *keyIter = NULL ) const ;
      BSONObj toBson( BufBuilder *bb = NULL ) const ;
      INT32 toRecord( const BSONObj keyPattern,
                      BSONObjBuilder &resultBuilder ) const ;
      // convert to std::string
      std::string toString(BOOLEAN isArray = FALSE, BOOLEAN full=FALSE) const
      {
         return toBson().toString(isArray, full);
      }

      BOOLEAN hasNullOrUndefined() const ;

      BOOLEAN isUndefined() const ;

      // get raw data
      OSS_INLINE const CHAR *data() const
      {
         return (const CHAR *) _keyData ;
      }

      // get the size of key
      INT32 dataSize() const ;

      // calculate hash value for index key
      UINT32 toHash() const ;

      // check if a key is valid
      OSS_INLINE BOOLEAN isValid() const
      {
         return (_keyData != NULL) ;
      }
      // whether the data is compact format
      OSS_INLINE BOOLEAN isCompactFormat() const
      {
         return *_keyData != IsBSON ;
      }
   } ;
   typedef class _ixmKey ixmKey ;

   // owned index key means the keyitself allocated buffer
   class _ixmKeyOwned : public _ixmKey
   {
      void operator=(const _ixmKeyOwned&);
   public:
      // convert from BSON to key
      _ixmKeyOwned ( const BSONObj &obj, BOOLEAN convert = TRUE ) ;
      // make a copy of key
      _ixmKeyOwned ( const _ixmKey &r ) ;
      // make empty key
      _ixmKeyOwned () {_keyData = NULL; }
   private:
      StackBufBuilder _b ;
      // create standard BSON object as key
      void _traditional ( const BSONObj &obj ) ;
   } ;
   typedef class _ixmKeyOwned ixmKeyOwned ;
}

#endif
