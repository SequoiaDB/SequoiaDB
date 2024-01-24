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

   Source File Name = mthMergeSelector.hpp

   Descriptive Name = Method Selector

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains functions for creating a new
   record based on old record and selecting rule.

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
#include "mthMergeSelector.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
using namespace bson ;
using namespace std ;
/*******************************************************************************
    ALL USAGE OF QGM COMPONENT MUST BE PROTECTED BY TRY/CATCH
*******************************************************************************/
namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_FLDNMCHK, "fieldNameCheck" )
   static INT32 fieldNameCheck ( const CHAR *pFieldName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_FLDNMCHK );
      // field name can't be NULL and can't be empty
      PD_CHECK ( pFieldName && pFieldName[0] != '\0',
                 SDB_INVALIDARG, error, PDWARNING,
                 "Target field name must exist and can't be empty" ) ;

      // field name shouldn't include .
      PD_CHECK ( ossStrchr ( pFieldName, '.' ) == NULL,
                 SDB_INVALIDARG, error, PDWARNING,
                 "Target field name should not include '.': %s",
                 pFieldName ) ;
      // field name can't start with $
      PD_CHECK ( pFieldName[0] != '$',
                 SDB_INVALIDARG, error, PDWARNING,
                 "Target field name should not start with '$': %s",
                 pFieldName ) ;
   done :
      PD_TRACE_EXITRC ( SDB_FLDNMCHK, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MRGSE_INIT, "_MergeSelectorElement::init" )
   INT32 _MergeSelectorElement::init ( const CHAR *pSourceName,
                                       const CHAR *pTargetName,
                                       const CHAR *pOuterAlias,
                                       const CHAR *pInnerAlias )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MRGSE_INIT );
      _side  = MERGE_SELECTOR_SIDE_UNKNOWN ;
      const CHAR *pDotSource = (CHAR*)ossStrchr ( pSourceName, '.' ) ;
      PD_CHECK ( pDotSource, SDB_INVALIDARG, error, PDWARNING,
                 "Unable to find alias for source" ) ;
      // temporarily set it to 0
      *(CHAR*)pDotSource = '\0' ;
      // make sure outer and inner doesn't use same alias
      PD_CHECK ( ossStrcmp ( pOuterAlias, pInnerAlias ) != 0,
                 SDB_INVALIDARG, error, PDWARNING,
                 "alias for outer and inner side shouldn't be same" ) ;
      // find which side is the field coming from
      if ( ossStrcmp ( pSourceName, pOuterAlias ) == 0 )
         _side = MERGE_SELECTOR_SIDE_OUTER ;
      else if ( ossStrcmp ( pSourceName, pInnerAlias ) == 0 )
         _side = MERGE_SELECTOR_SIDE_INNER ;
      // make sure it's either outer or inner
      PD_CHECK ( MERGE_SELECTOR_SIDE_UNKNOWN != _side,
                 SDB_INVALIDARG, error, PDWARNING,
                 "Alias %s doesn't match any outer (%s) or inner (%s)",
                 pSourceName, pOuterAlias, pInnerAlias ) ;
      // source name should be 1 byte after '.' ( skip alias )
      _pSourceName = pDotSource + 1 ;
      if ( pTargetName )
      {
         rc = fieldNameCheck ( pTargetName ) ;
         PD_RC_CHECK ( rc, PDWARNING,
                       "Invalid target field name: %s, rc = %d",
                       pTargetName ) ;
         _pTargetName = pTargetName ;
      }
      else
      {
         // locate to the last '.' and get the embedded field name
         _pTargetName = (CHAR*)ossStrrchr ( _pSourceName, '.' ) ;
         if ( _pTargetName )
            _pTargetName++ ;
         else
            _pTargetName = _pSourceName ;
      }
      PD_LOG ( PDDEBUG, "Initialized MergeSelectorElement with\n"
               "outerAlias: %s\n"
               "innerAlias: %s\n"
               "userAlias:  %s\n"
               "sourceField: %s\n"
               "targetField: %s\n",
               pOuterAlias, pInnerAlias, pSourceName,
               _pSourceName, _pTargetName ) ;
   done :
      // restore back to .
      if ( pDotSource )
         *(CHAR*)pDotSource = '.' ;
      PD_TRACE_EXITRC ( SDB__MRGSE_INIT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMRGSEL__ADDSEL, "_mthMergeSelector::_addSelector" )
   INT32 _mthMergeSelector::_addSelector ( const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMRGSEL__ADDSEL );
      // when we add selector, we have 2 mode
      // 1) specify outer side in field name
      // 2) specify inner side in field name
      // if user does not specify the side, we return error
      // each element must be <string>:<string|null> mode, with original alias to
      // target alias.
      // if target is null, we keep using original alias ( remove outer/inner
      // side field name if exists )
      MergeSelectorElement me ;
      if ( ele.type() == String )
         rc = me.init ( ele.fieldName (), ele.valuestr (),
                        _pOuterAlias, _pInnerAlias ) ;
      else if ( ele.type() == jstNULL )
         rc = me.init ( ele.fieldName (), NULL,
                        _pOuterAlias, _pInnerAlias ) ;
      else
         PD_RC_CHECK ( SDB_INVALIDARG, PDWARNING,
                       "Invalid selector type %d, expect string or null",
                       ele.type() ) ;
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to init selector, rc = %d", rc ) ;
      _selectorElements.push_back(me) ;
   done :
      PD_TRACE_EXITRC ( SDB__MTHMRGSEL__ADDSEL, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMRGSEL_LDPAN, "_mthMergeSelector::loadPattern" )
   INT32 _mthMergeSelector::loadPattern ( const BSONObj &selectorPattern,
                                          const CHAR *pOuterAlias,
                                          const CHAR *pInnerAlias )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMRGSEL_LDPAN );
      _selectorPattern = selectorPattern.copy() ;
      BSONObjIterator i(_selectorPattern) ;
      SDB_ASSERT ( pOuterAlias, "outer alias can't be NULL" ) ;
      SDB_ASSERT ( pInnerAlias, "inner alias can't be NULL" ) ;
      _pOuterAlias = pOuterAlias ;
      _pInnerAlias = pInnerAlias ;
      INT32 numEle = 0 ;
      while ( i.more() )
      {
         rc = _addSelector(i.next() ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to parse match pattern %d", numEle ) ;
         ++numEle ;
      }

      _initialized = TRUE ;
   done :
      PD_TRACE_EXITRC ( SDB__MTHMRGSEL_LDPAN, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMRGSEL__BLDNEWOBJ, "_mthMergeSelector::_buildNewObj" )
   INT32 _mthMergeSelector::_buildNewObj ( BSONObjBuilder &b,
                                           const BSONObj &obj,
                                           const CHAR *pSourceName,
                                           const CHAR *pTargetName )
   {
      PD_TRACE_ENTRY ( SDB__MTHMRGSEL__BLDNEWOBJ );
      BSONElement ele = obj.getFieldDotted ( pSourceName ) ;
      if ( ele.eoo() )
      {
         // if we are not able to find the field, let's build null using
         // target name field name
         b.appendNull ( pTargetName ) ;
      }
      else
      {
         b.appendAs ( ele, pTargetName ) ;
      }
      PD_TRACE_EXIT ( SDB__MTHMRGSEL__BLDNEWOBJ );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMRGSEL_SELECT, "_mthMergeSelector::select" )
   INT32 _mthMergeSelector::select ( const BSONObj &outer,
                                     const BSONObj &inner,
                                     BSONObj &target )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMRGSEL_SELECT );
      SDB_ASSERT(_initialized, "The selector has not been initialized, please "
                 "call 'loadPattern' before using it" ) ;

      // create a builder with 10% extra space for buffer
      BSONObjBuilder builder ( (int)(target.objsize()*1.1));

      // index for select, should be less than _selectorElements.size()
      SINT32 selectorIndex = 0 ;

      // create a new object based on the source
      // "" is empty root, builder is BSONObjBuilder
      // es is our iterator, and selectorIndex is the current selector we are
      // going to apply
      // when this call returns SDB_OK, we should call builder.obj() to create
      // BSONObject from the builder.
      while ( (selectorIndex)<(SINT32)_selectorElements.size() )
      {
         MergeSelectorElement &ele = _selectorElements[selectorIndex] ;
         if ( ele._side == MERGE_SELECTOR_SIDE_OUTER )
         {
            rc = _buildNewObj ( builder, outer, ele._pSourceName,
                                ele._pTargetName ) ;
         }
         else
         {
            rc = _buildNewObj ( builder, inner, ele._pSourceName,
                                ele._pTargetName ) ;
         }
         PD_RC_CHECK ( rc, PDWARNING,
                       "Failed to build new obj for %s:%s, rc = %d",
                       ele._pSourceName, ele._pTargetName, rc ) ;
         selectorIndex++ ;
      }
      // now target owns the builder buffer, since obj() will decouple() the
      // buffer from builder, and assign holder to the new BSONObj
      target=builder.obj();
   done :
      PD_TRACE_EXITRC ( SDB__MTHMRGSEL_SELECT, rc );
      return rc ;
   error :
      goto done ;
   }

}
