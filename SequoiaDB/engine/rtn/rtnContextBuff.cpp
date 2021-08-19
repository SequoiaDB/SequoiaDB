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

   Source File Name = rtnContextBuff.cpp

   Descriptive Name = Runtime Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime Context helper
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnContextBuff.hpp"
#include "rtnContext.hpp"

using namespace bson;

namespace engine
{

   /*
      _rtnObjBuff implement
   */
   _rtnObjBuff::_rtnObjBuff ( const _rtnObjBuff &right )
   {
      _owned = FALSE ;
      _pBuff = NULL ;
      this->operator=( right ) ;
   }

   _rtnObjBuff::~_rtnObjBuff ()
   {
      release() ;
   }

   _rtnObjBuff& _rtnObjBuff::operator=( const _rtnObjBuff &right )
   {
      if ( _pBuff && _owned )
      {
         CHAR *pTmp = ( CHAR* )_pBuff ;
         SDB_THREAD_FREE( pTmp ) ;
      }
      _owned = FALSE ;
      _pBuff = right._pBuff ;
      _buffSize = right._buffSize ;
      _recordNum = right._recordNum ;
      _curOffset = right._curOffset ;

      return *this ;
   }

   INT32 _rtnObjBuff::truncate( UINT32 num )
   {
      INT32 rc          = SDB_OK ;
      INT32 offset      = 0 ;
      INT32 recordNum   = 0 ;

      if ( num >= (UINT32)_recordNum )
      {
         goto done ;
      }

      while( ossAlign4( (UINT32)offset ) < (UINT32)_buffSize &&
             (UINT32)recordNum < num )
      {
         offset = ossAlign4( (UINT32)offset ) ;
         try
         {
            BSONObj objTemp( &_pBuff[ offset ] ) ;
            offset += objTemp.objsize() ;
            ++recordNum ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Failed to create bson object: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      if ( offset < _buffSize )
      {
         _buffSize = offset ;
      }

      if ( offset < _curOffset )
      {
         _curOffset = offset ;
      }

      if ( recordNum < _recordNum )
      {
         _recordNum = recordNum ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnObjBuff::getOwned()
   {
      INT32 rc = SDB_OK ;

      if ( !_owned && _pBuff )
      {
         CHAR *pBuff = ( CHAR* )SDB_THREAD_ALLOC( _buffSize ) ;
         if ( pBuff )
         {
            // copy buf data to own
            ossMemcpy( pBuff, _pBuff, _buffSize ) ;
            _pBuff = pBuff ;
            _owned = TRUE ;
         }
         else
         {
            rc = SDB_OOM ;
         }
      }
      return rc ;
   }

   void _rtnObjBuff::release()
   {
      if ( _pBuff && _owned )
      {
         SDB_THREAD_FREE( _pBuff ) ;
      }
      _owned = FALSE ;
      _pBuff = NULL ;
      _buffSize = 0 ;
      _recordNum = 0 ;
      _curOffset = 0 ;
   }

   /*
      _rtnContextBuf implement
   */
   _rtnContextBuf::_rtnContextBuf()
   :_rtnObjBuff( NULL, 0, 0 )
   {
      _pBuffCounter  = NULL ;
      _pBuffLock     = NULL ;
      _released      = TRUE ;
      _pOrgBuff      = NULL ;
      _startFrom     = 0 ;
   }

   _rtnContextBuf::_rtnContextBuf( const _rtnContextBuf &right )
   :_rtnObjBuff( right )
   {
      _pBuffCounter = right._pBuffCounter ;
      _pBuffLock    = right._pBuffLock ;
      _released     = right._released ;
      _object       = right._object ;
      _pOrgBuff     = right._pOrgBuff ;
      _startFrom    = right._startFrom ;

      if ( !_released )
      {
         ++(*_pBuffCounter) ;
      }
   }

   _rtnContextBuf::_rtnContextBuf( const CHAR *pBuff, INT32 buffLen,
                                   INT32 recordNum )
   :_rtnObjBuff( pBuff, buffLen, recordNum )
   {
      _pBuffCounter  = NULL ;
      _pBuffLock     = NULL ;
      _released      = TRUE ;
      _pOrgBuff      = NULL ;
      _startFrom     = 0 ;
   }

   _rtnContextBuf::_rtnContextBuf( const BSONObj &obj )
   :_rtnObjBuff( obj.objdata(), obj.objsize(), 1 )
   {
      _pBuffCounter  = NULL ;
      _pBuffLock     = NULL ;
      _released      = TRUE ;
      _object        = obj ;
      _pOrgBuff      = NULL ;
      _startFrom     = 0 ;
   }

   _rtnContextBuf::~_rtnContextBuf()
   {
      release () ;
   }

   INT32 _rtnContextBuf::getOwned()
   {
      INT32 rc = SDB_OK ;

      if ( !_object.isEmpty() )
      {
         _object     = _object.getOwned() ;
         _pBuff      = _object.objdata() ;
         _buffSize   = _object.objsize() ;
         _recordNum  = 1 ;
      }
      else if ( !_pBuffCounter )
      {
         rc = _rtnObjBuff::getOwned() ;
      }
      return rc ;
   }

   _rtnContextBuf& _rtnContextBuf::operator=( const _rtnContextBuf &right )
   {
      // release cur
      release () ;

      _rtnObjBuff::operator=( right ) ;

      _pBuffCounter = right._pBuffCounter ;
      _pBuffLock = right._pBuffLock ;
      _released = right._released ;
      _object   = right._object ;
      _pOrgBuff = right._pOrgBuff ;
      _startFrom = right._startFrom ;

      // increase counter
      if ( !_released && NULL != _pBuffCounter )
      {
         ++(*_pBuffCounter) ;
      }

      return *this ;
   }

   void _rtnContextBuf::release()
   {
      if ( !_released )
      {
         SDB_ASSERT( *_pBuffCounter > 0, "Counter must > 0" ) ;
         --(*_pBuffCounter) ;
         if ( 0 == *_pBuffCounter  )
         {
            if ( *RTN_GET_CONTEXT_FLAG( _pOrgBuff ) == 0 )
            {
               CHAR *pRealPtr = RTN_BUFF_TO_REAL_PTR( _pOrgBuff ) ;
               SDB_THREAD_FREE( pRealPtr ) ;
            }
            else
            {
               _pBuffLock->release_r() ;
            }
         }

         _pBuffCounter  = NULL ;
         _pBuffLock     = NULL ;
         _released      = TRUE ;
         _pOrgBuff      = NULL ;
      }

      _startFrom = 0 ;

      _rtnObjBuff::release() ;
   }

   void _rtnContextBuf::_reference( INT32 * pCounter, ossRWMutex *pMutex )
   {
      _pBuffCounter = pCounter ;
      _pBuffLock = pMutex ;

      ++(*_pBuffCounter) ;
      _released  = FALSE ;
   }

}

