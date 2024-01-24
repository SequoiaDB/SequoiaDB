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

   Source File Name = clsAutoIncItem.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/08/2018  LSQ Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_AUTOINC_ITEM_HPP__
#define CLS_AUTOINC_ITEM_HPP__

#include "utilPooledObject.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.h"
#include "clsBase.hpp"
#include "utilMap.hpp"
#include "utilGlobalID.hpp"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{
   /*
      define generated type enum
   */
   enum _AUTOINC_GEN_TYPE
   {
      AUTOINC_GEN_ALWAYS = 0,
      AUTOINC_GEN_STRICT,
      AUTOINC_GEN_DEFAULT
   } ;
   typedef enum _AUTOINC_GEN_TYPE AUTOINC_GEN_TYPE ;

   /*
      define Auto-Increment Identification
   */
   union _clsAutoIncID
   {
      struct
      {
         utilSequenceID seqID ;
         AUTOINC_GEN_TYPE genType ;
      } ;
      CHAR data[ sizeof(utilSequenceID) + sizeof(AUTOINC_GEN_TYPE) ] ;

      BOOLEAN operator< ( const _clsAutoIncID &r ) const
      {
         return ossMemcmp( data, r.data, sizeof(data) ) < 0 ? TRUE : FALSE ;
      }

      BOOLEAN operator== ( const _clsAutoIncID &r ) const
      {
         return ossMemcmp( data, r.data, sizeof(data) ) == 0 ? TRUE : FALSE ;
      }
   } ;
   typedef union _clsAutoIncID clsAutoIncID ;
   typedef ossPoolSet< clsAutoIncID > clsAutoIncIDSet ;

   /*
      define _clsAutoIncItem
   */
   class _clsAutoIncSet ;
   class _clsAutoIncItem : public _utilPooledObject
   {
   friend class _clsAutoIncSet ;
   friend class _clsAutoIncIterator ;
   typedef _utilStringMap<_clsAutoIncItem*, 1>  AUTOINC_ITEM_MAP ;
   typedef AUTOINC_ITEM_MAP::iterator           AUTOINC_ITEM_MAP_IT ;
   typedef AUTOINC_ITEM_MAP::value_type         AUTOINC_ITEM_MAP_VAL ;
   typedef AUTOINC_ITEM_MAP::const_iterator     AUTOINC_ITEM_MAP_CONST_IT ;

   public:
      _clsAutoIncItem() ;
      ~_clsAutoIncItem() ;

      const CHAR*          fieldName() const { return _fieldName ; }
      const CHAR*          fieldFullName() const { return _fieldFullName ; }
      const CHAR*          sequenceName() const { return _sequenceName ; }
      const utilSequenceID sequenceID() const { return _sequenceID ; }
      const CHAR*          generated() const ;
      static INT32         validFieldName( const CHAR *pName ) ;
      static INT32         validGenerated( const CHAR *generated ) ;
      static INT32         validAutoIncOption( const BSONObj& option ) ;

      AUTOINC_GEN_TYPE     generatedType() const { return _generatedType ; }
      const clsAutoIncID   ID() const ;

      BOOLEAN              hasSubField() const { return _pSubFieldMap ? TRUE :FALSE ; }
      const _clsAutoIncItem*  findItem( const CHAR *pName ) const ;
      UINT32               itemCount() const { return _pSubFieldMap->size(); }

      _clsAutoIncItem *    parent() { return _pParent ; } ;
   protected:

      INT32             init( const BSONObj &obj ) ;
      INT32             merge( _clsAutoIncItem *pItem ) ;

   protected:
      void              _clear() ;
      INT32             _init( const CHAR* fieldName,
                               const CHAR* sequenceName,
                               const utilSequenceID sequenceID,
                               const AUTOINC_GEN_TYPE generated,
                               const CHAR* fullName = NULL ) ;

   private:
      const CHAR*       _fieldName ;
      const CHAR*       _fieldFullName ;
      const CHAR*       _sequenceName ;
      utilSequenceID    _sequenceID ;
      AUTOINC_GEN_TYPE  _generatedType ;

      AUTOINC_ITEM_MAP* _pSubFieldMap ;
      _clsAutoIncItem*  _pParent ;
      ossPoolString     _fieldStr ;

   } ;
   typedef _clsAutoIncItem clsAutoIncItem ;

   /*
      _clsAutoIncSet define
   */
   class _clsAutoIncSet : public _utilPooledObject
   {
   friend class _clsAutoIncItem ;
   friend class _clsAutoIncIterator ;
   typedef clsAutoIncItem::AUTOINC_ITEM_MAP           AUTOINC_ITEM_MAP ;
   typedef clsAutoIncItem::AUTOINC_ITEM_MAP_IT        AUTOINC_ITEM_MAP_IT ;
   typedef clsAutoIncItem::AUTOINC_ITEM_MAP_VAL       AUTOINC_ITEM_MAP_VAL ;
   typedef clsAutoIncItem::AUTOINC_ITEM_MAP_CONST_IT  AUTOINC_ITEM_MAP_CONST_IT ;

   public:
      _clsAutoIncSet() ;
      ~_clsAutoIncSet() ;

   public:
      INT32    init( const BSONElement &ele ) ;
      void     clear() ;

      INT32    insert( const BSONObj &obj ) ;
      UINT32   fieldCount() const { return _fieldCount ; }
      UINT32   itemCount() const { return _mapItem.size() ; }
      UINT32   getEleSize() const { return _eleSize ; }

      const clsAutoIncIDSet&     getIDs() const { return _idSet ; }
      const clsAutoIncItem*      findItem( const CHAR *pName ) const ;
      const ossPoolVector<BSONObj>&    getFields() const { return _vecFields ; }
      const BSONObj              toBson() const ;
      const clsAutoIncItem*      find( const CHAR *pName ) const ;
      void                       erase( const CHAR *pName ) ;

   protected:

      INT32    _initAItem( const BSONObj &obj ) ;
      void     _clear() ;
      UINT32   _calcEleSize( const AUTOINC_ITEM_MAP &map ) ;

   private:
      BSONObj              _objInfo ;
      AUTOINC_ITEM_MAP     _mapItem ;
      UINT32               _fieldCount ;

      ossPoolVector<BSONObj>  _vecFields ;
      clsAutoIncIDSet         _idSet ;
      UINT32                  _eleSize ;

   } ;
   typedef _clsAutoIncSet clsAutoIncSet ;

   /*
      define iterator of clsAutoIncItem
   */
   class _clsAutoIncIterator : public _utilPooledObject
   {
   typedef clsAutoIncSet::AUTOINC_ITEM_MAP           AUTOINC_ITEM_MAP ;
   typedef clsAutoIncSet::AUTOINC_ITEM_MAP_CONST_IT  AUTOINC_ITEM_MAP_CONST_IT ;

   public:
      enum _MODE
      {
         NON_RECURS = 0,   // find first level item
         RECURS            // find only leaf item
      } ;
      typedef enum _MODE MODE ;
      _clsAutoIncIterator( const _clsAutoIncSet &set, MODE mode = NON_RECURS ) ;
      _clsAutoIncIterator( const _clsAutoIncItem &item, MODE mode = NON_RECURS ) ;
      BOOLEAN more() ;
      const _clsAutoIncItem* next() ;

   private:
      MODE _mode ;
      const AUTOINC_ITEM_MAP*    _pMap ;
      AUTOINC_ITEM_MAP_CONST_IT  _it ;
      ossPoolList< const AUTOINC_ITEM_MAP*>   _mapTrace ;
      ossPoolList< AUTOINC_ITEM_MAP_CONST_IT> _itTrace ;
   } ;
   typedef _clsAutoIncIterator clsAutoIncIterator ;

}

#endif //CLS_AUTOINC_ITEM_HPP__
