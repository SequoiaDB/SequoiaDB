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

   Source File Name = mthMatchNode.cpp

   Descriptive Name = Method MatchNode

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains functions for matcher, which
   indicates whether a record matches a given matching rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2016  LinYouBin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthMatchNode.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{

   // compare to sort operators
   BOOLEAN mthCompareNode( _mthMatchNode * left, _mthMatchNode * right )
   {
      if ( left->getWeight() < right->getWeight() )
      {
         return TRUE ;
      }
      else if ( left->getWeight() > right->getWeight() )
      {
         return FALSE ;
      }
      else
      {
         INT32 res = ossStrcmp( left->getFieldName(), right->getFieldName() ) ;
         if ( res < 0 )
         {
            return TRUE ;
         }
         else if ( res > 0 )
         {
            return FALSE ;
         }
      }
      return left->getType() < right->getType() ;
   }

   void mthContextClearRecordInfoSafe( _mthMatchTreeContext *context )
   {
      if ( NULL != context )
      {
         context->clearRecordInfo() ;
      }
   }

   _mthMatchTreeContext::_mthMatchTreeContext()
   {
      _dollarList.clear() ;
      _elements.clear() ;
      _fieldName.clear() ;

      _isReturnMatchExecuted = FALSE ;
      _hasExpand             = FALSE ;
      _hasReturnMatch        = FALSE ;
      _isUseElement          = FALSE ;
      _isDollarListEnabled   = FALSE ;

      _parameters            = NULL ;
   }

   _mthMatchTreeContext::~_mthMatchTreeContext()
   {
      clear() ;
   }

   /* clear the record relate information to execute the next
      _mthMatchTree.matches */
   void _mthMatchTreeContext::clearRecordInfo()
   {
      _dollarList.clear() ;
      _elements.clear() ;
      _fieldName.clear() ;

      _isReturnMatchExecuted = FALSE ;
      _hasExpand             = FALSE ;
      _hasReturnMatch        = FALSE ;
      _isUseElement          = FALSE ;
   }

   void _mthMatchTreeContext::clear()
   {
      clearRecordInfo() ;
      _isDollarListEnabled = FALSE ;
   }

   BOOLEAN _mthMatchTreeContext::hasExpand()
   {
      return _hasExpand ;
   }

   void _mthMatchTreeContext::setHasExpand( BOOLEAN hasExpand )
   {
      _hasExpand = hasExpand ;
   }

   BOOLEAN _mthMatchTreeContext::hasReturnMatch()
   {
      return _hasReturnMatch ;
   }

   void _mthMatchTreeContext::setHasReturnMatch( BOOLEAN hasReturnMatch )
   {
      _hasReturnMatch = hasReturnMatch ;
   }

   BOOLEAN _mthMatchTreeContext::isUseElement()
   {
      return _isUseElement ;
   }

   void _mthMatchTreeContext::setIsUseElement( BOOLEAN isUseElement )
   {
      _isUseElement = isUseElement ;
   }

   BOOLEAN _mthMatchTreeContext::isReturnMatchExecuted()
   {
      return _isReturnMatchExecuted ;
   }

   void _mthMatchTreeContext::setReturnMatchExecuted( BOOLEAN isExecuted )
   {
      _isReturnMatchExecuted = isExecuted ;
   }

   INT32 _mthMatchTreeContext::setFieldName( const CHAR *name )
   {
      return _fieldName.setFieldName( name ) ;
   }

   const CHAR *_mthMatchTreeContext::getFieldName()
   {
      return _fieldName.getFieldName() ;
   }

   INT32 _mthMatchTreeContext::getDollarResult( INT32 index, INT32 &value )
   {
      UINT32 i = 0 ;
      for ( i = 0 ; i < _dollarList.size() ; i++ )
      {
         INT32 tmpIndex = ( _dollarList[i] >> 32 ) & 0xFFFFFFFF ;
         if ( index == tmpIndex )
         {
            value = _dollarList[i] & 0xFFFFFFFF ;
            return SDB_OK ;
         }
      }

      return SDB_INVALIDARG ;
   }

   INT32 _mthMatchTreeContext::_replaceDollar()
   {
      INT32 rc = SDB_OK ;
      _utilString< MTH_MATCH_FIELD_STATIC_NAME_LEN > result ;
      const CHAR *src   = NULL ;
      const CHAR *start = NULL ;
      const CHAR *p     = NULL ;
      INT32 dollarIndex = 0 ;

      src = _fieldName.getFieldName() ;
      p   = ossStrstr( src, ".$" ) ;
      if ( NULL == p )
      {
         goto done ;
      }

      start = src ;
      while ( TRUE )
      {
         INT32 realValue = 0 ;
         rc = result.append( start, p - start ) ;
         PD_RC_CHECK( rc, PDERROR, "append value failed:value=%s,rc=%d",
                      start, rc ) ;

         // abc.$10.def    p->.$
         rc = ossStrToInt ( p + 2, &dollarIndex ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse number:p=%s,rc=%d",
                      p, rc ) ;

         rc = getDollarResult( dollarIndex, realValue ) ;
         PD_RC_CHECK( rc, PDERROR, "getDollarResult failed:rc=%d", rc ) ;

         // "."
         rc = result.append( p, 1 ) ;
         PD_RC_CHECK( rc, PDERROR, "append value failed:value=%s,rc=%d",
                      p, rc ) ;

         rc = result.appendINT32( realValue ) ;
         PD_RC_CHECK( rc, PDERROR, "append value failed:value=%d,rc=%d",
                      realValue, rc ) ;

         start = ossStrchr( p + 1, MTH_FIELDNAME_SEP ) ;
         if ( NULL == start )
         {
            goto done ;
         }

         p = ossStrstr( start, ".$" ) ;
         if ( NULL == p )
         {
            break ;
         }
      }

      rc = result.append( start, ossStrlen( start ) ) ;
      PD_RC_CHECK( rc, PDERROR, "append value failed:value=%s,rc=%d",
                   start, rc ) ;

      _fieldName.clear() ;
      rc = _fieldName.setFieldName( result.str() ) ;
      PD_RC_CHECK( rc, PDERROR, "set fieldName failed:fieldName=%s,rc=%d",
                   result.str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchTreeContext::resolveFieldName()
   {
      INT32 rc = SDB_OK ;

      if ( _hasExpand || _hasReturnMatch )
      {
         if ( _isDollarListEnabled )
         {
            rc = _replaceDollar() ;
            PD_RC_CHECK( rc, PDERROR, "replaceDollar failed:rc=%d", rc ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchTreeContext::saveElement( INT32 index )
   {
      return _elements.append( index ) ;
   }

   INT32 _mthMatchTreeContext::subElements( INT32 offset, INT32 len )
   {
      INT32 rc        = SDB_OK ;
      INT32 srcSize   = 0 ;
      INT32 srcSubLen = 0 ;

      if ( _elements.size() == 0 )
      {
         goto done ;
      }

      srcSize = ( INT32 )_elements.size() ;
      if ( offset >= srcSize )
      {
         _elements.clear() ;
         goto done ;
      }

      if ( offset >= 0 )
      {
         srcSubLen = srcSize - offset ;
      }
      else
      {
         INT32 tmpOffset = offset +  srcSize ;
         if ( tmpOffset < 0 )
         {
            _elements.clear() ;
            goto done ;
         }

         offset    = tmpOffset ;
         srcSubLen = srcSize - offset ;
      }

      if ( len >= 0 && len < srcSubLen )
      {
         srcSubLen = len ;
      }

      {
         _utilArray< INT32 > dst ;
         INT32 i = 0 ;
         for ( i = 0 ; i < srcSubLen ; i++ )
         {
            rc = dst.append( _elements[ offset + i ] ) ;
            PD_RC_CHECK( rc, PDERROR, "append element failed:rc=%d", rc ) ;
         }

         _elements = dst ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthMatchTreeContext::setObj( const BSONObj &obj )
   {
      _originalObj = obj ;
   }

   void _mthMatchTreeContext::enableDollarList()
   {
      _isDollarListEnabled = TRUE ;
      _dollarList.clear() ;
   }

   void _mthMatchTreeContext::disableDollarList()
   {
      _isDollarListEnabled = FALSE ;
      _dollarList.clear() ;
   }

   BOOLEAN _mthMatchTreeContext::isDollarListEnabled()
   {
      return _isDollarListEnabled ;
   }

   void _mthMatchTreeContext::appendDollarList( vector<INT64> &dollarList )
   {
      UINT32 i = 0 ;
      for ( i = 0 ; i < dollarList.size() ; i++ )
      {
         _dollarList.push_back( dollarList[i] ) ;
      }
   }

   void _mthMatchTreeContext::getDollarList( vector<INT64> *dollarList )
   {
      UINT32 i = 0 ;

      if ( NULL == dollarList )
      {
         return ;
      }

      dollarList->clear() ;

      for ( i = 0 ; i < _dollarList.size() ; i++ )
      {
         dollarList->push_back( _dollarList[i] ) ;
      }
   }

   string _mthMatchTreeContext::toString()
   {
      UINT32 i = 0 ;
      stringstream result ;
      result << "isDollarListEnabled:" << _isDollarListEnabled << endl ;
      if ( _isDollarListEnabled )
      {
         for ( ; i < _dollarList.size() ; i++ )
         {
            //(temp>>32)&0xFFFFFFFF) // num = (temp&0xFFFFFFFF) ;
            INT32 dollarNum = ( _dollarList[i] >> 32 ) & 0xFFFFFFFF ;
            INT32 objIndex  = _dollarList[i] & 0xFFFFFFFF ;

            result << "\t$" << dollarNum << "=" << objIndex << endl ;
         }
      }

      result << "hasExpand:" << _hasExpand << endl ;
      result << "hasReturnMatch:" << _hasReturnMatch << endl ;
      if ( _hasExpand || _hasReturnMatch )
      {
         result << "fieldName:" << _fieldName.getFieldName() << endl ;
      }

      result << "isUseElement:" << _isUseElement << endl ;
      if ( _isUseElement )
      {
         result << "elementSize:" << _elements.size() << endl ;
         UINT32 i = 0 ;
         for ( i = 0 ; i < _elements.size() ; i++ )
         {
            result << "\t[" << i << "]:" << _elements[i] << endl ;
         }
      }

      return result.str() ;
   }

   BSONElement _mthMatchTreeContext::getParameter ( INT8 index )
   {
      if ( NULL != _parameters )
      {
         return _parameters->getParam( index ) ;
      }
      return BSONElement() ;
   }

   const RTN_ELEMENT_SET * _mthMatchTreeContext::getParamValueSet ( INT8 index )
   {
      if ( NULL != _parameters )
      {
         return _parameters->getValueSet( index ) ;
      }
      return NULL ;
   }

   BOOLEAN _mthMatchTreeContext::paramDoneByPred ( INT8 index )
   {
      if ( _parameters )
      {
         return _parameters->isDoneByPred( index ) ;
      }
      return FALSE ;
   }

   //********************** _mthMatchNodeIterator ***************************
   _mthMatchNodeIterator::_mthMatchNodeIterator( _mthMatchNode *node )
   {
      _node  = node ;
      _index = 0 ;
   }

   _mthMatchNodeIterator::~_mthMatchNodeIterator()
   {
      _node  = NULL ;
      _index = 0 ;
   }

   _mthMatchNodeIterator::_mthMatchNodeIterator()
   {
   }

   _mthMatchNodeIterator::_mthMatchNodeIterator(
                                          const _mthMatchNodeIterator &right )
   {
   }

   BOOLEAN _mthMatchNodeIterator::more()
   {
      return _index < _node->_children.size() ;
   }

   _mthMatchNode* _mthMatchNodeIterator::next()
   {
      UINT32 i = _index++ ;
      if ( i < _node->_children.size() )
      {
         return _node->_children[ i ] ;
      }

      return NULL ;
   }

   //********************** _mthNodeConfig **************************
   const mthNodeConfig *mthGetDefaultNodeConfigPtr ()
   {
      static mthNodeConfig defaultConfig ;

      return &defaultConfig ;
   }

   const mthNodeConfig &mthGetDefaultNodEConfig ()
   {
      return (*mthGetDefaultNodeConfigPtr()) ;
   }

   //********************** _mthMatchConfig *************************
   _mthMatchConfig::_mthMatchConfig ()
   {
      _matchConfig = mthGetDefaultNodeConfigPtr() ;
   }

   _mthMatchConfig::_mthMatchConfig ( const mthNodeConfig *configPtr )
   {
      if ( NULL == configPtr )
      {
         _matchConfig = mthGetDefaultNodeConfigPtr() ;
      }
      else
      {
         _matchConfig = configPtr ;
      }
   }

   _mthMatchConfig::~_mthMatchConfig ()
   {
      _matchConfig = NULL ;
   }

   _mthMatchConfigHolder::_mthMatchConfigHolder ()
   : _mthMatchConfig( &_stackMatchConfig )
   {
      setMatchConfig( mthGetDefaultNodEConfig() ) ;
   }

   _mthMatchConfigHolder::_mthMatchConfigHolder ( const mthNodeConfig &config )
   : _mthMatchConfig( &_stackMatchConfig )
   {
      setMatchConfig( config ) ;
   }

   _mthMatchConfigHolder::~_mthMatchConfigHolder ()
   {
   }

   //********************** _mthMatchNode ***************************
   _mthMatchNode::_mthMatchNode( _mthNodeAllocator *allocator,
                                 const mthNodeConfig *config )
                 :_mthMatchConfig( config ), _allocator( allocator ),
                  _parent( NULL ), _idx_in_parent( -1 ),
                  _isUnderLogicNot( FALSE )
   {
   }

   _mthMatchNode::~_mthMatchNode()
   {
      _allocator = NULL ;
      clear() ;
   }

   void* _mthMatchNode::operator new ( size_t size,
                                       _mthNodeAllocator *allocator )
   {
      void *p = NULL ;
      if ( size > 0 )
      {
         // In order to know if the memory is allocated by malloc() when
         // deleting the object, reserve space for a flag at the head of the
         // allocated space.
         size_t reserveSize = size + MTH_MEM_TYPE_SIZE ;
         if ( allocator )
         {
            p = allocator->allocate( reserveSize ) ;
         }

         if ( NULL == p )
         {
            p = SDB_THREAD_ALLOC( reserveSize ) ;
            if ( NULL == p )
            {
               goto error ;
            }
            *(INT32 *)p = MTH_MEM_BY_DFT_ALLOCATOR ;
         }
         else
         {
            *(INT32 *)p = MTH_MEM_BY_USER_ALLOCATOR ;
         }
         // Seek address which can actually be used by the user.
         p = (CHAR *)p + MTH_MEM_TYPE_SIZE ;
      }

   done:
      return p ;
   error:
      goto done ;
   }

   void _mthMatchNode::operator delete( void *p )
   {
      if ( p )
      {
         void *beginAddr = (void *)( (CHAR *)p - MTH_MEM_TYPE_SIZE ) ;
         // Only release memory allocted by SDB_THREAD_FREE().
         // Objects allocated by instances of _utilAllocator(allocator is not
         // NULL in new) will not be released seperately, as they are allocated
         // in a stack. They space is released when the allocator is destroyed.
         if ( MTH_MEM_BY_DFT_ALLOCATOR == *(INT32 *)beginAddr )
         {
            SDB_THREAD_FREE( beginAddr ) ;
         }
      }
   }

   void _mthMatchNode::operator delete( void *p, _mthNodeAllocator *allocator )
   {
      _mthMatchNode::operator delete( p ) ;
   }

   string _mthMatchNode::toString()
   {
      BSONObj obj = toBson() ;
      return obj.toString() ;
   }

   const CHAR* _mthMatchNode::getFieldName()
   {
      return _fieldName.getFieldName() ;
   }

   INT32 _mthMatchNode::init( const CHAR *fieldName,
                              const BSONElement &element )
   {
      INT32 rc = SDB_OK ;
      rc = _fieldName.setFieldName( fieldName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "set fieldName failed:fieldName=%s,rc=%d",
                 fieldName, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      clear() ;
      goto done ;
   }

   //just clear parameter itself
   void _mthMatchNode::clear()
   {
      _parent          = NULL ;
      _idx_in_parent   = -1 ;
      _isUnderLogicNot = FALSE ;
      _children.clear() ;
      _fieldName.clear() ;
   }

   INT32 _mthMatchNode::addChild( _mthMatchNode *child )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != child, "child must not be null" ) ;

      _children.push_back( child ) ;
      if ( EN_MATCH_OPERATOR_LOGIC_NOT == getType() || isUnderLogicNot() )
      {
         child->rollIsUnderLogicNot() ;
      }

      child->_idx_in_parent = _children.size() - 1 ;
      child->_parent        = this ;

      return rc ;
   }

   void _mthMatchNode::delChild( _mthMatchNode *child )
   {
      SDB_ASSERT( NULL != child, "child must not be null" ) ;
      SDB_ASSERT( child->_idx_in_parent < _children.size(),
                  "index must smaller than size" ) ;
      SDB_ASSERT( _children[ child->_idx_in_parent ] == child,
                  "child must be true" ) ;

      UINT32 index = child->_idx_in_parent ;
      _children.erase( _children.begin() + index ) ;

      for ( ; index < _children.size() ; index++ )
      {
         _children[ index ]->_idx_in_parent-- ;
      }

      if ( EN_MATCH_OPERATOR_LOGIC_NOT == getType() || isUnderLogicNot() )
      {
         child->rollIsUnderLogicNot() ;
      }
   }

   void _mthMatchNode::rollIsUnderLogicNot()
   {
      UINT32 i = 0 ;
      _isUnderLogicNot = !_isUnderLogicNot ;

      for ( ; i < _children.size() ; i++ )
      {
         _children[ i ]->rollIsUnderLogicNot() ;
      }
   }

   BOOLEAN _mthMatchNode::isUnderLogicNot()
   {
      return _isUnderLogicNot ;
   }

   UINT32 _mthMatchNode::getChildrenCount()
   {
      return _children.size() ;
   }

   _mthMatchNode* _mthMatchNode::getParent()
   {
      return _parent ;
   }

   void _mthMatchNode::sortByWeight()
   {
      for ( UINT32 i = 0; i < _children.size() ; i++ )
      {
         _mthMatchNode *child = _children[ i ] ;
         child->sortByWeight() ;
      }

      if ( _children.size() > 1 )
      {
         std::sort( _children.begin(), _children.end(), mthCompareNode ) ;

         // rewrite _idx_in_parent
         for ( UINT32 i = 0; i < _children.size() ; i++ )
         {
            _mthMatchNode *child = _children[ i ] ;
            child->_idx_in_parent = i ;
         }
      }
   }

   BOOLEAN _mthMatchNode::hasDollarFieldName()
   {
      //default is false
      return FALSE ;
   }

   INT32 _mthMatchNode::calcPredicate( rtnPredicateSet &predicateSet,
                                       const rtnParamList * paramList )
   {
      UINT32 i = 0 ;
      for ( ; i < _children.size() ; i++ )
      {
         _mthMatchNode *child = _children[ i ] ;
         child->calcPredicate( predicateSet, paramList ) ;
      }

      return SDB_OK ;
   }

   INT32 _mthMatchNode::extraEqualityMatches( BSONObjBuilder &builder,
                                              const rtnParamList *parameters )
   {
      UINT32 i = 0 ;
      for ( ; i < _children.size() ; i++ )
      {
         _mthMatchNode *child = _children[ i ] ;
         child->extraEqualityMatches( builder, parameters ) ;
      }

      return SDB_OK ;
   }

}


