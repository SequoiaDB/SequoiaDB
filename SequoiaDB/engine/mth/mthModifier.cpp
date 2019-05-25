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

   Source File Name = mthModifier.cpp

   Descriptive Name = Method Modifier

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains functions for creating a
   new record based on old record and modify rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include <algorithm>
#include "pd.hpp"
#include "mthModifier.hpp"
#include "dms.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "utilMath.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{
#define ADD_CHG_FIELD_VALUE( builder, fieldName, value, strChg ) \
      do { \
         if ( builder ) \
         { \
            BSONObjBuilder subBuilder ( builder->subobjStart ( strChg ) ) ; \
            subBuilder.append ( fieldName, value ) ; \
            subBuilder.done () ; \
         } \
      } while ( 0 )

#define ADD_CHG_ELEMENT( builder, ele, strChg ) \
   do { \
      if ( builder ) \
      { \
         BSONObjBuilder subBuilder ( builder->subobjStart ( strChg ) ) ; \
         subBuilder.append ( ele ) ; \
         subBuilder.done () ; \
      } \
   } while ( 0 )

#define ADD_CHG_ELEMENT_AS( builder, ele, eleFieldName, strChg ) \
   do { \
      if ( builder ) \
      { \
         BSONObjBuilder subBuilder ( builder->subobjStart ( strChg ) ) ; \
         subBuilder.appendAs ( ele, eleFieldName ) ; \
         subBuilder.done () ; \
      } \
   } while ( 0 )

#define ADD_CHG_UNSET_FIELD( builder, fieldName ) \
   do { \
      if ( builder ) \
      { \
         BSONObjBuilder subBuilder ( builder->subobjStart ( "$unset" ) ) ; \
         subBuilder.append ( fieldName, "" ) ; \
         subBuilder.done () ; \
      } \
   } while ( 0 )

#define ADD_CHG_NUMBER( builder, fieldName, value, strChg ) \
   do { \
      if ( builder ) \
      { \
         BSONObjBuilder subBuilder ( builder->subobjStart ( strChg ) ) ; \
         subBuilder.append ( fieldName, value ) ; \
         subBuilder.done () ; \
      } \
   } while ( 0 )

#define ADD_CHG_ARRAY_OBJ( builder, obj, objFiledName, strChg ) \
   do { \
      if ( builder ) \
      { \
         BSONObjBuilder subBuilder ( builder->subobjStart ( strChg ) ) ; \
         subBuilder.appendArray ( objFiledName, obj ) ; \
         subBuilder.done () ; \
      } \
   } while ( 0 )

#define SET_ARRAY_POS_NAME    "pos"
#define SET_ARRAY_OBJS_NAME   "objs"

   /*
      _mthModifier implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__ADDMDF, "_mthModifier::_addModifier" )
   INT32 _mthModifier::_addModifier ( const BSONElement &ele, ModType type )
   {
      INT32 rc = SDB_OK ;
      INT32 dollarNum = 0 ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__ADDMDF );

      if ( RENAME == type && ( ele.type() != String ||
           ossStrchr ( ele.valuestr(), '.' ) != NULL ||
           ossStrncmp ( ele.valuestr(), "$", 1 ) == 0 ) )
      {
         PD_LOG_MSG ( PDERROR, "Rename field must be string,and can't start "
                      "with '$', and can't include '.', %s",
                      ele.toString().c_str() ) ;
         goto error ;
      }
      else if ( INC == type && !ele.isNumber() )
      {
         PD_LOG_MSG ( PDERROR, "$inc field must be number, %s",
                      ele.toString().c_str() ) ;
         goto error ;
      }
      else if ( ( PUSH_ALL == type || PULL_ALL == type ||
                  PULL_ALL_BY == type ) &&
                Array != ele.type () )
      {
         PD_LOG_MSG ( PDERROR, "$push_all/pull_all/pull_all_by field must "
                      "be array, %s", ele.toString().c_str() ) ;
         goto error ;
      }
      else if ( POP == type && !ele.isNumber() )
      {
         PD_LOG_MSG ( PDERROR, "$pop field must be number, %s",
                      ele.toString().c_str() ) ;
         goto error ;
      }
      else if ( ( BITOR == type || BITAND == type ||
                  BITXOR == type || BITNOT == type ) &&
                !ele.isNumber() )
      {
         PD_LOG_MSG ( PDERROR, "bitor/bitand/bitxor/bitnot field must be "
                      "array, %s", ele.toString().c_str() ) ;
         goto error ;
      }
      else if ( BIT == type && !ele.isABSONObj() )
      {
         PD_LOG_MSG ( PDERROR, "bit field must be object , %s",
                      ele.toString().c_str()) ;
         goto error ;
      }
      else if ( ADDTOSET == type && Array != ele.type() )
      {
         PD_LOG_MSG ( PDERROR, "addtoset field must be array, %s",
                      ele.toString().c_str() ) ;
         goto error ;
      }
      else if ( ( UNSET == type || RENAME == type ) &&
                ossStrcmp ( ele.fieldName(), DMS_ID_KEY_NAME ) == 0 )
      {
         PD_LOG_MSG ( PDERROR, "ID field can't be renamed or unset" ) ;
         goto error ;
      }
      else if ( ( REPLACE == type || KEEP == type ) &&
                ( NULL != ossStrchr( ele.fieldName(), '.' ) ||
                  '$' == ele.fieldName()[0] ) )
      {
         PD_LOG_MSG ( PDERROR, "$replace and $keep's field name can't start "
                      "with '$', and can't include '.', %s",
                      ele.toString().c_str() ) ;
         goto error ;
      }

      rc = mthCheckFieldName ( ele.fieldName(), dollarNum ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Faild field name : %s", ele.fieldName() ) ;
         goto error ;
      }

      if ( REPLACE == type &&
           0 == ossStrcmp( ele.fieldName(), DMS_ID_KEY_NAME ) )
      {
         _isReplaceID = TRUE ;
      }

      if ( KEEP == type )
      {
         _keepKeys.insert( ele.fieldName() ) ;
      }
      else
      {
         ModifierElement me( ele, type, dollarNum ) ;
         _modifierElements.push_back( me ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__ADDMDF, rc );
      return rc ;
   error :
      rc = SDB_INVALIDARG ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPINCMDF, "_mthModifier::_applyIncModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyIncModifier ( const CHAR *pRoot, Builder &bb,
                                           const BSONElement &in,
                                           ModifierElement &me )
   {
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPINCMDF );
      INT32 rc        = SDB_OK ;
      BSONElement elt = me._toModify ;
      BSONType a      = in.type() ;
      BSONType b      = elt.type() ;

      if ( NumberDecimal == a || NumberDecimal == b )
      {
         bsonDecimal inc ;
         bsonDecimal decimal ;

         decimal = in.numberDecimal() ;
         inc     = elt.numberDecimal() ;
         if ( inc.isZero() )
         {
            bb.append ( in ) ;
         }
         else
         {
            bsonDecimal result ;

            rc = decimal.add( inc, result ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "decimal add failed:v1=%s,v2=%s,rc=%d", 
                           decimal.toString().c_str(),
                           inc.toString().c_str(), rc ) ;
               goto error ;
            }

            rc = result.updateTypemod( decimal.getTypemod() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "result is out of precision:result=%s,"
                           "precision=%d,scale=%d,rc=%d",
                           result.toString().c_str(),
                           decimal.getPrecision(), decimal.getScale(), rc ) ;
               goto error ;
            }

            bb.append ( in.fieldName(), result ) ;
            ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;
            ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, result, "$set" ) ;
         }
      }
      else if ( in.isNumber() && 0 != elt.numberDouble() )
      {
         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;

         if ( NumberDouble == a || NumberDouble == b )
         {
            bb.append ( in.fieldName(), in.numberDouble()+elt.numberDouble()) ;
            ADD_CHG_NUMBER ( _dstChgBuilder, pRoot,
                             in.numberDouble()+elt.numberDouble(), "$set" ) ;
         }
         else if ( NumberLong == a || NumberLong == b )
         {
            INT64 arg1 = in.numberLong() ;
            INT64 arg2 = elt.numberLong() ;
            INT64 result = arg1 + arg2 ;
            if ( !utilAddIsOverflow( arg1, arg2, result) )
            {
               bb.append ( in.fieldName(), result) ;
               ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, result, "$set" ) ;
            }
            else if ( !_strictDataMode )
            {
               bsonDecimal decimalE ;
               bsonDecimal decimalArg ;
               bsonDecimal decimalResult ;

               decimalE   = in.numberDecimal() ;
               decimalArg = elt.numberDecimal() ;
               rc = decimalE.add( decimalArg, decimalResult ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to add decimal:%s+%s,rc=%d",
                          decimalE.toString().c_str(),
                          decimalArg.toString().c_str(), rc ) ;
                  goto error ;
               }
               bb.append( in.fieldName(), decimalResult ) ;
               ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, decimalResult, "$set" ) ;
            }
            else
            {
               rc = SDB_VALUE_OVERFLOW ;
               PD_LOG( PDERROR, "overflow happened, field: %s(%lld, inc: %lld), rc = %d",
                       in.fieldName(), arg1, arg2, rc ) ;
               goto error ;

            }
         }
         else
         {
            INT32 arg1 = in.numberInt();
            INT32 arg2 = elt.numberInt() ;

            INT32 result = arg1 + arg2 ;
            INT64 result64 = (INT64)arg1 + (INT64)arg2 ;
            if ( result64 == (INT64)result )
            {
               bb.append ( in.fieldName(), result ) ;
               ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, result, "$set" ) ;
            }
            else if ( !_strictDataMode )
            {
               bb.append ( in.fieldName(), result64) ;
               ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, result64, "$set" ) ;
            }
            else 
            {
               rc = SDB_VALUE_OVERFLOW ;
               PD_LOG( PDERROR, "overflow happened, field: %s(%d, inc: %d), rc = %d",
                       in.fieldName(), arg1, arg2, rc ) ;
               goto error ;
            }
         }
      }
      else
      {
         bb.append ( in ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB__MTHMDF__APPINCMDF ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPSETMDF, "_mthModifier::_applySetModifier" )
   template<class Builder>
   INT32 _mthModifier::_applySetModifier( const CHAR *pRoot, Builder &bb,
                                          const BSONElement &in,
                                          ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPSETMDF ) ;

      if ( 0 != in.woCompare( me._toModify, false ) )
      {
         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;
         ADD_CHG_ELEMENT_AS ( _dstChgBuilder, me._toModify, pRoot, "$set" ) ;
         bb.appendAs ( me._toModify, in.fieldName() ) ;
      }
      else
      {
         bb.append ( in ) ;
      }

      PD_TRACE_EXITRC ( SDB__MTHMDF__APPSETMDF, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPPSHMDF, "_mthModifier::_applyPushModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyPushModifier ( const CHAR *pRoot, Builder &bb,
                                            const BSONElement &in,
                                            ModifierElement &me )
   {
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPPSHMDF );
      INT32 rc= SDB_OK ;
      if ( Array != in.type() )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }

      {
         BSONObjBuilder sub ( bb.subarrayStart ( in.fieldName() ) ) ;
         BSONObjIterator i ( in.embeddedObject() ) ;
         INT32 n = 0 ;
         while ( i.more() )
         {
            sub.append( i.next() ) ;
            n++ ;
         }
         sub.appendAs ( me._toModify, sub.numStr(n) ) ;
         BSONObj newObj = sub.done() ;

         _buildSetArray( _dstChgBuilder, pRoot, n, me._toModify ) ;
         _buildSetArray( _srcChgBuilder, pRoot, n, BSONArrayBuilder().arr() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPPSHMDF, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPPSHALLMDF, "_mthModifier::_applyPushAllModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyPushAllModifier ( const CHAR *pRoot, Builder & bb,
                                               const BSONElement & in,
                                               ModifierElement & me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPPSHALLMDF );
      if ( in.type() != Array )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }
      if ( me._toModify.type() != Array )
      {
         PD_LOG_MSG ( PDERROR, "pushed data type is not array: %s",
                      me._toModify.toString().c_str());
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      {
         BSONObjBuilder sub ( bb.subarrayStart ( in.fieldName() ) ) ;
         BSONObjIterator i ( in.embeddedObject()) ;
         INT32 n = 0 ;
         INT32 pushNum = 0 ;
         while ( i.more() )
         {
            sub.append( i.next() ) ;
            n++ ;
         }

         INT32 beginPos = n ;

         i = BSONObjIterator ( me._toModify.embeddedObject()) ;
         while( i.more() )
         {
            sub.appendAs( i.next(), sub.numStr(n++) ) ;
            ++pushNum ;
         }
         BSONObj newObj = sub.done() ;

         if ( 0 != pushNum )
         {
            _buildSetArray( _dstChgBuilder, pRoot, beginPos,
                            me._toModify.embeddedObject() ) ;
            _buildSetArray( _srcChgBuilder, pRoot, beginPos,
                            BSONArrayBuilder().arr() ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPPSHALLMDF, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPPLLMDF, "_mthModifier::_applyPullModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyPullModifier ( const CHAR *pRoot, Builder &bb,
                                            const BSONElement &in,
                                            ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPPLLMDF );
      if ( in.type() != Array )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }
      {
         BSONObjBuilder sub ( bb.subarrayStart ( in.fieldName() ) ) ;
         INT32 n = 0 ;
         BOOLEAN changed = FALSE ;
         INT32 changedPos = 0 ;
         BSONObjIterator i ( in.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            BOOLEAN allowed = TRUE ;
            if ( PULL == me._modType || PULL_BY == me._modType )
            {
               allowed = ! _pullElementMatch ( ele, me._toModify,
                                               ( PULL == me._modType ?
                                                 TRUE : FALSE ) ) ;
            }
            else
            {
               BSONObjIterator j ( me._toModify.embeddedObject() ) ;
               while ( j.more() )
               {
                  BSONElement eleM = j.next() ;
                  if ( _pullElementMatch( ele, eleM,
                                          ( PULL_ALL == me._modType ?
                                            TRUE : FALSE ) ) )
                  {
                     allowed = FALSE ;
                     break ;
                  }
               }
            }

            if ( allowed )
            {
               sub.appendAs ( ele, sub.numStr(n++) ) ;
            }
            else
            {
               if ( !changed )
               {
                  changedPos = n ;
               }
               changed = TRUE ;
            }
         }
         BSONObj newObj = sub.done() ;

         if ( changed )
         {
            _buildSetArray( _dstChgBuilder, pRoot, changedPos, -1, newObj ) ;
            _buildSetArray( _srcChgBuilder, pRoot, changedPos, -1, in.embeddedObject() ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPPLLMDF, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPPOPMDF, "_mthModifier::_applyPopModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyPopModifier ( const CHAR *pRoot, Builder &bb,
                                           const BSONElement &in,
                                           ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPPOPMDF );
      if ( Array != in.type() )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }

      if ( !me._toModify.isNumber() )
      {
         PD_LOG_MSG ( PDERROR, "pop data type must be a number: %s",
                      me._toModify.toString().c_str());
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      if ( me._toModify.number() == 0 )
      {
         bb.append ( in ) ;
         goto done ;
      }
      {
         BSONObjBuilder sub ( bb.subarrayStart ( in.fieldName() ) ) ;
         INT32 n = 0 ;
         INT32 changedPos = 0 ;
         if ( me._toModify.number() < 0 )
         {
            changedPos = 0 ;
            INT32 m = (INT32)me._toModify.number() ;
            BSONObjIterator i ( in.embeddedObject() ) ;
            while ( i.more() )
            {
               m++ ;
               BSONElement be = i.next() ;
               if ( m > 0 )
               {
                  sub.appendAs( be, sub.numStr(n++) ) ;
               }
            }
         }
         else
         {
            INT32 count = 0 ;
            INT32 m = (INT32)me._toModify.number() ;
            BSONObjIterator i ( in.embeddedObject() ) ;
            while ( i.more())
            {
               i.next() ;
               count++ ;
            }
            count = count - m ;
            changedPos = count > 0 ? count : 0 ;
            i = BSONObjIterator ( in.embeddedObject() ) ;
            while ( i.more() )
            {
               if ( count > 0 )
               {
                  sub.appendAs( i.next(), sub.numStr(n++) ) ;
               }
               else
               {
                  break ;
               }

               count-- ;
            }
         }
         BSONObj newObj = sub.done() ;

         _buildSetArray( _dstChgBuilder, pRoot, changedPos, -1, newObj ) ;
         _buildSetArray( _srcChgBuilder, pRoot, changedPos, -1, in.embeddedObject() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPPOPMDF, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPBITMDF, "_mthModifier::_applyBitModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyBitModifier ( const CHAR *pRoot, Builder &bb,
                                           const BSONElement &in,
                                           ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPBITMDF );
      BSONElement elt = me._toModify ;
      BSONType a = in.type() ;
      BSONType b = elt.type() ;
      BOOLEAN change = FALSE ;

      if ( NumberLong == a || NumberInt == a )
      {
         if ( NumberLong == a || NumberLong == b )
         {
            INT64 result = 0 ;
            rc = _bitCalc ( me._modType, in.numberLong(),
                            elt.numberLong(), result ) ;
            if ( SDB_OK == rc )
            {
               change = TRUE ;
               bb.append ( in.fieldName(), ( long long )result ) ;
               ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, (long long)result,
                                "$set" ) ;
            }
         }
         else if ( NumberInt == a || NumberInt == b )
         {
            INT32 result = 0 ;
            rc = _bitCalc ( me._modType, in.numberInt(),
                            elt.numberInt(), result ) ;
            if ( SDB_OK == rc )
            {
               change = TRUE ;
               bb.append ( in.fieldName(), (int)result ) ;
               ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, (int)result, "$set" ) ;
            }
         }
      }

      if ( change )
      {
         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;
      }
      else
      {
         bb.append ( in ) ;
      }
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPBITMDF, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPBITMDF2, "_mthModifier::_applyBitModifier2" )
   template<class Builder>
   INT32 _mthModifier::_applyBitModifier2 ( const CHAR *pRoot, Builder &bb,
                                            const BSONElement &in,
                                            ModifierElement &me )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MTHMDF__APPBITMDF2 ) ;

      if ( NumberInt != in.type() && NumberLong != in.type() )
      {
         bb.append ( in ) ;
         goto done ;
      }
      {
         ModType type = UNKNOW ;
         BSONObjIterator it ( me._toModify.embeddedObject () ) ;
         INT32 x = in.numberInt () ;
         INT64 y = in.numberLong () ;

         while ( it.more () )
         {
            BSONElement e = it.next () ;
            if ( NumberInt != e.type () && NumberLong != e.type () )
            {
               PD_LOG_MSG ( PDERROR, "bit object field elements must be int or "
                            "long, %s", e.toString( TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto done ;
            }
            {
               string oprName = "$bit" ;
               oprName += e.fieldName () ;
               type = _parseModType ( oprName.c_str() ) ;
            }
            if ( UNKNOW == type )
            {
               PD_LOG_MSG ( PDERROR, "unknow bit operator, %s",
                            e.toString( TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto done ;
            }

            rc = _bitCalc ( type, x, e.numberInt(), x ) ;
            if ( SDB_OK != rc )
            {
               goto done ;
            }
            rc = _bitCalc ( type, y, e.numberLong(), y ) ;
            if ( SDB_OK != rc )
            {
               goto done ;
            }
         }

         if ( ( INT64 ) x != y )
         {
            bb.append ( in.fieldName(), y ) ;
            ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, y, "$set" ) ;
         }
         else
         {
            bb.append ( in.fieldName(), x ) ;
            ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, x, "$set" ) ;
         }
         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPBITMDF2, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPADD2SETMDF, "_mthModifier::_applyAddtoSetModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyAddtoSetModifier ( const CHAR *pRoot, Builder &bb,
                                                const BSONElement &in,
                                                ModifierElement & me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPADD2SETMDF ) ;
      if ( Array != in.type() )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }
      if ( Array != me._toModify.type() )
      {
        PD_LOG_MSG ( PDERROR, "added data type is not array: %s",
                     me._toModify.toString().c_str()) ;
        rc = SDB_INVALIDARG ;
        goto done ;
      }
      {
         BSONObjBuilder sub ( bb.subarrayStart ( in.fieldName() ) ) ;
         BSONObjIterator i ( in.embeddedObject() ) ;
         BSONObjIterator j ( me._toModify.embeddedObject() ) ;
         INT32 n = 0 ;
         BSONElementSet eleset ;
         while ( j.more() )
         {
            eleset.insert( j.next() ) ;
         }
         while ( i.more() )
         {
            BSONElement cur = i.next() ;
            sub.append( cur ) ;
            n++ ;
            eleset.erase( cur ) ;
         }
         INT32 orgNum = n ;
         BSONElementSet::iterator it ;
         for ( it = eleset.begin(); it != eleset.end(); it++ )
         {
            sub.appendAs( (*it), sub.numStr(n++) ) ;
         }
         BSONObj newObj = sub.done() ;

         if ( orgNum != n )
         {
            _buildSetArray( _dstChgBuilder, pRoot, orgNum, -1, newObj ) ;
            _buildSetArray( _srcChgBuilder, pRoot, orgNum, -1, in.embeddedObject() ) ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPADD2SETMDF, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__BITCALC, "_mthModifier::_bitCalc" )
   template<class VType>
   INT32 _mthModifier::_bitCalc ( ModType type, VType l, VType r, VType & out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__BITCALC );
      switch ( type )
      {
         case BITAND :
            out = l & r ;
            break ;
         case BITOR :
            out = l | r ;
            break ;
         case BITNOT :
            out = ~l ;
            break ;
         case BITXOR :
            out = l ^ r ;
            break ;
         default :
            PD_LOG_MSG ( PDERROR, "unknow bit modifier type[%d]", type ) ;
            rc = SDB_INVALIDARG ;
            break ;
      }
      PD_TRACE_EXITRC ( SDB__MTHMDF__BITCALC, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPBITMDF3, "_mthModifier::_appendBitModifier" )
   template<class Builder>
   INT32 _mthModifier::_appendBitModifier ( const CHAR *pRoot,
                                            const CHAR *pShort,
                                            Builder &bb, INT32 in,
                                            ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPBITMDF3 );
      BSONElement elt = me._toModify ;
      BSONType b = elt.type () ;

      if ( NumberLong == b )
      {
         INT64 result = 0 ;
         rc = _bitCalc ( me._modType, (INT64)in, elt.numberLong(), result ) ;
         if ( SDB_OK == rc )
         {
            bb.append ( pShort, result ) ;
            ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, result, "$set" ) ;
         }
      }
      else if ( NumberInt == b )
      {
         INT32 result = 0 ;
         rc = _bitCalc ( me._modType, in, elt.numberInt(), result ) ;
         if ( SDB_OK == rc )
         {
            bb.append ( pShort, result ) ;
            ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, result, "$set" ) ;
         }
      }

      PD_TRACE_EXITRC ( SDB__MTHMDF__APPBITMDF3, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPBITMDF22, "_mthModifier::_appendBitModifier2" )
   template<class Builder>
   INT32 _mthModifier::_appendBitModifier2 ( const CHAR *pRoot,
                                             const CHAR *pShort,
                                             Builder &bb, INT32 in,
                                             ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPBITMDF22 );
      ModType type = UNKNOW ;
      BSONObjIterator it ( me._toModify.embeddedObject () ) ;
      INT32 x = in ;
      INT64 y = (INT64)in ;

      while ( it.more () )
      {
         BSONElement e = it.next () ;

         if ( NumberInt != e.type() && NumberLong != e.type() )
         {
            PD_LOG_MSG ( PDERROR, "bit object field elements must be int "
                         "or long, %s", e.toString( TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto done ;
         }

         {
            string oprName = "$bit" ;
            oprName += e.fieldName () ;
            type = _parseModType ( oprName.c_str() ) ;
         }
         if ( UNKNOW == type )
         {
            PD_LOG_MSG ( PDERROR, "unknow bit operator, %s",
                         e.toString( TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto done ;
         }

         rc = _bitCalc ( type, x, e.numberInt(), x ) ;
         if ( SDB_OK != rc )
         {
            goto done ;
         }
         rc = _bitCalc ( type, y, e.numberLong(), y ) ;
         if ( SDB_OK != rc )
         {
            goto done ;
         }
      }

      if ( (INT64)x != y )
      {
         bb.append ( pShort, y ) ;
         ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, y, "$set" ) ;
      }
      else
      {
         bb.append ( pShort, x ) ;
         ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, x, "$set" ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPBITMDF22, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPSETARRMDF1, "_mthModifier::_applySetArrayModifier" )
   template<class Builder>
   INT32 _mthModifier::_applySetArrayModifier ( const CHAR *pRoot, Builder &bb,
                                                const BSONElement &in,
                                                ModifierElement &me )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MTHMDF__APPSETARRMDF1 ) ;

      BSONElement beModify = me._toModify ;
      const CHAR *pShort = in.fieldName() ;
      INT32 beginPos = 0, endPos = 0 ;
      BSONObj arr ;

      if ( Array != in.type() )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            rc = _appendSetArrayModifier( pRoot, pShort, bb, me ) ;
            if ( SDB_OK == rc )
            {
               ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;
            }
            else
            {
               PD_LOG_MSG ( PDERROR,
                            "Failed to apply $setarray [%s], rc: %d",
                            beModify.toString( TRUE, TRUE ).c_str(),
                            rc ) ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }

      rc = _parseSetArray( beModify, beginPos, endPos, arr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG ( PDERROR,
                      "Failed to parse $setarray [%s], rc: %d",
                      beModify.toString( TRUE, TRUE ).c_str(),
                      rc ) ;
         goto done ;
      }

      {
         BSONArrayBuilder newbb( bb.subarrayStart ( pShort ) ) ;

         INT32 idx = 0 ;
         BSONObjIterator iter ( in.embeddedObject()) ;
         while ( iter.more() && idx < beginPos )
         {
            newbb << iter.next() ;
            idx++ ;
         }

         if ( idx < beginPos )
         {
            INT32 tmpBeginPos = beginPos ;

            beginPos = idx ;
            endPos = -1 ;

            while ( idx < tmpBeginPos )
            {
               newbb.appendNull() ;
               idx ++ ;
            }

            BSONObjIterator iterNew ( arr ) ;
            while ( iterNew.more() )
            {
               newbb << iterNew.next() ;
            }
         }
         else
         {
            INT32 addedCnt = 0, deletedCnt = 0 ;

            BSONObjIterator iterNew ( arr ) ;
            while ( iterNew.more() )
            {
               newbb << iterNew.next() ;
               addedCnt ++ ;
            }

            if ( beginPos <= endPos )
            {
               while ( iter.more() && idx <= endPos )
               {
                  iter.next() ;
                  idx ++ ;
                  deletedCnt ++ ;
               }

               if ( addedCnt != deletedCnt || !iter.more() )
               {
                  endPos = -1 ;
               }

               while ( iter.more() )
               {
                  newbb << iter.next() ;
                  idx ++ ;
               }
            }
         }

         BSONObj newObj = newbb.done() ;
         _buildSetArray( _dstChgBuilder, pRoot, beginPos, endPos, newObj ) ;
         _buildSetArray( _srcChgBuilder, pRoot, beginPos, endPos, in.embeddedObject() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__MTHMDF__APPSETARRMDF1, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPSETARRMDF2, "_mthModifier::_appendSetArrayModifier" )
   template<class Builder>
   INT32 _mthModifier::_appendSetArrayModifier ( const CHAR *pRoot,
                                                 const CHAR *pShort,
                                                 Builder &bb,
                                                 ModifierElement &me )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MTHMDF__APPSETARRMDF2 ) ;

      BSONElement beModify = me._toModify ;
      INT32 beginPos = 0, endPos = 0 ;
      BSONObj arr ;

      rc = _parseSetArray( beModify, beginPos, endPos, arr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG ( PDERROR,
                      "Failed to parse $setarray [%s], rc: %d",
                      beModify.toString( TRUE, TRUE ).c_str(), rc ) ;
         goto done ;
      }

      {
         BSONArrayBuilder tmpbb ( bb.subarrayStart( pShort ) ) ;

         INT32 idx = 0 ;
         for ( idx = 0 ; idx < beginPos ; idx ++ )
         {
            tmpbb.appendNull() ;
         }

         BSONObjIterator iter( arr ) ;
         while ( iter.more() )
         {
            tmpbb << iter.next() ;
            idx ++ ;
         }
         BSONObj newObj = tmpbb.done() ;

         ADD_CHG_ARRAY_OBJ ( _dstChgBuilder, newObj, pRoot, "$set" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__MTHMDF__APPSETARRMDF2, rc ) ;
      return rc ;
   }

   INT32 _mthModifier::_parseSetArray( const BSONElement &toModify,
                                       INT32 &beginPos, INT32 &endPos,
                                       BSONObj &arr )
   {
      INT32 rc = SDB_OK ;
      BSONObj boModify ;
      BSONElement bePos ;

      if ( Object != toModify.type() )
      {
         PD_LOG_MSG( PDERROR,
                     "$setarray input must be an object: %s",
                     toModify.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      boModify = toModify.embeddedObject() ;

      if ( !boModify.hasField( SET_ARRAY_POS_NAME ) )
      {
         PD_LOG_MSG( PDERROR,
                     "$setarray must have the %s field",
                     SET_ARRAY_POS_NAME ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      bePos = boModify.getField( SET_ARRAY_POS_NAME ) ;
      if ( NumberInt == bePos.type() )
      {
         beginPos = bePos.numberInt() ;
         if ( beginPos < 0 )
         {
            PD_LOG_MSG( PDERROR,
                        "The %s field in $setarray must be a positive integer",
                        SET_ARRAY_POS_NAME ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         endPos = -1 ;
      }
      else if ( Array == bePos.type() )
      {
         INT32 idx = 0 ;
         BSONObjIterator i( bePos.embeddedObject() ) ;
         while ( i.more() && idx < 2 )
         {
            BSONElement beTmp = i.next() ;
            if ( NumberInt == beTmp.type() )
            {
               if ( 0 == idx )
               {
                  beginPos = beTmp.numberInt() ;
               }
               else if ( 1 == idx )
               {
                  endPos = beTmp.numberInt() ;
               }
            }
            else
            {
               PD_LOG_MSG( PDERROR,
                           "The %s field in $setarray must be an array with 2 integers",
                           SET_ARRAY_POS_NAME ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            idx ++ ;
         }
         if ( 2 != idx || i.more() )
         {
            PD_LOG_MSG( PDERROR,
                        "The %s field in $setarray must be an array with 2 "
                        "integers", SET_ARRAY_POS_NAME ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( beginPos < 0 )
         {
            PD_LOG_MSG( PDERROR,
                        "The beginPos [%d] of %s field in $setarray must be a "
                        "positive integer", beginPos, SET_ARRAY_POS_NAME ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( endPos < 0 )
         {
            PD_LOG_MSG( PDERROR,
                        "The endPos [%d] of %s field in $setarray must be a "
                        "positive integer", endPos, SET_ARRAY_POS_NAME ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( beginPos > endPos )
         {
            PD_LOG_MSG( PDERROR,
                        "The beginPos [%d] of %s field in $setarray must be <= the "
                        "endPos [%d]", beginPos, SET_ARRAY_POS_NAME, endPos ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else
      {
         PD_LOG_MSG( PDERROR,
                     "The %s field in $setarray must be an integer or an array",
                     SET_ARRAY_POS_NAME ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !boModify.hasField( SET_ARRAY_OBJS_NAME ) )
      {
         PD_LOG_MSG( PDERROR,
                     "$setarray input %s must have %s field",
                     toModify.toString( TRUE, TRUE ).c_str(),
                     SET_ARRAY_OBJS_NAME ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( Array != boModify.getField( SET_ARRAY_OBJS_NAME ).type() )
      {
         PD_LOG_MSG( PDERROR,
                     "%s field in $setarray input %s must be an array",
                     SET_ARRAY_OBJS_NAME,
                     toModify.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      arr = boModify.getObjectField( SET_ARRAY_OBJS_NAME ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   template<class Builder>
   void _mthModifier::_buildSetArray ( Builder *builder, const CHAR *pRoot,
                                       INT32 beginPos, INT32 endPos,
                                       const BSONObj &arr )
   {
      if ( !builder )
      {
         return ;
      }

      SDB_ASSERT( beginPos >= 0, "beginPos must >= 0" ) ;

      if ( beginPos <= 10 && endPos < 0 )
      {
         ADD_CHG_ARRAY_OBJ( builder, arr, pRoot, "$set" ) ;
      }
      else
      {
         BSONObjBuilder subBuilder( builder->subobjStart( "$setarray" ) ) ;
         BSONObjBuilder sub2Builder( subBuilder.subobjStart( pRoot ) ) ;

         if ( beginPos <= endPos )
         {
            BSONObjBuilder posBuilder( sub2Builder.subarrayStart( SET_ARRAY_POS_NAME ) ) ;
            posBuilder.append( posBuilder.numStr( 0 ), beginPos ) ;
            posBuilder.append( posBuilder.numStr( 1 ), endPos ) ;
            posBuilder.done() ;

            BSONArrayBuilder arrBuilder( sub2Builder.subarrayStart( SET_ARRAY_OBJS_NAME ) ) ;
            BSONObjIterator iter( arr ) ;
            INT32 pos = 0 ;
            while ( iter.more() && pos < beginPos )
            {
               iter.next() ;
               pos ++ ;
            }
            while ( iter.more() && pos <= endPos )
            {
               arrBuilder << iter.next() ;
               pos ++ ;
            }
            arrBuilder.done() ;
         }
         else
         {
            sub2Builder.append( SET_ARRAY_POS_NAME, beginPos ) ;

            BSONArrayBuilder arrBuilder( sub2Builder.subarrayStart( SET_ARRAY_OBJS_NAME ) ) ;
            BSONObjIterator iter( arr ) ;
            INT32 pos = 0 ;
            while ( iter.more() && pos < beginPos )
            {
               iter.next() ;
               pos ++ ;
            }
            while ( iter.more() )
            {
               arrBuilder << iter.next() ;
            }
            arrBuilder.done() ;
         }

         sub2Builder.done() ;
         subBuilder.done() ;
      }
   }

   template<class Builder>
   void _mthModifier::_buildSetArray ( Builder *builder, const CHAR *pRoot,
                                       INT32 beginPos, const BSONObj &arr )
   {
      if ( !builder )
      {
         return ;
      }

      SDB_ASSERT( beginPos >= 0, "beginPos must >= 0" ) ;

      if ( beginPos == 0 )
      {
         ADD_CHG_ARRAY_OBJ( builder, arr, pRoot, "$set" ) ;
      }
      else
      {
         BSONObjBuilder subBuilder( builder->subobjStart( "$setarray" ) ) ;
         BSONObjBuilder sub2Builder( subBuilder.subobjStart( pRoot ) ) ;
         sub2Builder.append( SET_ARRAY_POS_NAME, beginPos ) ;
         sub2Builder.appendArray( SET_ARRAY_OBJS_NAME, arr ) ;
         sub2Builder.done() ;
         subBuilder.done() ;
      }
   }

   template<class Builder>
   void _mthModifier::_buildSetArray ( Builder *builder, const CHAR *pRoot,
                                       INT32 beginPos, const BSONElement &ele )
   {
      if ( !builder )
      {
         return ;
      }

      SDB_ASSERT( beginPos >= 0, "beginPos must >= 0" ) ;

      if ( beginPos == 0 )
      {
         BSONObjBuilder subBuilder( builder->subobjStart ( "$set" ) ) ;
         BSONArrayBuilder arrBuilder( subBuilder.subarrayStart( pRoot ) ) ;
         arrBuilder << ele ;
         arrBuilder.done() ;
         subBuilder.done() ;
      }
      else
      {
         BSONObjBuilder subBuilder( builder->subobjStart( "$setarray" ) ) ;
         BSONObjBuilder sub2Builder( subBuilder.subobjStart( pRoot ) ) ;
         sub2Builder.append( SET_ARRAY_POS_NAME, beginPos ) ;
         BSONArrayBuilder arrBuilder( sub2Builder.subarrayStart( SET_ARRAY_OBJS_NAME ) ) ;
         arrBuilder << ele ;
         arrBuilder.done() ;
         sub2Builder.done() ;
         subBuilder.done() ;
      }
   }

   BOOLEAN _mthModifier::_pullElementMatch( BSONElement& org,
                                            BSONElement& toMatch,
                                            BOOLEAN fullMatch )
   {
      if ( toMatch.type() != Object )
      {
         return org.valuesEqual(toMatch) ;
      }
      if ( org.type() != Object )
      {
         return FALSE ;
      }

      if ( !fullMatch )
      {
         BSONObj objOrg = org.embeddedObject() ;
         BSONObjIterator itr( toMatch.embeddedObject() ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;
            BSONElement o = objOrg.getField( e.fieldName() ) ;

            if ( o.eoo() || 0 != e.woCompare( o, false ) )
            {
               return FALSE ;
            }
         }
         return TRUE ;
      }
      return org.woCompare(toMatch, FALSE) == 0 ;
   }


   template<class Builder>
   void _mthModifier::_applyUnsetModifier(Builder &b)
   {}

   void _mthModifier::_applyUnsetModifier(BSONArrayBuilder &b)
   {
      b.appendNull() ;
   }
   ModType _mthModifier::_parseModType ( const CHAR * field )
   {
      SDB_ASSERT ( field, "field can't be NULL " ) ;

      if ( MTH_OPERATOR_EYECATCHER == field[0] )
      {
         if ( field[1] == 'a' )
         {
            if ( field[2] == 'd' && field[3] == 'd' &&
                 field[4] == 't' && field[5] == 'o' &&
                 field[6] == 's' && field[7] == 'e' &&
                 field[8] == 't' && field[9] == 0 )
            {
               return ADDTOSET ;
            }
         }
         else if ( field[1] == 'b' )
         {
            if ( field[2] == 'i' && field[3] == 't' )
            {
               if ( field[4] == 0 )
               {
                  return BIT ;
               }
               else if ( field[4]=='a'&&field[5]=='n'&&
                         field[6]=='d'&&field[7]==0 )
               {
                  return BITAND ;
               }
               else if ( field[4]=='o'&&field[5]=='r'&&
                         field[6]==0 )
               {
                  return BITOR ;
               }
               else if ( field[4]=='n'&&field[5]=='o'&&
                         field[6]=='t'&&field[7]==0 )
               {
                  return BITNOT ;
               }
               else if ( field[4]=='x'&&field[5]=='o'&&
                         field[6]=='r'&&field[7]==0 )
               {
                  return BITXOR ;
               }
            }
         }
         else if ( field[1] == 'i' )
         {
            if ( field[2] == 'n' && field[3] == 'c' &&
                 field[4] == 0 )
            {
               return INC ;
            }
         }
         else if ( field[1] == 'k' )
         {
            if ( field[2] == 'e' && field[3] == 'e' &&
                 field[4] == 'p' && field[5] == 0 )
            {
               return KEEP ;
            }
         }
         else if ( field[1] == 'p' )
         {
            if ( field[2] == 'u' )
            {
               if ( field[3] == 'l' && field[4] == 'l' )
               {
                  if ( field[5] == 0 )
                  {
                     return PULL ;
                  }
                  else if ( field[5]=='_'&&field[6]=='b'&&
                            field[7]=='y'&&field[8]=='\0' )
                  {
                     return PULL_BY ;
                  }
                  else if ( field[5]=='_'&&field[6]=='a'&&
                            field[7]=='l'&&field[8]=='l'&&
                            field[9]==0 )
                  {
                     return PULL_ALL ;
                  }
                  else if ( field[9]=='_'&&field[10]=='b'&&
                            field[11]=='y'&&field[12]=='\0' )
                  {
                     return PULL_ALL_BY ;
                  }
               }
               else if ( field[3]=='s' && field[4] == 'h' )
               {
                  if ( field[5] == 0 )
                  {
                     return PUSH ;
                  }
                  else if ( field[5]=='_'&&field[6]=='a'&&
                            field[7]=='l'&&field[8]=='l'&&
                            field[9]==0 )
                  {
                     return PUSH_ALL ;
                  }
               }
            } //u
            else if (field[2] == 'o' )
            {
               if ( field[3] == 'p' && field[4] == 0 )
               {
                  return POP ;
               }
            }
         }// p
         else if ( field[1] == 'r' )
         {
            if ( field[2] == 'e' && field[3] == 'n' &&
                 field[4] == 'a' && field[5] == 'm' &&
                 field[6] == 'e' && field[7] == 0 )
            {
               return RENAME ;
            }
            else if ( field[2] == 'e' && field[3] == 'p' &&
                      field[4] == 'l' && field[5] == 'a' &&
                      field[6] == 'c' && field[7] == 'e' && field[8] == 0 )
            {
               return REPLACE ;
            }
         } // r
         else if ( field[1] == 's' )
         {
            if ( field[2] == 'e' && field[3] == 't' )
            {
               if ( field[4] == 0 )
               {
                  return SET ;
               }
               else if ( field[4] == 'a' && field[5] == 'r' &&
                         field[6] == 'r' && field[7] == 'a' &&
                         field[8] == 'y' && field[9] == 0 )
               {
                  return SETARRAY ;
               }
            }
         }
         else if ( field[1] == 'u' )
         {
            if ( field[2] == 'n' && field[3] == 's' &&
                 field[4] == 'e' && field[5] == 't' &&
                 field[6] == 0 )
            {
               return UNSET ;
            }
         }
         else if ( field[ 1 ] == 'n' )
         {
            if ( field[2] == 'u' && field[3] == 'l' &&
                 field[4] == 'l' && field[5] == 0 )
            {
               return NULLOPR ;
            }
         }
      }

      return UNKNOW ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF_PASELE, "_mthModifier::_parseElement" )
   INT32 _mthModifier::_parseElement ( const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF_PASELE );
      SDB_ASSERT ( ele.type() != Undefined, "Undefined element type" ) ;
      ModType type = _parseModType( ele.fieldName () ) ;
      if ( UNKNOW == type )
      {
         PD_LOG_MSG ( PDERROR, "Updator operator[%s] error", ele.fieldName () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( REPLACE == type )
      {
         _isReplace = TRUE ;
         if ( _modifierBits & MTH_MODIFIER_FIELD_OPR_BIT )
         {
            PD_LOG_MSG( PDERROR, "Operator[$replace] can't be used with "
                        "others", ele.fieldName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _modifierBits |= MTH_MODIFIER_RECORD_OPR_BIT ;
      }
      else if ( KEEP != type )
      {
         if ( _modifierBits & MTH_MODIFIER_RECORD_OPR_BIT )
         {
            PD_LOG_MSG( PDERROR, "Operator[%d] can't be used with $replace",
                        type ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _modifierBits |= MTH_MODIFIER_FIELD_OPR_BIT ;
      }

      switch ( ele.type() )
      {
      case Object:
      {
         BSONObjIterator j( ele.embeddedObject() ) ;
         while ( j.more () )
         {
            rc = _addModifier ( j.next(), type ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         } // while
         break ;
      }
      default:
      {
         PD_LOG_MSG ( PDERROR, "each element in modifier pattern must "
                      "be object" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF_PASELE, rc );
      return rc ;
   error :
      goto done ;
   }

   /*
      _compareFieldNames1 implement
   */
   const CHAR *_compareFieldNames1::getDollarValue ( const CHAR *s,
                                                     BOOLEAN *pUnknowDollar )
   {
      const CHAR *retStr = NULL ;

      if ( pUnknowDollar )
      {
         *pUnknowDollar = FALSE ;
      }

      if ( _dollarList && *s && '$' == *s )
      {
         INT64 temp = 0 ;
         INT32 dollarNum = 0 ;
         INT32 num = -1 ;

         if ( SDB_OK != ossStrToInt ( s + 1, &dollarNum ) )
         {
            goto done ;
         }

         for ( vector<INT64>::iterator it = _dollarList->begin() ;
               it != _dollarList->end() ; ++it )
         {
            temp = *it ;
            if ( dollarNum == ((temp>>32)&0xFFFFFFFF) )
            {
               num = (temp&0xFFFFFFFF) ;
               ossMemset ( _dollarBuff, 0, sizeof(_dollarBuff) ) ;
               ossSnprintf ( _dollarBuff, MTH_DOLLAR_FIELD_SIZE, "%d", num ) ;
               retStr = &_dollarBuff[0] ;
               goto done ;
            }
         }

         if ( pUnknowDollar )
         {
            *pUnknowDollar = TRUE ;
         }
      }

   done:
      return retStr ? retStr : s ;
   }

   INT32 _compareFieldNames1::_lexNumCmp ( const CHAR *s1,
                                           const CHAR *s2 )
   {
      BOOLEAN p1 = FALSE ;
      BOOLEAN p2 = FALSE ;
      BOOLEAN n1 = FALSE ;
      BOOLEAN n2 = FALSE ;

      CHAR   *e1 = NULL ;
      CHAR   *e2 = NULL ;

      INT32   len1   = 0 ;
      INT32   len2   = 0 ;
      INT32   result = 0 ;

      CHAR t1[MTH_DOLLAR_FIELD_SIZE+1] = {0} ;
      CHAR t2[MTH_DOLLAR_FIELD_SIZE+1] = {0} ;

      if ( _dollarList && *s1 && '$' == *s1 )
      {
         INT64 temp = 0 ;
         INT32 dollarNum = 0 ;
         INT32 num = -1 ;

         if ( SDB_OK == ossStrToInt ( s1 + 1, &dollarNum ) )
         {
            for ( vector<INT64>::iterator it = _dollarList->begin() ;
                  it != _dollarList->end() ; ++it )
            {
               temp = *it ;
               if ( dollarNum == ((temp>>32)&0xFFFFFFFF) )
               {
                  num = (temp&0xFFFFFFFF) ;
                  ossSnprintf ( t1, MTH_DOLLAR_FIELD_SIZE, "%d", num ) ;
                  s1 = t1 ;
                  break ;
               }
            }
         }
      }

      if ( _dollarList && *s2 && '$' == *s2 )
      {
         INT64 temp = 0 ;
         INT32 dollarNum = 0 ;
         INT32 num = -1 ;

         if ( SDB_OK == ossStrToInt ( s2 + 1, &dollarNum ) )
         {
            for ( vector<INT64>::iterator it = _dollarList->begin() ;
                  it != _dollarList->end() ; ++it )
            {
               temp = *it ;
               if ( dollarNum == ((temp>>32)&0xFFFFFFFF) )
               {
                  num = (temp&0xFFFFFFFF) ;
                  ossSnprintf ( t2, MTH_DOLLAR_FIELD_SIZE, "%d", num ) ;
                  s2 = t2 ;
                  break ;
               }
            }
         }
      }

      while( *s1 && *s2 )
      {
         p1 = ( *s1 == (CHAR)255 ) ;
         p2 = ( *s2 == (CHAR)255 ) ;
         if ( p1 && !p2 )
         {
            return 1 ;
         }
         if ( !p1 && p2 )
         {
            return -1 ;
         }

         n1 = _isNumber( *s1 ) ;
         n2 = _isNumber( *s2 ) ;

         if ( n1 && n2 )
         {
            while ( *s1 == '0' )
            {
               ++s1 ;
            }
            while ( *s2 == '0' )
            {
               ++s2 ;
            }

            e1 = (CHAR *)s1 ;
            e2 = (CHAR *)s2 ;
            while ( _isNumber ( *e1 ) )
            {
               ++e1 ;
            }
            while ( _isNumber ( *e2 ) )
            {
               ++e2 ;
            }

            len1 = (INT32)( e1 - s1 ) ;
            len2 = (INT32)( e2 - s2 ) ;

            if ( len1 > len2 )
            {
               return 1 ;
            }
            else if ( len2 > len1 )
            {
               return -1 ;
            }
            else if ( ( result = ossStrncmp ( s1, s2, len1 ) ) != 0 )
            {
               return result ;
            }
            s1 = e1 ;
            s2 = e2 ;
            continue ;
         }

         if ( n1 )
         {
            return 1 ;
         }

         if ( n2 )
         {
            return -1 ;
         }

         if ( *s1 > *s2 )
         {
            return 1 ;
         }

         if ( *s2 > *s1 )
         {
            return -1 ;
         }

         ++s1 ;
         ++s2 ;
      }

      if ( *s1 )
      {
         return 1 ;
      }
      if ( *s2 )
      {
         return -1 ;
      }
      return 0 ;
   }

   FieldCompareResult _compareFieldNames1::compField ( const CHAR* l,
                                                       const CHAR* r,
                                                       UINT32 *pLeftPos,
                                                       UINT32 *pRightPos )
   {
      const CHAR *pLTmp = l ;
      const CHAR *pRTmp = r ;
      const CHAR *pLDot = NULL ;
      const CHAR *pRDot = NULL ;
      INT32 result = 0 ;

      if ( pLeftPos )
      {
         *pLeftPos = 0 ;
      }
      if ( pRightPos )
      {
         *pRightPos = 0 ;
      }

      while ( pLTmp && pRTmp && *pLTmp && *pRTmp )
      {
         pLDot = ossStrchr( pLTmp, '.' ) ;
         pRDot = ossStrchr( pRTmp, '.' ) ;

         if ( pLDot )
         {
            *(CHAR*)pLDot = 0 ;
         }
         if ( pRDot )
         {
            *(CHAR*)pRDot = 0 ;
         }
         result = _lexNumCmp( pLTmp, pRTmp ) ;
         if ( pLDot )
         {
            *(CHAR*)pLDot = '.' ;
         }
         if ( pRDot )
         {
            *(CHAR*)pRDot = '.' ;
         }

         if ( result < 0 )
         {
            return LEFT_BEFORE ;
         }
         else if ( result > 0 )
         {
            return RIGHT_BEFORE ;
         }

         pLTmp = pLDot ? pLDot + 1 : NULL ;
         pRTmp = pRDot ? pRDot + 1 : NULL ;

         if ( pLeftPos )
         {
            *pLeftPos = pLTmp ? pLTmp - l : ossStrlen( l ) ;
         }
         if ( pRightPos )
         {
            *pRightPos = pRTmp ? pRTmp - r : ossStrlen( r ) ;
         }
      }

      if ( !pLTmp || !*pLTmp )
      {
         if ( !pRTmp || !*pRTmp )
         {
            return SAME ;
         }
         return RIGHT_SUBFIELD ;
      }
      return LEFT_SUBFIELD ;
   }

   void _mthModifier::modifierSort()
   {
      std::sort( _modifierElements.begin(),
                 _modifierElements.end(),
                 _compareFieldNames2( _dollarList ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF_LDPTN, "_mthModifier::loadPattern" )
   INT32 _mthModifier::loadPattern ( const BSONObj &modifierPattern,
                                     vector<INT64> *dollarList,
                                     BOOLEAN ignoreTypeError,
                                     const BSONObj* shardingKey,
                                     BOOLEAN strictDataMode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF_LDPTN );
      _modifierPattern = modifierPattern.copy() ;
      INT32 eleNum = 0 ;
      _dollarList = dollarList ;
      _ignoreTypeError = ignoreTypeError ;
      _fieldCompare.setDollarList( _dollarList ) ;

      BSONObjIterator i( _modifierPattern ) ;
      while ( i.more() )
      {
         rc = _parseElement(i.next() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to parse match "
                         "pattern[%s, pos: %d], rc: %d",
                         modifierPattern.toString().c_str(), eleNum,
                         rc ) ;
            goto error ;
         }
         eleNum ++ ;
      }

      if ( !_isReplace && _keepKeys.size() > 0 )
      {
         PD_LOG_MSG( PDERROR, "Operator $keep can only be used with "
                     "$replace") ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( _isReplace && !_isReplaceID )
      {
         _keepKeys.insert( DMS_ID_KEY_NAME ) ;
      }

      modifierSort() ;

      if ( NULL != shardingKey && !shardingKey->isEmpty() )
      {
         _shardingKeyGen = SDB_OSS_NEW _ixmIndexKeyGen(
                              *shardingKey, GEN_OBJ_KEEP_FIELD_NAME ) ;
         if ( NULL == _shardingKeyGen )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR,
                        "Failed to create new sharding key gen, rc=%d", rc ) ;
            goto error ;
         }
      }

      _initialized = TRUE ;
      _strictDataMode = strictDataMode ;

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF_LDPTN, rc );
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN _mthModifier::_dupFieldName ( const BSONElement &l,
                                         const BSONElement &r )
   {
      return !l.eoo() && !r.eoo() && (l.rawdata() != r.rawdata()) &&
        ossStrncmp(l.fieldName(),r.fieldName(),ossStrlen(r.fieldName()))==0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPNEW, "_mthModifier::_appendNew" )
   template<class Builder>
   INT32 _mthModifier::_appendNew ( const CHAR *pRoot, const CHAR *pShort,
                                    Builder& b, SINT32 *modifierIndex )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPNEW );
      ModifierElement *me = &_modifierElements[(*modifierIndex)] ;

      switch ( me->_modType )
      {
      case INC:
      case SET:
      {
         try
         {
            b.appendAs ( me->_toModify, pShort ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG_MSG ( ( _ignoreTypeError ? PDINFO : PDERROR ),
                         "Failed to append for %s: %s",
                         me->_toModify.toString().c_str(), e.what() ) ;
            if ( !_ignoreTypeError )
            {
               rc = SDB_INVALIDARG ;
               goto done ;
            }
         }
         ADD_CHG_ELEMENT_AS ( _dstChgBuilder, me->_toModify, pRoot, "$set" ) ;
         break ;
      }
      case UNSET:
      case PULL:
      case PULL_BY:
      case PULL_ALL:
      case PULL_ALL_BY:
      case POP:
      case RENAME:
      {
         PD_LOG_MSG ( PDERROR, "Unexpected codepath" ) ;
         rc = SDB_SYS ;
         goto done ;
      }
      case PUSH:
      {
         BSONObjBuilder bb ( b.subarrayStart( pShort ) ) ;
         bb.appendAs ( me->_toModify, bb.numStr(0) ) ;
         BSONObj newObj = bb.done() ;

         ADD_CHG_ARRAY_OBJ ( _dstChgBuilder, newObj, pRoot, "$set" ) ;
         break ;
      }
      case PUSH_ALL:
      {
         if ( me->_toModify.type() != Array )
         {
            PD_LOG_MSG ( PDERROR, "pushed data type is not array: %s",
                         me->_toModify.toString().c_str());
            rc = SDB_INVALIDARG ;
            goto done ;
         }

         b.appendAs ( me->_toModify, pShort ) ;
         ADD_CHG_ELEMENT_AS ( _dstChgBuilder, me->_toModify, pRoot, "$set" ) ;
         break ;
      }
      case ADDTOSET:
      {
         if ( Array != me->_toModify.type() )
         {
           PD_LOG_MSG ( PDERROR, "added data type is not array: %s",
                        me->_toModify.toString().c_str()) ;
           rc = SDB_INVALIDARG ;
           goto done ;
         }
         BSONObjBuilder bb (b.subarrayStart( pShort ) ) ;
         BSONObjIterator j ( me->_toModify.embeddedObject() ) ;
         INT32 n = 0 ;
         BSONElementSet eleset ;
         while ( j.more() )
         {
            eleset.insert( j.next() ) ;
         }
         BSONElementSet::iterator it ;
         for ( it = eleset.begin(); it != eleset.end(); it++ )
         {
            bb.appendAs((*it), bb.numStr(n++)) ;
         }
         BSONObj newObj = bb.done() ;

         if ( n != 0 )
         {
            ADD_CHG_ARRAY_OBJ ( _dstChgBuilder, newObj, pRoot, "$set" ) ;
         }
         break ;
      }
      case BITXOR:
      case BITNOT:
      case BITAND:
      case BITOR:
         rc = _appendBitModifier ( pRoot, pShort, b, 0, *me ) ;
         break ;
      case BIT:
         rc = _appendBitModifier2 ( pRoot, pShort, b, 0, *me ) ;
         break ;
      case SETARRAY :
         rc = _appendSetArrayModifier( pRoot, pShort, b, *me ) ;
         break ;
      default:
         PD_LOG_MSG ( PDERROR, "unknow modifier type[%d]", me->_modType ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      if ( SDB_OK == rc )
      {
         _incModifierIndex( modifierIndex ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPNEW, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPNEWFRMMODS, "_mthModifier::_appendNewFromMods" )
   template<class Builder>
   INT32 _mthModifier::_appendNewFromMods ( CHAR **ppRoot,
                                            INT32 &rootBufLen,
                                            INT32 rootLen,
                                            UINT32 modifierRootLen,
                                            Builder &b,
                                            SINT32 *modifierIndex,
                                            BOOLEAN hasCreateNewRoot )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPNEWFRMMODS ) ;
      ModifierElement *me = &_modifierElements[(*modifierIndex)] ;
      BOOLEAN hasUnknowDollar = FALSE ;
      const CHAR *pDollar = NULL ;
      INT32 newRootLen = rootLen ;


      const CHAR *fieldName = me->_toModify.fieldName() ;
      const CHAR *temp = fieldName + modifierRootLen ;
      const CHAR *dot = ossStrchr ( temp, '.' ) ;

      if ( UNSET == me->_modType ||
           PULL == me->_modType ||
           PULL_BY == me->_modType ||
           PULL_ALL == me->_modType ||
           PULL_ALL_BY == me->_modType ||
           POP == me->_modType ||
           RENAME == me->_modType ||
           NULLOPR == me->_modType )
      {
         _incModifierIndex( modifierIndex ) ;
         goto done ;
      }

      if ( dot )
      {
         *(CHAR*)dot = 0 ;
      }
      pDollar = _fieldCompare.getDollarValue( temp, &hasUnknowDollar ) ;
      temp = *ppRoot + newRootLen ;
      rc = mthAppendString( ppRoot, rootBufLen, newRootLen, pDollar, -1,
                            &newRootLen ) ;
      if ( dot )
      {
         *(CHAR*)dot = '.' ;
      }

      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to append string, rc: %d", rc ) ;
         goto error ;
      }
      else if ( hasUnknowDollar )
      {
         _incModifierIndex( modifierIndex ) ;
         goto done ;
      }

      if ( !hasCreateNewRoot )
      {
         ADD_CHG_UNSET_FIELD ( _srcChgBuilder, *ppRoot ) ;
         hasCreateNewRoot = TRUE ;
      }

      if ( dot )
      {
         BSONObjBuilder bb ( b.subobjStart( temp ) ) ;
         const BSONObj obj ;
         BSONObjIteratorSorted es( obj ) ;
         rc = mthAppendString ( ppRoot, rootBufLen, newRootLen, ".", 1,
                                &newRootLen ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to append string, rc: %d", rc ) ;
            goto error ;
         }

         rc = _buildNewObj ( ppRoot, rootBufLen, newRootLen,
                             bb, es, modifierIndex, hasCreateNewRoot ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to build new object for %s, rc: %d",
                         me->_toModify.toString().c_str(), rc ) ;
            goto error ;
         }
         bb.done() ;
      }
      else
      {
         try
         {
            rc = _appendNew ( *ppRoot, temp, b, modifierIndex ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG_MSG ( PDERROR, "Failed to append for %s: %s",
                         me->_toModify.toString().c_str(), e.what() );
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to append for %s, rc: %d",
                         me->_toModify.toString().c_str(), rc ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPNEWFRMMODS, rc );
      return rc ;
   error :
      goto done ;
   }
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__ALYCHG, "_mthModifier::_applyChange" )
   template<class Builder>
   INT32 _mthModifier::_applyChange ( CHAR **ppRoot,
                                      INT32 &rootBufLen,
                                      INT32 rootLen,
                                      BSONElement &e,
                                      Builder &b,
                                      SINT32 *modifierIndex )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__ALYCHG ) ;
      ModifierElement *me = &_modifierElements[(*modifierIndex)] ;

      switch ( me->_modType )
      {
      case INC:
         rc = _applyIncModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case SET:
         rc = _applySetModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case UNSET:
      {
         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, e, *ppRoot, "$set" ) ;
         ADD_CHG_ELEMENT_AS ( _dstChgBuilder, me->_toModify, *ppRoot,
                              "$unset" ) ;
         _applyUnsetModifier( b ) ;
         break ;
      }
      case PUSH:
         rc = _applyPushModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case PUSH_ALL:
         rc = _applyPushAllModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case PULL:
      case PULL_BY:
      case PULL_ALL:
      case PULL_ALL_BY:
         rc = _applyPullModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case POP:
         rc = _applyPopModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case BITNOT:
      case BITXOR:
      case BITAND:
      case BITOR:
         rc = _applyBitModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case BIT:
         rc = _applyBitModifier2 ( *ppRoot, b, e, *me ) ;
         break ;
      case ADDTOSET:
         rc = _applyAddtoSetModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case RENAME:
      {
         const CHAR *pDotR = ossStrrchr( *ppRoot, '.' ) ;
         UINT32 pos = pDotR ? ( pDotR - *ppRoot + 1 ) : 0 ;
         string newNameStr( *ppRoot, pos ) ;
         newNameStr += me->_toModify.valuestr() ;

         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, e, *ppRoot, "$set" ) ;
         ADD_CHG_UNSET_FIELD ( _srcChgBuilder, newNameStr ) ;

         ADD_CHG_UNSET_FIELD ( _dstChgBuilder, *ppRoot ) ;
         ADD_CHG_ELEMENT_AS ( _dstChgBuilder, e, newNameStr, "$set" ) ;

         b.appendAs ( e, me->_toModify.valuestr() ) ;
         break ;
      }
      case NULLOPR:
         break ;
      case SETARRAY :
         rc = _applySetArrayModifier( *ppRoot, b, e, *me ) ;
         break ;
      default :
         PD_LOG_MSG ( PDERROR, "unknow modifier type[%d]", me->_modType ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      if ( SDB_OK == rc )
      {
         _incModifierIndex( modifierIndex ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__ALYCHG, rc );
      return rc ;
   }

   template<class Builder>
   INT32 _mthModifier::_buildNewObjReplace( Builder &b,
                                            BSONObjIteratorSorted &es )
   {
      while ( es.more() && _keepKeys.size() > 0 )
      {
         BSONElement e = es.next() ;
         if ( _keepKeys.count( e.fieldName() ) )
         {
            b.append( e ) ;
         }

         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, e, e.fieldName(), 
                              "$replace" ) ;
      }

      UINT32 i = 0 ;
      while ( i < _modifierElements.size() )
      {
         const CHAR *pTmpFieldName = _modifierElements[i]._toModify.fieldName() ;
         ADD_CHG_ELEMENT_AS ( _dstChgBuilder, _modifierElements[i]._toModify, 
                              pTmpFieldName, "$replace" ) ;
         b.append( _modifierElements[i]._toModify ) ;
         ++i ;
      }

      set<string>::iterator it = _keepKeys.begin() ;
      while ( it != _keepKeys.end() )
      {
         ADD_CHG_FIELD_VALUE ( _dstChgBuilder, *it, 1, "$keep" ) ;
         ++it ;
      }

      return SDB_OK  ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__BLDNEWOBJ, "_mthModifier::_buildNewObj" )
   template<class Builder>
   INT32 _mthModifier::_buildNewObj ( CHAR **ppRoot,
                                      INT32 &rootBufLen,
                                      INT32 rootLen,
                                      Builder &b,
                                      BSONObjIteratorSorted &es,
                                      SINT32 *modifierIndex,
                                      BOOLEAN hasCreateNewRoot )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__BLDNEWOBJ ) ;

      BSONElement e ;
      BSONElement prevE ;
      UINT32 compareLeftPos = 0 ;
      INT32 newRootLen = rootLen ;

      if ( _isReplace )
      {
         rc = _buildNewObjReplace( b, es ) ;
         goto done ;
      }

      e = es.next() ;

      while( !e.eoo() && (*modifierIndex)<(SINT32)_modifierElements.size() )
      {
         if ( _dupFieldName(prevE, e))
         {
            b.append( e ) ;
            prevE = e ;
            e = es.next() ;
            continue ;
         }
         prevE = e ;

         (*ppRoot)[rootLen] = '\0' ;
         newRootLen = rootLen ;

         rc = mthAppendString ( ppRoot, rootBufLen, newRootLen,
                                e.fieldName(), -1, &newRootLen ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to append string, rc: %d", rc ) ;

         /*FieldCompareResult cmp = compareDottedFieldNames (
               _modifierElements[(*modifierIndex)]._toModify.fieldName(),
               *ppRoot ) ;*/
         FieldCompareResult cmp = _fieldCompare.compField (
               _modifierElements[(*modifierIndex)]._toModify.fieldName(),
               *ppRoot, &compareLeftPos, NULL ) ;





         switch ( cmp )
         {
         case LEFT_SUBFIELD:
            if ( e.type() != Object && e.type() != Array )
            {
               PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                            "Invalid field type: %s", e.toString().c_str() ) ;
               if ( _ignoreTypeError )
               {
                  _incModifierIndex( modifierIndex ) ;
                  continue ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }

            rc = mthAppendString ( ppRoot, rootBufLen, newRootLen, ".", 1,
                                   &newRootLen ) ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to append string, rc: %d", rc ) ;

            if ( e.type() == Object )
            {
               BSONObjBuilder bb(b.subobjStart(e.fieldName()));
               BSONObjIteratorSorted bis(e.Obj());

               rc = _buildNewObj ( ppRoot, rootBufLen, newRootLen,
                                   bb, bis, modifierIndex,
                                   hasCreateNewRoot ) ;
               if ( rc )
               {
                  PD_LOG_MSG ( PDERROR, "Failed to build object: %s, rc: %d",
                               e.toString().c_str(), rc ) ;
                  goto error ;
               }
               bb.done() ;
            }
            else
            {
               BSONArrayBuilder ba( b.subarrayStart( e.fieldName() ) ) ;
               BSONObjIteratorSorted bis(e.embeddedObject());
               rc = _buildNewObj ( ppRoot, rootBufLen, newRootLen,
                                   ba, bis, modifierIndex,
                                   hasCreateNewRoot ) ;
               if ( rc )
               {
                  PD_LOG_MSG ( PDERROR, "Failed to build array: %s, rc: %d",
                               e.toString().c_str(), rc ) ;
                  goto error ;
               }
               ba.done() ;
            }
            e = es.next() ;
            break ;

         case LEFT_BEFORE:


            (*ppRoot)[rootLen] = '\0' ;
            newRootLen = rootLen ;
            rc = _appendNewFromMods ( ppRoot, rootBufLen, newRootLen,
                                      compareLeftPos, b,
                                      modifierIndex, hasCreateNewRoot ) ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to append for %s, rc: %d",
                          _modifierElements[(*modifierIndex)
                          ]._toModify.toString().c_str(), rc ) ;

            break ;

         case SAME:
            try
            {
               rc = _applyChange ( ppRoot, rootBufLen, newRootLen, e, b,
                                   modifierIndex ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG_MSG ( PDERROR, "Failed to apply changes for %s: %s",
                            _modifierElements[(*modifierIndex)
                            ]._toModify.toString().c_str(),
                            e.what() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            if ( rc )
            {
               PD_LOG_MSG ( PDERROR, "Failed to apply change for %s, rc: %d",
                            _modifierElements[(*modifierIndex)
                            ]._toModify.toString().c_str(), rc ) ;
               goto error ;
            }
            e=es.next();
            break ;

         case RIGHT_BEFORE:

            b.append(e) ;
            e=es.next() ;
            break ;

         case RIGHT_SUBFIELD:
         default :
            PD_LOG_MSG ( PDERROR, "Reaching unexpected codepath, cmp( %s, %s, "
                         "res: %d )", _modifierElements[(*modifierIndex)
                         ]._toModify.toString().c_str(),
                         *ppRoot, cmp ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      while ( !e.eoo() )
      {
         b.append(e) ;
         e=es.next() ;
      }

      while ( (*modifierIndex) < (SINT32)_modifierElements.size() )
      {
         (*ppRoot)[rootLen] = '\0' ;
         newRootLen = rootLen ;

         /*FieldCompareResult cmp = compareDottedFieldNames (
               _modifierElements[(*modifierIndex)]._toModify.fieldName(),
               *ppRoot ) ;*/
         FieldCompareResult cmp = _fieldCompare.compField (
               _modifierElements[(*modifierIndex)]._toModify.fieldName(),
               *ppRoot, &compareLeftPos, NULL ) ;
         if ( LEFT_SUBFIELD == cmp )
         {
            rc = _appendNewFromMods ( ppRoot, rootBufLen, newRootLen,
                                      compareLeftPos, b,
                                      modifierIndex, hasCreateNewRoot ) ;
            if ( rc )
            {
               PD_LOG_MSG ( PDERROR, "Failed to append for %s, rc: %d",
                            _modifierElements[(*modifierIndex)
                            ]._toModify.toString().c_str(), rc );
               goto error ;
            }
         }
         else
         {
            goto done ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__BLDNEWOBJ, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF_MODIFY, "_mthModifier::modify" )
   INT32 _mthModifier::modify ( const BSONObj &source, BSONObj &target,
                                BSONObj *srcID, BSONObj *srcChange,
                                BSONObj *dstID, BSONObj *dstChange,
                                BSONObj *srcShardingKey,
                                BSONObj *dstShardingKey )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF_MODIFY );
      SDB_ASSERT(_initialized, "The modifier has not been initialized, please "
                 "call 'loadPattern' before using it" ) ;
      SDB_ASSERT(target.isEmpty(), "target should be empty") ;

      CHAR *pBuffer = NULL ;
      INT32 bufferSize = 0 ;

      if ( _dollarList && _dollarList->size() > 0 )
      {
         modifierSort() ;
      }

      BSONObjBuilder builder ( (int)(source.objsize()*1.1));
      BSONObjIteratorSorted es(source) ;


      SINT32 modifierIndex = -1 ;
      _incModifierIndex( &modifierIndex ) ;

      pBuffer = (CHAR*)SDB_OSS_MALLOC ( SDB_PAGE_SIZE ) ;
      if ( !pBuffer )
      {
         PD_LOG_MSG ( PDERROR, "Failed to allocate buffer for select" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      bufferSize = SDB_PAGE_SIZE ;
      pBuffer[0] = '\0' ;

      if ( srcChange )
      {
         _srcChgBuilder = SDB_OSS_NEW BSONObjBuilder ( (int)(source.objsize()*0.3) ) ;
         if ( !_srcChgBuilder )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG ( PDERROR, "Failed to alloc memory for src BSONObjBuilder" ) ;
            goto error ;
         }
      }
      if ( dstChange )
      {
         _dstChgBuilder = SDB_OSS_NEW BSONObjBuilder ( (int)(source.objsize()*0.3) ) ;
         if ( !_dstChgBuilder )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG ( PDERROR, "Failed to alloc memory for dst BSONObjBuilder" ) ;
            goto error ;
         }
      }

      rc = _buildNewObj ( &pBuffer, bufferSize, 0, builder, es,
                          &modifierIndex, FALSE ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to modify target, rc: %d", rc ) ;
         goto error ;
      }
      target=builder.obj();

      if ( srcID )
      {
         BSONObjBuilder srcIDBuilder ;
         BSONElement id = source.getField ( DMS_ID_KEY_NAME ) ;
         if ( id.eoo() )
         {
            PD_LOG_MSG ( PDERROR, "Failed to get source oid, %s",
                         source.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         srcIDBuilder.append ( id ) ;
         *srcID = srcIDBuilder.obj() ;
      }
      if ( srcChange )
      {
         *srcChange = _srcChgBuilder->obj () ;
      }
      if ( dstID )
      {
         BSONObjBuilder dstIDBuilder ;
         BSONElement id = target.getField ( DMS_ID_KEY_NAME ) ;
         if ( id.eoo() )
         {
            PD_LOG_MSG ( PDERROR, "Failed to get target oid, %s",
                         target.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         dstIDBuilder.append ( id ) ;
         *dstID = dstIDBuilder.obj() ;
      }
      if ( dstChange )
      {
         *dstChange = _dstChgBuilder->obj () ;
      }

      if ( NULL != _shardingKeyGen )
      {
         if ( NULL != srcShardingKey )
         {
            BSONObjSet keySet ;
            rc = _shardingKeyGen->getKeys( source, keySet, NULL, TRUE, TRUE, TRUE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "Failed to get sharding key from obj: %s, rc=%d",
                        source.toString().c_str(), rc ) ;
               goto error ;
            }

            if ( keySet.size() == 1 )
            {
               *srcShardingKey = *keySet.begin() ;
            }
         }

         if ( NULL != dstShardingKey )
         {
            BSONObjSet keySet ;
            rc = _shardingKeyGen->getKeys( target, keySet, NULL, TRUE, TRUE, TRUE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "Failed to get sharding key from obj: %s, rc=%d",
                        target.toString().c_str(), rc ) ;
               goto error ;
            }

            if ( keySet.size() == 1 )
            {
               *dstShardingKey = *keySet.begin() ;
            }
         }
      }

   done :
      if ( pBuffer )
      {
         SDB_OSS_FREE ( pBuffer ) ;
      }
      if ( _srcChgBuilder )
      {
         SDB_OSS_DEL _srcChgBuilder ;
         _srcChgBuilder = NULL ;
      }
      if ( _dstChgBuilder )
      {
         SDB_OSS_DEL _dstChgBuilder ;
         _dstChgBuilder = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__MTHMDF_MODIFY, rc );
      return rc ;
   error :
      goto done ;
   }

}

