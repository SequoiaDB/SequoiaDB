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

   Source File Name = qgmOptiTree.hpp

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

#ifndef QGMOPTITREE_HPP_
#define QGMOPTITREE_HPP_

#include "qgmDef.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "qgmPtrTable.hpp"
#include "qgmParamTable.hpp"
#include "ossUtil.hpp"
#include "sqlGrammar.hpp"
#include "pd.hpp"
#include "sdbInterface.hpp"

using namespace std ;

namespace engine
{
   enum QGM_OPTI_TYPE
   {
      QGM_OPTI_TYPE_SELECT  = 0 ,
      QGM_OPTI_TYPE_SORT,
      QGM_OPTI_TYPE_FILTER,
      QGM_OPTI_TYPE_AGGR,
      QGM_OPTI_TYPE_SCAN,
      QGM_OPTI_TYPE_JOIN,
      QGM_OPTI_TYPE_JOIN_CONDITION,
      QGM_OPTI_TYPE_INSERT,
      QGM_OPTI_TYPE_DELETE,
      QGM_OPTI_TYPE_UPDATE,
      QGM_OPTI_TYPE_COMMAND,
      QGM_OPTI_TYPE_MTHMCHSEL,
      QGM_OPTI_TYPE_MTHMCHSCAN,
      QGM_OPTI_TYPE_MTHMCHFILTER,
      QGM_OPTI_TYPE_SPLIT,

      // must be the last
      QGM_OPTI_NODE_MAX
   } ;

   const CHAR* getQgmOptiTypeDesp( QGM_OPTI_TYPE type ) ;

   struct _qgmOpStream : public SDBObject
   {
      qgmOPFieldVec  stream ;
      _qgmField      alias ;

      _qgmOpStream *next ;

      _qgmOpStream()
      :next( NULL )
      {
      }

      ~_qgmOpStream()
      {
         SAFE_OSS_DELETE( next ) ;
      }

      BOOLEAN find( const _qgmDbAttr &field ) ;
      BOOLEAN isWildCard() const ;
   } ;
   typedef struct _qgmOpStream qgmOpStream ;

   class _qgmOptiTreeNode ;
   typedef _qgmOptiTreeNode qgmOptiTreeNode ;

   class _qgmOprUnit : public _utilPooledObject
   {
      public:
         _qgmOprUnit ( QGM_OPTI_TYPE type ) ;
         _qgmOprUnit ( const _qgmOprUnit &right,
                       BOOLEAN delDup = FALSE ) ;
         virtual ~_qgmOprUnit () ;

         QGM_OPTI_TYPE  getType() const { return _type ; }
         virtual BOOLEAN isOptional() const { return _optional ; }
         UINT32         getNodeID() const { return _nodeID ; }
         BOOLEAN        isNodeIDValid() const { return 0 != _nodeID ; }
         qgmOPFieldVec* getFields() { return &_fields ; }
         BOOLEAN        isWildCardField() const ;
         void           setOptional( BOOLEAN optional ) ;
         void           setNodeID( UINT32 nodeID ) ;
         void           setFields( const qgmOPFieldVec &fields,
                                   BOOLEAN delDup = FALSE ) ;
         void           resetNodeID() ;

         INT32          addOpField( const qgmOpField &field,
                                    BOOLEAN delDup = FALSE ) ;
         INT32          addOpField( const qgmOPFieldVec &fields,
                                    BOOLEAN delDup = FALSE ) ;

         string         toString() const ;

         INT32          replaceRele( const qgmField &newRele ) ;
         INT32          replaceFieldAlias( const qgmOPFieldPtrVec &fieldAlias ) ;
         INT32          restoreFieldAlias( const qgmOPFieldPtrVec &fieldAlias ) ;

         INT32          clearAllFieldAlias() ;

         UINT32         getFieldAlias( qgmOPFieldPtrVec &aliases ) ;

         void   setDispatchAlias( const qgmField &alias ) { _dispatchAlias = alias ; }
         const qgmField& getDispatchAlias() const { return _dispatchAlias ; }
         void   clearDispatchAlias() { _dispatchAlias.clear() ; }

         BOOLEAN hasExpr() const ;

      protected:
         virtual void   _toString( stringstream &ss ) const ;
         virtual INT32  _replaceRele( const qgmField &newRele ) ;
         virtual INT32  _replaceFieldAlias( const qgmOPFieldPtrVec &fieldAlias ) ;
         virtual INT32  _restoreFieldAlias( const qgmOPFieldPtrVec &fieldAlias ) ;
      protected:
         QGM_OPTI_TYPE        _type ;
         BOOLEAN              _optional ;
         UINT32               _nodeID ;
         qgmOPFieldVec        _fields ;

         qgmField             _dispatchAlias ;

         qgmFieldVec          _reles ;

   } ;
   typedef _qgmOprUnit qgmOprUnit ;
   typedef ossPoolVector< qgmOprUnit* >      qgmOprUnitPtrVec ;

   typedef ossPoolVector< qgmOptiTreeNode* > qgmOptiTreeNodePtrVec ;
   struct _qgmConditionNode ;

   class _qgmOptTree ;

   class _qgmOptiTreeNode : public _utilPooledObject
   {
      friend class _qgmOptTree ;
   public:
      _qgmOptiTreeNode( QGM_OPTI_TYPE type, qgmPtrTable *table,
                        _qgmParamTable *param ) ;
      _qgmOptiTreeNode( QGM_OPTI_TYPE type, const qgmField &alias,
                        _qgmPtrTable *table, qgmParamTable *param ) ;
      virtual ~_qgmOptiTreeNode() ;

      virtual INT32        init () { return SDB_OK ; }
      virtual INT32        done () { return SDB_OK ; }

      qgmOptiTreeNode*     getParent() ;
      void                 setParent( qgmOptiTreeNode *parent ) ;
      UINT32               getSubNodes( qgmOptiTreeNodePtrVec &subNodes ) ;
      UINT32               getSubNodeCount() ;
      qgmOptiTreeNode*     getSubNode( UINT32 pos ) ;
      qgmOptiTreeNode*     getNextSubNode( qgmOptiTreeNode *subNode ) ;
      INT32                removeSubNode( qgmOptiTreeNode *subNode ) ;
      INT32                updateSubNode( qgmOptiTreeNode *oldNode,
                                          qgmOptiTreeNode *newNode ) ;

      UINT32               getOprUnitCount() ;
      UINT32               getOprUnits( qgmOprUnitPtrVec &oprUnitVec ) ;
      qgmOprUnit*          getOprUnitByType( QGM_OPTI_TYPE type,
                                             const qgmField & field = qgmField() ) ;
      qgmOprUnit*          getOprUnit( UINT32 pos ) ;

      UINT32               getNodeID() const { return _nodeID ; }
      BOOLEAN              validSelfAlias() const { return !_alias.empty() ; }
      const qgmField&      getAlias( BOOLEAN recursive = FALSE )
      {
         if ( !recursive || QGM_OPTI_TYPE_JOIN == _type || !_alias.empty() )
            return _alias ;
         return getSubNode(0)->getAlias( recursive ) ;
      }
      void                 setAlias( const qgmField &alias ) { _alias = alias ; }

      enum PUSH_FROM { FROM_UP ,FROM_DOWN };
      INT32                pushOprUnit( qgmOprUnit *oprUnit,
                                        qgmOptiTreeNode *fromNode,
                                        PUSH_FROM from ) ;
      INT32                removeOprUnit( qgmOprUnit *oprUnit,
                                          BOOLEAN release,
                                          BOOLEAN updateLocal ) ;
      INT32                updateChange( qgmOprUnit *oprUnit ) ;

      INT32                checkPrivileges( ISession *session ) const;

   public:
      virtual INT32     outputSort( qgmOPFieldVec &sortFields ) ;
      virtual INT32     outputStream( qgmOpStream &stream ) = 0 ;
      virtual BOOLEAN   isEmpty() { return FALSE ;}
      virtual INT32     handleHints( QGM_HINS &hints ) { return SDB_OK ;}
      virtual BOOLEAN validateBeforeChange( QGM_OPTI_TYPE type ) const
      {
         return getType() == type ;
      }

      virtual string toString() const ;

      string toTotalString() ;

      INT32 extend( _qgmOptiTreeNode *&exNode ) ;

      INT32 addChild( _qgmOptiTreeNode *child ) ;

      BOOLEAN hasChildren() const
      {
         return !_children.empty() ;
      }

      OSS_INLINE _qgmParamTable *getParam(){return _param ;}
      OSS_INLINE _qgmPtrTable *getPtrT(){return _table ;}

      QGM_OPTI_TYPE getType() const { return _type ; }

      void notReleaseChildren() { _releaseChildren = FALSE ; }

      void dump() const ;

   protected:
      UINT32 _getSubAlias( qgmFieldVec &aliases ) const ;
      INT32  _onPushOprUnit( qgmOprUnit *oprUnit, PUSH_FROM from ) ;
      void   _upFields( qgmOPFieldVec &fields ) ;
      void   _toTotalString( stringstream &ss, const string &fill,
                             qgmOptiTreeNode *node ) ;

      virtual UINT32 _getFieldAlias( qgmOPFieldPtrVec &fieldAlias,
                                     BOOLEAN getAll ) ;
      virtual INT32 _pushOprUnit( qgmOprUnit *oprUnit, PUSH_FROM from ) ;
      virtual INT32 _removeOprUnit( qgmOprUnit *oprUnit ) ;
      virtual INT32 _updateChange( qgmOprUnit *oprUnit ) ;

   private:
      virtual INT32 _extend( _qgmOptiTreeNode *&exNode ) ;
      virtual INT32 _checkPrivileges( ISession *session ) const ;

   public:
      qgmOptiTreeNodePtrVec      _children ;
      qgmField                   _alias ;
      _qgmOptiTreeNode           *_father ;
      QGM_OPTI_TYPE              _type ;
      BOOLEAN                    _releaseChildren ;
      _qgmPtrTable               *_table ;
      _qgmParamTable             *_param ;
      QGM_HINS                   _hints ;

   protected:
      qgmOprUnitPtrVec           _oprUnits ;

   private:
      UINT32                     _nodeID ;

   } ;

   class _qgmOptTree : public _utilPooledObject
   {
      public:
         class _iterator : public SDBObject
         {
            friend class _qgmOptTree ;
            protected:
               _iterator ( qgmOptiTreeNode *pNode ) { _pCurNode = pNode ; }
            public:
               _iterator () { _pCurNode = NULL ; }
               _iterator ( const _iterator &right )
               {
                  _pCurNode = right._pCurNode ;
               }
               virtual ~_iterator () { _pCurNode = NULL ; }

            public:
               qgmOptiTreeNode* operator*() { return _pCurNode ; }
               qgmOptiTreeNode* operator->() { return _pCurNode ; }
               _iterator& operator++() { _next() ; return *this ; }
               _iterator& operator++( int ) { _next() ; return *this ; }
               _iterator& operator=( const _iterator &right )
               {
                  _pCurNode = right._pCurNode ;
                  return *this ;
               }
               BOOLEAN operator== ( const _iterator &right ) const
               {
                  return _pCurNode == right._pCurNode ? TRUE : FALSE ;
               }
               BOOLEAN operator!= ( const _iterator &right ) const
               {
                  return _pCurNode != right._pCurNode ? TRUE : FALSE ;
               }

            protected:
               virtual void _next() ;

            protected:
               qgmOptiTreeNode      *_pCurNode ;
         } ;
         typedef _iterator iterator ;

         class _reverse_iterator : public _iterator
         {
            friend class _qgmOptTree ;
            protected:
               _reverse_iterator ( qgmOptiTreeNode *pNode )
                  :_iterator ( pNode ) {}
            public:
               _reverse_iterator () {}
               _reverse_iterator ( const _reverse_iterator &right )
               {
                  _pCurNode = right._pCurNode ;
               }
               virtual ~_reverse_iterator () {}

            public:
               _reverse_iterator& operator++() { _next() ; return *this ; }
               _reverse_iterator& operator++( int ) { _next() ; return *this ; }
               _reverse_iterator& operator=( const _reverse_iterator &right )
               {
                  _pCurNode = right._pCurNode ;
                  return *this ;
               }
               BOOLEAN operator== ( const _reverse_iterator &right ) const
               {
                  return _pCurNode == right._pCurNode ? TRUE : FALSE ;
               }
               BOOLEAN operator!= ( const _reverse_iterator &right ) const
               {
                  return _pCurNode != right._pCurNode ? TRUE : FALSE ;
               }

            protected:
               virtual void _next() ;
         } ;
         typedef _reverse_iterator reverse_iterator ;

      public:
         _qgmOptTree ( qgmOptiTreeNode *rootNode ) ;
         ~_qgmOptTree () ;

         qgmOptiTreeNode* getRoot() { return _pRoot ; }

         const CHAR       *treeName () const ;

         UINT32           getNodeCount() ;

      public:
         iterator             begin() ;
         iterator             end() ;
         reverse_iterator     rbegin() ;
         reverse_iterator     rend() ;

         iterator             erase( iterator it ) ;
         reverse_iterator     erase( reverse_iterator rit ) ;

         qgmOptiTreeNode*     createNode( QGM_OPTI_TYPE nodeType ) ;
         INT32                insertBetween( qgmOptiTreeNode *parent,
                                             qgmOptiTreeNode *sub,
                                             qgmOptiTreeNode *newNode ) ;

      protected:
         INT32                _removeNode( qgmOptiTreeNode *pNode ) ;
         void                 _prepare( qgmOptiTreeNode *treeNode ) ;

      private:
         qgmOptiTreeNode            *_pRoot ;
         qgmPtrTable                *_prtTable ;
         qgmParamTable              *_paramTable ;

   } ;
   typedef _qgmOptTree qgmOptTree ;

}

#endif

