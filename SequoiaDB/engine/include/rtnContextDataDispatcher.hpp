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

   Source File Name = rtnContextDataDispatcher.hpp

   Descriptive Name = RunTime Context Data Dispatcher Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context to dispatch data.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/14/2017  HGM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNCONTEXTDATADISPATCHER_HPP_
#define RTNCONTEXTDATADISPATCHER_HPP_

#include "rtnContext.hpp"

namespace engine
{

   /*
      _IRtnCtxDataProcessor define
    */
   enum RTN_CTX_DATA_PROCESSOR_TYPE
   {
      RTN_CTX_PROCESSOR_NONE     = -200,
      RTN_CTX_PROCESSOR_BEGIN    = -200,
      RTN_CTX_EXPLAIN_PROCESSOR  = -199,
      RTN_CTX_PROCESSOR_END      = -100,
   } ;

   class _IRtnCtxDataProcessor
   {
      public :
         _IRtnCtxDataProcessor () ;

         virtual ~_IRtnCtxDataProcessor () ;

         virtual RTN_CTX_DATA_PROCESSOR_TYPE getProcessType () const = 0 ;

         virtual INT32 processData ( INT64 dataID, const CHAR * data,
                                     INT32 dataSize, INT32 dataNum ) = 0 ;

         OSS_INLINE virtual INT32 checkData ( INT64 dataID, const CHAR * data,
                                              INT32 dataSize, INT32 dataNum )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 checkSubContext ( INT64 dataID )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual BOOLEAN needCheckData () const
         {
            return FALSE ;
         }

         OSS_INLINE virtual BOOLEAN needCheckSubContext () const
         {
            return FALSE ;
         }
   } ;

   typedef ossPoolList< _IRtnCtxDataProcessor * > rtnCtxDataProcessorList ;

   /*
      _rtnCtxDataDispatcher define
    */
   class _rtnCtxDataDispatcher
   {
      public :
         _rtnCtxDataDispatcher () ;
         ~_rtnCtxDataDispatcher () ;

         void registerProcessor ( _IRtnCtxDataProcessor * processor ) ;
         void unregisterProcessor ( _IRtnCtxDataProcessor * processor ) ;
         void unregisterAllProcessors () ;

         OSS_INLINE BOOLEAN hasProcessor () const
         {
            return _hasProcessor ;
         }

         OSS_INLINE BOOLEAN needCheckData () const
         {
            return _needCheckData ;
         }

         OSS_INLINE BOOLEAN needCheckSubContext () const
         {
            return _needCheckSubContext ;
         }

      protected :
         INT32 _processData ( INT64 processorType,
                              INT64 dataID,
                              const CHAR * data,
                              INT32 dataSize,
                              INT32 dataNum ) ;

         INT32 _checkSubContext ( INT64 dataID ) ;

         void _clearFlags () ;

      protected :
         BOOLEAN                 _hasProcessor ;
         BOOLEAN                 _needCheckData ;
         BOOLEAN                 _needCheckSubContext ;
         rtnCtxDataProcessorList _processors ;
   } ;

}

#endif //RTNCONTEXTDATADISPATCHER_HPP_
