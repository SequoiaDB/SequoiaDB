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

   Source File Name = mthModifier.hpp

   Descriptive Name = Method Modifier Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for modify
   operation, which is changing a data record based on modification rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTHMODIFIER_HPP_
#define MTHMODIFIER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <vector>
#include <set>
#include "ossUtil.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"
#include "mthCommon.hpp"
#include "ixmIndexKey.hpp"

using namespace bson ;

namespace engine
{
   /*
      ModType define
   */
   enum ModType
   {
      INC = 0,
      SET,
      PUSH,
      PUSH_ALL,
      PULL,
      PULL_BY,
      PULL_ALL,
      PULL_ALL_BY,
      POP,
      UNSET,
      BITNOT,
      BITXOR,
      BITAND,
      BITOR,
      BIT,
      ADDTOSET,
      RENAME,
      NULLOPR,
      REPLACE,
      KEEP,
      SETARRAY,

      UNKNOW
   } ;

   /*
      _ModifierElement define
   */
   class _ModifierElement : public SDBObject
   {
   public :
      BSONElement _toModify ; // the element to modify
      ModType     _modType ;
      INT32       _dollarNum ;

      _ModifierElement ( const BSONElement &e, ModType type,
                         INT32 dollarNum = 0 )
      {
         _toModify = e ;
         _modType = type ;
         _dollarNum = dollarNum ;
      }
   } ;
   typedef _ModifierElement ModifierElement ;


   #define MTH_DOLLAR_FIELD_SIZE          (10)
   /*
      _compareFieldNames1 define
   */
   class _compareFieldNames1
   {
   private:
      vector<INT64> *_dollarList ;
      CHAR           _dollarBuff[ MTH_DOLLAR_FIELD_SIZE + 1 ] ;

   private:
      OSS_INLINE BOOLEAN _isNumber( CHAR c )
      {
         return c >= '0' && c <= '9' ;
      }

      INT32 _lexNumCmp ( const CHAR *s1, const CHAR *s2 ) ;

   public:
      _compareFieldNames1( vector<INT64> *dollarList = NULL )
      : _dollarList(NULL)
      {
         _dollarList = dollarList ;
         ossMemset( _dollarBuff, 0, sizeof(_dollarBuff) ) ;
      }

      void setDollarList( vector<INT64> *dollarList )
      {
         _dollarList = dollarList ;
      }

      const CHAR *getDollarValue ( const CHAR *s,
                                   BOOLEAN *pUnknowDollar = NULL ) ;

      FieldCompareResult compField( const char* l, const char* r,
                                    UINT32 *pLeftPos = NULL,
                                    UINT32 *pRightPos = NULL ) ;

   } ;

   /*
      _compareFieldNames2 define
   */
   class _compareFieldNames2
   {
   private:
      vector<INT64> *_dollarList ;

   public:
      _compareFieldNames2( vector<INT64> *dollarList = NULL )
      : _dollarList(NULL)
      {
         _dollarList = dollarList ;
      }

      BOOLEAN operator () ( const ModifierElement &l,
                            const ModifierElement &r ) const
      {
         _compareFieldNames1 compare ( _dollarList ) ;
         FieldCompareResult result = compare.compField(
            l._toModify.fieldName(), r._toModify.fieldName() ) ;
         return ( (result == RIGHT_SUBFIELD) || (result == LEFT_BEFORE) ) ;
      }
   } ;

   #define MTH_MODIFIER_FIELD_OPR_BIT              0x00000001
   #define MTH_MODIFIER_RECORD_OPR_BIT             0x00000002

   /*
      _mthModifier define
   */
   class _mthModifier : public SDBObject
   {
   private :
      BSONObjBuilder *_srcChgBuilder ;
      BSONObjBuilder *_dstChgBuilder ;

      BSONObj _modifierPattern ;
      BOOLEAN _initialized ;
      vector<ModifierElement> _modifierElements ;
      UINT32  _modifierBits ;

      _ixmIndexKeyGen *_shardingKeyGen ;

      set<string>    _keepKeys ;
      BOOLEAN        _isReplaceID ;
      BOOLEAN        _isReplace ;

      vector<INT64> *_dollarList ;
      _compareFieldNames1  _fieldCompare ;
      BOOLEAN        _ignoreTypeError ;
      BOOLEAN        _strictDataMode ;

      INT32 _addModifier ( const BSONElement &ele, ModType type ) ;
      INT32 _parseElement ( const BSONElement &ele ) ;
      ModType _parseModType ( const CHAR *field ) ;
      OSS_INLINE void _incModifierIndex( INT32 *modifierIndex ) ;

      template<class VType>
      INT32 _bitCalc ( ModType type, VType l, VType r, VType &out );

      BOOLEAN _dupFieldName ( const BSONElement &l,
                              const BSONElement &r ) ;
      BOOLEAN _pullElementMatch( BSONElement& org,
                                 BSONElement& toMatch,
                                 BOOLEAN fullMatch ) ;
      template<class Builder>
      void _applyUnsetModifier(Builder &b) ;

      void _applyUnsetModifier(BSONArrayBuilder &b) ;

      template<class Builder>
      INT32 _applyIncModifier ( const CHAR *pRoot, Builder &bb,
                                const BSONElement &in,
                                ModifierElement &me ) ;
      template<class Builder>
      INT32 _applySetModifier ( const CHAR *pRoot, Builder &bb,
                                const BSONElement &in,
                                ModifierElement &me ) ;
      template<class Builder>
      INT32 _applyPushModifier ( const CHAR *pRoot, Builder &bb,
                                 const BSONElement &in,
                                 ModifierElement &me ) ;
      template<class Builder>
      INT32 _applyPushAllModifier ( const CHAR *pRoot, Builder &bb,
                                    const BSONElement &in,
                                    ModifierElement &me ) ;
      template<class Builder>
      INT32 _applyPullModifier ( const CHAR *pRoot, Builder &bb,
                                 const BSONElement &in,
                                 ModifierElement &me ) ;
      template<class Builder>
      INT32 _applyPopModifier ( const CHAR *pRoot, Builder &bb,
                                const BSONElement &in,
                                ModifierElement &me ) ;
      template<class Builder>
      INT32 _applyBitModifier ( const CHAR *pRoot, Builder &bb,
                                const BSONElement &in,
                                ModifierElement &me ) ;
      template<class Builder>
      INT32 _applyBitModifier2 ( const CHAR *pRoot, Builder &bb,
                                 const BSONElement &in,
                                 ModifierElement &me ) ;
      template<class Builder>
      INT32 _applyAddtoSetModifier ( const CHAR *pRoot, Builder &bb,
                                     const BSONElement &in,
                                     ModifierElement &me ) ;
      template<class Builder>
      INT32 _appendBitModifier ( const CHAR *pRoot, const CHAR *pShort,
                                 Builder &bb, INT32 in,
                                 ModifierElement &me ) ;
      template<class Builder>
      INT32 _appendBitModifier2 ( const CHAR *pRoot, const CHAR *pShort,
                                  Builder &bb, INT32 in,
                                  ModifierElement &me ) ;

      template<class Builder>
      INT32 _applySetArrayModifier ( const CHAR *pRoot, Builder &bb,
                                     const BSONElement &in,
                                     ModifierElement &me ) ;

      template<class Builder>
      INT32 _appendSetArrayModifier ( const CHAR *pRoot, const CHAR *pShort,
                                      Builder &bb, ModifierElement &me ) ;

      INT32 _parseSetArray( const BSONElement &toModify, INT32 &beginPos,
                            INT32 &endPos, BSONObj &arr ) ;

      template<class Builder>
      OSS_INLINE void _buildSetArray ( Builder *builder, const CHAR *pRoot,
                                       INT32 beginPos, INT32 endPos,
                                       const BSONObj &arr ) ;

      template<class Builder>
      OSS_INLINE void _buildSetArray ( Builder *builder, const CHAR *pRoot,
                                       INT32 beginPos, const BSONObj &arr ) ;

      template<class Builder>
      OSS_INLINE void _buildSetArray ( Builder *builder, const CHAR *pRoot,
                                       INT32 beginPos, const BSONElement &ele ) ;

      template<class Builder>
      INT32 _applyChange ( CHAR **ppRoot,
                           INT32 &rootBufLen,
                           INT32 rootLen,
                           BSONElement &e,
                           Builder &b,
                           SINT32 *modifierIndex ) ;

      template<class Builder>
      INT32 _appendNew ( const CHAR *pRoot, const CHAR *pShort,
                         Builder& b, SINT32 *modifierIndex ) ;

      template<class Builder>
      INT32 _appendNewFromMods ( CHAR **ppRoot,
                                 INT32 &rootBufLen,
                                 INT32 rootLen,
                                 UINT32 modifierRootLen,
                                 Builder &b,
                                 SINT32 *modifierIndex,
                                 BOOLEAN hasCreateNewRoot ) ;
      template<class Builder>
      INT32 _buildNewObj ( CHAR **ppRoot,
                           INT32 &rootBufLen,
                           INT32 rootLen,
                           Builder &b,
                           BSONObjIteratorSorted &es,
                           SINT32 *modifierIndex,
                           BOOLEAN hasCreateNewRoot ) ;

      template<class Builder>
      INT32 _buildNewObjReplace( Builder &b, BSONObjIteratorSorted &es ) ;
   public :
      _mthModifier ()
      {
         _initialized   = FALSE ;
         _srcChgBuilder = NULL ;
         _dstChgBuilder = NULL ;
         _dollarList    = NULL ;
         _ignoreTypeError = TRUE ;
         _modifierBits  = 0 ;
         _isReplace     = FALSE ;
         _isReplaceID   = FALSE ;
         _shardingKeyGen = NULL ;
         _strictDataMode = FALSE ;
      }
      ~_mthModifier()
      {
         _modifierElements.clear() ;
         SAFE_OSS_DELETE( _shardingKeyGen ) ;
      }
      INT32 loadPattern ( const BSONObj &modifierPattern,
                          vector<INT64> *dollarList = NULL,
                          BOOLEAN ignoreTypeError = TRUE,
                          const BSONObj* shardingKey = NULL,
                          BOOLEAN strictDataMode = FALSE ) ;
      void modifierSort() ;
      INT32 modify ( const BSONObj &source, BSONObj &target,
                     BSONObj *srcID = NULL,
                     BSONObj *srcChange = NULL,
                     BSONObj *dstID = NULL,
                     BSONObj *dstChange = NULL,
                     BSONObj *srcShardingKey = NULL,
                     BSONObj *dstShardingKey = NULL ) ;
      OSS_INLINE BOOLEAN isInitialized () { return _initialized ; }
   } ;
   typedef _mthModifier mthModifier ;

   /*
      OSS_INLINE function
   */
   OSS_INLINE void _mthModifier::_incModifierIndex( INT32 *modifierIndex )
   {
      INT32 tmpModifierIndex = *modifierIndex ;
      ++(*modifierIndex) ;
      while ( *modifierIndex < (INT32)_modifierElements.size() )
      {
         if ( _dollarList && _dollarList->size() == 0 &&
              _modifierElements[*modifierIndex]._dollarNum > 0 )
         {
            ++(*modifierIndex) ;
            continue ;
         }
         else if ( !mthCheckUnknowDollar(
            _modifierElements[*modifierIndex]._toModify.fieldName(),
            _dollarList ) )
         {
            ++(*modifierIndex) ;
            continue ;
         }
         else if ( tmpModifierIndex >= 0 )
         {
            FieldCompareResult cmp = _fieldCompare.compField (
              _modifierElements[tmpModifierIndex]._toModify.fieldName(),
              _modifierElements[*modifierIndex]._toModify.fieldName() ) ;
            if ( SAME == cmp || RIGHT_SUBFIELD == cmp )
            {
               ++(*modifierIndex) ;
               continue ;
            }
         }
         break ;
      }
   }

}

#endif //MTHMODIFIER_HPP_

