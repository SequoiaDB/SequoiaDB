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

   Source File Name = utilCommon.cpp

   Descriptive Name = Process MoDel Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for SequoiaDB,
   and all other process-initialization code.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilCommon.hpp"
#include "ossUtil.hpp"
#include "msg.h"
#include "ossLatch.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include <sstream>

using namespace bson ;

namespace engine
{

   SDB_ROLE utilGetRoleEnum( const CHAR *role )
   {
      if ( NULL == role )
         return SDB_ROLE_MAX;
      else if ( *role == 0 ||
                0 == ossStrcasecmp( role, SDB_ROLE_STANDALONE_STR ) )
         return SDB_ROLE_STANDALONE ;
      else if ( 0 == ossStrcasecmp( role, SDB_ROLE_DATA_STR ) )
         return SDB_ROLE_DATA;
      else if ( 0 == ossStrcasecmp( role, SDB_ROLE_CATALOG_STR ) )
         return SDB_ROLE_CATALOG;
      else if ( 0 == ossStrcasecmp( role, SDB_ROLE_COORD_STR ) )
         return SDB_ROLE_COORD;
      else if ( 0 == ossStrcasecmp( role, SDB_ROLE_OM_STR ) )
         return SDB_ROLE_OM ;
      else if ( 0 == ossStrcasecmp( role, SDB_ROLE_OMA_STR ) )
         return SDB_ROLE_OMA ;
      else
         return SDB_ROLE_MAX;
   }

   const CHAR* utilDBRoleStr( SDB_ROLE dbrole )
   {
      switch ( dbrole )
      {
         case SDB_ROLE_DATA :
            return SDB_ROLE_DATA_STR ;
         case SDB_ROLE_COORD :
            return SDB_ROLE_COORD_STR ;
         case SDB_ROLE_CATALOG :
            return SDB_ROLE_CATALOG_STR ;
         case SDB_ROLE_STANDALONE :
            return SDB_ROLE_STANDALONE_STR ;
         case SDB_ROLE_OM :
            return SDB_ROLE_OM_STR ;
         case SDB_ROLE_OMA :
            return SDB_ROLE_OMA_STR ;
         default :
            break ;
      }
      return "" ;
   }

   const CHAR* utilDBRoleShortStr( SDB_ROLE dbrole )
   {
      switch ( dbrole )
      {
         case SDB_ROLE_DATA :
            return "D" ;
         case SDB_ROLE_COORD :
            return "S" ;
         case SDB_ROLE_CATALOG :
            return "C" ;
         default :
            break ;
      }
      return "" ;
   }

   SDB_ROLE utilShortStr2DBRole( const CHAR * role )
   {
      if ( NULL == role )
         return SDB_ROLE_MAX;
      if ( 0 == ossStrcasecmp( role, "D" ) )
         return SDB_ROLE_DATA;
      else if ( 0 == ossStrcasecmp( role, "C" ) )
         return SDB_ROLE_CATALOG;
      else if ( 0 == ossStrcasecmp( role, "S" ) )
         return SDB_ROLE_COORD;
      else
         return SDB_ROLE_MAX;
   }

   SDB_TYPE utilGetTypeEnum( const CHAR * type )
   {
      if ( NULL == type )
      {
         return SDB_TYPE_MAX ;
      }
      else if ( 0 == *type ||
                0 == ossStrcasecmp( type, SDB_TYPE_DB_STR ) )
      {
         return SDB_TYPE_DB ;
      }
      else if ( 0 == ossStrcasecmp( type, SDB_TYPE_OM_STR ) )
      {
         return SDB_TYPE_OM ;
      }
      else if ( 0 == ossStrcasecmp( type, SDB_TYPE_OMA_STR ) )
      {
         return SDB_TYPE_OMA ;
      }
      else
      {
         return SDB_TYPE_MAX ;
      }
   }

   const CHAR* utilDBTypeStr( SDB_TYPE type )
   {
      switch ( type )
      {
         case SDB_TYPE_DB :
            return SDB_TYPE_DB_STR ;
         case SDB_TYPE_OM :
            return SDB_TYPE_OM_STR ;
         case SDB_TYPE_OMA :
            return SDB_TYPE_OMA_STR ;
         default :
            break ;
      }
      return "Unknow" ;
   }

   SDB_TYPE utilRoleToType( SDB_ROLE role )
   {
      switch ( role )
      {
         case SDB_ROLE_DATA :
         case SDB_ROLE_COORD :
         case SDB_ROLE_CATALOG :
         case SDB_ROLE_STANDALONE :
            return SDB_TYPE_DB ;
         case SDB_ROLE_OM :
            return SDB_TYPE_OM ;
         case SDB_ROLE_OMA :
            return SDB_TYPE_OMA ;
         default :
            break ;
      }
      return SDB_TYPE_MAX ;
   }

   const CHAR* utilDBStatusStr( SDB_DB_STATUS dbStatus )
   {
      switch( dbStatus )
      {
         case SDB_DB_NORMAL :
            return SDB_DB_NORMAL_STR ;
         case SDB_DB_SHUTDOWN :
            return SDB_DB_SHUTDOWN_STR ;
         case SDB_DB_REBUILDING :
            return SDB_DB_REBUILDING_STR ;
         case SDB_DB_FULLSYNC :
            return SDB_DB_FULLSYNC_STR ;
         case SDB_DB_OFFLINE_BK :
            return SDB_DB_OFFLINE_BK_STR ;
         default :
            break ;
      }
      return "Unknow" ;
   }

   SDB_DB_STATUS utilGetDBStatusEnum( const CHAR *status )
   {
      if ( NULL == status )
      {
         return SDB_DB_STATUS_MAX ;
      }
      else if ( 0 == *status ||
                0 == ossStrcasecmp( status, SDB_DB_NORMAL_STR ) )
      {
         return SDB_DB_NORMAL ;
      }
      else if ( 0 == ossStrcasecmp( status, SDB_DB_SHUTDOWN_STR ) )
      {
         return SDB_DB_SHUTDOWN ;
      }
      else if ( 0 == ossStrcasecmp( status, SDB_DB_REBUILDING_STR ) )
      {
         return SDB_DB_REBUILDING ;
      }
      else if ( 0 == ossStrcasecmp( status, SDB_DB_FULLSYNC_STR ) )
      {
         return SDB_DB_FULLSYNC ;
      }
      else if ( 0 == ossStrcasecmp( status, SDB_DB_OFFLINE_BK_STR ) )
      {
         return SDB_DB_OFFLINE_BK ;
      }
      else
      {
         return SDB_DB_STATUS_MAX ;
      }
   }

   const CHAR* utilDataStatusStr( BOOLEAN dataIsOK, SDB_DB_STATUS dbStatus )
   {
      if ( dataIsOK )
      {
         return SDB_DATA_NORMAL_STR ;
      }
      if ( SDB_DB_REBUILDING == dbStatus || SDB_DB_FULLSYNC == dbStatus )
      {
         return SDB_DATA_REPAIR_STR ;
      }
      return SDB_DATA_FAULT_STR ;
   }

   std::string utilDBModeStr( UINT32 dbMode )
   {
      std::stringstream ss ;
      BOOLEAN hasAdd = FALSE ;

      if ( SDB_DB_MODE_READONLY & dbMode )
      {
         ss << SDB_DB_MODE_READONLY_STR ;
         hasAdd = TRUE ;
      }
      if ( SDB_DB_MODE_DEACTIVATED & dbMode )
      {
         if ( hasAdd )
         {
            ss << " | " ;
         }
         ss << SDB_DB_MODE_DEACTIVATED_STR ;
      }
      return ss.str() ;
   }

   UINT32 utilGetDBModeFlag( const string &mode )
   {
      UINT32 modeFlag = 0 ;
      vector< string > parsed ;
      utilSplitStr( mode, parsed, "| \t" ) ;

      for ( UINT32 i = 0 ; i < parsed.size() ; ++i )
      {
         if ( 0 == ossStrcasecmp( SDB_DB_MODE_READONLY_STR,
                                  parsed[ i ].c_str() ) )
         {
            modeFlag |= SDB_DB_MODE_READONLY ;
         }
         else if ( 0 == ossStrcasecmp( SDB_DB_MODE_DEACTIVATED_STR,
                                       parsed[ i ].c_str() ) )
         {
            modeFlag |= SDB_DB_MODE_DEACTIVATED ;
         }
      }

      return modeFlag ;
   }

   static INT32 _utilStrToFTMask( const CHAR *pStr, UINT32 &ftMask )
   {
      if ( !pStr || !*pStr )
      {
         return SDB_OK ;
      }

      const CHAR *start = pStr ;

      while( ' ' == *start || '\t' == *start )
      {
         ++start ;
      }

      if ( !*start )
      {
         return SDB_OK ;
      }

      const CHAR *end = start + ossStrlen( start ) - 1 ;
      while( end != start && ( ' ' == *end || '\t' == *end ) )
      {
         --end ;
      }
      UINT32 len = end - start + 1 ;
      /// compare
      if ( 0 == ossStrncasecmp( start, PMD_FT_MASK_NOSPC_STR, len ) )
      {
         ftMask |= PMD_FT_MASK_NOSPC ;
      }
      else if ( 0 == ossStrncasecmp( start, PMD_FT_MASK_DEADSYNC_STR, len ) )
      {
         ftMask |= PMD_FT_MASK_DEADSYNC ;
      }
      else if ( 0 == ossStrncasecmp( start, PMD_FT_MASK_SLOWNODE_STR, len ) )
      {
         ftMask |= PMD_FT_MASK_SLOWNODE ;
      }
      else if ( 0 == ossStrncasecmp( start, "NONE", len ) )
      {
         /// do nothing
      }
      else if ( 0 == ossStrncasecmp( start, "ALL", len ) )
      {
         ftMask = PMD_FT_MASK_ALL ;
      }
      else
      {
         return SDB_INVALIDARG ;
      }

      return SDB_OK ;
   }

   INT32 utilStrToFTMask( const CHAR *pStr, UINT32 &ftMask )
   {
      INT32 rc = SDB_OK ;
      ftMask = 0 ;

      if ( !pStr || !*pStr )
      {
         return SDB_OK ;
      }
      const CHAR *p = pStr ;
      CHAR *p1 = NULL ;
      while( p && *p )
      {
         p1 = (CHAR*)ossStrchr( p, '|' ) ;
         if ( p1 )
         {
            *p1 = 0 ;
         }
         rc = _utilStrToFTMask( p, ftMask ) ;
         if ( p1 )
         {
            *p1 = '|' ;
         }
         if ( rc )
         {
            break ;
         }
         p = p1 ? p1 + 1 : NULL ;
      }
      return rc ;
   }

   static void _utilAppendOrString( CHAR *pBuffer, INT32 bufSize,
                                    const CHAR *flagStr )
   {
      if ( 0 != *pBuffer )
      {
         ossStrncat( pBuffer, "|",
                     bufSize - ossStrlen( pBuffer ) ) ;
      }
      ossStrncat( pBuffer, flagStr, bufSize - ossStrlen( pBuffer ) ) ;
   }

   void utilFTMaskToStr( UINT32 ftMask, CHAR *pBuff, UINT32 size )
   {
      ossMemset ( pBuff, 0, size ) ;

      if ( OSS_BIT_TEST ( ftMask, PMD_FT_MASK_NOSPC ) )
      {
         _utilAppendOrString( pBuff, size, PMD_FT_MASK_NOSPC_STR ) ;
      }
      if ( OSS_BIT_TEST ( ftMask, PMD_FT_MASK_DEADSYNC ) )
      {
         _utilAppendOrString( pBuff, size, PMD_FT_MASK_DEADSYNC_STR ) ;
      }
      if ( OSS_BIT_TEST ( ftMask, PMD_FT_MASK_SLOWNODE ) )
      {
         _utilAppendOrString( pBuff, size, PMD_FT_MASK_SLOWNODE_STR ) ;
      }
   }

   BOOLEAN utilCheckInstanceID ( UINT32 instanceID, BOOLEAN includeUnknown )
   {
      if ( ( includeUnknown &&
             NODE_INSTANCE_ID_UNKNOWN == instanceID ) ||
           ( instanceID > NODE_INSTANCE_ID_MIN &&
             instanceID < NODE_INSTANCE_ID_MAX ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   void utilBuildErrorBson( BSONObjBuilder &builder,
                            INT32 flags, const CHAR *detail,
                            BOOLEAN *pRollback )
   {
      // check flags
      if ( flags < -SDB_MAX_ERROR || flags > SDB_MAX_WARNING )
      {
         PD_LOG ( PDERROR, "Error code error[rc:%d]", flags ) ;
         flags = SDB_SYS ;
      }

      try
      {
         builder.append ( OP_ERRNOFIELD, flags ) ;
         builder.append ( OP_ERRDESP_FIELD, getErrDesp ( flags ) ) ;

         if ( detail )
         {
            builder.append ( OP_ERR_DETAIL, detail ) ;
         }
         else
         {
            builder.append( OP_ERR_DETAIL, "" ) ;
         }
         if ( pRollback )
         {
            builder.appendBool ( FIELD_NAME_ROLLBACK,
                                 *pRollback ? TRUE : FALSE ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Build return object failed: %s", e.what() ) ;
      }
   }

   BSONObj utilGetErrorBson( INT32 flags, const CHAR *detail,
                             BOOLEAN *pRollback )
   {
      static BSONObj _retObj [SDB_MAX_ERROR + SDB_MAX_WARNING + 1] ;
      static BOOLEAN _init = FALSE ;
      static ossSpinXLatch _lock ;

      // init retobj
      if ( FALSE == _init )
      {
         _lock.get() ;
         if ( FALSE == _init )
         {
            for ( SINT32 i = -SDB_MAX_ERROR; i <= SDB_MAX_WARNING ; i ++ )
            {
               BSONObjBuilder berror ;
               berror.append ( OP_ERRNOFIELD, i ) ;
               berror.append ( OP_ERRDESP_FIELD, getErrDesp ( i ) ) ;
               berror.append ( OP_ERR_DETAIL, "" ) ;
               _retObj[ i + SDB_MAX_ERROR ] = berror.obj() ;
            }
            _init = TRUE ;
         }
         _lock.release() ;
      }

      // check flags
      if ( flags < -SDB_MAX_ERROR || flags > SDB_MAX_WARNING )
      {
         PD_LOG ( PDERROR, "Error code error[rc:%d]", flags ) ;
         flags = SDB_SYS ;
      }

      // return new obj
      if ( ( detail && *detail != 0  ) || pRollback )
      {
         BSONObjBuilder bb ;
         bb.append ( OP_ERRNOFIELD, flags ) ;
         bb.append ( OP_ERRDESP_FIELD, getErrDesp ( flags ) ) ;

         if ( detail )
         {
            bb.append ( OP_ERR_DETAIL, detail ) ;
         }
         else
         {
            bb.append( OP_ERR_DETAIL, "" ) ;
         }
         if ( pRollback )
         {
            bb.appendBool ( FIELD_NAME_ROLLBACK, *pRollback ? TRUE : FALSE ) ;
         }
         return bb.obj() ;
      }
      // return fix obj
      return _retObj[ SDB_MAX_ERROR + flags ] ;
   }

   struct _utilShellRCItem
   {
      UINT32      _src ;
      INT32       _rc ;
      UINT32      _end ;
   } ;
   typedef _utilShellRCItem utilShellRCItem ;

   #define MAP_SHELL_RC_ITEM( src, rc )   { src, rc, 0 },

   utilShellRCItem* utilGetShellRCMap()
   {
      static utilShellRCItem s_srcMap[] = {
         // map begin
         MAP_SHELL_RC_ITEM( SDB_SRC_SUC, SDB_OK )
         MAP_SHELL_RC_ITEM( SDB_SRC_IO, SDB_IO )
         MAP_SHELL_RC_ITEM( SDB_SRC_PERM, SDB_PERM )
         MAP_SHELL_RC_ITEM( SDB_SRC_OOM, SDB_OOM )
         MAP_SHELL_RC_ITEM( SDB_SRC_INTERRUPT, SDB_INTERRUPT )
         MAP_SHELL_RC_ITEM( SDB_SRC_SYS, SDB_SYS )
         MAP_SHELL_RC_ITEM( SDB_SRC_NOSPC, SDB_NOSPC )
         MAP_SHELL_RC_ITEM( SDB_SRC_TIMEOUT, SDB_TIMEOUT )
         MAP_SHELL_RC_ITEM( SDB_SRC_NETWORK, SDB_NETWORK )
         MAP_SHELL_RC_ITEM( SDB_SRC_INVALIDPATH, SDB_INVALIDPATH )
         MAP_SHELL_RC_ITEM( SDB_SRC_CANNOT_LISTEN, SDB_NET_CANNOT_LISTEN )
         MAP_SHELL_RC_ITEM( SDB_SRC_CAT_AUTH_FAILED, SDB_CAT_AUTH_FAILED )
         MAP_SHELL_RC_ITEM( SDB_SRC_INVALIDARG, SDB_INVALIDARG )
         // map end
         { 0, 0, 1 }
      } ;
      return &s_srcMap[0] ;
   }

   UINT32 utilRC2ShellRC( INT32 rc )
   {
      utilShellRCItem *pEntry = utilGetShellRCMap() ;
      while( 0 == pEntry->_end )
      {
         if ( rc == pEntry->_rc )
         {
            return pEntry->_src ;
         }
         ++pEntry ;
      }
      if ( rc >= 0 )
      {
         return rc ;
      }
      return SDB_SRC_SYS ;
   }

   INT32 utilShellRC2RC( UINT32 src )
   {
      utilShellRCItem *pEntry = utilGetShellRCMap() ;
      while( 0 == pEntry->_end )
      {
         if ( src == pEntry->_src )
         {
            return pEntry->_rc ;
         }
         ++pEntry ;
      }
      if ( (INT32)src <= 0 )
      {
         return src ;
      }
      return SDB_SYS ;
   }

}

