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

   Source File Name = ossEDU.hpp

   Descriptive Name = Operating System Services Engine Dispatchable Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains structure for EDU
   signal/exception handlings.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_EDU_HPP
#define OSS_EDU_HPP
#include "core.hpp"
#include "oss.hpp"
#include "ossSignal.hpp"

namespace engine
{
#if defined(_LINUX)
   void ossStackTrace( OSS_HANDPARMS, const CHAR * dumpDir ) ;
   void ossEDUCodeTrapHandler( OSS_HANDPARMS ) ;
#elif defined(_WINDOWS)
   void ossStackTrace( LPEXCEPTION_POINTERS lpEP, const CHAR * dumpDir ) ;
   SINT32 ossEDUExceptionFilter( LPEXCEPTION_POINTERS lpEP ) ;
#endif
   void ossSetTrapExceptionPath ( const CHAR *path ) ;
   const CHAR* ossGetTrapExceptionPath () ;

   //
   // class oss_edu_data
   //    edu specific data for oss component
   //
   struct _oss_edu_data
   {
   public :
   #define OSS_EDU_DATA_EYE_CATCHER ( 0xBEEFC0FFEE00 )
      UINT64 ossEDUEyeCatcher1 ;
      UINT64 ossEDUFlag ;
      SINT32 _depth ;
      SINT32 _nestedDepth ;
   #if defined (_LINUX)
      OSS_SIGFUNCPTR ossEDUNestedSignalHandler ;
      // This long jump buffer is used to handle nested signal
      // and it is used by ossEDUNestedSignalHandler
   #endif
   #if defined (_LINUX)
      sigjmp_buf ossNestedSignalHanderJmpBuf ;
   #elif defined (_WINDOWS)
      jmp_buf ossNestedSignalHanderJmpBuf ;
   #endif
      UINT64 ossEDUEyeCatcher2 ;

      void init()
      {
         ossEDUEyeCatcher1 = OSS_EDU_DATA_EYE_CATCHER ;
         ossEDUFlag = 0 ;
         _depth = 0 ;
         _nestedDepth = 0 ;
      #if defined (_LINUX)
         ossEDUNestedSignalHandler = 0 ;
      #endif
         ossEDUEyeCatcher2 = OSS_EDU_DATA_EYE_CATCHER ;
      }
   } ;
   typedef _oss_edu_data oss_edu_data ;

   oss_edu_data* ossGetThreadEDUData() ;

   #define OSS_ENTER_SIGNAL_HANDLER( p )                                     \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_depth++ ;                                                     \
      }                                                                      \
   }

   #define OSS_LEAVE_SIGNAL_HANDLER( p )                                     \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_depth-- ;                                                     \
      }                                                                      \
   }

   #define OSS_AM_I_INSIDE_SIGNAL_HANDLER( p )                               \
   ( ( NULL != (p) ) && ( (p)->_depth > 0 ) )

   #define OSS_INVOKE_NESTED_SIGNAL_HANDLER( p )                             \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_nestedDepth++ ;                                               \
      }                                                                      \
   }

   #define OSS_LEAVE_NESTED_SIGNAL_HANDLER( p )                              \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_nestedDepth-- ;                                               \
      }                                                                      \
   }

   #define OSS_AM_I_HANDLING_NESTED_SIGNAL( p )                              \
   ( ( NULL != (p) ) && ( (p)->_nestedDepth > 0 ) )

   #define OSS_SET_NESTED_HANDLER_LEVEL( p, l )                              \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_nestedDepth = l ;                                             \
      }                                                                      \
   }

   #define OSS_CLEAR_NESTED_HANDLER_LEVEL( p )                               \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_nestedDepth = 0 ;                                             \
      }                                                                      \
   }

   #define OSS_GET_NESTED_HANDLER_LEVEL( p, l )                              \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         l = (p)->_nestedDepth  ;                                            \
      }                                                                      \
   }
}
#if defined (_LINUX)
#define OSS_MAX_SIGAL         (_NSIG-1)

/*
   ossSigSet define
*/
class _ossSigSet : public SDBObject
{
public:
   _ossSigSet () ;
   ~_ossSigSet () ;

   void emptySet () ;
   void fillSet () ;
   void sigDel ( INT32 sigNum ) ;
   void sigAdd ( INT32 sigNum ) ;
   BOOLEAN isMember ( INT32 sigNum ) ;

private:
   INT32       _sigArray[OSS_MAX_SIGAL+1] ;

};
typedef _ossSigSet ossSigSet ;
typedef void (*SIG_HANDLE)( INT32 sigNum ) ;

INT32 ossRegisterSignalHandle ( ossSigSet &sigSet, SIG_HANDLE handle ) ;

#endif //_LINUX


#endif
