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

   Source File Name = utilRecycleItem.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for recycle item.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_RECYCLE_RETURN_INFO_HPP__
#define UTIL_RECYCLE_RETURN_INFO_HPP__

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "utilRecycleItem.hpp"
#include "utilString.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   #define UTIL_RETURN_MASK_EMPTY         ( 0x00 )
   #define UTIL_RETURN_MASK_REPLACE_CS    ( 0x01 )
   #define UTIL_RETURN_MASK_REPLACE_CL    ( 0x02 )
   #define UTIL_RETURN_MASK_RENAME_CS     ( 0x04 )
   #define UTIL_RETURN_MASK_RENAME_CL     ( 0x08 )
   #define UTIL_RETURN_MASK_CHANGEUID_CL  ( 0x10 )
   #define UTIL_RETURN_MASK_RENAME        ( UTIL_RETURN_MASK_RENAME_CS | \
                                            UTIL_RETURN_MASK_RENAME_CL | \
                                            UTIL_RETURN_MASK_CHANGEUID_CL )
   #define UTIL_RETURN_MASK_ALL           ( 0xFF )

   typedef ossPoolSet< ossPoolString > UTIL_RETURN_NAME_SET ;
   typedef UTIL_RETURN_NAME_SET::iterator UTIL_RETURN_NAME_SET_IT ;
   typedef UTIL_RETURN_NAME_SET::const_iterator UTIL_RETURN_NAME_SET_CIT ;
   typedef ossPoolMap< ossPoolString, ossPoolString > UTIL_RETURN_NAME_MAP ;
   typedef UTIL_RETURN_NAME_MAP::iterator UTIL_RETURN_NAME_MAP_IT ;
   typedef UTIL_RETURN_NAME_MAP::const_iterator UTIL_RETURN_NAME_MAP_CIT ;
   typedef ossPoolMap< utilCLUniqueID, utilCLUniqueID > UTIL_RETURN_UID_MAP ;
   typedef UTIL_RETURN_UID_MAP::iterator UTIL_RETURN_UID_MAP_IT ;
   typedef UTIL_RETURN_UID_MAP::const_iterator UTIL_RETURN_UID_MAP_CIT ;

   /*
      _utilReturnRenameMode define
    */
   typedef enum _utilReturnRenameMode
   {
      UTIL_RETURN_NO_RENAME,
      UTIL_RETURN_PART_RENAME,
      UTIL_RETURN_FULL_RENAME
   } utilReturnRenameMode ;

   #define UTIL_RETURN_NO_RENAME    ( 0x00 )
   // collection space part is renamed ( before "." in full name )
   #define UTIL_RETURN_RENAMED_CS   ( 0x01 )
   // collection part is renamed ( after "." in full name )
   #define UTIL_RETURN_RENAMED_CL   ( 0x02 )
   //
   #define UTIL_RETURN_IGNORED      ( 0x04 )

   /*
      _utilReturnNameInfo define
    */
   class _utilReturnNameInfo : public _utilPooledObject
   {
   public:
      _utilReturnNameInfo()
      : _origName( NULL ),
        _rtrnName( NULL ),
        _renameMask( UTIL_RETURN_NO_RENAME )
      {
         _rtrnNameBuf[ 0 ] = '\0' ;
      }

      _utilReturnNameInfo( const CHAR *originName )
      : _origName( originName ),
        _rtrnName( originName ),
        _renameMask( UTIL_RETURN_NO_RENAME )
      {
         _rtrnNameBuf[ 0 ] = '\0' ;
      }

      ~_utilReturnNameInfo()
      {
      }

      void init( const CHAR *originName )
      {
         _origName = originName ;
         _rtrnName = originName ;
         _rtrnNameBuf[ 0 ] = '\0' ;
         _renameMask = UTIL_RETURN_NO_RENAME ;
      }

      void setReturnName( const CHAR *returnName )
      {
         _rtrnName = returnName ;
      }

      void setReturnName( const CHAR *returnName,
                          UINT8 renameMask )
      {
         _rtrnName = returnName ;
         OSS_BIT_SET( _renameMask, renameMask ) ;
      }

      void setReturnName( const CHAR *returnCSName,
                          const CHAR *returnCLShortName,
                          UINT8 renameMask )
      {
         ossSnprintf( _rtrnNameBuf, UTIL_ORIGIN_NAME_SZ, "%s.%s",
                      returnCSName, returnCLShortName ) ;
         _rtrnName = _rtrnNameBuf ;
         OSS_BIT_SET( _renameMask, renameMask ) ;
      }

      const CHAR *getOriginName() const
      {
         return _origName ;
      }

      const CHAR *getReturnName() const
      {
         return _rtrnName ;
      }

      void setIgnored() const
      {
         OSS_BIT_TEST( _renameMask, UTIL_RETURN_IGNORED ) ;
      }

      UINT8 getRenameMask() const
      {
         return _renameMask ;
      }

      BOOLEAN isRenamed() const
      {
         return UTIL_RETURN_NO_RENAME != _renameMask ;
      }

      BOOLEAN isIgnored() const
      {
         return OSS_BIT_TEST( _renameMask, UTIL_RETURN_IGNORED ) ;
      }

   protected:
      const CHAR *   _origName ;
      const CHAR *   _rtrnName ;
      CHAR           _rtrnNameBuf[ UTIL_ORIGIN_NAME_SZ + 1 ] ;
      UINT8          _renameMask ;
   } ;

   typedef class _utilReturnNameInfo utilReturnNameInfo ;

   /*
      _utilReturnUIDInfo define
    */
   class _utilReturnUIDInfo : public _utilPooledObject
   {
   public:
      _utilReturnUIDInfo()
      : _origUID( UTIL_UNIQUEID_NULL ),
        _rtrnUID( UTIL_UNIQUEID_NULL ),
        _isChanged( FALSE )
      {
      }

      _utilReturnUIDInfo( utilCLUniqueID origUID )
      : _origUID( origUID ),
        _rtrnUID( origUID ),
        _isChanged( FALSE )
      {
      }

      ~_utilReturnUIDInfo()
      {
      }

      void init( utilCLUniqueID origUID )
      {
         _origUID = origUID ;
         _rtrnUID = origUID ;
         _isChanged = FALSE ;
      }

      void setReturnUID( utilCLUniqueID rtrnUID )
      {
         _rtrnUID = rtrnUID ;
         _isChanged = TRUE ;
      }

      utilCLUniqueID getOriginUID() const
      {
         return _origUID ;
      }

      utilCLUniqueID getReturnUID() const
      {
         return _rtrnUID ;
      }

      BOOLEAN isChanged() const
      {
         return _isChanged ;
      }

   protected:
      utilCLUniqueID _origUID ;
      utilCLUniqueID _rtrnUID ;
      BOOLEAN        _isChanged ;
   } ;

   typedef class _utilReturnUIDInfo utilReturnUIDInfo ;

   /*
      _utilRecycleReturnInfo define
    */
   // information for returning recycle item
   class _utilRecycleReturnInfo : public _utilPooledObject
   {
   public:
      _utilRecycleReturnInfo() ;
      ~_utilRecycleReturnInfo() ;

      INT32 addReplaceCS( const CHAR *csName ) ;
      INT32 addReplaceCL( const CHAR *clName ) ;

      INT32 addRenameCS( const CHAR *origCSName, const CHAR *returnCSName ) ;
      INT32 addRenameCL( const CHAR *origCLName, const CHAR *returnCLName ) ;
      INT32 addChangeUIDCL( utilCLUniqueID origCLUniqueID,
                            utilCLUniqueID returnCLUniqueID ) ;

      INT32 toBSON( bson::BSONObjBuilder &builder, UINT8 mask ) const ;
      INT32 fromBSON( const bson::BSONObj &object, UINT8 mask ) ;

      const UTIL_RETURN_NAME_SET &getReplaceCS() const
      {
         return _replaceCSSet ;
      }

      const UTIL_RETURN_NAME_SET &getReplaceCL() const
      {
         return _replaceCLSet ;
      }

      BOOLEAN hasRenameCS() const
      {
         return !( _renameCSMap.empty() ) ;
      }

      BOOLEAN hasRenameCL() const
      {
         return !( _renameCLMap.empty() ) ;
      }

      BOOLEAN hasChangeUIDCL() const
      {
         return !( _changeUIDCL.empty() ) ;
      }

      INT32 getReturnCSName( utilReturnNameInfo &info ) const ;
      INT32 getReturnCLName( utilReturnNameInfo &info ) const ;

      INT32 getReturnCLUID( utilReturnUIDInfo &info ) const ;

      void clear() ;

      // rebuild BSON object with given mask
      static INT32 rebuildBSON( const bson::BSONObj &object,
                                bson::BSONObjBuilder &builder,
                                UINT8 mask ) ;

   protected:
      INT32 _buildReplaceCS( bson::BSONObjBuilder &builder ) const ;
      INT32 _buildReplaceCL( bson::BSONObjBuilder &builder ) const ;
      INT32 _buildRenameCS( bson::BSONObjBuilder &builder ) const ;
      INT32 _buildRenameCL( bson::BSONObjBuilder &builder ) const ;
      INT32 _buildChangeUIDCL( bson::BSONObjBuilder &builder ) const ;

      static INT32 _buildNameSet( const CHAR *fieldName,
                                  const UTIL_RETURN_NAME_SET &nameSet,
                                  bson::BSONObjBuilder &builder ) ;
      static INT32 _buildNameMap( const CHAR *fieldName,
                                  const UTIL_RETURN_NAME_MAP &nameMap,
                                  bson::BSONObjBuilder &builder ) ;
      static INT32 _buildUIDMap( const CHAR *fieldName,
                                 const UTIL_RETURN_UID_MAP &nameMap,
                                 bson::BSONObjBuilder &builder ) ;
      INT32 _parseReplaceCS( const bson::BSONObj &object ) ;
      INT32 _parseReplaceCL( const bson::BSONObj &object ) ;
      INT32 _parseRenameCS( const bson::BSONObj &object ) ;
      INT32 _parseRenameCL( const bson::BSONObj &object ) ;
      INT32 _parseChangeUIDCL( const bson::BSONObj &object ) ;

      BOOLEAN _getRenameCS( const CHAR *origCSName,
                            const CHAR *&returnCSName ) const ;
      BOOLEAN _getRenameCL( const CHAR *origCLName,
                            const CHAR *&returnCLName ) const ;
      BOOLEAN _getChangeUIDCL( utilCLUniqueID origCLUID,
                               utilCLUniqueID &returnCLUID ) const ;

      static INT32 _parseNameSet( const CHAR *fieldName,
                                  const bson::BSONObj &object,
                                  UTIL_RETURN_NAME_SET &nameSet ) ;
      static INT32 _parseNameMap( const CHAR *fieldName,
                                  const bson::BSONObj &object,
                                  UTIL_RETURN_NAME_MAP &nameMap ) ;
      static INT32 _parseUIDMap( const CHAR *fieldName,
                                 const bson::BSONObj &object,
                                 UTIL_RETURN_UID_MAP &uidMap ) ;

      static BOOLEAN _getRename( const UTIL_RETURN_NAME_MAP &nameMap,
                                 const CHAR *origName,
                                 const CHAR *&returnName ) ;
      static BOOLEAN _getChangeUID( const UTIL_RETURN_UID_MAP &uidMap,
                                    utilCLUniqueID origUID,
                                    utilCLUniqueID &returnUID ) ;

   protected:
      // collection spaces to be replaced
      // which will be dropped during return recycle item
      UTIL_RETURN_NAME_SET _replaceCSSet ;
      // collections to be replaced
      // which will be dropped during return recycle item
      UTIL_RETURN_NAME_SET _replaceCLSet ;

      // collection spaces to be renamed
      // which will be renamed during return recycle item
      UTIL_RETURN_NAME_MAP _renameCSMap ;
      // collections to be renamed
      // which will be renamed during return recycle item
      UTIL_RETURN_NAME_MAP _renameCLMap ;

      // collections to be changed unique ID
      // which will be changed unique ID during return recycle item
      UTIL_RETURN_UID_MAP  _changeUIDCL ;
   } ;

   typedef class _utilRecycleReturnInfo utilRecycleReturnInfo ;

}

#endif // UTIL_RECYCLE_RETURN_INFO_HPP__
