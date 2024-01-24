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

   Source File Name = qgmOptiTree.cpp

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

******************************************************************************/

#include "qgmOptiTree.hpp"
#include "pd.hpp"
#include "qgmUtil.hpp"
#include <iostream>

#include "qgmOptiSort.hpp"
#include "qgmOptiSelect.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

namespace engine
{
   const CHAR* getQgmOptiTypeDesp( QGM_OPTI_TYPE type )
   {
      switch ( type )
      {
         case QGM_OPTI_TYPE_SELECT :
            return "Select" ;
            break ;
         case QGM_OPTI_TYPE_SORT :
            return "Sort" ;
            break ;
         case QGM_OPTI_TYPE_FILTER :
            return "Filter" ;
            break ;
         case QGM_OPTI_TYPE_AGGR :
            return "Aggr" ;
            break ;
         case QGM_OPTI_TYPE_SCAN :
            return "Scan" ;
            break ;
         case QGM_OPTI_TYPE_JOIN :
            return "Join" ;
            break ;
         case QGM_OPTI_TYPE_JOIN_CONDITION :
            return "JoinCondition" ;
            break ;
         case QGM_OPTI_TYPE_INSERT :
            return "Insert" ;
            break ;
         case QGM_OPTI_TYPE_DELETE :
            return "Delete" ;
            break ;
         case QGM_OPTI_TYPE_UPDATE :
            return "Update" ;
            break ;
         case QGM_OPTI_TYPE_MTHMCHSEL :
            return "mthMacherSelect";
            break;
         case QGM_OPTI_TYPE_MTHMCHSCAN :
            return "mthMacherScan";
            break;
         case QGM_OPTI_TYPE_MTHMCHFILTER :
            return "mthMacherFilter";
            break;
         case QGM_OPTI_TYPE_SPLIT :
            return "Split By" ;
            break ;
         case QGM_OPTI_TYPE_COMMAND :
            return "Command" ;
            break ;
         default :
            break ;
      }
      return "Unknow" ;
   }

   static void dumpSelf( const _qgmOptiTreeNode *node,
                         INT32 indent = 0 )
   {
      cout << string( indent * 4, ' ') << "|--"
           << node->toString() << endl ;
      qgmOptiTreeNodePtrVec::const_iterator itr = node->_children.begin() ;
      for ( ; itr != node->_children.end(); itr++ )
      {
         dumpSelf( *itr, indent+1 ) ;
      }
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPSTREAM_FIND, "_qgmOpStream::find" )
   BOOLEAN _qgmOpStream::find( const _qgmDbAttr &field )
   {
      /// eg: select T.A from ( select a as A from table ) as T ;
      /// field.relegation = T, field.attr = A, stream.alias = T
      /// field.relegation must equal to stream.alias.
      /// field.attr can be either selector's alias or selector's name.
      PD_TRACE_ENTRY( SDB__QGMOPSTREAM_FIND ) ;
      BOOLEAN found = FALSE ;

      if ( NULL != next )
      {
         found = next->find( field ) ;
         if ( found )
         {
            goto done ;
         }
      }

      if ( alias != field.relegation() )
      {
         PD_LOG( PDDEBUG, "relegation of field[%s] is not valid the same with"
                 "output alias[%s]", field.toString().c_str(),
                 alias.toString().c_str() ) ;
         goto done ;
      }

      {
         qgmOPFieldVec::const_iterator itr = stream.begin() ;
         for ( ; itr != this->stream.end(); itr++ )
         {
            if ( !itr->alias.empty() )
            {
               if ( field.attr().isSubfix( itr->alias, TRUE ) )
               {
                  found = TRUE ;
                  goto done ;
               }
               else
               {
                  continue ;
               }
            }

            if ( SQL_GRAMMAR::FUNC != itr->type )
            {
               if ( SQL_GRAMMAR::WILDCARD== itr->type )
               {
                  found = TRUE ;
                  goto done ;
               }

               if ( field.attr().isSubfix( itr->value.attr(), TRUE ) )
               {
                  found = TRUE ;
                  goto done ;
               }
            }
         }
      }

   done:
      PD_TRACE_EXIT( SDB__QGMOPSTREAM_FIND ) ;
      return found ;
   }

   BOOLEAN _qgmOpStream::isWildCard() const
   {
      if ( 1 == stream.size() && SQL_GRAMMAR::WILDCARD == stream[0].type )
      {
         return TRUE ;
      }
      return FALSE ;
   }

//////// _qgmOprUnit
   _qgmOprUnit::_qgmOprUnit( QGM_OPTI_TYPE type )
   : _type ( type )
   {
      _optional   = FALSE ;
      _nodeID     = 0 ;
   }

   _qgmOprUnit::_qgmOprUnit( const _qgmOprUnit &right, BOOLEAN delDup )
   {
      _type = right._type ;
      _optional = TRUE ;
      _nodeID = right._nodeID ;

      addOpField( right._fields, delDup ) ;
   }

   _qgmOprUnit::~_qgmOprUnit()
   {
   }

   BOOLEAN _qgmOprUnit::isWildCardField() const
   {
      if ( 1 == _fields.size() && SQL_GRAMMAR::WILDCARD == _fields[0].type )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   void _qgmOprUnit::setOptional( BOOLEAN optional )
   {
      _optional = optional ;
   }

   void _qgmOprUnit::setNodeID( UINT32 nodeID )
   {
      _nodeID = nodeID ;
   }

   void _qgmOprUnit::resetNodeID()
   {
      _nodeID = 0 ;
   }

   void _qgmOprUnit::setFields( const qgmOPFieldVec & fields, BOOLEAN delDup )
   {
      _fields.clear() ;
      addOpField( fields, delDup ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPRUNIT_ADDOPFIELD, "_qgmOprUnit::addOpField" )
   INT32 _qgmOprUnit::addOpField( const qgmOpField & field, BOOLEAN delDup )
   {
      PD_TRACE_ENTRY( SDB__QGMOPRUNIT_ADDOPFIELD ) ;
      INT32 rc = SDB_OK ;
      if ( delDup && isFromOne( field, _fields, FALSE ) )
      {
         goto done ;
      }

      _fields.push_back( field ) ;

      if ( QGM_OPTI_TYPE_FILTER == getType() &&
           ( field.type != SQL_GRAMMAR::DBATTR &&
             field.type != SQL_GRAMMAR::WILDCARD ) )
      {
         (*(--_fields.end())).type = SQL_GRAMMAR::DBATTR ;
      }

      if ( delDup && !field.alias.empty() )
      {
         (*(--_fields.end())).alias.clear() ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMOPRUNIT_ADDOPFIELD, rc ) ;
      return rc ;
   }

   INT32 _qgmOprUnit::addOpField( const qgmOPFieldVec &fields, BOOLEAN delDup )
   {
      qgmOPFieldVec::const_iterator cit = fields.begin() ;
      while ( cit != fields.end() )
      {
         addOpField( *cit, delDup ) ;
         ++cit ;
      }
      return SDB_OK ;
   }

   string _qgmOprUnit::toString() const
   {
      stringstream ss ;

      ss << "Type:" << _type << "[" << getQgmOptiTypeDesp( _type ) << "]" ;
      ss << ",Optional:" << ( isOptional() ? "TRUE" : "FALSE" ) ;
      ss << ",NodeID:" << _nodeID ;

      if ( _fields.size() > 0 )
      {
         ss << ",Fields:{" ;
         UINT32 index = 0 ;
         while ( index < _fields.size() )
         {
            if ( index != 0 )
            {
               ss << "," ;
            }
            ss << _fields[index].toString() ;
            ++index ;
         }
         ss << "}" ;
      }

      _toString( ss ) ;

      return ss.str() ;
   }

   void _qgmOprUnit::_toString( stringstream & ss ) const
   {
   }

   INT32 _qgmOprUnit::replaceRele( const qgmField & newRele )
   {
      replaceFieldRele( _fields, newRele ) ;
      return _replaceRele( newRele ) ;
   }

   INT32 _qgmOprUnit::_replaceRele( const qgmField & newRele )
   {
      return SDB_OK ;
   }

   INT32 _qgmOprUnit::clearAllFieldAlias()
   {
      clearFieldAlias( _fields ) ;
      return SDB_OK ;
   }

   UINT32 _qgmOprUnit::getFieldAlias( qgmOPFieldPtrVec & aliases )
   {
      UINT32 count = 0 ;
      aliases.clear() ;

      qgmOPFieldVec::iterator it = _fields.begin() ;
      while ( it != _fields.end() )
      {
         if ( !(*it).alias.empty() )
         {
            aliases.push_back( &(*it) ) ;
            ++count ;
         }
         ++it ;
      }
      return count ;
   }

   INT32 _qgmOprUnit::replaceFieldAlias( const qgmOPFieldPtrVec & fieldAlias )
   {
      if ( !isWildCardField() )
      {
         BOOLEAN copyAlias = QGM_OPTI_TYPE_FILTER == getType() ? TRUE : FALSE ;
         downFieldsByFieldAlias( _fields, fieldAlias, copyAlias,
                                 isOptional() ) ;
      }
      return _replaceFieldAlias( fieldAlias ) ;
   }

   INT32 _qgmOprUnit::_replaceFieldAlias( const qgmOPFieldPtrVec & fieldAlias )
   {
      return SDB_OK ;
   }

   INT32 _qgmOprUnit::restoreFieldAlias( const qgmOPFieldPtrVec & fieldAlias )
   {
      if ( !isWildCardField() )
      {
         BOOLEAN clearAlias = QGM_OPTI_TYPE_FILTER == getType() ? TRUE : FALSE ;
         upFieldsByFieldAlias( _fields, fieldAlias, clearAlias ) ;
      }
      return _restoreFieldAlias( fieldAlias ) ;
   }

   INT32 _qgmOprUnit::_restoreFieldAlias( const qgmOPFieldPtrVec & fieldAlias )
   {
      return SDB_OK ;
   }

   BOOLEAN _qgmOprUnit::hasExpr() const
   {
      BOOLEAN r = FALSE ;
      qgmOPFieldVec::const_iterator itr = _fields.begin() ;
      for ( ; itr != _fields.end(); ++itr )
      {
         if ( !( itr->expr.isEmpty() ) )
         {
            r = TRUE ;
            break ;
         }
      }

      return r ;
   }

//////// _qgmOptiTreeNode
   _qgmOptiTreeNode::_qgmOptiTreeNode( QGM_OPTI_TYPE type,
                                       qgmPtrTable *table,
                                       qgmParamTable *param )
   :_father( NULL ),
    _type( type ),
    _releaseChildren( TRUE ),
    _table( table ),
    _param( param )
   {
      _nodeID = 0 ;
   }

   _qgmOptiTreeNode::_qgmOptiTreeNode( QGM_OPTI_TYPE type,
                                       const qgmField &alias,
                                       qgmPtrTable *table,
                                       qgmParamTable *param )
   :_alias( alias ),
    _father( NULL ),
    _type( type ),
    _releaseChildren( TRUE ),
    _table( table ),
    _param( param )
   {
      _nodeID = 0 ;
   }

   _qgmOptiTreeNode::~_qgmOptiTreeNode()
   {
      if ( _releaseChildren )
      {
         qgmOptiTreeNodePtrVec::iterator itr = _children.begin() ;
         for ( ; itr != _children.end(); itr++ )
         {
            SAFE_OSS_DELETE( *itr ) ;
         }
      }

      _children.clear() ;
      _father = NULL ;

      // release oprUnit
      qgmOprUnitPtrVec::iterator it = _oprUnits.begin() ;
      while ( it != _oprUnits.end() )
      {
         SDB_OSS_DEL *it ;
         ++it ;
      }
      _oprUnits.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTITREENODE_ADDCHILD, "_qgmOptiTreeNode::addChild" )
   INT32 _qgmOptiTreeNode::addChild( _qgmOptiTreeNode *child )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTITREENODE_ADDCHILD ) ;
      INT32 rc = SDB_OK ;
      if ( NULL == child )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      try
      {
         _children.push_back( child ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "unexpected err happend: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMOPTITREENODE_ADDCHILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   qgmOptiTreeNode* _qgmOptiTreeNode::getParent()
   {
      return _father ;
   }

   void _qgmOptiTreeNode::setParent( qgmOptiTreeNode * parent )
   {
      _father = parent ;
   }

   UINT32 _qgmOptiTreeNode::getSubNodes( qgmOptiTreeNodePtrVec & subNodes )
   {
      subNodes = _children ;
      return subNodes.size() ;
   }

   UINT32 _qgmOptiTreeNode::getSubNodeCount()
   {
      return _children.size() ;
   }

   qgmOptiTreeNode* _qgmOptiTreeNode::getSubNode( UINT32 pos )
   {
      if ( pos >= _children.size() )
      {
         return NULL ;
      }
      return _children[pos] ;
   }

   qgmOptiTreeNode* _qgmOptiTreeNode::getNextSubNode( qgmOptiTreeNode * subNode )
   {
      UINT32 index = 0 ;
      qgmOptiTreeNode *subNodeTmp = NULL ;

      while ( NULL != ( subNodeTmp = getSubNode( index++ ) ) )
      {
         if ( subNodeTmp == subNode )
         {
            return getSubNode( index ) ; ;
         }
      }

      return NULL ;
   }

   INT32 _qgmOptiTreeNode::removeSubNode( qgmOptiTreeNode * subNode )
   {
      qgmOptiTreeNodePtrVec::iterator it = _children.begin() ;
      while ( it != _children.end() )
      {
         if ( subNode == *it )
         {
            _children.erase( it ) ;
            break ;
         }
         ++it ;
      }

      return SDB_OK ;
   }

   INT32 _qgmOptiTreeNode::updateSubNode( qgmOptiTreeNode * oldNode,
                                          qgmOptiTreeNode * newNode )
   {
      qgmOptiTreeNodePtrVec::iterator it = _children.begin() ;
      while ( it != _children.end() )
      {
         if ( oldNode == *it )
         {
            *it = newNode ;
            return SDB_OK ;
         }
         ++it ;
      }

      PD_LOG_MSG( PDERROR, "The node does not found" ) ;
      return SDB_SYS ;
   }

   UINT32 _qgmOptiTreeNode::getOprUnitCount()
   {
      return _oprUnits.size() ;
   }

   UINT32 _qgmOptiTreeNode::getOprUnits( qgmOprUnitPtrVec & oprUnitVec )
   {
      oprUnitVec.clear() ;

      UINT32 count = 0 ;
      qgmOprUnitPtrVec::iterator it = _oprUnits.begin() ;
      while ( it != _oprUnits.end() )
      {
         oprUnitVec.push_back( *it ) ;
         ++it ;
         ++count ;
      }

      return count ;
   }

   qgmOprUnit* _qgmOptiTreeNode::getOprUnitByType( QGM_OPTI_TYPE type,
                                                   const qgmField & field )
   {
      qgmOprUnitPtrVec::iterator it = _oprUnits.begin() ;
      while ( it != _oprUnits.end() )
      {
         qgmOprUnit *pOprUnit = *it ;
         if ( type == pOprUnit->getType() &&
              ( field.empty() || field == pOprUnit->getDispatchAlias() ) )
         {
            return pOprUnit ;
         }
         ++it ;
      }
      return NULL ;
   }

   qgmOprUnit* _qgmOptiTreeNode::getOprUnit( UINT32 pos )
   {
      if ( pos >= _oprUnits.size() )
      {
         return NULL ;
      }
      return _oprUnits[ pos ] ;
   }

   UINT32 _qgmOptiTreeNode::_getSubAlias( qgmFieldVec & aliases ) const
   {
      UINT32 index = 0 ;
      while ( index < _children.size() )
      {
         aliases.push_back( _children[index]->getAlias( TRUE ) ) ;
         ++index ;
      }
      return index ;
   }

   UINT32 _qgmOptiTreeNode::_getFieldAlias( qgmOPFieldPtrVec & fieldAlias,
                                            BOOLEAN getAll )
   {
      return 0 ;
   }

   INT32 _qgmOptiTreeNode::_pushOprUnit( qgmOprUnit * oprUnit, PUSH_FROM from )
   {
      return SDB_SYS ;
   }

   INT32 _qgmOptiTreeNode::_removeOprUnit( qgmOprUnit * oprUnit )
   {
      return SDB_SYS ;
   }

   INT32 _qgmOptiTreeNode::_updateChange( qgmOprUnit * oprUnit )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTITREENODE__ONPUSHOPRUNIT, "_qgmOptiTreeNode::_onPushOprUnit" )
   INT32 _qgmOptiTreeNode::_onPushOprUnit( qgmOprUnit * oprUnit, PUSH_FROM from )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTITREENODE__ONPUSHOPRUNIT ) ;
      INT32 rc = SDB_OK ;

      if ( FROM_UP == from )
      {
         if ( QGM_OPTI_TYPE_SORT == oprUnit->getType() )
         {
            // do nothing
         }
         else if ( QGM_OPTI_TYPE_FILTER == oprUnit->getType() )
         {
            qgmOPFieldPtrVec aliases ;
            qgmOprUnit *otherUnit = NULL ;
            UINT32 index = 0 ;

            if ( oprUnit->getFieldAlias( aliases ) > 0 )
            {
               while ( NULL != ( otherUnit = getOprUnit( index++ ) ) )
               {
                  if ( otherUnit == oprUnit ||
                       ( !otherUnit->getDispatchAlias().empty() &&
                         otherUnit->getDispatchAlias() !=
                         oprUnit->getDispatchAlias() ) )
                  {
                     continue ;
                  }
                  rc = otherUnit->restoreFieldAlias( aliases ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }

                  rc = updateChange( otherUnit ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
               }
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "oprUnit[%s] type error",
                        oprUnit->toString().c_str() ) ;
            goto error ;
         }
      }
      else
      {
         /*if ( QGM_OPTI_TYPE_SORT == oprUnit->getType() )
         {
            qgmOprUnit *otherFilter = getOprUnitByType( QGM_OPTI_TYPE_FILTER ) ;
            if ( otherFilter )
            {
               UINT32 fieldCount = otherFilter->getFields()->size() ;
               otherFilter->addOpField( *oprUnit->getFields(), TRUE ) ;
               if ( fieldCount != otherFilter->getFields()->size() )
               {
                  rc = updateChange( otherFilter ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
               }
            }
         }*/
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTITREENODE__ONPUSHOPRUNIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _qgmOptiTreeNode::_upFields( qgmOPFieldVec & fields )
   {
      qgmOPFieldPtrVec fieldAlias ;

      // step1: restore field alias
      if ( _getFieldAlias( fieldAlias, FALSE ) > 0 )
      {
         upFieldsByFieldAlias( fields, fieldAlias, FALSE ) ;
      }

      // step2: restore table alais
      if ( validSelfAlias() )
      {
         replaceFieldRele( fields, getAlias() ) ;
      }

      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTITREENODE_PUSHOPRUNIT, "_qgmOptiTreeNode::pushOprUnit" )
   INT32 _qgmOptiTreeNode::pushOprUnit( qgmOprUnit * oprUnit,
                                        qgmOptiTreeNode *fromNode,
                                        qgmOptiTreeNode::PUSH_FROM from )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTITREENODE_PUSHOPRUNIT ) ;
      INT32 rc = SDB_OK ;
      qgmOPFieldPtrVec fieldAlias ;

      if ( NULL == fromNode )
      {
         goto subPush ;
      }

      rc = fromNode->_onPushOprUnit( oprUnit, from ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "On node[%s] push oprUnit[%s] failed, rc: %d",
                 fromNode->toString().c_str(), oprUnit->toString().c_str() ) ;
         goto error ;
      }

      if ( qgmOptiTreeNode::FROM_UP == from )
      {
         BOOLEAN getAll = FALSE ;

         // step1: replace table alias
         if ( validSelfAlias() )
         {
            qgmFieldVec subAlias ;
            if ( _getSubAlias( subAlias ) > 1 )
            {
               PD_LOG_MSG( PDERROR,
                           "Push oprUnit[%s] failed, Node[%s] sub alias"
                           " has %d, can't match", oprUnit->toString().c_str(),
                           toString().c_str(), subAlias.size() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( subAlias.size() == 0 )
            {
               oprUnit->replaceRele( qgmField() ) ;
            }
            else
            {
               oprUnit->replaceRele( subAlias[0] ) ;
               if ( subAlias[0].empty() )
               {
                  getAll = TRUE ;
               }
            }
         }

         // step2: replace field alias
         if ( _getFieldAlias( fieldAlias, getAll ) > 0 )
         {
            oprUnit->replaceFieldAlias( fieldAlias ) ;
         }
      }
      else
      {
         // step1: restore field alias
         if ( fromNode->_getFieldAlias( fieldAlias, FALSE ) > 0 )
         {
            oprUnit->restoreFieldAlias( fieldAlias ) ;
         }

         // step2: restore table alais
         if ( fromNode->validSelfAlias() )
         {
            oprUnit->replaceRele( fromNode->getAlias() ) ;
         }
      }

   subPush:
      oprUnit->clearDispatchAlias() ;

      if ( qgmOptiTreeNode::FROM_UP == from )
      {
         rc = _pushOprUnit( oprUnit, from ) ;
      }
      else
      {
         if ( !oprUnit->isNodeIDValid() )
         {
            rc = SDB_SYS ;
            PD_LOG_MSG( PDERROR,
                        "OprUnit[%s] nodeid[%d] not valid, can't push up",
                        oprUnit->toString().c_str(), oprUnit->getNodeID() ) ;
            goto error ;
         }
         else if ( getOprUnitByType( oprUnit->getType() ) )
         {
            PD_LOG_MSG( PDERROR,
                        "The same type with the oprUnit[%s] exist",
                        oprUnit->toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         _oprUnits.push_back( oprUnit ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMOPTITREENODE_PUSHOPRUNIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTITREENODE_RMOPRUNIT, "_qgmOptiTreeNode::removeOprUnit" )
   INT32 _qgmOptiTreeNode::removeOprUnit( qgmOprUnit * oprUnit,
                                          BOOLEAN release,
                                          BOOLEAN updateLocal )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTITREENODE_RMOPRUNIT ) ;
      INT32 rc = SDB_OK ;
      BOOLEAN bFind = FALSE ;
      qgmOprUnitPtrVec::iterator it = _oprUnits.begin() ;
      while ( it != _oprUnits.end() )
      {
         if ( *it == oprUnit )
         {
            bFind = TRUE ;
            break ;
         }
         ++it ;
      }

      if ( !bFind )
      {
         PD_LOG_MSG( PDERROR, "Can't find the oprUnit[%s] in node[%s]",
                     oprUnit->toString().c_str(), toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( updateLocal )
      {
         rc = _removeOprUnit( oprUnit ) ;
         if ( SDB_OK != rc )
         {
            return rc ;
         }
      }

      // remove from oprUnits
      _oprUnits.erase( it ) ;

      if ( release )
      {
         SDB_OSS_DEL oprUnit ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTITREENODE_RMOPRUNIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTITREENODE_UPCHANGE, "_qgmOptiTreeNode::updateChange" )
   INT32 _qgmOptiTreeNode::updateChange( qgmOprUnit * oprUnit )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTITREENODE_UPCHANGE ) ;
      INT32 rc = SDB_OK ;

      if ( !validateBeforeChange( oprUnit->getType() ) )
      {
         goto done ;
      }
#ifdef _DEBUG
      {
         BOOLEAN bFind = FALSE ;
         qgmOprUnitPtrVec::iterator it = _oprUnits.begin() ;
         while ( it != _oprUnits.end() )
         {
            if ( *it == oprUnit )
            {
               bFind = TRUE ;
               break ;
            }
            ++it ;
         }

         if ( !bFind )
         {
            PD_LOG_MSG( PDERROR, "Can't find the oprUnit[%s] in node[%s]",
                        oprUnit->toString().c_str(), toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
#endif //_DEBUG

      rc = _updateChange( oprUnit ) ;

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTITREENODE_UPCHANGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTITREENODE_CHECK_PRIVILEGES, "_qgmOptiTreeNode::checkPrivileges" )
   INT32 _qgmOptiTreeNode::checkPrivileges( ISession *session ) const
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY(SDB__QGMOPTITREENODE_CHECK_PRIVILEGES);
      rc = _checkPrivileges( session );
      PD_RC_CHECK( rc, PDERROR, "Node %s check privileges failed", toString().c_str(), rc );

      for ( qgmOptiTreeNodePtrVec::const_iterator it = _children.begin(); it != _children.end();
            ++it )
      {
         rc = (*it)->checkPrivileges( session );
         if ( SDB_OK != rc )
         {
            goto error;
         }
      }
      done:
         PD_TRACE_EXITRC( SDB__QGMOPTITREENODE_CHECK_PRIVILEGES, rc ) ;
         return rc;
      error:
         goto done;
   }

   INT32 _qgmOptiTreeNode::_checkPrivileges( ISession *session ) const
   {
      return SDB_OK;
   }

   void _qgmOptiTreeNode::dump() const
   {
      dumpSelf( this ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTITREENODE_OUTPUTSORT, "_qgmOptiTreeNode::outputSort" )
   INT32 _qgmOptiTreeNode::outputSort( qgmOPFieldVec & sortFields )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTITREENODE_OUTPUTSORT ) ;
      // find in self, if not found, call subNode's output sort
      INT32 rc = SDB_OK ;
      qgmOprUnit *sortUnit = getOprUnitByType( QGM_OPTI_TYPE_SORT ) ;
      if ( sortUnit )
      {
         sortFields = *(sortUnit->getFields() ) ;
      }
      else if ( getSubNodeCount() > 0 )
      {
         rc = getSubNode(0)->outputSort( sortFields ) ;
      }

      if ( SDB_OK == rc && sortFields.size() > 0 )
      {
         _upFields( sortFields ) ;
      }

      PD_TRACE_EXITRC( SDB__QGMOPTITREENODE_OUTPUTSORT, rc ) ;
      return rc ;
   }

   string _qgmOptiTreeNode::toString() const
   {
      stringstream ss ;

      ss << "type:" << getQgmOptiTypeDesp( _type ) << ", nodeID:" << _nodeID ;
      if ( !_alias.empty() )
      {
         ss << ",alias:" << _alias.toString();
      }

      return ss.str() ;
   }

   string _qgmOptiTreeNode::toTotalString()
   {
      stringstream ss ;

      _toTotalString( ss, "", this ) ;

      return ss.str() ;
   }

   void _qgmOptiTreeNode::_toTotalString( stringstream &ss, const string &fill,
                                          qgmOptiTreeNode *node )
   {
      if ( NULL == node )
      {
         return ;
      }

      UINT32 i = 0 ;
      qgmOptiTreeNode *child = NULL ;

      ss << fill << node->toString() << '\n' ;
      while ( ( child = node->getSubNode( i ) ) != NULL )
      {
         _toTotalString( ss, fill + "--" , child ) ;
         i++ ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTITREENODE_EXTEND, "_qgmOptiTreeNode::extend" )
   INT32 _qgmOptiTreeNode::extend( _qgmOptiTreeNode *&exNode )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTITREENODE_EXTEND ) ;
      INT32 rc = SDB_OK ;
      _qgmOptiTreeNode *ex = NULL ;
      qgmOptiTreeNodePtrVec::iterator itr = _children.begin() ;
      for ( ; itr != _children.end(); itr++ )
      {
         rc = (*itr)->extend( ex ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         SDB_ASSERT( NULL != ex, "impossible" ) ;
         *itr = ex ;
         ex = NULL ;
      }

      rc = _extend( ex ) ;

      if ( SDB_OK != rc )
      {
         goto error ;
      }

      exNode = ex ;
   done:
      PD_TRACE_EXITRC( SDB__QGMOPTITREENODE_EXTEND, rc ) ;
      return rc ;
   error:
      if ( NULL != ex )
      {
         delete ex ;
         ex = NULL ;
      }
      goto done ;
   }

   INT32 _qgmOptiTreeNode::_extend( _qgmOptiTreeNode *&exNode )
   {
       exNode = this ;
       return SDB_OK ;
   }

//////////////_qgmOptTree
   void _qgmOptTree::_iterator::_next ()
   {
      qgmOptiTreeNode *nextNode = _pCurNode->getSubNode( 0 ) ;
      qgmOptiTreeNode *parent = NULL ;

      if ( nextNode )
      {
         _pCurNode = nextNode ;
         goto done ;
      }

      parent = _pCurNode->getParent() ;
      while ( parent )
      {
         nextNode = parent->getNextSubNode( _pCurNode ) ;
         if ( !nextNode )
         {
            _pCurNode = parent ;
            parent = _pCurNode->getParent() ;
         }
         else
         {
            _pCurNode = nextNode ;
            goto done ;
         }
      }
      _pCurNode = NULL ;

   done:
      return ;
   }

   void _qgmOptTree::_reverse_iterator::_next ()
   {
      qgmOptiTreeNode *parent = _pCurNode->getParent() ;
      qgmOptiTreeNode *nextNode = NULL ;

      if ( !parent )
      {
         _pCurNode = NULL ;
         goto done ;
      }

      nextNode = parent->getNextSubNode( _pCurNode ) ;
      if ( !nextNode )
      {
         _pCurNode = parent ;
         goto done ;
      }

      while ( nextNode )
      {
         _pCurNode = nextNode ;
         nextNode = _pCurNode->getSubNode( 0 ) ;
      }

   done:
      return ;
   }

   _qgmOptTree::iterator _qgmOptTree::begin()
   {
      return _iterator( _pRoot ) ;
   }

   _qgmOptTree::iterator _qgmOptTree::end()
   {
      return _iterator( NULL ) ;
   }

   _qgmOptTree::reverse_iterator _qgmOptTree::rbegin()
   {
      qgmOptiTreeNode *beginNode = _pRoot ;
      while ( beginNode->hasChildren() )
      {
         beginNode = beginNode->getSubNode( 0 ) ;
      }

      return _reverse_iterator( beginNode ) ;
   }

   _qgmOptTree::reverse_iterator _qgmOptTree::rend()
   {
      return _reverse_iterator( NULL ) ;
   }

   INT32 _qgmOptTree::_removeNode( qgmOptiTreeNode * pNode )
   {
      INT32 rc = SDB_OK ;

      if ( 1 != pNode->getSubNodeCount() )
      {
         PD_LOG( PDSEVERE, "Remove node[%s] sub node not 1[%d]",
                 pNode->toString().c_str(), pNode->getSubNodeCount() ) ;
         rc = SDB_SYS ;
         SDB_ASSERT( FALSE, "impossible" ) ;
         goto error ;
      }

      if ( pNode == _pRoot )
      {
         _pRoot = _pRoot->getSubNode( 0 ) ;
         _pRoot->setParent( NULL ) ;
      }
      else
      {
         qgmOptiTreeNode *parent = pNode->getParent() ;
         qgmOptiTreeNode *subNode = NULL ;
         if ( NULL != ( subNode = pNode->getSubNode( 0 ) ) )
         {
            subNode->setParent( parent ) ;
            // don't change children order
            parent->updateSubNode( pNode, subNode ) ;
         }
         else
         {
            parent->removeSubNode( pNode ) ;
         }
      }

      // release node
      pNode->notReleaseChildren() ;
      SDB_OSS_DEL pNode ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _qgmOptTree::iterator _qgmOptTree::erase( _qgmOptTree::iterator it )
   {
      qgmOptiTreeNode *pCurNode = *it ;
      _qgmOptTree::iterator nextIt = ++it ;

      if ( SDB_OK != _removeNode( pCurNode ) )
      {
         return it ;
      }

      return nextIt ;
   }

   _qgmOptTree::reverse_iterator _qgmOptTree::erase( _qgmOptTree::reverse_iterator rit )
   {
      qgmOptiTreeNode *pCurNode = *rit ;
      _qgmOptTree::reverse_iterator nextRit = ++rit ;

      if ( SDB_OK != _removeNode( pCurNode ) )
      {
         return rit ;
      }

      return nextRit ;
   }

#define QGM_OPTI_MIN_NODE_ID           (0x00000001)
#define QGM_OPTI_ROOT_NODE_ID          (0x003FFFFF)
#define QGM_OPTI_MAX_NODE_ID           (0x007FFFFF)

   _qgmOptTree::_qgmOptTree( qgmOptiTreeNode *rootNode )
   {
      _pRoot = rootNode ;
      _prtTable = _pRoot->getPtrT() ;
      _paramTable = _pRoot->getParam() ;

      _pRoot->setParent( NULL ) ;
      _pRoot->_nodeID = QGM_OPTI_ROOT_NODE_ID ;

      _prepare( _pRoot ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTTREE__PREPARE, "_qgmOptTree::_prepare" )
   void _qgmOptTree::_prepare( qgmOptiTreeNode * treeNode )
   {
      // one: set parent
      // two: set node id
      PD_TRACE_ENTRY( SDB__QGMOPTTREE__PREPARE ) ;
      UINT32 index = 0 ;
      qgmOptiTreeNode *subNode = NULL ;
      while ( NULL != ( subNode =  treeNode->getSubNode( index ) ) )
      {
         subNode->setParent( treeNode ) ;
         subNode->_nodeID =
         ( treeNode->_nodeID + ( QGM_OPTI_MAX_NODE_ID << index ) ) / 2 ;

         _prepare( subNode ) ;

         ++index ;
      }

      PD_TRACE_EXIT( SDB__QGMOPTTREE__PREPARE ) ;
      return ;
   }

   _qgmOptTree::~_qgmOptTree()
   {
      _pRoot = NULL ;
      _prtTable = NULL ;
      _paramTable = NULL ;
   }

   const CHAR* _qgmOptTree::treeName() const
   {
      // TODO:XUJIANHUI
      return "" ;
   }

   UINT32 _qgmOptTree::getNodeCount()
   {
      UINT32 count = 0 ;
      iterator it = begin() ;
      while ( it != end() )
      {
         ++count ;
         ++it ;
      }
      return count ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTTREE_INSERTBETWEEN, "_qgmOptTree::insertBetween" )
   INT32 _qgmOptTree::insertBetween( qgmOptiTreeNode * parent,
                                     qgmOptiTreeNode * sub,
                                     qgmOptiTreeNode * newNode )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTTREE_INSERTBETWEEN ) ;
      INT32 rc = SDB_OK ;

      if ( !newNode )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !parent && !sub ) // insert first node
      {
         PD_CHECK( !_pRoot, SDB_SYS, error, PDERROR, "root must be NULL" ) ;
         _pRoot = newNode ;
         _pRoot->_nodeID = QGM_OPTI_ROOT_NODE_ID ;
      }
      else if ( !parent ) // insert before root node
      {
         PD_CHECK( sub == _pRoot, SDB_SYS, error, PDERROR,
                   "root node must be the same with sub node" ) ;
         _pRoot->setParent( newNode ) ;
         newNode->addChild( _pRoot ) ;
         newNode->_nodeID = ( _pRoot->getNodeID() + QGM_OPTI_MIN_NODE_ID ) / 2 ;
         _pRoot = newNode ;
      }
      else if ( !sub ) // add child node
      {
         newNode->setParent( parent ) ;
         newNode->_nodeID = ( parent->getNodeID() + QGM_OPTI_MAX_NODE_ID ) / 2 ;
         parent->addChild( newNode ) ;
      }
      else // insert between parent and sub node
      {
         newNode->_nodeID = ( parent->getNodeID() + sub->getNodeID() ) / 2 ;
         newNode->setParent( parent ) ;
         sub->setParent( newNode ) ;
         newNode->addChild( sub ) ;
         parent->updateSubNode( sub, newNode ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTTREE_INSERTBETWEEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   qgmOptiTreeNode* _qgmOptTree::createNode( QGM_OPTI_TYPE nodeType )
   {
      qgmOptiTreeNode *newNode = NULL ;

      switch ( nodeType )
      {
         case QGM_OPTI_TYPE_SORT :
            newNode = SDB_OSS_NEW qgmOptiSort( _prtTable, _paramTable ) ;
            break ;
         case QGM_OPTI_TYPE_FILTER :
            newNode = SDB_OSS_NEW qgmOptiSelect( _prtTable, _paramTable ) ;
            newNode->_type = QGM_OPTI_TYPE_FILTER ;
            break ;
         default :
            PD_LOG( PDERROR, "Create node type[%s] error",
                    getQgmOptiTypeDesp( nodeType ) ) ;
            break ;
      }

      return newNode ;
   }

}
