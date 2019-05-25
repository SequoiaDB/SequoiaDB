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

   Source File Name = qgmDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMDEF_HPP_
#define QGMDEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sqlGrammar.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.h"
#include "mthCommon.hpp"
#include "utilStr.hpp"
#include "qgmSelectorExpr.hpp"

using namespace bson ;

namespace engine
{
   class _qgmPtrTable ;

   /*
      _qgmField define
   */
   class _qgmField : public SDBObject
   {
   public:
      const static UINT32 npos ;

   private:
      _qgmPtrTable *_ptrTable ;
      const CHAR *_begin ;
      UINT32 _size ;

   public:
      _qgmField() ;
      _qgmField( const _qgmField &field ) ;
      ~_qgmField() ;

      _qgmField &operator=(const _qgmField &field ) ;

      BOOLEAN empty() const
      {
         return 0 == _size ? TRUE : FALSE ;
      }

      void clear()
      {
         _begin = "" ;
         _size = 0 ;
      }

      BOOLEAN operator==( const _qgmField &field ) const ;
      BOOLEAN operator!=( const _qgmField &field ) const ;
      BOOLEAN operator<( const _qgmField &field ) const ;

      BOOLEAN isSubfix( const _qgmField &field,
                        BOOLEAN includeSame = FALSE,
                        UINT32 *pPos = NULL ) const ;

      _qgmField subField( UINT32 pos, UINT32 size = _qgmField::npos ) const ;
      _qgmField rootField() const ;
      _qgmField lastField() const ;
      _qgmField nextField( const _qgmField &cur ) const ;
      _qgmField preField( const _qgmField &cur ) const ;

      void      replace( UINT32 pos, UINT32 size,
                         const _qgmField &field ) ;

      BOOLEAN   isArrayIndexFormat() const ;
      BOOLEAN   isDotted() const ;

      const CHAR *begin() const
      {
         return _begin ;
      }

      UINT32 size() const
      {
         return _size ;
      }

      string toString() const
      {
         if ( _begin )
         {
            return string( _begin, _size ) ;
         }
         return "" ;
      }

      string toFieldName() const ;

      _qgmPtrTable* ptrTable() { return _ptrTable ; }

      friend class _qgmPtrTable ;
   } ;
   typedef class _qgmField qgmField ;

   /*
      _qgmDbAttr define
   */
   class _qgmDbAttr : public SDBObject
   {
   public:
      _qgmDbAttr( const qgmField &relegation,
                  const qgmField &attr )
      :_relegation(relegation),
       _attr(attr)
      {
      }

      _qgmDbAttr(){}

      _qgmDbAttr( const _qgmDbAttr &attr )
      :_relegation( attr._relegation),
       _attr( attr._attr )
       {
       }

      _qgmDbAttr &operator=( const _qgmDbAttr &attr )
      {
         _relegation = attr._relegation ;
         _attr = attr._attr ;
         return *this ;
      }

      virtual ~_qgmDbAttr(){}

   public:
      OSS_INLINE const qgmField &relegation() const { return _relegation ;}

      OSS_INLINE const qgmField &attr() const { return _attr ;}

      OSS_INLINE qgmField &relegation() { return _relegation ;}

      OSS_INLINE qgmField &attr() { return _attr ;}

      OSS_INLINE BOOLEAN empty()const
      {
         return _relegation.empty() && _attr.empty() ;
      }

      OSS_INLINE BOOLEAN operator==( const _qgmDbAttr &attr ) const
      {
         return _relegation == attr._relegation
                && _attr == attr._attr ;
      }

      OSS_INLINE string toString() const
      {
         if ( !_relegation.empty() && !_attr.empty() )
         {
            stringstream ss ;
            ss << _relegation.toString()
               << "."
               << _attr.toString() ;
            return ss.str() ;
         }
         else
         {
            return _relegation.empty() ?
                   _attr.toString() : _relegation.toString() ;
         }
      }

      OSS_INLINE string toFieldName() const
      {
         if ( !_relegation.empty() && !_attr.empty() )
         {
            stringstream ss ;
            ss << _relegation.toString()
               << "."
               << _attr.toFieldName() ;
            return ss.str() ;
         }
         else
         {
            return _relegation.empty() ?
                   _attr.toFieldName() : _relegation.toString() ;
         }
      }

      OSS_INLINE BOOLEAN operator<( const _qgmDbAttr &attr ) const
      {
         if ( _relegation < attr._relegation )
         {
            return TRUE ;
         }
         else if ( _relegation == attr._relegation )
         {
            return _attr < attr._attr ;
         }
         else
         {
            return FALSE ;
         }
      }
   private:
      qgmField _relegation ;
      qgmField _attr ;
   } ;
   typedef class _qgmDbAttr qgmDbAttr ;
   typedef vector< qgmDbAttr* > qgmDbAttrPtrVec ;
   typedef vector< qgmDbAttr >  qgmDbAttrVec ;

   /*
      _qgmOpField define
   */
   struct _qgmOpField : public SDBObject
   {
      qgmDbAttr value ;
      qgmField alias ;
      INT32 type ;
      _qgmSelectorExpr expr ;

      _qgmOpField()
      :type(SQL_GRAMMAR::SQLMAX)
      {}

      _qgmOpField( const _qgmOpField &field )
      :value( field.value ),
       alias( field.alias ),
       type( field.type ),
       expr( field.expr )
      {
      }

      _qgmOpField &operator=( const _qgmOpField &field )
      {
         value = field.value ;
         alias = field.alias ;
         type = field.type ;
         expr = field.expr ;
         return *this ;
      }

      _qgmOpField( const qgmDbAttr &attr, INT32 attrType )
      :value( attr ), type( attrType )
      {
      }

      virtual ~_qgmOpField(){}

      BOOLEAN operator==( const _qgmOpField &field )
      {
         return value == field.value
                && alias == field.alias
                && type == field.type ; 
      }


      BOOLEAN isFrom( const _qgmOpField &right, BOOLEAN useAlias = TRUE ) const
      {
         if ( value == right.value || right.type == SQL_GRAMMAR::WILDCARD
              || ( useAlias && value.attr() == right.alias ) )
         {
            return TRUE ;
         }
         return FALSE ;
      }

      BOOLEAN empty() const
      {
         return value.empty() && alias.empty() ;
      }

      string toString() const
      {
         stringstream ss ;
         ss << "value:" << value.toString() ;
         ss << ",type:" << type ;
         if ( !alias.empty() )
         {
            ss << ",alias:" << alias.toString() ;
         }
         if ( !expr.isEmpty() )
         {
            ss << ",expr:" << expr.toString() ;
         }
         return ss.str() ;
      }
   } ;
   typedef struct _qgmOpField qgmOpField ;
   typedef std::vector< qgmOpField >  qgmOPFieldVec ;
   typedef std::vector< qgmOpField* > qgmOPFieldPtrVec ;

   /*
      _qgmFetchOut define
   */
   struct _qgmFetchOut : public SDBObject
   {
      _qgmField alias ;
      BSONObj obj ;
      _qgmFetchOut *next ;

      _qgmFetchOut()
      :next( NULL )
      {
      }

      virtual ~_qgmFetchOut()
      {
         next = NULL ;
      }

      INT32 element( const _qgmDbAttr &attr, BSONElement &ele )const ;

      void elements( std::vector<BSONElement> &eles ) const ;

      BSONObj mergedObj() const ;
   } ;
   typedef struct _qgmFetchOut qgmFetchOut ;

   typedef struct _varItem
   {
      qgmDbAttr      _fieldName ;
      qgmDbAttr      _varName ;
   }varItem ;

   typedef vector< varItem >     QGM_VARLIST ;

   struct _qgmHint : public SDBObject
   {
      qgmField value ;
      vector<qgmOpField> param ;
   } ;
   typedef struct _qgmHint qgmHint ;

   typedef vector<qgmHint>  QGM_HINS ;

   enum QGM_JOIN_ACHIEVE
   {
      QGM_JOIN_ACHIEVE_NL = 0,
      QGM_JOIN_ACHIEVE_HASH,
      QGM_JOIN_ACHIEVE_MERGE,
   } ;

   class _qgmValueTuple
   {
   public:
      _qgmValueTuple( CHAR *data, UINT32 len, BOOLEAN format ) ;
      ~_qgmValueTuple() ;

   private:
      struct _tuple
      {
         UINT32 len ;
         INT16 type ;
         UINT16 flag ;
      } ;

   public:
      INT32 setValue( UINT32 dataLen, const void *data, INT16 type ) ;
      INT32 setValue( const bsonDecimal &decimal ) ;

      const void *getValue() const
      {
         return NULL == _row ?
                NULL :
                ( const void * )( _row + sizeof( _tuple ) ) ;
      }

      UINT32 getValueLen() const
      {
         return NULL == _row ?
                0 :
                ( ( _tuple * )_row )->len - sizeof( _tuple ) ;
      }

      INT16 getValueType() const
      {
         return NULL == _row ?
                ( INT16 )bson::EOO :
                ( ( _tuple * )_row )->type ;
      }

   public:
      INT32 add( const _qgmValueTuple &right, _qgmValueTuple &result ) const ;
      INT32 sub( const _qgmValueTuple &right, _qgmValueTuple &result ) const ;
      INT32 multiply( const _qgmValueTuple &right, 
                      _qgmValueTuple &result ) const;
      INT32 divide( const _qgmValueTuple &right, 
                    _qgmValueTuple &result ) const ;
      INT32 mod( const _qgmValueTuple &right, _qgmValueTuple &result ) const ;

   private:
      INT32 _ensureMem( UINT32 dataLen ) ;
      INT32 _getDecimal( bsonDecimal &decimal ) const ;
   private:
      CHAR *_row ;
      UINT32 _len ;
      BOOLEAN _isOwn ;
   } ;
}

#endif // QGMDEF_HPP_

