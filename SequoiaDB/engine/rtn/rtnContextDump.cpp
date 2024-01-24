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

   Source File Name = rtnContextDump.cpp

   Descriptive Name = RunTime Dump Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/26/2017  David Li  Split from rtnContext.cpp

   Last Changed =

*******************************************************************************/
#include "rtnContextDump.hpp"
#include "monDump.hpp"

namespace engine
{
   /*
    *  _rtnContextDump implement
    */

   RTN_CTX_AUTO_REGISTER(_rtnContextDump, RTN_CONTEXT_DUMP, "DUMP")

   _rtnContextDump::_rtnContextDump( INT64 contextID, UINT64 eduID )
   : _rtnContextBase( contextID, eduID ),
     _mthMatchTreeHolder()
   {
      _numToReturn = -1 ;
      _numToSkip   = 0 ;
      _pFetch      = NULL ;
      _ownnedFetch = FALSE ;
   }

   _rtnContextDump::~_rtnContextDump()
   {
      if ( _pFetch && _ownnedFetch )
      {
         SDB_OSS_DEL _pFetch ;
         _pFetch = NULL ;
      }
      _ownnedFetch = FALSE ;
   }

   void _rtnContextDump::setMonFetch( rtnFetchBase *pFetch,
                                      BOOLEAN ownned )
   {
      if ( _pFetch && _ownnedFetch )
      {
         SDB_OSS_DEL _pFetch ;
      }
      _pFetch = pFetch ;
      _ownnedFetch = ownned ;
   }

   void _rtnContextDump::setMonProcessor( IRtnMonProcessorPtr monProcessorPtr )
   {
      _monProcessorPtr = monProcessorPtr ;
   }

   const CHAR* _rtnContextDump::name() const
   {
      return "DUMP" ;
   }

   RTN_CONTEXT_TYPE _rtnContextDump::getType () const
   {
      return RTN_CONTEXT_DUMP ;
   }

   INT32 _rtnContextDump::open ( const BSONObj &selector,
                                 const BSONObj &matcher,
                                 INT64 numToReturn,
                                 INT64 numToSkip )
   {
      INT32 rc = SDB_OK ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }

      if ( !selector.isEmpty() )
      {
         try
         {
            rc = _selector.loadPattern ( selector ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed loading pattern for selector: %s: %s",
                     selector.toString().c_str(), e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed loading selector: %s, rc = %d",
                     selector.toString().c_str(), rc ) ;
            goto error ;
         }
      }
      if ( !matcher.isEmpty() )
      {
         try
         {
            rc = createMatchTree() ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to create new matcher, rc: %d", rc ) ;
               goto error ;
            }
            rc = getMatchTree()->loadPattern ( matcher ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed loading pattern for matcher: %s: %s",
                     matcher.toString().c_str(), e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed loading matcher: %s, rc = %d",
                     matcher.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      _numToReturn = numToReturn ;
      _numToSkip   = numToSkip > 0 ? numToSkip : 0 ;

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextDump::monAppend( const BSONObj & result )
   {
      INT32 rc = SDB_OK ;
      BSONObj tempObj ;
      BOOLEAN isMatch = TRUE ;

      if ( 0 == _numToReturn )
      {
         rc = SDB_DMS_EOC ;
         goto done ;
      }

      try
      {
         // let's see if it's what we want
         if ( getMatchTree() && getMatchTree()->isInitialized() )
         {
            rc = getMatchTree()->matches ( result, isMatch ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to match record, rc: %d", rc ) ;
         }
         // if it matches
         if ( isMatch )
         {
            if ( _numToSkip > 0 )
            {
               --_numToSkip ;
               goto done ;
            }

            // if we don't want all fields, let's select the interested fields
            if ( _selector.isInitialized() )
            {
               rc = _selector.select ( result, tempObj ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build select record, "
                            "rc: %d", rc ) ;
            }
            else
            {
               tempObj = result ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to match or select from object: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( isMatch )
      {
         rc = append ( tempObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to append to context, rc = %d",
                      rc ) ;

         if ( _numToReturn > 0 )
         {
            --_numToReturn ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   void _rtnContextDump::_onDataEmpty ()
   {
      if ( !_hitEnd &&
           isEmpty() &&
           ( 0 == _numToReturn ||
             NULL == _pFetch ||
             _pFetch->isHitEnd() ) )
      {
         _hitEnd = TRUE ;
      }
   }

   INT32 _rtnContextDump::_prepareData( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasOut = FALSE ;
      IRtnMonProcessor *pMonProcessor = _monProcessorPtr.get() ;

      if ( !_pFetch || _pFetch->isHitEnd() )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      while( isEmpty() )
      {
         INT32 index = 0 ;
         BSONObj obj ;

         if ( cb->isInterrupted() )
         {
            rc = SDB_INTERRUPT ;
            goto error ;
         }

         for ( ; index < RTN_CONTEXT_GETNUM_ONCE ; ++index )
         {
            if ( _pFetch->isHitEnd() || 0 == getNumToReturn() )
            {
               rc = SDB_DMS_EOC ;
               _hitEnd = TRUE ;
               break ;
            }
            rc = _pFetch->fetch( obj ) ;
            if ( rc )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  if ( ! pMonProcessor )
                  {
                     goto error ;
                  }
               }
               else
               {
                  PD_LOG( PDERROR, "MonFetch[%s] fetch object failed, rc: %d",
                          _pFetch->getName(), rc ) ;
                  goto error ;
               }
            }

            if ( pMonProcessor )
            {
               if ( SDB_OK == rc )
               {
                  rc = pMonProcessor->pushIn( obj ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Push obj[%s] to processor failed, rc: %d",
                             obj.toString().c_str(), rc ) ;
                     goto error ;
                  }
               }

               do
               {
                  rc = pMonProcessor->output( obj, hasOut ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Get output from processor failed, "
                             "rc: %d", rc ) ;
                     goto error ;
                  }
                  if ( hasOut )
                  {
                     rc = monAppend( obj ) ;
                     if ( rc )
                     {
                        PD_LOG( PDERROR, "Append obj[%s] to context failed, "
                                "rc: %d", obj.toString().c_str(), rc ) ;
                        goto error ;
                     }
                  }
                  else if ( _pFetch->isHitEnd() )
                  {
                     rc = pMonProcessor->done( hasOut ) ;
                     if ( rc )
                     {
                        PD_LOG( PDERROR, "Done processor failed, rc: %d",
                                rc ) ;
                        goto error ;
                     }
                  }
               } while( hasOut && !pMonProcessor->eof() ) ;
            }
            else
            {
               /// add to context
               rc = monAppend( obj ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Append obj[%s] to context failed, rc: %d",
                          obj.toString().c_str(), rc ) ;
                  goto error ;
               }
            }
         } /// end for

         if ( SDB_DMS_EOC == rc )
         {
            break ;
         }
      }

      if ( !isEmpty() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextDump::_toString( stringstream &ss )
   {
      if ( NULL != getMatchTree() &&
           !getMatchTree()->getMatchPattern().isEmpty() )
      {
         ss << ",Matcher:" << getMatchTree()->getMatchPattern().toString() ;
      }
      if ( _numToReturn > 0 )
      {
         ss << ",NumToReturn:" << _numToReturn ;
      }
      if ( _numToSkip > 0 )
      {
         ss << ",NumToSkip:" << _numToSkip ;
      }
   }
}

