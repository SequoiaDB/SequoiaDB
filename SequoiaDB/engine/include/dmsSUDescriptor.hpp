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

   Source File Name = dmsSUDescriptor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_SU_DESCRIPTOR_HPP_
#define SDB_DMS_SU_DESCRIPTOR_HPP_

#include "dms.hpp"
#include "dmsExtDataHandler.hpp"
#include "utilUniqueID.hpp"

namespace engine
{

   #define DMS_SU_NAME_SZ DMS_COLLECTION_SPACE_NAME_SZ

   /*
      _dmsStorageInfo defined
    */
   struct _dmsStorageInfo
   {
      UINT32      _pageSize ;
      CHAR        _suName [ DMS_SU_NAME_SZ + 1 ] ; // storage unit file name is
                                                   // foo.0 / foo.1, where foo
                                                   // is suName, and 0/1 are
                                                   // _sequence
      UINT32      _sequence ;
      UINT64      _secretValue ;
      UINT32      _lobdPageSize ;

      UINT32      _overflowRatio ;
      UINT32      _extentThreshold ;

      BOOLEAN     _enableSparse ;
      BOOLEAN     _directIO ;
      UINT32      _cacheMergeSize ;
      UINT32      _pageAllocTimeout ;

      /// Data is OK
      BOOLEAN     _dataIsOK ;
      UINT64      _curLSNOnStart ;

      DMS_STORAGE_TYPE _type ;
      IDmsExtDataHandler *_extDataHandler ;

      utilCSUniqueID _csUniqueID ;

      UINT64      _createTime ;
      UINT64      _updateTime ;

      _dmsStorageInfo ()
      {
         _pageSize      = DMS_PAGE_SIZE_DFT ;
         ossMemset( _suName, 0, sizeof( _suName ) ) ;
         _sequence      = 0 ;
         _secretValue   = 0 ;
         _lobdPageSize  = DMS_DO_NOT_CREATE_LOB ;

         _overflowRatio = 0 ;
         _extentThreshold = 0 ;
         _enableSparse = FALSE ;
         _directIO = FALSE ;
         _cacheMergeSize = 0 ;
         _pageAllocTimeout = 0 ;

         _dataIsOK       = FALSE ;
         _curLSNOnStart  = ~0 ;
         _type = DMS_STORAGE_NORMAL ;
         _extDataHandler = NULL ;

         _csUniqueID     = UTIL_UNIQUEID_NULL ;

         _createTime     = 0 ;
         _updateTime     = 0 ;
      }
   } ;
   typedef struct _dmsStorageInfo dmsStorageInfo ;

   /*
      _dmsSUDescriptor define
    */
   class _dmsSUDescriptor : public SDBObject
   {
   public:
      _dmsSUDescriptor( const CHAR *pSUName,
                        UINT32 csUniqueID,
                        UINT32 sequence,
                        INT32 pageSize,
                        INT32 lobPageSize,
                        DMS_STORAGE_TYPE type,
                        UINT32 overflowRatio,
                        UINT32 extentThreshold,
                        BOOLEAN enableSparse,
                        BOOLEAN directIO,
                        UINT32 cacheMergeSize,
                        UINT32 pageAllocTimeout,
                        BOOLEAN dataIsOK,
                        UINT64 curLSNOnStart,
                        IDmsExtDataHandler *extDataHandler )
      {
         SDB_ASSERT( pSUName, "name can't be null" ) ;

         if ( 0 == pageSize )
         {
            pageSize = DMS_PAGE_SIZE_DFT ;
         }

         if ( 0 == lobPageSize )
         {
            lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
         }

         _storageInfo._pageSize = pageSize ;
         _storageInfo._lobdPageSize = lobPageSize ;
         ossStrncpy( _storageInfo._suName, pSUName, DMS_SU_NAME_SZ ) ;
         _storageInfo._suName[DMS_SU_NAME_SZ] = 0 ;
         _storageInfo._csUniqueID = csUniqueID ;
         _storageInfo._sequence = sequence ;
         _storageInfo._overflowRatio = overflowRatio ;
         _storageInfo._extentThreshold = extentThreshold ;
         _storageInfo._enableSparse = enableSparse ;
         _storageInfo._directIO = directIO ;
         _storageInfo._cacheMergeSize = cacheMergeSize ;
         _storageInfo._pageAllocTimeout = pageAllocTimeout ;
         _storageInfo._dataIsOK = dataIsOK ;
         _storageInfo._curLSNOnStart = curLSNOnStart ;
         // make secret value
         _storageInfo._secretValue = ossPack32To64( (UINT32)( time( NULL ) ),
                                                    (UINT32)( ossRand()*239641 ) ) ;
         _storageInfo._type = type ;
         _storageInfo._extDataHandler = extDataHandler ;
      }

      ~_dmsSUDescriptor() = default ;
      _dmsSUDescriptor( const _dmsSUDescriptor &o ) = delete ;
      _dmsSUDescriptor &operator =( const _dmsSUDescriptor & ) = delete ;

      const dmsStorageInfo &getStorageInfo() const
      {
         return _storageInfo ;
      }

      dmsStorageInfo &getStorageInfo()
      {
         return _storageInfo ;
      }

      const CHAR *getSUName() const
      {
         return _storageInfo._suName ;
      }

      utilCSUniqueID getCSUniqueID() const
      {
         return _storageInfo._csUniqueID ;
      }

      void setCSUniqueID( utilCSUniqueID csUniqueID )
      {
         _storageInfo._csUniqueID = csUniqueID ;
      }

      UINT32 getPageSize() const
      {
         return _storageInfo._pageSize ;
      }

      UINT32 getLobPageSize() const
      {
         return _storageInfo._lobdPageSize ;
      }

   protected:
      dmsStorageInfo _storageInfo ;
   } ;

   typedef class _dmsSUDescriptor dmsSUDescriptor ;

}

#endif // SDB_DMS_SU_DESCRIPTOR_HPP_