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

   Source File Name = rtn.cpp

   Descriptive Name = Runtime

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime helper functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "rtn.hpp"
#include "dms.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsScanner.hpp"
#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "pmdStartup.hpp"
#include "pmdStartupHistoryLogger.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnExtDataHandler.hpp"
#include "rtnIXScannerFactory.hpp"

namespace fs = boost::filesystem ;
namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETINTELE, "rtnGetIntElement" )
   INT32 rtnGetIntElement ( const BSONObj &obj, const CHAR *fieldName,
                            INT32 &value )
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETINTELE );
      SDB_ASSERT ( fieldName, "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be Integer",
                 obj.toString().c_str()) ;
      value = ele.numberInt() ;
   done :
      PD_TRACE_EXITRC ( SDB_RTNGETINTELE, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETSTRELE, "rtnGetStringElement" )
   INT32 rtnGetStringElement ( const BSONObj &obj, const CHAR *fieldName,
                               const CHAR **value )
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETSTRELE );
      SDB_ASSERT ( fieldName && value, "field name and value can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be String",
                 obj.toString().c_str()) ;
      *value = ele.valuestr() ;
   done :
      PD_TRACE_EXITRC ( SDB_RTNGETSTRELE, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETSTDSTRELE, "rtnGetSTDStringElement" )
   INT32 rtnGetSTDStringElement ( const BSONObj &obj, const CHAR *fieldName,
                                  string &value )
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETSTDSTRELE ) ;
      SDB_ASSERT ( fieldName, "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be String",
                 obj.toString().c_str()) ;
      value = ele.valuestr() ;
   done :
      PD_TRACE_EXITRC ( SDB_RTNGETSTDSTRELE, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETOBJELE, "rtnGetObjElement" )
   INT32 rtnGetObjElement ( const BSONObj &obj, const CHAR *fieldName,
                            BSONObj &value )
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETOBJELE );
      SDB_ASSERT ( fieldName , "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( Object == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be Object",
                 obj.toString().c_str()) ;
      value = ele.embeddedObject() ;
   done :
      PD_TRACE_EXITRC ( SDB_RTNGETOBJELE, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETARRAYELE, "rtnGetArrayElement" )
   INT32 rtnGetArrayElement ( const BSONObj &obj, const CHAR *fieldName,
                              BSONObj &value )
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETARRAYELE );
      SDB_ASSERT ( fieldName , "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( Array == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be Array",
                 obj.toString().c_str()) ;
      value = ele.embeddedObject() ;
   done :
      PD_TRACE_EXITRC ( SDB_RTNGETARRAYELE, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETBOOLELE, "rtnGetBooleanElement" )
   INT32 rtnGetBooleanElement ( const BSONObj &obj, const CHAR *fieldName,
                                BOOLEAN &value )
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETBOOLELE );
      SDB_ASSERT ( fieldName , "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( Bool == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be Bool",
                 obj.toString().c_str()) ;
      value = ele.boolean() ;
   done :
      PD_TRACE_EXITRC ( SDB_RTNGETBOOLELE, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETLONGELE, "rtnGetNumberLongElement" )
   INT32 rtnGetNumberLongElement ( const BSONObj &obj, const CHAR *fieldName,
                                   INT64 &value )
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETLONGELE );
      SDB_ASSERT ( fieldName, "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be number",
                 obj.toString().c_str()) ;
      value = ele.numberLong() ;
   done :
      PD_TRACE_EXITRC ( SDB_RTNGETLONGELE, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETDOUBLEELE, "rtnGetDoubleElement" )
   INT32 rtnGetDoubleElement ( const BSONObj &obj, const CHAR *fieldName,
                               double &value )
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETLONGELE );
      SDB_ASSERT ( fieldName, "field name can't be NULL" ) ;
      BSONElement ele = obj.getField ( fieldName ) ;
      PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                 "Can't locate field '%s': %s",
                 fieldName,
                 obj.toString().c_str() ) ;
      PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDDEBUG,
                 "Unexpected field type : %s, supposed to be number",
                 obj.toString().c_str()) ;
      value = ele.numberDouble() ;
   done :
      PD_TRACE_EXITRC ( SDB_RTNGETLONGELE, rc );
      return rc ;
   error :
      goto done ;
   }

   BSONObj rtnUniqueKeyNameObj( const BSONObj & obj )
   {
      CHAR szTmp[ 5 ] = { 0 } ;
      UINT32 i = 0 ;
      BSONObjBuilder builder ;
      BSONObjIterator it ( obj ) ;
      while ( it.more() )
      {
         ossSnprintf( szTmp, 4, "%d", i++ ) ;
         builder.appendAs( it.next(), szTmp ) ;
      }
      return builder.obj() ;
   }

   BSONObj rtnNullKeyNameObj( const BSONObj &obj )
   {
      BSONObjBuilder builder ;
      BSONObjIterator it ( obj ) ;
      while ( it.more() )
      {
         builder.appendAs( it.next(), "" ) ;
      }
      return builder.obj() ;
   }

   // reallocate buffer and check
   // upper limit for this allocation is 2GB
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNREALLOCBUFF, "rtnReallocBuffer" )
   INT32 rtnReallocBuffer ( CHAR **ppBuffer, INT32 *bufferSize,
                            INT32 newLength, INT32 alignmentSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNREALLOCBUFF );
      if ( newLength > *bufferSize )
      {
         CHAR *pOrgBuff = *ppBuffer ;
         newLength = ossRoundUpToMultipleX ( newLength, alignmentSize ) ;
         PD_CHECK ( newLength >= 0, SDB_INVALIDARG, error, PDERROR,
                    "new buffer overflow" ) ;
         *ppBuffer = (CHAR*)SDB_OSS_REALLOC ( *ppBuffer,
                                              sizeof(CHAR)*(newLength) ) ;
         if ( !*ppBuffer )
         {
            PD_LOG( PDERROR, "Failed to realloc %d bytes memory", newLength ) ;
            *ppBuffer = pOrgBuff ;
            rc = SDB_OOM ;
            goto error ;
         }
         *bufferSize = newLength ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNREALLOCBUFF, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCORRECTCS1, "rtnCorrectCollectionSpaceFile" )
   INT32 rtnCorrectCollectionSpaceFile( const CHAR *dataPath,
                                        const CHAR *indexPath,
                                        const CHAR *lobPath,
                                        const CHAR *lobMetaPath,
                                        UINT32 sequence,
                                        const utilRenameLog& renameLog )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCORRECTCS1 ) ;

      const CHAR* pathList[]    = { dataPath,
                                    indexPath,
                                    lobPath,
                                    lobMetaPath } ;
      const CHAR* extNameList[] = { DMS_DATA_SU_EXT_NAME,
                                    DMS_INDEX_SU_EXT_NAME,
                                    DMS_LOB_DATA_SU_EXT_NAME,
                                    DMS_LOB_META_SU_EXT_NAME } ;
      const CHAR* csNameList[]  = { renameLog.oldName,
                                    renameLog.newName } ;
      UINT8 pathCnt   = sizeof( pathList ) / sizeof( const CHAR* ) ;
      UINT8 csNameCnt = sizeof( csNameList ) / sizeof( const CHAR* ) ;

      SDB_ASSERT( ( sizeof(extNameList)/sizeof(const CHAR*) )== pathCnt,
                  "ext name cnt must equals to path cnt" ) ;

      for( UINT8 i = 0 ; i < pathCnt ; i++ )
      {
         const CHAR* basePath = pathList[i] ;
         const CHAR* extName = extNameList[i] ;

         for( UINT8 j = 0 ; j < csNameCnt ; j++ )
         {
            const CHAR* csName = csNameList[j] ;
            CHAR fileName[ DMS_SU_FILENAME_SZ + 1 ] = { 0 } ;
            CHAR fullFileName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
            BOOLEAN fileExist = FALSE ;

            ossSnprintf( fileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                         csName, sequence, extName ) ;

            rc = utilBuildFullPath( basePath, fileName, OSS_MAX_PATHSIZE,
                                    fullFileName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build full path by [%s] [%s], "
                         "rc: %d", basePath, fileName, rc ) ;

            rc = ossFile::exists( fullFileName, fileExist ) ;
            PD_RC_CHECK( rc, PDERROR,
                          "Failed to check existence of file[%s], rc: %d",
                          fullFileName, rc ) ;

            if ( fileExist )
            {
               rc = rtnCorrectCollectionSpaceFile( basePath, fileName,
                                                   renameLog ) ;
               PD_RC_CHECK( rc, PDERROR, "Correct cs file[%s] fail, rc: %d",
                            fullFileName, rc ) ;
               break ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCORRECTCS1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCORRECTCS2, "rtnCorrectCollectionSpaceFile" )
   INT32 rtnCorrectCollectionSpaceFile( const CHAR* pPath,
                                        const CHAR* pFileName,
                                        const utilRenameLog& renameLog )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCORRECTCS2 ) ;

      const CHAR *pDot = NULL ;
      UINT32 csnameSize = 0 ;
      CHAR newFileName[ DMS_SU_FILENAME_SZ + 1 ] = { 0 } ;
      CHAR curFullFileName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR newFullFileName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      OSSFILE file ;
      CHAR *pBuff = NULL ;
      dmsStorageUnitHeader *pHeader = NULL ;
      SINT64 readLen = 0 ;
      SINT64 writeLen = 0 ;
      BOOLEAN isOpened = FALSE ;
      BOOLEAN isChangeHeader = FALSE ;

      /// We need to correct file name and header from old cs name to new cs
      /// name.

      /// 1. get file full name
      pDot = ossStrchr ( pFileName, '.' ) ;
      ossSnprintf( newFileName, DMS_SU_FILENAME_SZ, "%s%s",
                   renameLog.newName, pDot ) ;

      rc = utilBuildFullPath( pPath, newFileName, OSS_MAX_PATHSIZE,
                              newFullFileName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build full path by [%s] [%s], "
                   "rc: %d", pPath, newFileName, rc ) ;

      rc = utilBuildFullPath( pPath, pFileName, OSS_MAX_PATHSIZE,
                              curFullFileName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build full path by [%s] [%s], "
                   "rc: %d", pPath, pFileName, rc ) ;

      /// 2. correct name field in header

      //  allocate buffer
      pBuff = ( CHAR* )SDB_THREAD_ALLOC( sizeof( dmsStorageUnitHeader ) ) ;
      if ( !pBuff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc memory[size:%d] failed, rc: %d",
                 sizeof( dmsStorageUnitHeader ), rc ) ;
         goto error ;
      }

      //  read header
      rc = ossOpen( curFullFileName, OSS_READWRITE, OSS_RU|OSS_WU|OSS_RG,
                    file ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to open file[%s], rc: %d", curFullFileName, rc ) ;
      isOpened = TRUE ;

      rc = ossSeekAndReadN( &file, 0,
                            sizeof( dmsStorageUnitHeader ), pBuff,
                            readLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read header, rc: %d", rc ) ;

      pHeader = (dmsStorageUnitHeader*)pBuff ;

      //  modify name
      if ( 0 == ossStrncmp( pHeader->_name, renameLog.oldName,
                            DMS_SU_NAME_SZ ) )
      {
         ossStrncpy( pHeader->_name, renameLog.newName, DMS_SU_NAME_SZ ) ;
         pHeader->_name[ DMS_SU_NAME_SZ ] = 0 ;
         isChangeHeader = TRUE ;
      }
      else if ( 0 == ossStrncmp( pHeader->_name, renameLog.newName,
                                 DMS_SU_NAME_SZ ) )
      {
         // it is ok, do nothing
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Name[%s] in header isn't [%s] or [%s], in file[%s]",
                 pHeader->_name, renameLog.oldName, renameLog.newName,
                 curFullFileName ) ;
         goto error ;
      }

      //  write to file
      if ( isChangeHeader )
      {
         rc = ossSeekAndWriteN( &file, 0,
                                (CHAR *)pHeader, sizeof( dmsStorageUnitHeader ),
                                writeLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write data in file[%s], rc: %d",
                      curFullFileName, rc ) ;

         ossFsync( &file ) ;
      }

      ossClose( file ) ;
      isOpened = FALSE ;

      /// 3. rename file name
      csnameSize = pDot - pFileName ;

      if ( 0 == ossStrncmp( pFileName, renameLog.oldName, csnameSize ) &&
           ossStrlen( renameLog.oldName ) == csnameSize )
      {
         rc = ossRenamePath( curFullFileName, newFullFileName ) ;
         PD_RC_CHECK( rc, PDERROR, "Rename file[%s] to [%s] failed, rc: %d",
                      curFullFileName, newFullFileName, rc ) ;
      }
      else if ( 0 == ossStrncmp( pFileName, renameLog.newName, csnameSize ) &&
                ossStrlen( renameLog.newName ) == csnameSize )
      {
         // it is ok, do nothing
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR,
                 "Collection space name in file name[%s], expect:[%s] or [%s]",
                 pFileName, renameLog.oldName, renameLog.newName ) ;
         goto error ;
      }

      PD_LOG( PDEVENT,
              "Correct cs file[%s] in path[%s] by rename log[old: %s, new: %s]",
              pFileName, pPath, renameLog.oldName, renameLog.newName ) ;

   done:
      if ( isOpened )
      {
         ossClose( file ) ;
         isOpened = FALSE ;
      }
      if ( pBuff )
      {
         SDB_THREAD_FREE( pBuff ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCORRECTCS2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // 1) file name must be <collectionspace>.<sequence>.<ext>
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNVERIFYCSFN, "rtnVerifyCollectionSpaceFileName" )
   BOOLEAN rtnVerifyCollectionSpaceFileName ( const CHAR *pFileName,
                                              CHAR *pSUName,
                                              UINT32 bufferSize,
                                              UINT32 &sequence,
                                              const CHAR *extFilter )
   {
      PD_TRACE_ENTRY ( SDB_RTNVERIFYCSFN ) ;

      BOOLEAN ret = FALSE ;
      UINT32 size = 0 ;
      const CHAR *pDot = ossStrchr ( pFileName, '.' ) ;
      const CHAR *pDotr = ossStrrchr ( pFileName, '.' ) ;

      if ( !pDot || !pDotr || pDotr - pDot <= 1 || pDot == pFileName ||
           *(pDotr + 1) == 0 || ossStrchr( pDot + 1, '.' ) != pDotr )
      {
         goto done ;
      }

      // ext check
      if ( extFilter && 0 != ossStrcmp( pDotr + 1, extFilter ) )
      {
         goto done ;
      }

      {
         sequence = 0 ;
         // check sequence
         const CHAR *pSeqPos = pDot + 1 ;
         while ( pSeqPos < pDotr )
         {
            if ( *pSeqPos >= '0' && *pSeqPos <= '9' )
            {
               sequence = 10 * sequence + ( *pSeqPos - '0' ) ;
            }
            else
            {
               goto done ;
            }
            ++pSeqPos ;
         }
      }

      size = pDot - pFileName ;
      if ( size > bufferSize )
      {
         goto done ;
      }

      // copy su name
      if ( pSUName )
      {
         ossStrncpy ( pSUName, pFileName, size ) ;
         pSUName[size] = 0 ;
      }

      ret = TRUE ;

   done :
      PD_TRACE1 ( SDB_RTNVERIFYCSFN, PD_PACK_INT(ret) );
      PD_TRACE_EXIT ( SDB_RTNVERIFYCSFN );
      return ret ;
   }

   SDB_FILE_TYPE rtnParseFileName( const CHAR *pFileName )
   {
      SDB_FILE_TYPE fileType = SDB_FILE_UNKNOW ;

      /// start up file
      if ( 0 == ossStrcmp( pFileName, PMD_STARTUP_FILE_NAME ) )
      {
         fileType = SDB_FILE_STARTUP ;
      }
      else if ( 0 == ossStrcmp( pFileName, PMD_STARTUPHST_FILE_NAME ) )
      {
         fileType = SDB_FILE_STARTUP_HST ;
      }
      else if ( 0 == ossStrcmp( pFileName, UTIL_RENAME_LOG_FILENAME ) )
      {
         fileType = SDB_FILE_RENAME_INFO ;
      }
      else
      {
         /// <csname>.<num>.<posfix>
         const CHAR *pDot = ossStrchr ( pFileName, '.' ) ;
         const CHAR *pDotr = ossStrrchr ( pFileName, '.' ) ;

         const CHAR *pSeqPos = NULL ;
         UINT32 size = 0 ;
         CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

         if ( !pDot || !pDotr || pDotr - pDot <= 1 || pDot == pFileName ||
              *(pDotr + 1) == 0 || ossStrchr( pDot + 1, '.' ) != pDotr )
         {
            goto done ;
         }

         // check sequence
         pSeqPos = pDot + 1 ;
         while ( pSeqPos < pDotr )
         {
            if ( *pSeqPos < '0' || *pSeqPos > '9' )
            {
               goto done ;
            }
            ++pSeqPos ;
         }

         // cs name check
         size = pDot - pFileName ;
         if ( size > DMS_COLLECTION_SPACE_NAME_SZ )
         {
            goto done ;
         }
         ossStrncpy ( csName, pFileName, size ) ;
         if ( SDB_OK != dmsCheckCSName( csName, TRUE ) )
         {
            goto done ;
         }

         // ext check
         if ( 0 == ossStrcmp( pDotr + 1, DMS_DATA_SU_EXT_NAME ) )
         {
            fileType = SDB_FILE_DATA ;
         }
         else if ( 0 == ossStrcmp( pDotr + 1, DMS_INDEX_SU_EXT_NAME ) )
         {
            fileType = SDB_FILE_INDEX ;
         }
         else if ( 0 == ossStrcmp( pDotr + 1, DMS_LOB_META_SU_EXT_NAME ) )
         {
            fileType = SDB_FILE_LOBM ;
         }
         else if ( 0 == ossStrcmp( pDotr + 1, DMS_LOB_DATA_SU_EXT_NAME ) )
         {
            fileType = SDB_FILE_LOBD ;
         }
      }

   done:
      return fileType ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNRESUMECLDICTCREATE, "rtnResumeClDictCreate" )
   static INT32 rtnResumeClDictCreate( const CHAR *csName, SDB_DMSCB *dmsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNRESUMECLDICTCREATE ) ;

      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsMBContext *context = NULL ;
      dmsMB *mb = NULL ;

      rc = dmsCB->nameToSUAndLock( csName, suID, &su ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get and lock collectionspace[%s], "
                 "rc: %d", csName, rc ) ;
         goto error ;
      }

      for ( UINT16 mbID = 0; mbID < DMS_MME_SLOTS; ++mbID )
      {
         // If the collection does not exist, lock will failed.
         rc = su->data()->getMBContext( &context, mbID,
                                        DMS_INVALID_CLID,
                                        DMS_INVALID_CLID,
                                        SHARED ) ;
         if ( rc )
         {
            if ( SDB_DMS_NOTEXIST == rc )
            {
               rc = SDB_OK ;
               continue ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;
               goto error ;
            }
         }

         mb = context->mb() ;

         /*
          * Three conditions should be matched to resume dictionary creating job
          * for a collection:
          * (1) 'Compressed' option is set as true
          * (2) 'CompressionType' is set as 'lzw'
          * (3) The dictionary extent id is invalid currently, which means the
          *     dictionary has not been created yet.
          *
          * The in use flag in mb is checked when taking the lock, so no need to
          * check here.
          */
         if ( OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_COMPRESSED )
              && ( UTIL_COMPRESSOR_LZW == mb->_compressorType )
              && ( DMS_INVALID_EXTENT == mb->_dictExtentID ) )
         {
            dmsCB->pushDictJob( dmsDictJob( su->CSID(), su->LogicalCSID(),
                                context->mbID(), context->clLID() ) ) ;
         }

         su->data()->releaseMBContext( context ) ;
      }

   done:
      if ( context )
      {
         su->data()->releaseMBContext( context ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
      }
      PD_TRACE_EXIT( SDB_RTNRESUMECLDICTCREATE ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnLoadCollectionSpace ( const CHAR *pCSName,
                                  const CHAR *dataPath,
                                  const CHAR *indexPath,
                                  const CHAR *lobPath,
                                  const CHAR *lobMetaPath,
                                  pmdEDUCB *cb,
                                  SDB_DMSCB *dmsCB,
                                  BOOLEAN checkOnly )
   {
      utilCSUniqueID *csUniqueIDInCata = NULL ;
      BSONObj clInfoInCata ;
      return rtnLoadCollectionSpace( pCSName, dataPath, indexPath, lobPath,
                                     lobMetaPath, cb, dmsCB, checkOnly,
                                     csUniqueIDInCata, clInfoInCata ) ;
   }

   // load a single collection name from given path
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOADCS, "rtnLoadCollectionSpace" )
   INT32 rtnLoadCollectionSpace ( const CHAR *pCSName,
                                  const CHAR *dataPath,
                                  const CHAR *indexPath,
                                  const CHAR *lobPath,
                                  const CHAR *lobMetaPath,
                                  pmdEDUCB *cb,
                                  SDB_DMSCB *dmsCB,
                                  BOOLEAN checkOnly,
                                  utilCSUniqueID *csUniqueIDInCata,
                                  const BSONObj& clInfoInCata )
   {
      SDB_ASSERT ( pCSName, "pCSName can't be NULL" ) ;
      SDB_ASSERT ( dataPath, "data path can't be NULL" ) ;
      SDB_ASSERT ( indexPath, "index path can't be NULL" ) ;
      SDB_ASSERT ( lobPath, "lob path can't be NULL" ) ;
      SDB_ASSERT ( lobMetaPath, "lob meta path can't be NULL" ) ;

      INT32 rc                                 = SDB_DMS_CS_NOTEXIST ;
      PD_TRACE_ENTRY ( SDB_RTNLOADCS );
      CHAR csName[ DMS_SU_NAME_SZ + 1 ]        = {0} ;
      UINT32 sequence                          = 0 ;
      dmsStorageUnit *storageUnit              = NULL ;
      pmdOptionsCB *optCB                      = pmdGetOptionCB() ;

      try
      {
         fs::path dbDir ( dataPath ) ;
         fs::directory_iterator end_iter ;

         if ( fs::exists(dbDir) && fs::is_directory(dbDir) )
         {
            for ( fs::directory_iterator dir_iter(dbDir);
                  dir_iter != end_iter; ++dir_iter )
            {
               if ( fs::is_regular_file( dir_iter->status() ) )
               {
                  // 1) file name must be <collectionspace>.<sequence>.<data>
                  const std::string fileName =
                     dir_iter->path().filename().string() ;
                  const CHAR *pFileName = fileName.c_str() ;

                  if ( rtnVerifyCollectionSpaceFileName( pFileName, csName,
                                                         DMS_SU_NAME_SZ,
                                                         sequence ) &&
                       ossStrncmp ( pCSName, csName, DMS_SU_NAME_SZ) == 0 )
                  {
                     PD_LOG ( PDDEBUG, "Found Filename %s in path %s",
                              pFileName, dataPath) ;

                     if ( !checkOnly )
                     {
                        storageUnit = SDB_OSS_NEW dmsStorageUnit( csName,
                                                                  UTIL_UNIQUEID_NULL,
                                                                  sequence,
                                                                  pmdGetBuffPool(),
                                                                  DMS_PAGE_SIZE_DFT,
                                                                  DMS_DEFAULT_LOB_PAGE_SZ,
                                                                  DMS_STORAGE_NORMAL,
                                                                  rtnGetExtDataHandler() ) ;
                        if ( !storageUnit )
                        {
                           PD_LOG_MSG ( PDERROR, "Failed to allocate "
                                        "dmsStorageUnit for %s",
                                        dir_iter->path().string().c_str() ) ;
                           rc = SDB_OOM ;
                           goto error ;
                        }
                        // open the storage unit without attempt to create new
                        // container, if we can't open the container, maybe
                        // it's just invalid, let's continue open other storage
                        // units without this one
                        rc = storageUnit->open ( dataPath,
                                                 indexPath,
                                                 lobPath,
                                                 lobMetaPath,
                                                 pmdGetSyncMgr(),
                                                 FALSE ) ;
                        if ( rc )
                        {
                           SDB_OSS_DEL storageUnit ;
                           storageUnit = NULL ;
                           PD_LOG ( PDWARNING, "Failed to open storage unit %s",
                                    dir_iter->path().string().c_str() ) ;
                           continue ;
                        }
                        /// set config
                        storageUnit->setSyncConfig( optCB->getSyncInterval(),
                                                    optCB->getSyncRecordNum(),
                                                    optCB->getSyncDirtyRatio() ) ;
                        storageUnit->setSyncDeep( optCB->isSyncDeep() ) ;
                        /// add collectionspace
                        rc = dmsCB->addCollectionSpace ( csName, sequence,
                                                         storageUnit, NULL,
                                                         NULL, FALSE ) ;
                        if ( rc )
                        {
                           SDB_OSS_DEL storageUnit ;
                           storageUnit = NULL ;
                           if ( SDB_DMS_CS_EXIST == rc )
                           {
                              PD_LOG ( PDWARNING, "Failed to add collection "
                                       "space[%s] because it's already exist",
                                       csName ) ;
                           }
                           else
                           {
                              PD_LOG ( PDWARNING, "Failed to add collection "
                                       "space[%s], rc = %d", csName, rc ) ;
                           }
                           continue ;
                        }

                        storageUnit = NULL ;

                        /// db.loadCS() may need to set unique id
                        if ( csUniqueIDInCata )
                        {
                           rc = dmsCB->changeUniqueID( csName,
                                                       *csUniqueIDInCata,
                                                       clInfoInCata,
                                                       cb, NULL, TRUE ) ;
                           PD_RC_CHECK( rc, PDERROR,
                                        "Failed to change unique id, rc: %d",
                                        rc ) ;
                        }

                        /*
                         * Scan all the collections, to check if any one should be
                         * put into the dictionary creating list. This should be
                         * done if the system restarted before the dictionary was
                         * created.
                         */
                        rc = rtnResumeClDictCreate( csName, dmsCB ) ;
                        PD_RC_CHECK( rc, PDERROR,
                                     "Failed to resume dictionary creating "
                                     "job for %s, rc: %d",
                                     csName, rc ) ;
                     }

                     rc = SDB_OK ;
                     PD_LOG( PDEVENT, "Load collectionspace[%s] succeed",
                             pCSName ) ;
                     goto done ;
                  } // if ( rtnVerifyCollectionSpaceFileName
               } //  if ( fs::is_regular_file(dir_iter->status()))
            } //for ( fs::directory_iterator dir_iter(dbDir)
         }
         else
         {
            PD_LOG_MSG ( PDERROR, "Given path %s is not a directory or not "
                         "exist", dataPath ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR, "Failed to iterate directory %s: %s",
                       dataPath, e.what() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNLOADCS, rc );
      return rc ;
   error :
      if ( storageUnit )
      {
         SDB_OSS_DEL storageUnit ;
      }
      goto done ;
   }

   // Load all collection spaces from database path
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOADCSS, "rtnLoadCollectionSpaces" )
   INT32 rtnLoadCollectionSpaces ( const CHAR *dataPath,
                                   const CHAR *indexPath,
                                   const CHAR *lobPath,
                                   const CHAR *lobMetaPath,
                                   SDB_DMSCB *dmsCB )
   {
      INT32 rc                                 = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNLOADCSS );
      CHAR csName [ DMS_SU_FILENAME_SZ + 1 ]   = {0} ;
      UINT32 sequence                          = 0 ;
      dmsStorageUnit *storageUnit              = NULL ;
      pmdOptionsCB *optCB                      = pmdGetOptionCB() ;

      SDB_ASSERT ( dataPath, "data path can't be NULL" ) ;
      SDB_ASSERT ( indexPath, "index path can't be NULL" ) ;
      SDB_ASSERT ( lobPath, "lob path can't be NULL" ) ;
      SDB_ASSERT ( lobMetaPath, "lob meta path can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;

      utilRenameLogger logger ;
      utilRenameLog renameLog ;
      BOOLEAN hasRenameInfo = TRUE ;

      rc = logger.init( UTIL_RENAME_LOGGER_READ ) ;
      if ( SDB_FNE == rc )
      {
         rc = SDB_OK ;
         hasRenameInfo = FALSE ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to init logger, rc: %d" , rc ) ;

      if ( hasRenameInfo )
      {
         rc = logger.load( renameLog ) ;
         if ( SDB_SYS == rc )
         {
            PD_LOG( PDWARNING, "Failed to load rename log file, rc: %d. "
                    "And delete rename log file" , rc ) ;
            logger.clear() ;
            hasRenameInfo = FALSE ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to load rename log file, rc: %d" , rc ) ;
            goto error ;
         }
      }

      try
      {
         fs::path dbDir( dataPath ) ;
         fs::directory_iterator end_iter ;
         if ( !fs::exists( dbDir ) || !fs::is_directory( dbDir ) )
         {
            PD_RC_CHECK ( SDB_INVALIDARG, PDERROR, "Given path %s is not a "
                          "directory or not exist", dataPath ) ;
         }

         // load all file
         for ( fs::directory_iterator dir_iter( dbDir );
               dir_iter != end_iter ; ++dir_iter )
         {
            if ( !fs::is_regular_file( dir_iter->status() ) )
            {
               continue ;
            }
            // 1) file name must be <collectionspace>.<sequence>.<data>
            const std::string fileName = dir_iter->path().filename().string() ;
            if ( rtnVerifyCollectionSpaceFileName( fileName.c_str(), csName,
                                                   DMS_SU_FILENAME_SZ,
                                                   sequence ) )
            {
               if ( hasRenameInfo &&
                    ( 0 == ossStrcmp( csName, renameLog.oldName ) ||
                    0 == ossStrcmp( csName, renameLog.newName ) ) )
               {
                  rc = rtnCorrectCollectionSpaceFile( dataPath, indexPath,
                                                      lobPath, lobMetaPath,
                                                      sequence, renameLog ) ;
                  PD_RC_CHECK( rc, PDERROR,
                               "Correct cs file failed, rc: %d", rc ) ;

                  if ( 0 == ossStrcmp( csName, renameLog.oldName ) )
                  {
                     ossStrcpy( csName, renameLog.newName );
                  }

                  rc = logger.clear() ;
                  if ( rc )
                  {
                     PD_LOG( PDWARNING,
                             "Failed to clear rename log, rc: %d" , rc ) ;
                  }
                  hasRenameInfo = FALSE ;
               }

               /// skip SYSTEMP file
               if ( 0 == ossStrcmp( csName, SDB_DMSTEMP_NAME ) )
               {
                  continue ;
               }

               storageUnit = SDB_OSS_NEW dmsStorageUnit ( csName,
                                                          UTIL_UNIQUEID_NULL,
                                                          sequence,
                                                          pmdGetBuffPool(),
                                                          DMS_PAGE_SIZE_DFT,
                                                          DMS_DEFAULT_LOB_PAGE_SZ,
                                                          DMS_STORAGE_NORMAL,
                                                          rtnGetExtDataHandler()) ;
               PD_CHECK ( storageUnit, SDB_OOM, error, PDERROR,
                          "Failed to allocate dmsStorageUnit for %s",
                          dir_iter->path().string().c_str() ) ;

               // open collection space file failed, need report error
               // and restart
               rc = storageUnit->open ( dataPath, indexPath, lobPath,
                                        lobMetaPath, pmdGetSyncMgr(), FALSE ) ;
               if ( rc )
               {
                  SDB_OSS_DEL storageUnit ;
                  storageUnit = NULL ;
                  PD_LOG ( PDSEVERE, "Failed to open storage unit[%s], rc: %d",
                           dir_iter->path().string().c_str(), rc ) ;
                  PMD_RESTART_DB( rc ) ;
                  goto error ;
               }
               /// set config
               storageUnit->setSyncConfig( optCB->getSyncInterval(),
                                           optCB->getSyncRecordNum(),
                                           optCB->getSyncDirtyRatio() ) ;
               storageUnit->setSyncDeep( optCB->isSyncDeep() ) ;
               /// add collectionspace
               rc = dmsCB->addCollectionSpace ( csName, sequence, storageUnit,
                                                NULL, NULL, FALSE ) ;
               if ( rc )
               {
                  SDB_OSS_DEL storageUnit ;
                  storageUnit = NULL ;
                  PD_LOG( PDSEVERE, "Failed to add collection "
                          "space[%s], rc: %d", csName, rc ) ;
                  PMD_RESTART_DB( rc ) ;
                  goto error ;
               }

               // Note: do not call onLoad here

               storageUnit = NULL ;

               /*
                * Scan all the collections, to check if any one should be
                * put into the dictionary creating list. This should be
                * done if the system restarted before the dictionary was
                * created.
                */
               rc = rtnResumeClDictCreate( csName, dmsCB ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to resume dictionary creating "
                            "job for %s, rc: %d", csName, rc ) ;
            } // if ( rtnVerifyCollectionSpaceFileName
            else if ( SDB_FILE_UNKNOW == rtnParseFileName( fileName.c_str() ) )
            {
               PD_LOG( PDWARNING, "Found unknow file[%s] when load collection "
                       "spaces, ignored", dir_iter->path().string().c_str() ) ;
            }
         } //for ( fs::directory_iterator dir_iter(dbDir)
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR, "Failed to iterate directory %s: %s",
                       dataPath, e.what() ) ;
      }

      if ( hasRenameInfo )
      {
         logger.clear() ;
         hasRenameInfo = FALSE ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNLOADCSS, rc ) ;
      return rc ;
   error :
      if ( storageUnit )
      {
         SDB_OSS_DEL storageUnit ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDELCSCTX, "rtnDelContextForCollectionSpace" )
   void rtnDelContextForCollectionSpace ( const CHAR *pCollectionSpace,
                                          _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB_RTNDELCSCTX ) ;

      SDB_ASSERT ( cb, "EDU control block can't be NULL" ) ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      // let's find out whether the collection space is held by this
      // EDU. If so we have to get rid of those contexts
      pmdEDUCB::SET_CONTEXT contextList ;
      cb->contextCopy( contextList ) ;

      pmdEDUCB::SET_CONTEXT::iterator it = contextList.begin() ;
      while ( it != contextList.end() )
      {
         SINT64 contextID = *it ;
         ++it ;

         // get each context
         rtnContext *ctx = rtnCB->contextFind ( contextID ) ;
         // if context doesn't exist or has not dmsStorageUnit
         if ( !ctx || NULL == ctx->getSU() )
         {
            continue ;
         }
         if ( ossStrcmp ( ctx->getSU()->CSName(), pCollectionSpace ) == 0 )
         {
            // if the su is held by myself, i have to kill the context
            // from global
            rtnCB->contextDelete( contextID, cb ) ;
         }
      }

      PD_TRACE_EXIT ( SDB_RTNDELCSCTX ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDELCSCOMMAND, "rtnDelCollectionSpaceCommand" )
   INT32 rtnDelCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                        _pmdEDUCB *cb,
                                        SDB_DMSCB *dmsCB,
                                        SDB_DPSCB *dpsCB,
                                        BOOLEAN sysCall,
                                        BOOLEAN dropFile )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNDELCSCOMMAND ) ;
      BOOLEAN writable = FALSE ;

      SDB_ASSERT ( pCollectionSpace, "collection space can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      // make sure the collectionspace length is not out of range
      UINT32 length = ossStrlen ( pCollectionSpace ) ;
      if ( length <= 0 || length > DMS_SU_NAME_SZ )
      {
         PD_LOG ( PDERROR, "Invalid length for collectionspace: %s, rc: %d",
                  pCollectionSpace, rc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE ;

      // let's find out whether the collection space is held by this
      // EDU. If so we have to get rid of those contexts
      if ( NULL != cb )
      {
         rtnDelContextForCollectionSpace( pCollectionSpace, cb ) ;
      }

      if ( dropFile )
      {
         rc = dmsCB->dropCollectionSpace ( pCollectionSpace, cb, dpsCB ) ;
      }
      else
      {
         rc = dmsCB->unloadCollectonSpace( pCollectionSpace, cb ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to %s collectionspace %s, "
                   "rc: %d", dropFile ? "drop" : "unload",
                   pCollectionSpace, rc ) ;

   done :
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNDELCSCOMMAND, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 rtnUnloadCollectionSpace( const CHAR * pCollectionSpace,
                                   _pmdEDUCB * cb,
                                   SDB_DMSCB * dmsCB )
   {
      INT32 rc = rtnDelCollectionSpaceCommand( pCollectionSpace, cb,
                                               dmsCB, NULL, TRUE,
                                               FALSE ) ;
      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "Unload collectionspace %s succeed",
                 pCollectionSpace ) ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNUNLOADALLCS, "rtnUnloadCollectionSpaces" )
   INT32 rtnUnloadCollectionSpaces( _pmdEDUCB * cb, SDB_DMSCB * dmsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNUNLOADALLCS ) ;

      MON_CS_LIST csList ;

      //dump all collectionspace
      dmsCB->dumpInfo( csList, TRUE ) ;
      MON_CS_LIST::const_iterator it = csList.begin() ;
      while ( it != csList.end() )
      {
         const _monCollectionSpace &cs = *it ;
         rc = rtnUnloadCollectionSpace ( cs._name, cb, dmsCB ) ;
         if ( SDB_OK != rc && SDB_DMS_CS_NOTEXIST != rc )
         {
            PD_LOG ( PDERROR, "Unload collectionspace[%s] failed[rc:%d]",
                     cs._name, rc ) ;
            break ;
         }
         ++it ;
      }

      PD_TRACE_EXITRC ( SDB_RTNUNLOADALLCS, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNFINDCL, "rtnFindCollection" )
   INT32 rtnFindCollection ( const CHAR *pCollection,
                             SDB_DMSCB *dmsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNFINDCL );
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s",
                  pCollection ) ;
         goto error ;
      }
      UINT16 cID ;
      rc = su->data()->findCollection ( pCollectionShortName, cID ) ;

   done :
      if ( DMS_INVALID_CS != suID )
         dmsCB->suUnlock ( suID ) ;
      PD_TRACE_EXITRC ( SDB_RTNFINDCL, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCSLOCK, "rtnCollectionSpaceLock" )
   INT32 rtnCollectionSpaceLock ( const CHAR *pCollectionSpaceName,
                                  SDB_DMSCB *dmsCB,
                                  BOOLEAN loadFile,
                                  dmsStorageUnit **ppsu,
                                  dmsStorageUnitID &suID,
                                  OSS_LATCH_MODE lockType,
                                  INT32 millisec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCSLOCK );
      SDB_ASSERT ( pCollectionSpaceName, "cs name can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT ( ppsu, "storage unit can't be NULL" ) ;

   retry:
      rc = dmsCB->nameToSUAndLock ( pCollectionSpaceName, suID,
                                    ppsu, lockType, millisec ) ;
      if ( SDB_OK == rc )
      {
         goto done ;
      }
      else if ( SDB_DMS_CS_NOTEXIST == rc && loadFile )
      {
         // comment out following part because there's problem to load
         // collection space after database is activated
         // thread 1:
         // close file handle    ->        remove file
         // thread 2:
         //   ....            create same cs  ....
         // if thread 2 happened in the middle of two steps from step 1, there's
         // very small timing hole, which makes some inconsistence
         // for now let's prevent automatically loading a collection space
         // without explicitly creating it
         PD_LOG ( PDINFO, "Collection Space %s does not exist in dms,"
                  " load from disk", pCollectionSpaceName ) ;

         rc = rtnLoadCollectionSpace ( pCollectionSpaceName,
                                       pmdGetOptionCB()->getDbPath(),
                                       pmdGetOptionCB()->getIndexPath(),
                                       pmdGetOptionCB()->getLobPath(),
                                       pmdGetOptionCB()->getLobMetaPath(),
                                       NULL, dmsCB, FALSE ) ;
         if ( rc )
         {
            if ( SDB_DMS_CS_NOTEXIST != rc )
            {
               PD_LOG( PDWARNING, "Unable to load collection %s from %s",
                       pCollectionSpaceName, pmdGetOptionCB()->getDbPath() ) ;
            }
            goto error ;
         }
         goto retry ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCSLOCK, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNRESOLVECLNAL, "rtnResolveCollectionNameAndLock" )
   INT32 rtnResolveCollectionNameAndLock ( const CHAR *pCollectionFullName,
                                           SDB_DMSCB *dmsCB,
                                           dmsStorageUnit **ppsu,
                                           const CHAR **ppCollectionName,
                                           dmsStorageUnitID &suID,
                                           OSS_LATCH_MODE lockType,
                                           INT32 millisec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNRESOLVECLNAL );
      SDB_ASSERT ( pCollectionFullName, "collection name can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT ( ppsu, "storage unit can't be NULL" ) ;
      CHAR *pDot = NULL ;
      CHAR *pDot1 = NULL ;
      CHAR strCollectionFullName [ DMS_COLLECTION_SPACE_NAME_SZ +
                                   DMS_COLLECTION_NAME_SZ + 2 ] = {0} ;
      if ( ossStrlen ( pCollectionFullName ) > DMS_COLLECTION_SPACE_NAME_SZ +
           DMS_COLLECTION_NAME_SZ + 1 )
      {
         PD_LOG_MSG ( PDERROR, "Collection name is too long: %s",
                      pCollectionFullName ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( strCollectionFullName, pCollectionFullName,
                   sizeof(strCollectionFullName) ) ;
      // find the "." in the middle of collection name
      pDot = (CHAR*)ossStrchr ( strCollectionFullName, '.' ) ;
      pDot1 = (CHAR*)ossStrrchr ( strCollectionFullName, '.' ) ;
      // if dot doesn't exist, or there are more than 1 dot, we print invalid
      // format error message
      if ( !pDot || (pDot !=pDot1) || strCollectionFullName == pDot )
      {
         PD_LOG_MSG ( PDERROR, "Invalid format for collection name: %s,"
                      "Expected format: <collectionspace>.<collectionname>",
                      pCollectionFullName ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // now let's set pDot to 0, so strCollectionFullName will be
      // collectionspace only
      *pDot = 0 ;
      if (ppCollectionName)
      {
         // now pDot+1 is pointing to the starting of collection, so we get the
         // difference between pDot+1 and strCollectionFullName, and plus
         // pCollectionFullName to get the pointer in the original string
         *ppCollectionName = pDot + 1 -&strCollectionFullName[0] +
                             pCollectionFullName ;
      }

      PD_TRACE2 ( SDB_RTNRESOLVECLNAL,
                  PD_PACK_UINT ( lockType ),
                  PD_PACK_STRING ( strCollectionFullName ) );

      rc = rtnCollectionSpaceLock ( strCollectionFullName, dmsCB,
                                    FALSE, ppsu, suID, lockType, millisec ) ;
      if ( rc )
      {
         PD_LOG ( PDINFO, "Failed to lock collection space %s, rc: %d",
                  strCollectionFullName, rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNRESOLVECLNAL, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNKILLCONTEXTS, "rtnKillContexts" )
   INT32 rtnKillContexts ( INT32 numContexts, INT64 *pContextIDs,
                           pmdEDUCB *cb, SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNKILLCONTEXTS );
      for ( INT32 i = 0; i< numContexts ; i++ )
      {
         if ( rtnCB->contextFind ( pContextIDs[i] ) &&
              !cb->contextFind ( pContextIDs[i] ) )
         {
            PD_LOG ( PDWARNING, "Context %lld is not owned by current session",
                     pContextIDs[i] ) ;
            // if the context doesn't owned by the current session, let's show
            // warning message and jump to next context
            continue ;
         }
         rtnCB->contextDelete ( pContextIDs[i], cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNKILLCONTEXTS, rc );
      return rc ;
   }

   BOOLEAN rtnIsCommand ( const CHAR *name )
   {
      if ( name && name[0] == '$' )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPARSERCOMMAND, "rtnParserCommand" )
   INT32 rtnParserCommand ( const CHAR *name, _rtnCommand **ppCommand )
   {
      INT32 rc = SDB_INVALIDARG ;
      PD_TRACE_ENTRY ( SDB_RTNPARSERCOMMAND );
      if ( ppCommand && rtnIsCommand ( name ) )
      {
         *ppCommand = getRtnCmdBuilder()->create ( &name[1] ) ;
         if ( *ppCommand )
         {
            rc = SDB_OK ;
         }
      }

      PD_TRACE_EXITRC ( SDB_RTNPARSERCOMMAND, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNRELEASECOMMAND, "rtnReleaseCommand" )
   INT32 rtnReleaseCommand ( _rtnCommand **ppCommand )
   {
      PD_TRACE_ENTRY ( SDB_RTNRELEASECOMMAND );
      if ( ppCommand && *ppCommand )
      {
         getRtnCmdBuilder()->release( *ppCommand ) ;
         *ppCommand = NULL ;
      }

      PD_TRACE_EXIT ( SDB_RTNRELEASECOMMAND );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNINITCOMMAND, "rtnInitCommand" )
   INT32 rtnInitCommand ( _rtnCommand *pCommand ,INT32 flags, INT64 numToSkip,
                          INT64 numToReturn, const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff, const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff )
   {
      INT32 rc = SDB_INVALIDARG ;
      PD_TRACE_ENTRY ( SDB_RTNINITCOMMAND );
      if ( pCommand )
      {
         try
         {
            rc = pCommand->init( flags, numToSkip, numToReturn, pMatcherBuff,
                                 pSelectBuff, pOrderByBuff, pHintBuff ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "init command[%s] exception[%s]", pCommand->name(),
                     e.what() ) ;
            rc = SDB_INVALIDARG ;
         }

         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "init command[%s] failed[rc=%d]",
                     pCommand->name(), rc ) ;
         }
      }

      PD_TRACE_EXITRC ( SDB_RTNINITCOMMAND, rc );
      return rc ;
   }

   INT32 dbRoleToSpaceNode ( SDB_ROLE role )
   {
      INT32 spaceNode = CMD_SPACE_NODE_NULL ;
      switch ( role )
      {
         case SDB_ROLE_DATA :
            spaceNode = CMD_SPACE_NODE_DATA ;
            break ;
         case SDB_ROLE_COORD :
            spaceNode = CMD_SPACE_NODE_COORD ;
            break ;
         case SDB_ROLE_CATALOG :
            spaceNode = CMD_SPACE_NODE_CATA ;
            break ;
         case SDB_ROLE_STANDALONE :
            spaceNode = CMD_SPACE_NODE_STANDALONE ;
            break ;
         case SDB_ROLE_OM :
            spaceNode = CMD_SPACE_NODE_OM ;
         default :
            break ;
      }
      return spaceNode ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNRUNCOMMAND, "rtnRunCommand" )
   INT32 rtnRunCommand ( _rtnCommand *pCommand, INT32 serviceType,
                         _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                         SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                         INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_INVALIDARG ;
      PD_TRACE_ENTRY ( SDB_RTNRUNCOMMAND );

      if ( pCommand )
      {
         // the space data check
         SDB_ROLE role = pmdGetKRCB()->getDBRole() ;
         if ( !(dbRoleToSpaceNode ( role ) & pCommand->spaceNode()) )
         {
            rc = SDB_RTN_CMD_NO_NODE_AUTH ;
            goto error ;
         }

         if ( !(serviceType & pCommand->spaceService()) )
         {
            rc = SDB_RTN_CMD_NO_SERVICE_AUTH ;
            goto error ;
         }
         pCommand->setFromService( serviceType ) ;

         try
         {
            rc = pCommand->doit( cb, dmsCB, rtnCB, dpsCB, w, pContextID ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "run command[%s] exception[%s]", pCommand->name(),
                     e.what() ) ;
         }

         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               PD_LOG ( PDDEBUG, "run command[%s] failed[end of the context]",
                        pCommand->name() ) ;
            }
            else
            {
               PD_LOG ( PDERROR, "run command[%s] failed[rc=%d]",
                        pCommand->name(), rc ) ;
            }
         }

         //sync
         if ( cb )
         {
            if ( SDB_OK == rc && pCommand->writable() && dpsCB )
            {
               rc = dpsCB->completeOpr( cb, w ) ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNRUNCOMMAND, rc );
      return rc ;
   error:
      goto done ;
   }

   string rtnMakeSUFileName( const string &csName, UINT32 sequence,
                             const string &extName )
   {
      CHAR tmp[10] = {0} ;

      ossSnprintf( tmp, 9, "%d", sequence ) ;
      string suFile = csName ;
      suFile += "." ;
      suFile += tmp ;
      suFile += "." ;
      suFile += extName ;

      return suFile ;
   }

   string rtnFullPathName( const string &path, const string &name )
   {
      string fullPathName = path ;
      if ( path.length() > 0 &&
           0 != ossStrncmp(&(path.c_str())[path.length()-1], OSS_FILE_SEP, 1 ) )
      {
         fullPathName += OSS_FILE_SEP ;
      }
      fullPathName += name ;

      return fullPathName ;
   }

   // Note that Only delete and update are calling this interface
   // In the future, if there are other cases, we should carefully
   // review the usage and decide what scanner to initialize with.
   // For now, we directly invoke rtnDiskIXScanner as we won't use
   // in memory index which is from last committed value.
   INT32 rtnGetIXScanner ( const CHAR *pCollectionShortName,
                           optAccessPlanRuntime *planRuntime,
                           dmsStorageUnit *su,
                           dmsMBContext *mbContext,
                           pmdEDUCB *cb,
                           dmsScanner **ppScanner,
                           DMS_ACCESS_TYPE accessType )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT ( pCollectionShortName, "collection name can't be NULL" ) ;
      SDB_ASSERT ( su, "su can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;
      SDB_ASSERT ( planRuntime, "planRuntime can't be NULL" ) ;
      SDB_ASSERT ( mbContext, "mb context can't be NULL" ) ;
      SDB_ASSERT ( ppScanner, "Scanner can't be NULL" ) ;

      mthMatchRuntime *matchRuntime = NULL ;

      rtnScannerFactory    f ;
      IXScannerType scanType = ( DPS_INVALID_TRANS_ID != cb->getTransID() ) ?
                                 SCANNER_TYPE_MERGE : SCANNER_TYPE_DISK ;
      // delete and update should also use scanner properly
      _rtnIXScanner * scanner     = NULL ;

      // first shared lock to get metadata
      rc = mbContext->mbLock( SHARED ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to lock collection[%s], rc = %d",
                  pCollectionShortName, rc ) ;
         goto error ;
      }

      {
         rtnPredicateList *predList = NULL ;
         // for index scan, we maintain context by runtime instead of by DMS
         ixmIndexCB indexCB ( planRuntime->getIndexCBExtent(), su->index(), NULL ) ;
         if ( !indexCB.isInitialized() )
         {
            PD_LOG ( PDERROR, "unable to get proper index control block" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         if ( indexCB.getLogicalID() != planRuntime->getIndexLID() )
         {
            PD_LOG( PDERROR, "Index[extent id: %d] logical id[%d] is not "
                    "expected[%d]", planRuntime->getIndexCBExtent(),
                    indexCB.getLogicalID(), planRuntime->getIndexLID() ) ;
            rc = SDB_IXM_NOTEXIST ;
            goto error ;
         }

         // get the predicate list
         predList = planRuntime->getPredList() ;
         SDB_ASSERT ( predList, "predList can't be NULL" ) ;

         // get the matcher from plan instead of manually loading it
         matchRuntime = planRuntime->getMatchRuntime( TRUE ) ;

         rc = f.createScanner( scanType, &indexCB, predList, su, cb, scanner ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      mbContext->mbStat()->_crudCB.increaseIxScan( 1 ) ;
      mbContext->mbUnlock() ;

      *ppScanner = SDB_OSS_NEW dmsIXScanner( su->data(), mbContext,
                                             matchRuntime, scanner, TRUE,
                                             accessType, -1, 0, 0 ) ;
      if ( !(*ppScanner) )
      {
         PD_LOG( PDERROR, "Unable to allocate memory for dms ixscanner" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      scanner = NULL ;

   done :
      return rc ;
   error :
      if ( scanner )
      {
         f.releaseScanner( scanner ) ;
      }
      mbContext->mbUnlock() ;
      goto done ;
   }

   INT32 rtnGetTBScanner ( const CHAR *pCollectionShortName,
                           optAccessPlanRuntime *planRuntime,
                           dmsStorageUnit *su,
                           dmsMBContext *mbContext,
                           pmdEDUCB *cb,
                           dmsScanner **ppScanner,
                           DMS_ACCESS_TYPE accessType )
   {
      INT32 rc                 = SDB_OK ;
      mthMatchRuntime *matchRuntime = planRuntime->getMatchRuntime() ;

      SDB_ASSERT ( pCollectionShortName, "collection name can't be NULL" ) ;
      SDB_ASSERT ( su, "su can't be NULL" ) ;
      SDB_ASSERT ( mbContext, "mb context can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;
      SDB_ASSERT ( ppScanner, "scanner can't be NULL" ) ;

      *ppScanner = SDB_OSS_NEW dmsTBScanner( su->data(), mbContext,
                                             matchRuntime, accessType,
                                             -1, 0, 0 ) ;
      if ( !(*ppScanner) )
      {
         PD_LOG( PDERROR, "Unable to allocate memory for dms tbscanner" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   static BOOLEAN _rtnCheckExcludeSelector ( const BSONElement & original )
   {
      BOOLEAN hasExclude = FALSE ;
      if ( Object == original.type() )
      {
         BSONObj subObject = original.embeddedObject() ;
         BSONObjIterator itr( subObject ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( 0 == ossStrcmp( ele.fieldName(), MTH_S_INCLUDE ) &&
                 ele.isNumber() &&
                 0 == ele.numberLong() )
            {
               hasExclude = TRUE ;
               break ;
            }
         }
      }
      return hasExclude ;
   }

   void rtnNeedResetSelector( const BSONObj &original,
                              const BSONObj &orderBy,
                              BOOLEAN &needReset )
   {
      needReset = FALSE ;
      if ( !original.isEmpty() &&
           !orderBy.isEmpty() )
      {
         BSONObjIterator itr( orderBy ) ;
         while ( itr.more() )
         {
            /// find({}, {a:null}).sort({a.b:1})
            /// we do not want to clear it's selector when
            /// query is like above. --yunwu
            BSONElement ele = itr.next() ;
            const CHAR *fieldName = ele.fieldName() ;
            BSONElement select = original.getField( fieldName ) ;
            if ( EOO == select.type() )
            {
               const CHAR * subField = fieldName ;
               while ( TRUE )
               {
                  CHAR * c = (CHAR *)ossStrchr( subField, '.' ) ;
                  if ( NULL == c )
                  {
                     needReset = TRUE ;
                     break ;
                  }

                  *c = '\0' ;
                  BSONElement select = original.getField( fieldName ) ;
                  *c = '.' ;
                  if ( EOO != select.type() )
                  {
                     if ( _rtnCheckExcludeSelector( select ) )
                     {
                        needReset = TRUE ;
                     }
                     break ;
                  }
                  subField = (const CHAR *)( c + 1 ) ;
               }
               if ( needReset )
               {
                  break ;
               }
            }
            else if ( _rtnCheckExcludeSelector( select ) )
            {
               needReset = TRUE ;
               break ;
            }
         }
      }

      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSYNCDB, "rtnSyncDB" )
   INT32 rtnSyncDB( _pmdEDUCB *cb, INT32 syncType,
                    const CHAR *pSpecCSName, BOOLEAN block )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNSYNCDB ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      UINT64 beginTick = pmdGetDBTick() ;
      DPS_LSN commitLSN ;
      BOOLEAN sync = FALSE ;
      MON_CS_LIST allCS ;
      BOOLEAN dmsLocked = FALSE ;
      BOOLEAN syncSpecCS = FALSE ;
      UINT32 syncCSNum = 0 ;

      if ( !dpsCB || !dpsCB )
      {
         /// do nothing
         goto done ;
      }

      /// init config
      if ( 0 == syncType )
      {
         sync = FALSE ;
      }
      else if ( syncType > 0 )
      {
         sync = TRUE ;
      }
      else
      {
         sync = pmdGetOptionCB()->isSyncDeep() ;
      }

      if ( pSpecCSName && *pSpecCSName )
      {
         syncSpecCS = TRUE ;
      }

      /// block write
      if ( block )
      {
         rc = dmsCB->blockWrite( cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Block write failed, rc: %d", rc ) ;
            goto error;
         }
         else
         {
            PD_LOG( PDINFO, "Block write operation succeed" ) ;
         }
         dmsLocked = TRUE ;
      }

      /// commit log
      if ( !syncSpecCS )
      {
         rc = dpsCB->commit( sync, &commitLSN ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to commit dps log: %d", rc ) ;
            goto error ;
         }
      }

      /// Dump all collectionspace, except SYSTEM
      dmsCB->dumpInfo( allCS, TRUE ) ;

      for ( MON_CS_LIST::const_iterator itr = allCS.begin() ;
            itr != allCS.end() ;
            ++itr )
      {
         const CHAR *csName = itr->_name ;
         dmsStorageUnit *su = NULL ;
         dmsStorageUnitID suID = DMS_INVALID_CS ;

         if ( syncSpecCS && 0 != ossStrcmp( pSpecCSName, csName ) )
         {
            continue ;
         }
         ++syncCSNum ;

         dmsCSMutexScope csLock( dmsCB, csName ) ;
         /// get cs lock
         rc = dmsCB->nameToSUAndLock ( csName, suID, &su, SHARED ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            /// may be dropped, continue ;
            continue ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get lock of cs[%s], rc: %d",
                    csName, rc ) ;
            continue ;
         }

         /// except SYSTEM
         if ( su->data()->isTempSU() )
         {
            dmsCB->suUnlock( suID ) ;
            continue ;
         }

         /// sync
         rc = su->sync( sync, cb ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Sync collectionspace[%s] to file failed, "
                    "rc: %d", csName, rc ) ;
            /// not report the error
         }
         dmsCB->suUnlock( suID ) ;
      }

      /// commit log again
      if ( !syncSpecCS )
      {
         rc = dpsCB->commit( sync, &commitLSN ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to commit dps log: %d", rc ) ;
            goto error ;
         }
      }
      else if ( syncSpecCS && 0 == syncCSNum )
      {
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error ;
      }

   done:
      if ( dmsLocked )
      {
         dmsCB->unblockWrite( cb ) ;
         PD_LOG( PDINFO, "Unblock write operation succeed" ) ;
      }
      if ( SDB_OK == rc )
      {
         if ( syncSpecCS )
         {
            PD_LOG( PDEVENT, "Sync collectionspace[%s] succeed, "
                    "deep:%s, cost(ms): %llu", pSpecCSName,
                    sync ? "true" : "false",
                    pmdGetTickSpanTime( beginTick ) ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Sync db succeed, commit lsn[%u.%llu], "
                    "deep: %s, cost(ms): %llu", commitLSN.version,
                    commitLSN.offset, sync ? "true" : "false",
                    pmdGetTickSpanTime( beginTick ) ) ;
         }
      }
      PD_TRACE_EXITRC( SDB_RTNSYNCDB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTESTCRTCL, "rtnTestAndCreateCL" )
   INT32 rtnTestAndCreateCL ( const CHAR *pCLFullName, pmdEDUCB *cb,
                              _SDB_DMSCB *dmsCB, _dpsLogWrapper *dpsCB,
                              BOOLEAN sys )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNTESTCRTCL ) ;

      rc = rtnTestCollectionCommand( pCLFullName, dmsCB ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc || SDB_DMS_NOTEXIST == rc )
      {
         rc = rtnCreateCollectionCommand( pCLFullName, 0, cb, dmsCB, dpsCB,
                                          UTIL_UNIQUEID_NULL,
                                          UTIL_COMPRESSOR_INVALID,
                                          FLG_CREATE_WHEN_NOT_EXIST, sys ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create collection[%s], rc: %d",
                      pCLFullName, rc ) ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Test collection[%s] failed, rc: %d", pCLFullName,
                 rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNTESTCRTCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTESTCRTIDX, "rtnTestAndCreateIndex" )
   INT32 rtnTestAndCreateIndex ( const CHAR *pCLFullName,
                                 const BSONObj &indexDef,
                                 pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _dpsLogWrapper *dpsCB, BOOLEAN sys,
                                 INT32 sortBufferSize )
   {
      INT32 rc          = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNTESTCRTIDX ) ;

      BOOLEAN isSame    = FALSE ;

      try
      {
         rc = rtnTestIndex( pCLFullName, "", dmsCB, &indexDef, &isSame ) ;
         if ( SDB_IXM_NOTEXIST == rc || ( SDB_OK == rc && isSame == FALSE ) )
         {
            if ( SDB_OK == rc && isSame == FALSE )
            {
               BSONElement ele = indexDef.getField( IXM_NAME_FIELD ) ;
               rc = rtnDropIndexCommand( pCLFullName, ele, cb, dmsCB,
                                         dpsCB, sys ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to drop index[%s] for "
                            "collection[%s], rc: %d", ele.valuestr(),
                            pCLFullName, rc ) ;
            }
            // create index
            rc = rtnCreateIndexCommand( pCLFullName, indexDef, cb, dmsCB,
                                        dpsCB, sys, sortBufferSize ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to create index[%s] for "
                         "collection[%s], rc: %d", indexDef.toString().c_str(),
                         pCLFullName, rc ) ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Test index[%s] for collection[%s] failed, "
                    "rc: %d", indexDef.toString().c_str(), pCLFullName, rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNTESTCRTIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNRESOLVECOLLECTIONNAME, "rtnResolveCollectionName" )
   INT32 rtnResolveCollectionName ( const CHAR *pInput, UINT32 inputLen,
                                    CHAR *pSpaceName, UINT32 spaceNameSize,
                                    CHAR *pCollectionName,
                                    UINT32 collectionNameSize )
   {
      INT32 rc = SDB_OK ;
      UINT32 curPos = 0 ;
      UINT32 i = 0 ;
      PD_TRACE_ENTRY ( SDB_RTNRESOLVECOLLECTIONNAME ) ;
      while ( pInput[curPos] != '.' )
      {
         if ( curPos >= inputLen )
         {
            PD_LOG_MSG ( PDERROR, "Invalid format for collection name: %s,"
                         "Expected format: <collectionspace>.<collectionname>",
                         pInput ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( i >= spaceNameSize )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pSpaceName[ i++ ] = pInput[ curPos++ ] ;
      }
      pSpaceName[i] = '\0' ;

      i = 0 ;
      ++curPos ;
      while ( curPos < inputLen )
      {
         if ( i >= collectionNameSize )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pCollectionName[ i++ ] = pInput[ curPos++ ] ;
      }
      pCollectionName[i] = '\0' ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNRESOLVECOLLECTIONNAME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNRESOLVECOLLECTIONSPACENAME, "rtnResolveCollectionSpaceName" )
   INT32 rtnResolveCollectionSpaceName ( const CHAR *pInput,
                                         UINT32 inputLen,
                                         CHAR *pSpaceName,
                                         UINT32 spaceNameSize )
   {
      INT32 rc = SDB_OK ;
      UINT32 curPos = 0 ;
      UINT32 i = 0 ;

      PD_TRACE_ENTRY ( SDB_RTNRESOLVECOLLECTIONSPACENAME ) ;
      while ( pInput[curPos] != '.' )
      {
         if ( curPos >= inputLen )
         {
            PD_LOG_MSG ( PDERROR, "Invalid format for collection name: %s,"
                         "Expected format: <collectionspace>.<collectionname>",
                         pInput ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( i >= spaceNameSize )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pSpaceName[ i++ ] = pInput[ curPos++ ] ;
      }
      pSpaceName[i] = '\0' ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNRESOLVECOLLECTIONSPACENAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCLINSAMECS, "rtnCollectionsInSameSpace" )
   INT32 rtnCollectionsInSameSpace ( const CHAR *pCLNameA, UINT32 lengthA,
                                     const CHAR *pCLNameB, UINT32 lengthB,
                                     BOOLEAN &inSameSpace )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_RTNCLINSAMECS ) ;

      CHAR csNameA[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
      CHAR csNameB[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;

      rc = rtnResolveCollectionSpaceName( pCLNameA, lengthA, csNameA,
                                          DMS_COLLECTION_SPACE_NAME_SZ ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = rtnResolveCollectionSpaceName( pCLNameB, lengthB, csNameB,
                                          DMS_COLLECTION_SPACE_NAME_SZ ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      inSameSpace = ( ossStrcmp( csNameA, csNameB ) == 0 ) ;

   done :
      PD_TRACE_EXITRC ( SDB_RTNCLINSAMECS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void rtnUnsetTransContext( _pmdEDUCB * cb,SDB_RTNCB *rtnCB )
   {
      if ( cb->contextNum() > 0 )
      {
         _pmdEDUCB::SET_CONTEXT setCtx ;
         _pmdEDUCB::SET_CONTEXT::iterator it ;
         rtnContextBase *pContext = NULL ;

         cb->contextCopy( setCtx ) ;

         for ( it = setCtx.begin() ; it != setCtx.end() ; ++it )
         {
            pContext = rtnCB->contextFind( *it, NULL ) ;
            if ( pContext )
            {
               pContext->setTransContext( FALSE ) ;
            }
         }
      }
   }

   INT32 rtnConvertIndexDef( BSONObj& indexDef )
   {
      INT32 rc = SDB_OK ;

      // 1. Message of old version, has "unique" field. Message since v3.2,
      // has "Unique" and "unique" field. But in engine, we just convert
      // "Unique" to "unique".
      // 2. unique/enforced/NotNull value support number. But in engine, we
      // convert { unique: 1 } => { unique: true } to prevent ambiguity.
      try
      {
         BSONObjBuilder builder ;
         BSONObjIterator i( indexDef ) ;
         while ( i.more() )
         {
            BSONElement e = i.next();
            // convert { Unique: true } => { unique: true }
            if ( 0 == ossStrcmp( e.fieldName(), IXM_UNIQUE_FIELD1 ) )
            {
               builder.append( IXM_UNIQUE_FIELD, e.trueValue() ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), IXM_ENFORCED_FIELD1 ) )
            {
               builder.append( IXM_ENFORCED_FIELD, e.trueValue() ) ;
            }
            // convert { unique: 1 } => { unique: true }
            else if ( 0 == ossStrcmp( e.fieldName(), IXM_UNIQUE_FIELD ) )
            {
               builder.append( IXM_UNIQUE_FIELD, e.trueValue() ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), IXM_ENFORCED_FIELD ) )
            {
               builder.append( IXM_ENFORCED_FIELD, e.trueValue() ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), IXM_NOTNULL_FIELD ) )
            {
               builder.append( IXM_NOTNULL_FIELD, e.trueValue() ) ;
            }
            else
            {
               builder.append( e ) ;
            }
         }
         indexDef = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOADCOLLECTIONDICT, "rtnLoadCollectionDict" )
   INT32 rtnLoadCollectionDict( const CHAR *pCollectionName,
                                const CHAR *dictionary,
                                UINT32 dictSize, BOOLEAN force )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOADCOLLECTIONDICT ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      dmsStorageUnit *su = NULL ;
      const CHAR *clShortName = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageDataCommon *data = NULL ;
      dmsMBContext *context = NULL ;

      rc = rtnResolveCollectionNameAndLock( pCollectionName, dmsCB, &su,
                                            &clShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Resolve collection name[%s] failed: %d",
                   pCollectionName, rc ) ;

      data = su->data() ;
      rc = data->getMBContext( &context, clShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Get mb context for collection[%s] failed: %d",
                   pCollectionName, rc ) ;

      rc = data->loadDictionary( context, dictionary, dictSize, force ) ;
      PD_RC_CHECK( rc, PDERROR, "Load dictionary for collection[%s] failed[%d]",
                   pCollectionName, rc ) ;

   done:
      if ( context )
      {
         data->releaseMBContext( context ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNLOADCOLLECTIONDICT, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

