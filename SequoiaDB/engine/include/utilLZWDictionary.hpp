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

   Source File Name = utilLZWDictionary.hpp

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
#ifndef UTIL_LZWDICTIONARY
#define UTIL_LZWDICTIONARY

#include "core.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "ossUtil.h"
#include "pd.hpp"
#include "utilDictionary.hpp"
#include "../bson/bson.h"
#include <map>

using namespace bson ;

namespace engine
{
   /* Maximum dictionary size. Should be the same with DMS_DICT_MAX_SIZE. */
   #define UTIL_DICT_MAX_SIZE            ( 4 * 1024 * 1024 )

   /*
    * Internally we use a 32bit buffer to do continuous bits reading and
    * writting, so the maximum node code should be less than 2^24(16M).
    */
   #define UTIL_INVALID_DICT_CODE        16777215

   /*
    * 0~255 represent 256 diffrent symbols(initial state of the dictionary),
    * duplicated strings can be handle only when more are added to the
    * dictionary.
    */
   #define UTIL_MAX_DICT_INIT_CODE        255
   #define UTIL_MIN_DICT_ITEM_NUM         257
   #define UTIL_MAX_DICT_ITEM_NUM         250000
   #define UTIL_MAX_DICT_BUILD_ITEM_NUM   ( UTIL_MAX_DICT_ITEM_NUM * 4 )
   #define UTIL_MAX_DICT_CODE_SIZE        18
   #define UTIL_MAX_DICT_STR_LEN          255
   #define UTIL_MAX_DICT_ADD_INFO_SIZE    1024

   /*
    * 16 = 4( cst item size) + 4( code map item size ) + 4( dst item size )
    *      + 4( max average length in string area )
    */
   #define UTIL_MAX_DICT_TOTAL_SIZE \
      ( sizeof( _utilLZWDictHead ) + UTIL_MAX_DICT_ITEM_NUM * 16 \
        + UTIL_MAX_DICT_ADD_INFO_SIZE )

   typedef UINT32 LZW_CODE ;

   #define DICT_BITS_UPBOUND( bits )     ( ( 1 << bits ) - 1 )

   /*
    * Used to perform byte(bits) reading and writting of compressed stream data.
    * As code of each dictionary node will be stored as part of the dictionary,
    * we use at least bits to represent a node as possible in order to save
    * space( both in memory and on disk). The number of bits may be not multiple
    * of 8, but we store the stream byte by byte. So the bits of one code may
    * be handled in more than one shot. This buffer is used to handle the
    * combination of the code bits.
    */
   struct _utilBitBuffer
   {
      UINT32 _buf ;
      UINT32 _n ;     // Number of bits remain in the buffer.
   } ;
   typedef _utilBitBuffer utilBitBuffer ;

   /*
    * Dictionary node in LZW. Each node represents a string, which linked by
    * prev of each node. This node contains the last character of the string.
    * All nodes which mark this node as 'prev' are linked by their 'next', and
    * this parent node points to the first child using 'first'.
    */
   struct _utilLZWNode
   {
      LZW_CODE  _code ;
      LZW_CODE  _prev ;   // Code of the previous part of the string
                          // (the last character excluded)
      LZW_CODE  _first ;  // first 'child'
      LZW_CODE  _next ;   // first 'brother'
      UINT32    _len ;    // Length of the string.
      UINT32    _refNum ; // Reference number, used during dictionary creation.
      BYTE      _ch ;     // Last character of the string this node represents.
      UINT8     _pad[3] ;
   } ;
   typedef _utilLZWNode utilLZWNode ;

   #define UTIL_VAR_LEN_FLAG_SIZE      2
   #define UTIL_MAX_DICT_SPLIT_NUM     ( 1 << UTIL_VAR_LEN_FLAG_SIZE )
   /* Codes in the dictionary are divided into two parts.
    * The preceding part contains the codes(0~x) which are really finally used
    * in the compressed data. The remaining codes are used for the searching
    * during compression and get the final string during decompression.
    */
   struct _utilLZWDictHead
   {
      utilDictHead _basic ;
      UINT32 _maxCode ;        /* Maximum code in the dictionary. */
      UINT32 _maxValidCode ;
      UINT8 _codeSize ;        /* Bit number to represent a code. */
      UINT8 _varLenFlagSize ;
      CHAR _reserve[2] ;
      UINT8 _splitInfo[ UTIL_MAX_DICT_SPLIT_NUM ] ;
   } ;
   typedef _utilLZWDictHead utilLZWDictHead ;

   /*
    * CST -- Compression String Table
    * DST -- Decompression String Table
    */
   typedef UINT32 CST_ITEM ;
   typedef UINT32 DST_ITEM ;

   /*
    * Length of item in CST is 4 bytes. It contains two parts.
    *  __________________________________
    * |_______|________|________|________|
    *
    * |-|--index of child item--|- char -|
    */
   /* Notice: When using the following macros, both item and child should be of
    * type UINT32
    */

   /* First bit of the CST item. It indicates whether this is a valid code. */
   #define CST_VALID_CODE_FLAG               0x80000000
   #define CST_INVALID_CHILD                 0x007FFFFF
   #define CST_GET_CHILD( item )   \
      ( (item & (~CST_VALID_CODE_FLAG) ) >> 8 )

   #define CST_SET_CHILD( item, child )   \
      ( (item) = ((item) & 0x800000FF) | ( (child) << 8 ) )

   #define CST_GET_CHAR( item )              ( (item) & 0x000000FF )

   #define CST_SET_CHAR( item, ch )       \
      ( (item) = ((item) & 0xFFFFFF00 ) | ch )

   #define CST_SET_VALID_CODE_FLAG( item ) \
      ((item)  = (item) | CST_VALID_CODE_FLAG )

   #define CST_IS_VALID_CODE( item ) ( ( item ) & CST_VALID_CODE_FLAG )
   /*
    * Length of item in DST is 4 bytes.
    *
    */
   #define DST_MAX_LOCAL_LEN                 3
   #define DST_REMOTE_FLAG                   0x80000000
   #define DST_LINK_FLAG                     0x40000000
   #define DST_LOCAL_LEN_MASK                0x60000000
   #define DST_REMOTE_LEN_MASK               0x000000FF
   #define DST_IS_REMOTE( item )             ( (item) & DST_REMOTE_FLAG )
   #define DST_SET_REMOTE_FLAG( item )    \
      ( (item) = ( (item) | DST_REMOTE_FLAG ) )
   #define DST_UNSET_REMOTE_FLAG( item )  \
      ( (item) = ( (item) & (~DST_REMOTE_FLAG) ) )

   #define DST_SET_LINK_FLAG( item ) \
      ( (item) = ( (item) | DST_LINK_FLAG ) )

   #define DST_LOCAL_LEN_SHIFT_NUM           29
   #define DST_SET_LOCAL_LEN( item, len ) \
      ( (item) = ( (item) | ( (len) << DST_LOCAL_LEN_SHIFT_NUM ) ) )
   #define DST_GET_LOCAL_LEN( item )  \
      ( ( (item) & DST_LOCAL_LEN_MASK ) >> DST_LOCAL_LEN_SHIFT_NUM )

   #define DST_SET_LOCAL_STR( item, str, len ) \
   do \
   { \
      UINT32 value = 0 ; \
      for ( UINT32 i = 0; i < len; ++i )  \
      {  \
         value <<= 8 ;  \
         value |= (UINT8)str[i] ;  \
      }  \
      item &= ( 0xffffffff << ( 8 * len ) ) ;   \
      item |= value ;  \
   } while ( 0 )

   #define DST_GET_LOCAL_STR( item, buff )   \
   do { \
      UINT32 len = DST_GET_LOCAL_LEN( item ) ; \
      UINT32 value = item & ( ~ ( 0xffffffff << ( 8 * len ) ) ) ; \
      for ( UINT32 i = 0; i < len; ++i )  \
      {  \
         buff[len - i - 1] = (CHAR)( value & 0x000000ff ) ; \
         value >>= 8 ;  \
      }  \
   } while ( 0 )

   #define DST_REMOTE_POS_SHIFT_NUM          8
   #define DST_REMOTE_POS_MASK               0x0fffff00

   #define DST_SET_REMOTE_POS( item, pos )   \
      ( (item) = ( ( (item) & (~DST_REMOTE_POS_MASK) )   \
                   | ( (pos) << DST_REMOTE_POS_SHIFT_NUM ) ) )

   #define DST_SET_REMOTE_LEN( item, len )   \
      ( (item) = ( ( (item) & (~DST_REMOTE_LEN_MASK) ) | ( len ) ) )

   #define DST_GET_REMOTE_POS( item )  \
      ( ( (item) & DST_REMOTE_POS_MASK) >> DST_REMOTE_POS_SHIFT_NUM )

   #define DST_GET_REMOTE_LEN( item )  \
      ( (item) & DST_REMOTE_LEN_MASK )

   class _utilLZWDictionary : public SDBObject
   {
   public:
      _utilLZWDictionary() ;
      ~_utilLZWDictionary() ;

      INT32 init() ;
      void reset() ;
      UINT32 getMaxNodeNum() { return _maxNodeNum ; }
      UINT32 getMaxValidCode() { return _head->_maxValidCode ; }
      UINT8 getCodeSize() { return _head->_codeSize; }

      OSS_INLINE LZW_CODE addStr( LZW_CODE preCode, UINT8 ch ) ;
      OSS_INLINE LZW_CODE findStr( LZW_CODE preCode, UINT8 ch ) ;
      OSS_INLINE UINT32 getStr( LZW_CODE code, BYTE *buff, UINT32 bufSize ) ;

      INT32 finalize( CHAR *buff, UINT32 &length ) ;

      /* Interfaces used by compression/decompression */
      OSS_INLINE void attach( const void *dictionary ) ;

      OSS_INLINE LZW_CODE findStrExt( const BYTE *str, UINT32 &length ) ;
      OSS_INLINE UINT32 getStrExt( LZW_CODE code, BYTE *buff,
                                   UINT32 buffSize ) ;
      OSS_INLINE UINT8 getVarLenFlagSize() { return _head->_varLenFlagSize ; }
      OSS_INLINE void getVarLenInfo( LZW_CODE code, UINT8 &lenIdx,
                                     UINT8 &splitSize ) ;
      OSS_INLINE UINT8 getVarLenSize( UINT8 lenIdx )
      {
         return _head->_splitInfo[lenIdx];
      }

   private:
      void _initFinalEnv( CHAR *buff, UINT32 bufLen ) ;
      INT32 _formatRemoteStr( LZW_CODE code, UINT32 &offset, UINT32 &len ) ;
      INT32 _formatOneCode( UINT32 &offset, LZW_CODE code ) ;
      INT32 _formatDst( UINT32 &size ) ;
      void _formatOneGrp( UINT32 parentIdx, UINT32 &nextIdx,
                          std::map<UINT32, UINT32> &indexMap ) ;
      UINT32 _calcCodeSize( UINT32 code ) ;
      void _formatAdditionalInfo( BSONObj &obj ) ;
      void _setVarLenSplitInfo() ;

      void _cstCheck() ;
      void _codeMapCheck() ;
      void _dstCheck() ;
      void _healthCheck() ;

      OSS_INLINE INT32 _cstBinSearch( UINT32 low, UINT32 high, BYTE ch ) ;
      OSS_INLINE UINT32 _dstGetRemoteStr( DST_ITEM item, CHAR *buff ) ;
      OSS_INLINE UINT32 _findStrIdx( const BYTE *str, UINT32 &length ) ;

      static BOOLEAN _sortNodeByRef( const _utilLZWNode *left,
                                     const _utilLZWNode *right ) ;
   private:
      /*
       * The following members are mainly used during the creation of the
       * dictionary(_head will be used during compression and decompression
       * too).
       */
      CHAR *_dictBuff ;          /* Buffer for temporary storage */
      _utilLZWDictHead *_head ;
      _utilLZWNode *_nodes ;
      UINT32 _maxNodeNum ;
      vector<_utilLZWNode*> _nodeSortVec ;

   private:
      CST_ITEM *_cst ;
      LZW_CODE *_codeMap ;
      DST_ITEM *_dst ;
      CHAR *_strArea ;
      BSONObj *_additionalInfo ;
   } ;
   typedef _utilLZWDictionary utilLZWDictionary ;

   /*
    * For dictionary storage, both in memory and on disk. The above structure
    * _utilLZWDictionary as well as all the nodes will be stored together
    */
   #define MIN_DICT_SIZE \
      ( sizeof( _utilLZWDictionary ) + sizeof( _utilLZWNode) * UTIL_MIN_DICT_ITEM_NUM )

   class _utilLZWContext : public SDBObject
   {
      friend class _utilLZWDictCreator ;
      friend class _utilCompressorLZW ;
   public:
      _utilLZWContext()
      {
         reset( FALSE ) ;
      }

      ~_utilLZWContext() {}

      OSS_INLINE void setDictionary( _utilLZWDictionary *dict )
      {
         SDB_ASSERT( dict, "Dictionary is invalid" ) ;
         _dictionary = dict ;
      }

      _utilLZWDictionary* getDictionary() { return _dictionary ; }

      OSS_INLINE void reset( BOOLEAN keepDict )
      {
         if ( !keepDict )
         {
            _dictionary = NULL ;
         }

         _stream = NULL ;
         _streamLen = 0 ;
         _streamPos = 0 ;
         _bitBuf._buf = 0 ;
         _bitBuf._n = 0 ;
      }

   private:
      utilLZWDictionary *_dictionary ;
      BYTE *_stream ;
      UINT32 _streamLen ;
      UINT32 _streamPos ;
      utilBitBuffer _bitBuf ;   /* bit buffer used during encoding and decoding */
   } ;
   typedef _utilLZWContext utilLZWContext ;

   class _utilLZWDictCreator : public _utilDictCreator
   {
   public:
      _utilLZWDictCreator() {}
      ~_utilLZWDictCreator()
      {
         reset() ;
      }

      /* The following member functions are used to handle the dictionary. */
      INT32 prepare() ;
      void reset() ;

      /*
       * Use the source data to build the dictionary for LZW. It can be called
       * multiple times, and the dictionary will grow until reaching the maximum
       * size.
       * This function should be called after prepare.
       */
      void build( const CHAR *src, UINT32 srcLen, BOOLEAN &full ) ;
      INT32 finalize( CHAR *buff, UINT32 &size ) ;

   private:
      _utilLZWDictionary _dictionary ;
   } ;
   typedef _utilLZWDictCreator utilLZWDictCreator ;

   #define UTIL_VAR_LEN_FLAG           0x80000000
   #define UTIL_VAR_LEN_LENGTH_MASK    0x00FFFFFF
   class _utilLZWHeader
   {
   public:
      _utilLZWHeader()
      : _value( 0 )
      {
      }

      void setVarLenFlag()   { _value |= UTIL_VAR_LEN_FLAG ; }
      BOOLEAN isVarLenCode() { return ( 0 != (_value & UTIL_VAR_LEN_FLAG ) ) ; }
      void setLength( UINT32 len )
      {
         _value = ( ( _value & UTIL_VAR_LEN_FLAG ) | len ) ;
      }
      UINT32 getLength()
      {
         return ( _value & UTIL_VAR_LEN_LENGTH_MASK ) ;
      }
   private:
      UINT32 _value ;
   } ;
   typedef _utilLZWHeader utilLZWHeader ;

   /* Add new string to the LZW dictionary. */
   OSS_INLINE LZW_CODE _utilLZWDictionary::addStr( LZW_CODE preCode, UINT8 ch )
   {
      LZW_CODE currCode ;
      LZW_CODE nextCode ;

      if ( _head->_maxCode + 1 == _maxNodeNum )
      {
         /* Dictionary is full. */
         return UTIL_INVALID_DICT_CODE ;
      }

      _head->_maxCode++ ;
      /*
       * The current code size is not enough to represent the code, so increase
       * it by 1 bit.
       */
      if ( _head->_maxCode == (UINT32)( 1 << _head->_codeSize ) )
      {
         _head->_codeSize++ ;
      }

      _nodes[_head->_maxCode]._prev = preCode ;
      _nodes[_head->_maxCode]._first = UTIL_INVALID_DICT_CODE ;

      if ( UTIL_INVALID_DICT_CODE == _nodes[preCode]._first )
      {
         _nodes[preCode]._first = _head->_maxCode ;
      }
      else
      {
         currCode = UTIL_INVALID_DICT_CODE ;
         nextCode = _nodes[preCode]._first ;
         while ( ( UTIL_INVALID_DICT_CODE != nextCode )
                 && ( ch > _nodes[nextCode]._ch ) )
         {
            currCode = nextCode ;
            nextCode = _nodes[currCode]._next ;
         }

         _nodes[_head->_maxCode]._next = nextCode ;
         if ( UTIL_INVALID_DICT_CODE == currCode )
         {
            _nodes[preCode]._first = _head->_maxCode ;
         }
         else
         {
            _nodes[currCode]._next = _head->_maxCode ;
         }
      }

      _nodes[_head->_maxCode]._ch = ch ;
      _nodes[_head->_maxCode]._len = _nodes[preCode]._len + 1 ;
      _nodes[_head->_maxCode]._refNum = 0 ;
      _nodeSortVec.push_back( &_nodes[_head->_maxCode] ) ;

      return _head->_maxCode ;
   }

   OSS_INLINE LZW_CODE _utilLZWDictionary::findStr( LZW_CODE preCode, UINT8 ch )
   {
      LZW_CODE nextCode ;

      if ( preCode <= UTIL_MAX_DICT_INIT_CODE )
      {
         _nodes[preCode]._refNum++ ;
      }

      for ( nextCode = _nodes[preCode]._first; nextCode != UTIL_INVALID_DICT_CODE;
            nextCode = _nodes[nextCode]._next )
      {
         if ( ch > _nodes[nextCode]._ch )
         {
            continue ;
         }

         if ( preCode == _nodes[nextCode]._prev && ch == _nodes[nextCode]._ch )
         {
            _nodes[nextCode]._refNum++ ;
            return nextCode ;
         }
         else
         {
            break ;
         }
      }

      return UTIL_INVALID_DICT_CODE ;
   }

   OSS_INLINE UINT32 _utilLZWDictionary::getStr( LZW_CODE code, BYTE *buff,
                                                 UINT32 bufSize )
   {
      UINT32 strLen = _nodes[code]._len ;
      UINT32 i = strLen ;
      SDB_ASSERT( bufSize >= strLen, "Invalid argument, bufSize too small" ) ;

      while ( code != UTIL_INVALID_DICT_CODE && i )
      {
         buff[--i] = _nodes[code]._ch ;
         code = _nodes[code]._prev ;
      }

      return strLen ;
   }

   OSS_INLINE INT32 _utilLZWDictionary::_cstBinSearch( UINT32 low, UINT32 high,
                                                       BYTE ch )
   {
      UINT32 mid ;
      BYTE currCh ;
      INT32 index = CST_INVALID_CHILD ;

      while ( low <= high )
      {
         mid = ( low + high ) / 2 ;
         currCh = CST_GET_CHAR( _cst[mid] ) ;
         if ( currCh == ch )
         {
            index = mid ;
            break ;
         }
         else if ( currCh < ch )
         {
            low = mid + 1 ;
         }
         else
         {
            high = mid - 1 ;
         }
      }

      return index ;
   }

   OSS_INLINE LZW_CODE _utilLZWDictionary::findStrExt( const BYTE *str,
                                                       UINT32 &length )
   {
      INT32 low = 0 ;
      INT32 high = 0 ;
      UINT32 currIdx = str[0] ;
      INT32 nextIdx = 0 ;
      UINT32 curPos = 1 ;
      UINT32 matchLen = 1 ;
      UINT32 i = 0 ;
      UINT32 upBound = _head->_maxValidCode - 1 ;

      while ( currIdx < upBound && curPos < length )
      {
         /* Get the begin and end indexes of the next level. */
         low = CST_GET_CHILD( _cst[currIdx] ) ;
         if ( CST_INVALID_CHILD == low )
         {
            /* If no child, this is the leaf. */
            break ;
         }

         i = 1 ;
         high = CST_INVALID_CHILD ;
         while ( currIdx + i < _head->_maxValidCode
                 && ( CST_INVALID_CHILD ==
                      ( high = CST_GET_CHILD( _cst[currIdx + i] ) ) ) )
         {
            ++i ;
         }

         if ( CST_INVALID_CHILD == high )
         {
            high = _head->_maxValidCode ;  /* Reached the end. */
         }
         else
         {
            high-- ;
         }

         nextIdx = _cstBinSearch( low, high, str[curPos] ) ;
         if ( CST_INVALID_CHILD == nextIdx )
         {
            break ;
         }

         currIdx = nextIdx ;
         matchLen++ ;
         curPos++ ;
      }

      length = matchLen ;
      SDB_ASSERT( _codeMap[currIdx] <= _head->_maxValidCode,
                  "Code out of range" ) ;
      return _codeMap[currIdx] ;
   }

   OSS_INLINE UINT32 _utilLZWDictionary::_findStrIdx( const BYTE *str,
                                                      UINT32 &length )
   {
      INT32 low = 0 ;
      INT32 high = 0 ;
      UINT32 currIdx = str[0] ;
      INT32 nextIdx = 0 ;
      UINT32 curPos = 1 ;
      UINT32 matchLen = 1 ;
      UINT32 i = 0 ;
      UINT32 upBound = _head->_maxCode - 1 ;

      while ( currIdx < upBound && curPos < length )
      {
         /* Get the begin and end indexes of the next level. */
         low = CST_GET_CHILD( _cst[currIdx] ) ;
         if ( CST_INVALID_CHILD == low )
         {
            /* If no child, this is the leaf. */
            break ;
         }

         i = 1 ;
         high = CST_INVALID_CHILD ;
         while ( currIdx + i < _head->_maxCode
                 && ( CST_INVALID_CHILD ==
                      ( high = CST_GET_CHILD( _cst[currIdx + i] ) ) ) )
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

         nextIdx = _cstBinSearch( low, high, str[curPos] ) ;
         if ( CST_INVALID_CHILD == nextIdx )
         {
            break ;
         }

         currIdx = nextIdx ;
         matchLen++ ;
         curPos++ ;
      }

      length = matchLen ;
      SDB_ASSERT( currIdx <= _head->_maxCode, "Code out of range" ) ;
      return currIdx ;
   }

   OSS_INLINE UINT32 _utilLZWDictionary::getStrExt( LZW_CODE code, BYTE *buff,
                                                    UINT32 buffSize )
   {
      UINT32 len = 0 ;

      if ( DST_IS_REMOTE( _dst[code] ) )
      {
         len = _dstGetRemoteStr( _dst[code], (CHAR*)buff ) ;
      }
      else
      {
         len = DST_GET_LOCAL_LEN( _dst[code] ) ;
         SDB_ASSERT( buffSize >= len, "buffer too small" ) ;
         DST_GET_LOCAL_STR( _dst[code], buff ) ;
      }

      return len ;
   }

   OSS_INLINE void _utilLZWDictionary::attach( const void *dictionary )
   {
      UINT32 pos = 0 ;
      UINT32 itemNum = 0 ;

      _head = ( utilLZWDictHead *)dictionary ;
      SDB_ASSERT( UTIL_DICT_LZW == _head->_basic._type,
                  "Dictionary type invalid" ) ;
      SDB_ASSERT( UTIL_LZW_DICT_VERSION == _head->_basic._version,
                  "Dictionary version invalid" ) ;

      itemNum = _head->_maxValidCode + 1 ;
      pos += sizeof( utilLZWDictHead ) ;
      _cst = ( CST_ITEM *)( (const CHAR*)dictionary + pos ) ;
      pos += sizeof( CST_ITEM ) * itemNum ;
      _codeMap = ( LZW_CODE * )( (const CHAR*)dictionary + pos ) ;
      pos += sizeof( LZW_CODE ) * itemNum ;
      _dst = ( DST_ITEM* )( (const CHAR*)dictionary + pos ) ;
      pos += sizeof( DST_ITEM ) * itemNum ;
      _strArea = (CHAR *)( (const CHAR*)dictionary + pos ) ;
   }

   OSS_INLINE UINT32 _utilLZWDictionary::_dstGetRemoteStr( DST_ITEM item,
                                                           CHAR *buff )
   {
      UINT32 pos ;
      LZW_CODE preCode ;
      UINT32 finalLen = 0 ;
      UINT32 offset = DST_GET_REMOTE_POS( item ) ;
      UINT32 len = DST_GET_REMOTE_LEN( item ) ;

      if ( 0 == ( item & DST_LINK_FLAG ) )
      {
         finalLen = len ;
         ossMemcpy( buff, _strArea + offset, finalLen ) ;
      }
      else
      {
         CHAR tmpBuff[256] = { 0 } ;
         pos = 256 ;

         do
         {
            pos = pos - len + 3 ;
            ossMemcpy( &tmpBuff[pos], _strArea + offset + 3, len - 3 ) ;
            preCode = (((UINT32)(BYTE)_strArea[offset]) << 16)
                      | (((UINT32)(BYTE)_strArea[offset + 1]) << 8)
                      | ((UINT32)(BYTE)_strArea[offset + 2]) ;
            offset = DST_GET_REMOTE_POS( _dst[preCode] ) ;
            len = DST_GET_REMOTE_LEN( _dst[preCode] ) ;
            if ( 0 == ( _dst[preCode] & DST_LINK_FLAG ) )
            {
               /* No pre link, copy and break */
               pos = pos - len ;
               ossMemcpy( &tmpBuff[pos], _strArea + offset, len ) ;
               break ;
            }
         } while ( TRUE ) ;

         finalLen = 256 - pos ;
         ossMemcpy( buff, &tmpBuff[pos], finalLen ) ;
      }

      return finalLen ;
   }

   OSS_INLINE void _utilLZWDictionary::getVarLenInfo( LZW_CODE code,
                                                      UINT8 &lenIdx,
                                                      UINT8 &splitSize )
   {
      for ( UINT8 i = 0; i < UTIL_MAX_DICT_SPLIT_NUM; ++i )
      {
         if ( code < ( LZW_CODE )DICT_BITS_UPBOUND( _head->_splitInfo[ i ] ) )
         {
            lenIdx = i ;
            splitSize = _head->_splitInfo[ i ] ;
            break ;
         }
      }
   }
}

#endif /* UTIL_LZWDICTIONARY */

