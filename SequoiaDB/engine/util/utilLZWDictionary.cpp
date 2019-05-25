/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilLZWDictionary.cpp

   Descriptive Name = Implementation of LZW dictionary.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2015  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include "utilCompressor.hpp"
#include "utilLZWDictionary.hpp"
#include "msgDef.h"
#include <algorithm>

using namespace std ;

namespace engine
{
   _utilLZWDictionary::_utilLZWDictionary()
   {
      _dictBuff = NULL ;
      _head = NULL ;
      _nodes = NULL ;
      _maxNodeNum = 0 ;
      _cst = NULL ;
      _codeMap = NULL ;
      _dst = NULL ;
      _strArea = NULL ;
      _additionalInfo = NULL ;
   }

   _utilLZWDictionary::~_utilLZWDictionary()
   {
      reset() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY_INIT, "_utilLZWDictionary::init" )
   INT32 _utilLZWDictionary::init()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY_INIT ) ;

      UINT32 totalSize = sizeof( utilLZWDictHead )
                       + sizeof( utilLZWNode ) * UTIL_MAX_DICT_BUILD_ITEM_NUM ;

      _dictBuff = ( CHAR *)SDB_OSS_MALLOC( totalSize ) ;
      PD_CHECK( _dictBuff, SDB_OOM, error, PDERROR,
                "Failed to allocate memory for dictionary, requested size: %d",
                totalSize ) ;

      _head = ( utilLZWDictHead *)_dictBuff ;
      _nodes = ( utilLZWNode *)( (CHAR *)_head + sizeof( utilLZWDictHead ) ) ;
      _maxNodeNum = UTIL_MAX_DICT_BUILD_ITEM_NUM ;
      _nodeSortVec.reserve( UTIL_MAX_DICT_BUILD_ITEM_NUM ) ;

      for ( UINT32 i = 0; i < UTIL_MAX_DICT_BUILD_ITEM_NUM; ++i )
      {
         _nodes[i]._code = UTIL_INVALID_DICT_CODE ;
         _nodes[i]._prev = UTIL_INVALID_DICT_CODE ;
         _nodes[i]._first = UTIL_INVALID_DICT_CODE ;
         _nodes[i]._next = UTIL_INVALID_DICT_CODE ;
         /*
          * 1 byte(8 bits) can represent 256 characters. Every search will start
          * with them.
          */
         if ( i < 256 )
         {
            _nodes[i]._ch = i ;
            _nodes[i]._len = 1 ;
            _nodes[i]._refNum = 1 ;
            _nodeSortVec.push_back( &_nodes[i] ) ;
         }
         else
         {
            _nodes[i]._len = 0 ;
            _nodes[i]._refNum = 0 ;
         }
      }

      _head->_basic._type = UTIL_DICT_LZW ;
      _head->_basic._version = UTIL_LZW_DICT_VERSION ;
      _head->_maxCode = 255 ;
      _head->_codeSize = 8 ;

   done:
      PD_TRACE_EXITRC( SDB__UTILLZWDICTIONARY_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY_RESET, "_utilLZWDictionary::reset" )
   void _utilLZWDictionary::reset()
   {
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY_RESET ) ;
      if ( _dictBuff )
      {
         SDB_OSS_FREE( _dictBuff ) ;
      }

      _dictBuff = NULL ;
      _head = NULL ;
      _nodes = NULL ;
      _maxNodeNum = 0 ;
      _nodeSortVec.clear() ;
      _cst = NULL ;
      _codeMap = NULL ;
      _dst = NULL ;
      _strArea = NULL ;
      _additionalInfo = NULL ;

      PD_TRACE_EXIT( SDB__UTILLZWDICTIONARY_RESET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY__FORMATONEGRP, "_utilLZWDictionary::_formatOneGrp" )
   void _utilLZWDictionary::_formatOneGrp( UINT32 parentIdx, UINT32 &nextIdx,
                                           std::map<UINT32, UINT32> &indexMap )
   {
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY__FORMATONEGRP ) ;
      LZW_CODE code = UTIL_INVALID_DICT_CODE ;
      UINT32 index = 0 ;
      UINT32 nextPos = nextIdx ;
      BOOLEAN posSet = FALSE ;

      /*
       * Set parent's child as nextIdx, if it does has child.
       * Copy all nodes of parent to the position starts from nextIdx.
       * Insert the code into the code map at the same time.
       */
      code = _nodes[indexMap[parentIdx]]._first ;

      if ( UTIL_INVALID_DICT_CODE == code )
      {
         CST_SET_CHILD( _cst[parentIdx], CST_INVALID_CHILD ) ;
         goto done ;
      }

      index = code ;

      do
      {
         if ( UTIL_INVALID_DICT_CODE != _nodes[index]._code )
         {
            SDB_ASSERT( nextIdx <= _head->_maxValidCode, "Index out of range" ) ;
            indexMap[nextIdx] = index ;
            CST_SET_CHAR( _cst[nextIdx], _nodes[index]._ch ) ;
            _codeMap[nextIdx] = _nodes[index]._code ;
            nextIdx++ ;
            if ( !posSet )
            {
               CST_SET_CHILD( _cst[parentIdx], nextPos ) ;
               posSet = TRUE ;
            }
         }
         index = _nodes[index]._next ;
      } while ( UTIL_INVALID_DICT_CODE != index ) ;

      if ( !posSet )
      {
         CST_SET_CHILD( _cst[parentIdx], CST_INVALID_CHILD ) ;
      }

   done:
      PD_TRACE_EXIT( SDB__UTILLZWDICTIONARY__FORMATONEGRP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY__INITFINALENV, "_utilLZWDictionary::_initFinalEnv" )
   void _utilLZWDictionary::_initFinalEnv( CHAR *buff, UINT32 bufLen)
   {
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY__INITFINALENV ) ;
      ossMemset( buff, 0, bufLen ) ;
      ossMemcpy( buff, _head, sizeof( utilLZWDictHead ) ) ;

      /*
       * The original space is managed using _dictBuff, _head can pointer to the
       * new position.
       */
      _head = ( utilLZWDictHead * )buff ;
      _setVarLenSplitInfo() ;
      attach( (void *)_head ) ;

      PD_TRACE_EXIT( SDB__UTILLZWDICTIONARY__INITFINALENV ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY__FORMATREMOTESTR, "_utilLZWDictionary::_formatRemoteStr" )
   INT32 _utilLZWDictionary::_formatRemoteStr( LZW_CODE code, UINT32 &offset,
                                               UINT32 &len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY__FORMATREMOTESTR ) ;
      BYTE strBuff[UTIL_MAX_DICT_STR_LEN] = { 0 } ;
      UINT32 preLen = 0 ;
      BOOLEAN link = FALSE ;
      UINT32 totalLen = 0 ;
      DST_ITEM *currItem = &_dst[_nodes[code]._code] ;
      LZW_CODE preCode = code ;
      DST_ITEM *tmpItem = NULL ;
      CHAR localStr[DST_MAX_LOCAL_LEN] = { 0 } ;
      LZW_CODE realCode = UTIL_INVALID_DICT_CODE ;

      UINT32 pos = UTIL_MAX_DICT_STR_LEN ;

      while ( TRUE )
      {
         if ( 0 == pos )
         {
            /* Should never hit this code. */
            SDB_ASSERT( FALSE, "pos is invalid" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         strBuff[--pos] = _nodes[preCode]._ch ;
         preCode = _nodes[preCode]._prev ;
         if ( UTIL_INVALID_DICT_CODE == preCode )
         {
            /* The total string is now in the buffer */
            break ;
         }

         realCode = _nodes[preCode]._code ;

         if ( 0 != _dst[realCode] )
         {
            preLen = _nodes[preCode]._len ;
            if ( preLen <= 4 )
            {
               /* Copy all the string */
               continue ;
            }
            else
            {
               /* Set the link information */
               pos -= 3 ;
               strBuff[pos] = (BYTE)((realCode & 0x00ff0000) >> 16) ;
               strBuff[pos + 1] = (BYTE)((realCode & 0x0000ff00) >> 8) ;
               strBuff[pos + 2] = (BYTE)(realCode & 0x000000ff) ;
               link = TRUE ;
               DST_SET_LINK_FLAG( *currItem ) ;
               break ;
            }
         }
      }

      totalLen = UTIL_MAX_DICT_STR_LEN - pos ;

      ossMemcpy( _strArea + offset, &strBuff[pos], totalLen ) ;

      DST_SET_REMOTE_FLAG( *currItem ) ;
      DST_SET_REMOTE_POS( *currItem, offset ) ;
      DST_SET_REMOTE_LEN( *currItem, totalLen ) ;

      preCode = _nodes[code]._prev ;
      for ( INT32 i = 1; preCode != UTIL_INVALID_DICT_CODE; ++i )
      {
         tmpItem = &_dst[_nodes[preCode]._code] ;
         if ( *tmpItem != 0 )
         {
            SDB_ASSERT( 0 != (*tmpItem) >> 29, "wrong value" ) ;
            break ;
         }

         if ( _nodes[preCode]._len > DST_MAX_LOCAL_LEN )
         {
            DST_SET_REMOTE_FLAG( *tmpItem ) ;
            if ( link )
            {
               DST_SET_LINK_FLAG( *tmpItem ) ;
            }
            DST_SET_REMOTE_POS( *tmpItem, offset ) ;
            DST_SET_REMOTE_LEN( *tmpItem, totalLen - i ) ;
         }
         else
         {
            DST_SET_LOCAL_LEN( *tmpItem, _nodes[preCode]._len ) ;
            getStr( preCode, (BYTE*)localStr, DST_MAX_LOCAL_LEN ) ;
            DST_SET_LOCAL_STR( *tmpItem, localStr, _nodes[preCode]._len ) ;
         }

         preCode = _nodes[preCode]._prev ;
      }

      offset += totalLen ;
      len = totalLen ;
   done:
      PD_TRACE_EXIT( SDB__UTILLZWDICTIONARY__FORMATREMOTESTR ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY__FORMATONECODE, "_utilLZWDictionary::_formatOneCode" )
   INT32 _utilLZWDictionary::_formatOneCode( UINT32 &offset, LZW_CODE code )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY__FORMATONECODE ) ;
      UINT32 len = _nodes[code]._len ;
      DST_ITEM *item = &_dst[_nodes[code]._code] ;
      CHAR buff[DST_MAX_LOCAL_LEN] = {0} ;
      LZW_CODE preCode ;

      if ( 0 != *item )
      {
         goto done ;
      }

      if ( len > DST_MAX_LOCAL_LEN )
      {
         rc = _formatRemoteStr( code, offset, len ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to format remote string, rc: %d", rc ) ;
      }
      else
      {
         DST_SET_LOCAL_LEN( *item, _nodes[code]._len ) ;
         getStr( code, (BYTE*)buff, DST_MAX_LOCAL_LEN ) ;
         DST_SET_LOCAL_STR( *item, buff, _nodes[code]._len ) ;

         /* Set all its substrings */
         preCode = _nodes[code]._prev ;
         while ( UTIL_INVALID_DICT_CODE != preCode )
         {
            item = &_dst[_nodes[preCode]._code] ;
            if ( 0 == *item )
            {
               DST_SET_LOCAL_LEN( *item, _nodes[preCode]._len ) ;
               getStr( preCode, (BYTE*)buff, DST_MAX_LOCAL_LEN ) ;
               DST_SET_LOCAL_STR( *item, buff, _nodes[preCode]._len ) ;
            }

            preCode = _nodes[preCode]._prev ;
         }
      }

   done:
      PD_TRACE_EXIT( SDB__UTILLZWDICTIONARY__FORMATONECODE ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY__FORMATDST, "_utilLZWDictionary::_formatDst" )
   INT32 _utilLZWDictionary::_formatDst( UINT32 &size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY__FORMATDST ) ;
      LZW_CODE currCode = _head->_maxCode + 1 ;
      UINT32 strOffset = 0 ;

      do
      {
         --currCode ;
         if ( UTIL_INVALID_DICT_CODE != _nodes[currCode]._code )
         {
            rc = _formatOneCode( strOffset, currCode ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to format dictionary item, rc: %d", rc ) ;
         }
      } while ( currCode > 0 ) ;

      size = _strArea - (CHAR*)_dst + strOffset ;
   done:
      PD_TRACE_EXIT( SDB__UTILLZWDICTIONARY__FORMATDST ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _utilLZWDictionary::_sortNodeByRef( const _utilLZWNode *left,
                                               const _utilLZWNode *right )
   {
      return left->_refNum > right->_refNum ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY__CALCCODESIZE, "_utilLZWDictionary::_calcCodeSize" )
   UINT32 _utilLZWDictionary::_calcCodeSize( UINT32 code )
   {
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY__CALCCODESIZE ) ;
      UINT32 codeSize = 1 ;
      while ( 0 != ( code >>= 1 ) )
      {
         codeSize++ ;
      }

      PD_TRACE_EXIT( SDB__UTILLZWDICTIONARY__CALCCODESIZE ) ;
      return codeSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY__ADDADDITIONALINFO, "_utilLZWDictionary::_addAdditionalInfo" )
   void _utilLZWDictionary::_formatAdditionalInfo( BSONObj &obj )
   {
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY__ADDADDITIONALINFO ) ;
      BSONObjBuilder builder ;
      ossTimestamp createTime ;
      CHAR timestampStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;

      ossGetCurrentTime( createTime ) ;
      ossTimestampToString( createTime, timestampStr ) ;
      builder.append( FIELD_NAME_DICT_CREATE_TIME, timestampStr ) ;

      obj = builder.obj() ;

      PD_TRACE_EXIT( SDB__UTILLZWDICTIONARY__ADDADDITIONALINFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY__SETVARLENSPLITINFO, "_utilLZWDictionary::_setVarLenSplitInfo" )
   void _utilLZWDictionary::_setVarLenSplitInfo()
   {
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY__SETVARLENSPLITINFO ) ;
      _head->_varLenFlagSize = UTIL_VAR_LEN_FLAG_SIZE ;
      _head->_splitInfo[ UTIL_MAX_DICT_SPLIT_NUM - 1 ] = _head->_codeSize ;
      _head->_splitInfo[ UTIL_MAX_DICT_SPLIT_NUM - 2 ] = _head->_codeSize - 3 ;
      _head->_splitInfo[ UTIL_MAX_DICT_SPLIT_NUM - 3 ] = _head->_codeSize - 5 ;
      _head->_splitInfo[ UTIL_MAX_DICT_SPLIT_NUM - 4 ] = _head->_codeSize - 7 ;
      PD_TRACE_EXIT( SDB__UTILLZWDICTIONARY__SETVARLENSPLITINFO ) ;
   }

   void _utilLZWDictionary::_cstCheck()
   {
      UINT32 low = 0 ;
      UINT32 high = 0 ;
      UINT32 index = 0 ;
      UINT32 i = 0 ;
      UINT32 compIndex = 0 ;

      while ( index <= UTIL_MAX_DICT_INIT_CODE )
      {
         SDB_ASSERT( index == (UINT32)CST_GET_CHAR( _cst[index] ),
                     "Wrong code in CST" ) ;
         index++ ;
      }

      for ( index = 0; index <= _head->_maxCode; ++index )
      {
         low = CST_GET_CHILD( _cst[index] ) ;
         if ( CST_INVALID_CHILD == low )
         {
            continue ;
         }

         i = 1 ;
         high = CST_INVALID_CHILD ;
         while ( index + i < _head->_maxCode
                 && ( CST_INVALID_CHILD ==
                      ( high = CST_GET_CHILD( _cst[index + i] ) ) ) )
         {
            ++i ;
         }

         if ( CST_INVALID_CHILD == high )
         {
            high = _head->_maxCode ;   /* Reached the end. */
         }
         else
         {
            high-- ;
         }

         for ( compIndex = low; compIndex < high; ++compIndex )
         {
            SDB_ASSERT( CST_GET_CHAR( _cst[compIndex] )
                        < CST_GET_CHAR( _cst[compIndex + 1] ),
                        "Wrong character in _cst" ) ;
         }
      }
   }

   void _utilLZWDictionary::_codeMapCheck()
   {
      CST_ITEM item  = 0 ;
      std::set<LZW_CODE> codeSet ;
      std::set<LZW_CODE>::iterator itr ;

      for ( UINT32 index = 0; index <= _head->_maxCode; ++index )
      {
         item = _cst[index] ;
         if ( CST_IS_VALID_CODE( item ) )
         {
            SDB_ASSERT( _codeMap[index] <= _head->_maxValidCode,
                        "Code out of valid range" ) ;
         }
         else
         {
            SDB_ASSERT( ( _codeMap[index] > _head->_maxValidCode )
                        && ( _codeMap[index] <= _head->_maxCode ),
                        "Code in wrong range" ) ;
         }

         itr = codeSet.find( _codeMap[index] ) ;
         SDB_ASSERT( itr == codeSet.end(), "Duplicated code found" ) ;
         codeSet.insert( _codeMap[index] ) ;
      }
   }

   void _utilLZWDictionary::_dstCheck()
   {

   }

   void _utilLZWDictionary::_healthCheck()
   {
      _cstCheck() ;
      _codeMapCheck() ;
      _dstCheck() ;

      PD_LOG( PDEVENT, "Dictioanry health check pass" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTIONARY_FINALIZE, "_utilLZWDictionary::finalize" )
   INT32 _utilLZWDictionary::finalize( CHAR *buff, UINT32 &length )
   {
      /* format the original dictionary into the final structure */
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILLZWDICTIONARY_FINALIZE ) ;
      LZW_CODE code = 0 ;
      UINT32 nextIdx = UTIL_MAX_DICT_INIT_CODE + 1 ;
      /*
       * indexMap is used to maintain the index in CST and the original index
       * (code) in the initial dicitonary. As the nodes are put into the final
       * dictionary CST level by level, their new indexes are different.
       */
      std::map<UINT32, UINT32> indexMap ;
      UINT32 dstSize = 0 ;
      BSONObj addInfo ;
      UINT32 writePos = 0 ;
      INT32 remainInitNode = 256 ;
      INT32 remainCode = UTIL_MAX_DICT_ITEM_NUM ;
      INT32 currentIdx = 0 ;
      #define GET_NODE_IDX(nodePtr) \
         ( ( (CHAR*)(nodePtr) - (CHAR*)_nodes ) / sizeof( _utilLZWNode ) )

      sort( _nodeSortVec.begin(), _nodeSortVec.end(), _sortNodeByRef ) ;

      for ( vector<_utilLZWNode*>::iterator itr = _nodeSortVec.begin();
            itr != _nodeSortVec.end(); ++itr )
      {
         currentIdx = GET_NODE_IDX( *itr ) ;
         if ( currentIdx <= UTIL_MAX_DICT_INIT_CODE )
         {
            remainInitNode-- ;
         }
         else
         {
            if ( remainCode <= remainInitNode )
            {
               continue ;
            }
         }

         (*itr)->_code = code ;
         _head->_maxValidCode = code ;
         remainCode-- ;
         code++ ;
         if ( UTIL_MAX_DICT_ITEM_NUM == code )
         {
            break ;
         }
      }

#ifdef _DEBUG
      for ( LZW_CODE tmpCode = 0; tmpCode <= UTIL_MAX_DICT_INIT_CODE; ++tmpCode )
      {
         SDB_ASSERT( UTIL_INVALID_DICT_CODE != _nodes[tmpCode]._code,
                     "Invalid code of initial node" ) ;
      }
#endif /* _DEBUG */

      _head->_codeSize = _calcCodeSize( _head->_maxValidCode ) ;

      _initFinalEnv( buff, length ) ;

      for ( LZW_CODE code = 0; code <= UTIL_MAX_DICT_INIT_CODE; ++code )
      {
         CST_SET_CHAR( _cst[code], _nodes[code]._ch ) ;
         _codeMap[code] = _nodes[code]._code ;
         indexMap[code] = code ;
      }

      for ( UINT32 index = 0; index < nextIdx; ++index )
      {
         _formatOneGrp( index, nextIdx, indexMap ) ;
      }

      rc = _formatDst( dstSize ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to format decompression string table, rc: %d", rc ) ;

      writePos = (CHAR*)_dst - (CHAR*)_head + dstSize ;

      _formatAdditionalInfo( addInfo ) ;

      if ( length < writePos + addInfo.objsize() )
      {
         PD_LOG( PDERROR, "Input buffer size too small, size: %u", length ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossMemcpy( buff + writePos, addInfo.objdata(), addInfo.objsize() ) ;
      length =  writePos + addInfo.objsize() ;

#ifdef _DEBUG
#endif /* _DEBUG */

   done:
      PD_TRACE_EXITRC( SDB__UTILLZWDICTIONARY_FINALIZE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTCREATOR_PREPARE, "_utilLZWDictionary::prepare" )
   INT32 _utilLZWDictCreator::prepare()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILLZWDICTCREATOR_PREPARE ) ;

      rc = _dictionary.init() ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to initialize dictionary, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILLZWDICTCREATOR_PREPARE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTCREATOR_RESET, "_utilLZWDictionary::reset" )
   void _utilLZWDictCreator::reset()
   {
      PD_TRACE_ENTRY( SDB__UTILLZWDICTCREATOR_RESET ) ;

      _dictionary.reset() ;

      PD_TRACE_EXIT( SDB__UTILLZWDICTCREATOR_RESET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTCREATOR_BUILD, "_utilLZWDictionary::build" )
   void _utilLZWDictCreator::build( const CHAR *source, UINT32 sourceLen,
                                    BOOLEAN &full )
   {
      PD_TRACE_ENTRY( SDB__UTILLZWDICTCREATOR_BUILD ) ;
      UINT8 ch = 0 ;
      UINT32 pos = 0 ;
      UINT32 strLen = 0 ;
      BOOLEAN restart = FALSE ;
      LZW_CODE code = UTIL_INVALID_DICT_CODE ;
      LZW_CODE nextCode = UTIL_INVALID_DICT_CODE ;
      LZW_CODE maxCode = _dictionary.getMaxNodeNum() - 1 ;

      ch = source[0] ;
      code = ch ;
      pos++ ;
      strLen++ ;
      full = FALSE ;

      for ( ; pos < sourceLen; ++pos )
      {
         if ( !restart )
         {
            ch = source[pos] ;
         }
         else
         {
            code = (UINT8)source[pos] ;
            strLen = 1 ;
            if ( pos + 1 == sourceLen )
            {
               break ;
            }

            ch = source[++pos] ;
            restart = FALSE ;
         }

         nextCode = _dictionary.findStr( code, ch ) ;
         if ( UTIL_INVALID_DICT_CODE == nextCode )
         {
            nextCode = _dictionary.addStr( code, ch ) ;
            if ( nextCode == maxCode )
            {
               /* Dictionary is full */
               full = TRUE ;
               break ;
            }

            code = ch ;
            strLen = 1 ;
         }
         else
         {
            code = nextCode ;
            strLen++ ;
            if ( strLen >= UTIL_MAX_DICT_STR_LEN )
            {
               restart = TRUE ;
            }
         }
      }

      PD_TRACE_EXIT( SDB__UTILLZWDICTCREATOR_BUILD ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILLZWDICTCREATOR_FINALIZE, "_utilLZWDictionary::finalize" )
   INT32 _utilLZWDictCreator::finalize( CHAR *buff, UINT32 &size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILLZWDICTCREATOR_FINALIZE ) ;
      rc = _dictionary.finalize( buff, size ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to finalize dictionary, rc: %d", rc ) ;
   done:
      PD_TRACE_EXITRC( SDB__UTILLZWDICTCREATOR_FINALIZE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

