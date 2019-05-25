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

   Source File Name = dmsStorageJob.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/10/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_STORAGE_JOB_HPP__
#define DMS_STORAGE_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"
#include "monDMS.hpp"
#include "ossLatch.hpp"
#include "ossEvent.hpp"

namespace engine
{

   class _dmsStorageBase ;
   class _dmsStorageUnit ;
   class _dmsMBContext ;
   class _dmsPageMap ;

   #define DMS_EXTEND_JOB_NAME_LEN           ( 200 )
   /*
      _dmsExtendSegmentJob define
   */
   class _dmsExtendSegmentJob : public _rtnBaseJob
   {
      public:
         _dmsExtendSegmentJob ( _dmsStorageBase *pSUBase ) ;
         virtual ~_dmsExtendSegmentJob () ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN reuseEDU() const { return TRUE ; }

      private:
         _dmsStorageBase            *_pSUBase ;
         CHAR                       _name[ DMS_EXTEND_JOB_NAME_LEN + 1 ] ;

   } ;
   typedef _dmsExtendSegmentJob  dmsExtendSegmentJob ;

   /*
      _dmsPageMappingDispatcher define
   */
   class _dmsPageMappingDispatcher : public SDBObject
   {
      public:
         _dmsPageMappingDispatcher() ;
         ~_dmsPageMappingDispatcher() ;

         INT32       active() ;

         BOOLEAN     dispatchItem( monCSName &item ) ;
         void        endDispatch() ;
         UINT32      prepare() ;

         ossEvent*   getEmptyEvent() ;
         ossEvent*   getNtyEvent() ;

         void        exitJob( BOOLEAN isControl ) ;

      protected:
         void        _checkAndStartJob( BOOLEAN needLock ) ;

      private:
         MON_CSNAME_VEC       _vecCSName ;
         ossSpinXLatch        _latch ;
         ossEvent             _ntyEvent ;
         ossAutoEvent         _emptyEvent ;

         BOOLEAN              _startCtrlJob ;
         UINT32               _curAgent ;
         UINT32               _idleAgent ;
   } ;
   typedef _dmsPageMappingDispatcher dmsPageMappingDispatcher ;

   /*
      _dmsPageMappingJob define
   */
   class _dmsPageMappingJob : public _rtnBaseJob
   {
      public:
         _dmsPageMappingJob( dmsPageMappingDispatcher *pDispatcher,
                             INT32 timeout = -1 ) ;
         virtual ~_dmsPageMappingJob() ;

         BOOLEAN isControlJob() const ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

      protected:
         void           _doUnit( const monCSName *pItem ) ;
         void           _doACollection( _dmsStorageUnit *su,
                                        _dmsMBContext *mbContext,
                                        _dmsPageMap *pPageMap ) ;

      private:
         dmsPageMappingDispatcher  *_pDispatcher ;
         INT32                      _timeout ;

   } ;
   typedef _dmsPageMappingJob dmsPageMappingJob ;

   /*
      Function define
   */
   INT32 startExtendSegmentJob ( EDUID *pEDUID, _dmsStorageBase *pSUBase ) ;

   INT32 dmsStartMappingJob( EDUID *pEDUID,
                             dmsPageMappingDispatcher *pDispatcher,
                             INT32 timeout ) ;

}

#endif //DMS_STORAGE_JOB_HPP__

