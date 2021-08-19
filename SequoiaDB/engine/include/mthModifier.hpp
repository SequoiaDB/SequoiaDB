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
#include "dms.hpp"
#include "msgDef.hpp"

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
   class _ModifierElement : public utilPooledObject
   {
   public :
      BSONElement _toModify ; // the element to modify
      const CHAR *_sourceField ; // Used in format {$field:<name>}, pointing to
                                 // the <name>, will be used in evaluation stage
                                 // to fetch the field from the source record.
      ModType     _modType ;
      INT32       _dollarNum ;

      _ModifierElement ( const BSONElement &e, ModType type,
                         INT32 dollarNum = 0 )
      {
         _toModify = e ;
         _sourceField = NULL ;
         _modType = type ;
         _dollarNum = dollarNum ;
      }

      virtual ~_ModifierElement() {}

      // Analyze the modifier. If it uses the '$field' format, get the field
      // name.
      virtual INT32 analyzeModifyEle() ;

      BOOLEAN isModifyByField() const
      {
         return ( NULL != _sourceField ) ;
      }

      const CHAR* getSourceFieldName() const
      {
         return _sourceField ;
      }
   } ;
   typedef _ModifierElement ModifierElement ;

   class _IncModifierElement : public _ModifierElement
   {
   public :
      BOOLEAN _isSimple ;
      BSONElement _valueEle ;
      BSONElement _default ;
      BSONElement _minEle ;
      bsonDecimal _minDecimal ;

      BSONElement _maxEle ;
      bsonDecimal _maxDecimal ;

      BSONObj _defaultResult ;

      _IncModifierElement( const BSONElement &e,
                           INT32 dollarNum = 0 )
                        : _ModifierElement( e, INC, dollarNum )
      {
         _isSimple = TRUE ;
      }

      _IncModifierElement( const BSONElement &e, const BSONElement &valueEle,
                           const BSONElement &myDefault,
                           const BSONElement &minEle,
                           const BSONElement &maxEle,
                           INT32 dollarNum = 0 )
                        : _ModifierElement( e, INC, dollarNum )
      {
         _valueEle = valueEle ;
         _default = myDefault ;
         _minEle = minEle ;
         _maxEle = maxEle ;
         _isSimple = FALSE ;
      }

      INT32 analyzeModifyEle() ;
      INT32 calcDefaultResult( BOOLEAN strictMode ) ;
      BOOLEAN isValidRange( BSONElement &resultEle ) ;
   } ;
   typedef _IncModifierElement IncModifierElement ;

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

      BOOLEAN operator () ( const ModifierElement *l,
                            const ModifierElement *r ) const
      {
         _compareFieldNames1 compare ( _dollarList ) ;
         FieldCompareResult result = compare.compField(
            l->_toModify.fieldName(), r->_toModify.fieldName() ) ;
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
   typedef ossPoolVector<ModifierElement*> MODIFIER_VEC ;

   private :
      BSONObjBuilder *_srcChgBuilder ;
      BSONObjBuilder *_dstChgBuilder ;
      BSONObjBuilder  _interBuilder ;

      BSONObj _modifierPattern ;
      BOOLEAN _initialized ;
      MODIFIER_VEC _modifierElements ;
      UINT32  _modifierBits ;
      BOOLEAN _hasModified ;

      BSONObj _tmpErrorObj ;
      BSONElement _errorFieldElement ;

      _ixmIndexKeyGen *_shardingKeyGen ;

      // add for replace begin
      ossPoolSet<string>    _keepKeys ;
      BOOLEAN        _isReplaceID ;
      BOOLEAN        _isReplace ;
      // add for replace end

      vector<INT64> *_dollarList ;
      _compareFieldNames1  _fieldCompare ;
      BOOLEAN        _ignoreTypeError ;
      BOOLEAN        _strictDataMode ;

      // For support of using {$field:<name>} format in some of updating
      // operators. The value of {$field:<name>} should be evaluated by using
      // the original record. So we set it for each record.
      BSONObj        _sourceRecord ;

      INT32 _addToKeepSet( const CHAR *fieldName ) ;
      INT32 _addToModifierVector( ModifierElement *element ) ;
      INT32 _addModifier ( const BSONElement &ele, ModType type ) ;
      INT32 _parseElement ( const BSONElement &ele ) ;

      /**
       * Parse simple $inc operation format, just increase by a value
       * the {$field:<name>} format.
       */
      INT32 _parseIncSimple( const BSONElement& ele, INT32 dollarNum ) ;

      /**
       * Parse complicated $inc operation format, an object is used.
       */
      INT32 _parseIncAdvance( const BSONElement& ele, INT32 dollarNum ) ;

      /**
       * Parse complicated $inc operation format, an option object is used.
       */
      INT32 _parseIncByOptObj( const BSONElement& ele,
                               const BSONElement& valueEle,
                               const BSONElement& defaultEle,
                               const BSONElement& minEle,
                               const BSONElement& maxEle,
                               INT32 dollarNum ) ;

      INT32 _parseInc( const BSONElement& ele, INT32 dollarNum ) ;
      ModType _parseModType ( const CHAR *field ) ;
      OSS_INLINE void _incModifierIndex( INT32 *modifierIndex ) ;

      INT32 _parseFullRecord( const BSONObj &record ) ;

      INT32 _setSourceRecord( const BSONObj &record ) ;

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
                                IncModifierElement &me ) ;
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

      // Process $setarray
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

      // if the original object has the element we asked to modify, then e is
      // the
      // original element, b is the builder, me is the info that we want to
      // modify
      // basically we need to take the original data from e, and use modifier
      // element me to make some change, and add into builder b
      template<class Builder>
      INT32 _applyChange ( CHAR **ppRoot,
                           INT32 &rootBufLen,
                           INT32 rootLen,
                           BSONElement &e,
                           Builder &b,
                           SINT32 *modifierIndex ) ;

      // when requested update want to change something that not exist in
      // original
      // object, we need to append the original object in those cases
      template<class Builder>
      INT32 _appendNew ( const CHAR *pRoot, const CHAR *pShort,
                         Builder& b, SINT32 *modifierIndex ) ;

      // Builder could be BSONObjBuilder or BSONArrayBuilder
      // _appendNewFromMods appends the current builder with the new field
      // root represent the current fieldName, me is the current modifier element
      // b is the builder, onedownseen represent the all subobjects have been
      // processed in the current object, and modifierIndex is the pointer for
      // current modifier
      template<class Builder>
      INT32 _appendNewFromMods ( CHAR **ppRoot,
                                 INT32 &rootBufLen,
                                 INT32 rootLen,
                                 UINT32 modifierRootLen,
                                 Builder &b,
                                 SINT32 *modifierIndex,
                                 BOOLEAN hasCreateNewRoot ) ;
      // Builder could be BSONObjBuilder or BSONArrayBuilder
      // This function is recursively called to build new object
      // The prerequisit is that _modifierElement is sorted, which supposed to
      // happen at end of loadPattern
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

      void _resetErrorElement() ;
      void _saveErrorElement( BSONElement &errorEle ) ;
      void _saveErrorElement( const CHAR *fieldName ) ;
      INT32 _getFieldModifier( const CHAR* fieldName, BSONElement& fieldEle ) ;

   public :
      _mthModifier ()
      : _interBuilder( 20 )
      {
         _initialized   = FALSE ;
         _srcChgBuilder = NULL ;
         _dstChgBuilder = NULL ;
         _dollarList    = NULL ;
         _ignoreTypeError = TRUE ;
         _modifierBits  = 0 ;
         _hasModified   = FALSE ;
         _isReplace     = FALSE ;
         _isReplaceID   = FALSE ;
         _shardingKeyGen = NULL ;
         _strictDataMode = FALSE ;
      }
      ~_mthModifier()
      {
         INT32 i = 0 ;
         while ( i < (SINT32)_modifierElements.size() )
         {
            ModifierElement *me = _modifierElements[i] ;
            SAFE_OSS_DELETE( me ) ;
            ++i ;
         }

         _modifierElements.clear() ;
         SAFE_OSS_DELETE( _shardingKeyGen ) ;
      }
      INT32 loadPattern ( const BSONObj &modifierPattern,
                          vector<INT64> *dollarList = NULL,
                          BOOLEAN ignoreTypeError = TRUE,
                          const BSONObj* shardingKey = NULL,
                          BOOLEAN strictDataMode = FALSE,
                          UINT32 logWriteMod = DMS_LOG_WRITE_MOD_INCREMENT ) ;
      void modifierSort() ;
      INT32 modify ( const BSONObj &source, BSONObj &target,
                     BSONObj *srcID = NULL,
                     BSONObj *srcChange = NULL,
                     BSONObj *dstID = NULL,
                     BSONObj *dstChange = NULL,
                     BSONObj *srcShardingKey = NULL,
                     BSONObj *dstShardingKey = NULL ) ;
      BSONElement getErrorElement() ;
      OSS_INLINE BOOLEAN isInitialized () const { return _initialized ; }
      OSS_INLINE BOOLEAN hasModified() const { return _hasModified ; }
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
              _modifierElements[*modifierIndex]->_dollarNum > 0 )
         {
            ++(*modifierIndex) ;
            continue ;
         }
         else if ( !mthCheckUnknowDollar(
            _modifierElements[*modifierIndex]->_toModify.fieldName(),
            _dollarList ) )
         {
            ++(*modifierIndex) ;
            continue ;
         }
         else if ( tmpModifierIndex >= 0 )
         {
            FieldCompareResult cmp = _fieldCompare.compField (
              _modifierElements[tmpModifierIndex]->_toModify.fieldName(),
              _modifierElements[*modifierIndex]->_toModify.fieldName() ) ;
            if ( SAME == cmp || RIGHT_SUBFIELD == cmp )
            {
               ++(*modifierIndex) ;
               continue ;
            }
         }
         break ;
      }
   }

   BOOLEAN mthIsBiggerNumberType( const BSONElement &left,
                                  const BSONElement &right ) ;

   /**
    * @brief Evaluate the fields to be updated by '$inc' operator, and put them
    *        into the builder for the final record.
    * @param existElement   The current field element to be modified.
    * @param inc            '$inc' value for the field to be modified.
    * @param strictMode     Whether overflow is allowed in calculation.
    * @param resBuilder     Builder for the final record.
    */
   INT32 mthModifierInc( const BSONElement& existElement,
                         const BSONElement &inc, BOOLEAN strictMode,
                         BSONObjBuilder &resBuilder ) ;
}

#endif //MTHMODIFIER_HPP_

