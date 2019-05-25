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

   Source File Name = qgmDump.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "qgmPlanContainer.hpp"
#include "qgmPlScan.hpp"

namespace engine
{
#define QGM_DUMP_BUFFER_SIZE_CHECK \
   if ( bufferSize <= 0 )          \
   {                               \
      rc = SDB_OOM ;               \
      goto error ;                 \
   }

   class _qgmOperatorElement : public SDBObject
   {
   public :
      INT32 _id ;
      CHAR _opID[32] ;
      CHAR _opName[32] ;

      std::vector<_qgmOperatorElement*> _children ;
      INT32 _selfWidth ;
      INT32 _totalWidth ;
      INT32 _pos ;
      INT32 _level ;
      CHAR *_pCLName ;
      _qgmOperatorElement()
      {
         _id = 0 ;
         ossMemset ( _opID, 0, sizeof(_opID) ) ;
         ossMemset ( _opName, 0, sizeof(_opName) ) ;
         _selfWidth = 0 ;
         _totalWidth = 0 ;
         _pos = 0 ;
         _level = 0 ;
         _pCLName = NULL ;
      }
      ~_qgmOperatorElement()
      {
         std::vector<_qgmOperatorElement*>::iterator it ;
         for ( it = _children.begin() ;
               it != _children.end() ;
               ++it )
         {
            SDB_OSS_DEL ( *it ) ;
         }
         _children.clear() ;
         if ( _pCLName )
            SDB_OSS_FREE ( _pCLName ) ;
      }
   } ;
   typedef class _qgmOperatorElement qgmOperatorElement ;

   static INT32 qgmCalcElement ( _qgmPlan *op, _qgmOperatorElement *ele,
                                 INT32 &id, INT32 level )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( op, "op can't be NULL" ) ;
      SDB_ASSERT ( ele, "ele can't be NULL" ) ;
      ele->_id = id ;
      ele->_level = level ;
      ossSnprintf ( ele->_opID, sizeof(ele->_opID), "( %d )", ele->_id ) ;
      ++id ;
      UINT32 subEle1 = 0 ;
      UINT32 subEle2 = 0 ;
      switch ( op->type() )
      {
      case QGM_PLAN_TYPE_INSERT :
         ossStrncpy ( ele->_opName, FIELD_NAME_INSERT, sizeof(ele->_opName) ) ;
         subEle1 = 1 ;
         subEle2 = 0 ;
         break ;
      case QGM_PLAN_TYPE_UPDATE :
         ossStrncpy ( ele->_opName, FIELD_NAME_UPDATE, sizeof(ele->_opName) ) ;
         subEle1 = 0 ;
         subEle2 = subEle1 ;
         break ;
      case QGM_PLAN_TYPE_DELETE :
         ossStrncpy ( ele->_opName, FIELD_NAME_DELETE, sizeof(ele->_opName) ) ;
         subEle1 = 0 ;
         subEle2 = subEle1 ;
         break ;
      case QGM_PLAN_TYPE_RETURN :
         ossStrncpy ( ele->_opName, FIELD_NAME_RETURN, sizeof(ele->_opName) ) ;
         subEle1 = 1 ;
         subEle2 = subEle1 ;
         break ;
      case QGM_PLAN_TYPE_NLJOIN :
         ossStrncpy ( ele->_opName, FIELD_NAME_NLJOIN, sizeof(ele->_opName) ) ;
         subEle1 = 2 ;
         subEle2 = subEle1 ;
         break ;
      case QGM_PLAN_TYPE_SORT :
         ossStrncpy ( ele->_opName, FIELD_NAME_SORT, sizeof(ele->_opName) ) ;
         subEle1 = 1 ;
         subEle2 = subEle1 ;
         break ;
      case QGM_PLAN_TYPE_SCAN :
         ossStrncpy ( ele->_opName, FIELD_NAME_SCAN, sizeof(ele->_opName) ) ;
         subEle1 = 0 ;
         subEle2 = subEle1 ;
         break ;
      case QGM_PLAN_TYPE_FILTER :
         ossStrncpy ( ele->_opName, FIELD_NAME_FILTER, sizeof(ele->_opName) ) ;
         subEle1 = 1 ;
         subEle2 = subEle1 ;
         break ;
      case QGM_PLAN_TYPE_AGGR :
         ossStrncpy ( ele->_opName, FIELD_NAME_AGGR, sizeof(ele->_opName) ) ;
         subEle1 = 1 ;
         subEle2 = subEle1 ;
         break ;
      case QGM_PLAN_TYPE_COMMAND :
         ossStrncpy ( ele->_opName, FIELD_NAME_CMD, sizeof(ele->_opName) ) ;
         subEle1 = 0 ;
         subEle2 = subEle1 ;
         break ;
      case QGM_PLAN_TYPE_SPLIT :
         ossStrncpy ( ele->_opName, FIELD_NAME_SPLITBY, sizeof(ele->_opName) ) ;
         subEle1 = 1 ;
         subEle2 = subEle1 ;
         break ;
      case QGM_PLAN_TYPE_HASHJOIN :
         ossStrncpy( ele->_opName, FIELD_NAME_HASHJOIN, sizeof(ele->_opName) ) ;
         subEle1 = 2 ;
         subEle2 = subEle1 ;
         break ;
      default :
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Unsupported operator" ) ;
         goto error ;
      }
      ele->_selfWidth = OSS_MAX ( ossStrlen ( ele->_opName ),
                                  ossStrlen ( ele->_opID ) ) ;
      PD_CHECK ( subEle1 == op->inputSize() || subEle2 == op->inputSize(),
                 SDB_INVALIDARG, error, PDERROR,
                 "operator does not have valid number of subelements\n"
                 "name: %s\n"
                 "expected elements: %d or %d\n"
                 "actual elements: %d",
                 ele->_opName, subEle1, subEle2, op->inputSize() ) ;
      if ( op->inputSize() == 0 )
      {
         ele->_totalWidth = ele->_selfWidth + 2 ;
         if ( op->type() == QGM_PLAN_TYPE_SCAN )
         {
            ele->_pCLName = ossStrdup (
                  ((qgmPlScan*)op)->collection().toString().c_str() ) ;
            PD_CHECK ( ele->_pCLName, SDB_OOM, error, PDERROR,
                       "Failed to allocate memory for collection name" ) ;
            ele->_totalWidth = OSS_MAX ( (UINT32)ele->_totalWidth,
                                         ossStrlen ( ele->_pCLName ) ) ;
         }
      }
      for ( UINT32 i = 0; i < op->inputSize(); ++i )
      {
         _qgmOperatorElement *newele = SDB_OSS_NEW _qgmOperatorElement() ;
         PD_CHECK ( newele, SDB_OOM, error, PDERROR,
                    "Failed to allocate memory for new element" ) ;
         ele->_children.push_back ( newele ) ;
         rc = qgmCalcElement ( op->input ( i ), newele, id, level+1 ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to calc element, rc = %d", rc ) ;
         ele->_totalWidth += newele->_totalWidth ;
      }
      ele->_totalWidth = OSS_MAX ( ele->_selfWidth + 2,
                                   ele->_totalWidth ) ;
      ele->_pos = ele->_totalWidth / 2 ;
      if ( ele->_pos % 2 != 0 )
         ele->_pos ++ ;
   done :
      return rc ;
   error :
      goto done ;
   }
   static INT32 qgmAssignPos ( _qgmOperatorElement *ele, INT32 startingOffset )
   {
      INT32 rc = SDB_OK ;
      ele->_pos += startingOffset ;
      for ( UINT32 i = 0; i < ele->_children.size(); i++ )
      {
         _qgmOperatorElement *child = ele->_children[i] ;
         rc = qgmAssignPos ( child, startingOffset ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to assign pos, rc = %d", rc ) ;
         startingOffset += child->_totalWidth ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }
   static INT32 qgmPrintln ( _qgmOperatorElement *ele, CHAR *&pBuffer,
                             INT32 &bufferSize,
                             INT32 line, INT32 &currentOffset )
   {
      INT32 rc = SDB_DMS_EOC ;
      INT32 tempRC = SDB_DMS_EOC ;
      INT32 slen = 0 ;
      INT32 temp = 0 ;
      if ( line >= 5 )
      {
         std::vector<_qgmOperatorElement*>::iterator it ;
         for ( it = ele->_children.begin();
               it != ele->_children.end() ;
               ++it )
         {
            tempRC = qgmPrintln ( *it, pBuffer, bufferSize, line-5,
                                  currentOffset ) ;
            if ( tempRC )
            {
               PD_CHECK ( SDB_DMS_EOC == tempRC, tempRC, error, PDERROR,
                          "Failed to println, rc = %d", tempRC ) ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         goto done ;
      }
      switch ( line % 5 )
      {
      case 0 :
         slen = ossStrlen ( ele->_opID ) ;
         temp = ele->_pos - currentOffset - slen/2 ;
         for ( INT32 i = 0; i < temp; ++i )
         {
            ossSnprintf ( pBuffer, bufferSize, "%c", ' ' ) ;
            bufferSize-- ;
            pBuffer++ ;
            currentOffset ++ ;
            QGM_DUMP_BUFFER_SIZE_CHECK
         }
         ossSnprintf ( pBuffer, bufferSize, "%s", ele->_opID ) ;
         bufferSize -= slen ;
         QGM_DUMP_BUFFER_SIZE_CHECK
         pBuffer += slen ;
         currentOffset += slen ;
         break ;
      case 1 :
         slen = ossStrlen ( ele->_opName ) ;
         temp = ele->_pos - currentOffset - slen/2 ;
         for ( INT32 i = 0; i < temp; ++i )
         {
            ossSnprintf ( pBuffer, bufferSize, "%c", ' ' ) ;
            bufferSize-- ;
            pBuffer++ ;
            currentOffset ++ ;
            QGM_DUMP_BUFFER_SIZE_CHECK
         }
         ossSnprintf ( pBuffer, bufferSize, "%s", ele->_opName ) ;
         bufferSize -= slen ;
         QGM_DUMP_BUFFER_SIZE_CHECK
         pBuffer += slen ;
         currentOffset += slen ;
         break ;
      case 2 :
         temp = ele->_pos - currentOffset ;
         for ( INT32 i = 0; i < temp; ++i )
         {
            ossSnprintf ( pBuffer, bufferSize, "%c", ' ' ) ;
            bufferSize-- ;
            pBuffer++ ;
            currentOffset ++ ;
            QGM_DUMP_BUFFER_SIZE_CHECK
         }
         ossSnprintf ( pBuffer, bufferSize, "%c", '|' ) ;
         bufferSize-- ;
         pBuffer++ ;
         currentOffset++ ;
         QGM_DUMP_BUFFER_SIZE_CHECK
         break ;
      case 3 :
         for ( UINT32 i = 0; i < ele->_children.size(); i++ )
         {
            _qgmOperatorElement *child = ele->_children[i] ;
            SDB_ASSERT ( child, "child can't be NULL" ) ;
            temp = child->_pos - currentOffset ;
            for ( INT32 j = 0; j < temp; ++j )
            {
               ossSnprintf ( pBuffer, bufferSize, "%c",
                             i == 0 ? ' ':'-' ) ;
               bufferSize-- ;
               pBuffer++ ;
               currentOffset ++ ;
               QGM_DUMP_BUFFER_SIZE_CHECK
            }
            ossSnprintf ( pBuffer, bufferSize, "%c",
                          ele->_children.size() == 1 ? '|' : '+' ) ;
            bufferSize-- ;
            pBuffer++ ;
            currentOffset ++ ;
            QGM_DUMP_BUFFER_SIZE_CHECK
         }
         if ( ele->_children.size() == 0 && ele->_pCLName )
         {
            slen = ossStrlen ( ele->_pCLName ) ;
            temp = ele->_pos - currentOffset - slen/2 ;
            for ( INT32 i = 0; i < temp; ++i )
            {
               ossSnprintf ( pBuffer, bufferSize, "%c", ' ' ) ;
               bufferSize-- ;
               pBuffer++ ;
               currentOffset ++ ;
               QGM_DUMP_BUFFER_SIZE_CHECK
            }
            ossSnprintf ( pBuffer, bufferSize, "%s", ele->_pCLName ) ;
            bufferSize -= slen ;
            QGM_DUMP_BUFFER_SIZE_CHECK
            pBuffer += slen ;
            currentOffset += slen ;
         }
         break ;
      case 4 :
         for ( UINT32 i = 0; i < ele->_children.size(); i++ )
         {
            _qgmOperatorElement *child = ele->_children[i] ;
            SDB_ASSERT ( child, "child can't be NULL" ) ;
            temp = child->_pos - currentOffset ;
            for ( INT32 i = 0; i < temp; ++i )
            {
               ossSnprintf ( pBuffer, bufferSize, "%c", ' ' ) ;
               bufferSize-- ;
               pBuffer++ ;
               currentOffset ++ ;
               QGM_DUMP_BUFFER_SIZE_CHECK
            }
            ossSnprintf ( pBuffer, bufferSize, "%c", '|' ) ;
            bufferSize-- ;
            pBuffer++ ;
            currentOffset ++ ;
            QGM_DUMP_BUFFER_SIZE_CHECK
         }
         break ;
      default :
         break ;
      }
      rc = SDB_OK ;
   done :
      return rc ;
   error :
      goto done ;
   }
   static INT32 qgmPrint ( _qgmOperatorElement *ele, CHAR *&pBuffer,
                           INT32 &bufferSize )
   {
      INT32 rc = SDB_OK ;
      INT32 line = 0 ;
      while ( TRUE )
      {
         INT32 currentOffset = 0 ;
         rc = qgmPrintln ( ele, pBuffer, bufferSize, line, currentOffset ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK ( rc, PDERROR, "Failed to println, rc = %d", rc ) ;
         }
         ++line ;
         ossSnprintf ( pBuffer, bufferSize, OSS_NEWLINE ) ;
         bufferSize -= ossStrlen ( OSS_NEWLINE ) ;
         pBuffer += ossStrlen ( OSS_NEWLINE ) ; ;
         QGM_DUMP_BUFFER_SIZE_CHECK
      }
   done :
      return rc ;
   error :
      goto done ;
   }
   static INT32 qgmDumpDetails ( _qgmPlan *op, CHAR *&pBuffer,
                                 INT32 &bufferSize, INT32 &id )
   {
      INT32 rc           = SDB_OK ;
      INT32 slen         = 0 ;
      CHAR idBuffer[128] = {0} ;
      const CHAR *pField = "" ;
      std::string ps = "" ;
      ossSnprintf ( idBuffer, sizeof(idBuffer), "(%d) ", id ) ;
      ++id ;
      switch ( op->type() )
      {
      case QGM_PLAN_TYPE_INSERT :
         pField = FIELD_NAME_INSERT ;
         break ;
      case QGM_PLAN_TYPE_UPDATE :
         pField = FIELD_NAME_UPDATE ;
         break ;
      case QGM_PLAN_TYPE_DELETE :
         pField = FIELD_NAME_DELETE ;
         break ;
      case QGM_PLAN_TYPE_RETURN :
         pField = FIELD_NAME_RETURN ;
         break ;
      case QGM_PLAN_TYPE_NLJOIN :
         pField = FIELD_NAME_NLJOIN ;
         break ;
      case QGM_PLAN_TYPE_SORT :
         pField = FIELD_NAME_SORT ;
         break ;
      case QGM_PLAN_TYPE_SCAN :
         pField = FIELD_NAME_SCAN ;
         break ;
      case QGM_PLAN_TYPE_FILTER :
         pField = FIELD_NAME_FILTER ;
         break ;
      case QGM_PLAN_TYPE_AGGR :
         pField = FIELD_NAME_AGGR ;
         break ;
      case QGM_PLAN_TYPE_COMMAND :
         pField = FIELD_NAME_CMD ;
         break ;
      case QGM_PLAN_TYPE_SPLIT :
         pField = FIELD_NAME_SPLITBY ;
         break ;
      case QGM_PLAN_TYPE_HASHJOIN :
         pField = FIELD_NAME_HASHJOIN ;
         break ;
      default :
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Unsupported operator" ) ;
         goto error ;
      }
      ossStrncat ( idBuffer, pField, ossStrlen ( pField ) ) ;
      ossStrncat ( idBuffer, OSS_NEWLINE, ossStrlen ( OSS_NEWLINE ) ) ;
      slen = ossStrlen ( idBuffer ) ;
      ossSnprintf ( pBuffer, bufferSize, "%s", idBuffer ) ;
      bufferSize -= slen ;
      QGM_DUMP_BUFFER_SIZE_CHECK
      pBuffer += slen ;

      ps = op->toString() ;
      pField = ps.c_str() ;
      slen = ps.size() ;
      ossSnprintf ( pBuffer, bufferSize, "%s",
                    pField ) ;
      bufferSize -= slen ;
      QGM_DUMP_BUFFER_SIZE_CHECK
      pBuffer += slen ;
      ossSnprintf ( pBuffer, bufferSize, OSS_NEWLINE ) ;
      bufferSize -= ossStrlen ( OSS_NEWLINE ) ;
      pBuffer += ossStrlen ( OSS_NEWLINE ) ; ;
      QGM_DUMP_BUFFER_SIZE_CHECK
      for ( UINT32 i = 0; i < op->inputSize(); ++i )
      {
         rc = qgmDumpDetails ( op->input ( i ), pBuffer,
                               bufferSize, id ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to dump details, rc = %d", rc ) ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }
   INT32 qgmDump ( _qgmPlanContainer *op, CHAR *pBuffer, INT32 bufferSize )
   {
      INT32 rc = SDB_OK ;
      INT32 id = 0 ;
      SDB_ASSERT ( op && pBuffer, "op and pBuffer can't be NULL" ) ;
      _qgmOperatorElement rootElement ;
      rc = qgmCalcElement ( op->plan(), &rootElement, id, 0 ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to calc element, rc = %d", rc ) ;
      rc = qgmAssignPos ( &rootElement, 0 ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to assign pos, rc = %d", rc ) ;
      rc = qgmPrint ( &rootElement, pBuffer, bufferSize ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to print, rc = %d", rc ) ;
      id = 0 ;
      rc = qgmDumpDetails ( op->plan(), pBuffer, bufferSize, id ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to dump details, rc = %d", rc ) ;
   done :
      return rc ;
   error :
      goto done ;
   }
}
