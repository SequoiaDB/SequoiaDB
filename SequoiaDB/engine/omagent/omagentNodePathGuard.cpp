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

   Source File Name = omagentNodeMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/04/2015  LZ Initial Draft

   Last Changed =

*******************************************************************************/
#include "omagentNodePathGuard.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "ossPath.hpp"
#include "ossIO.hpp"

namespace engine {

   _omaNodePathGuard::_omaNodePathGuard()
   {
      ossMemset( _nodeName, 0, sizeof( _nodeName ) ) ;
   }

   void _omaNodePathGuard::init( const CHAR *nodeName,
                                 pmdOptionsCB *options )
   {
      ossStrncpy( _nodeName, nodeName, OSS_MAX_SERVICENAME ) ;
      _nodeName[ OSS_MAX_SERVICENAME ] = 0 ;

      const CHAR *dbpath = options->getDbPath() ;
      UINT32 pathLen = ossStrlen( dbpath ) ;

      _nodePaths.push_back( dbpath ) ;
      if ( 0 != ossStrncmp( dbpath, options->getIndexPath(), pathLen ) )
      {
         _nodePaths.push_back( options->getIndexPath() ) ;
      }
      if ( 0 != ossStrncmp( dbpath, options->getLobPath(), pathLen ) )
      {
         _nodePaths.push_back( options->getLobPath() ) ;
      }
      if ( 0 != ossStrncmp( dbpath, options->getLobMetaPath(), pathLen ) )
      {
         _nodePaths.push_back( options->getLobMetaPath() ) ;
      }
      if ( 0 != ossStrncmp( dbpath, options->getDiagLogPath(), pathLen ) )
      {
         _nodePaths.push_back( options->getDiagLogPath() ) ;
      }
      if ( 0 != ossStrncmp( dbpath, options->getAuditLogPath(), pathLen ) )
      {
         _nodePaths.push_back( options->getAuditLogPath() ) ;
      }
      if ( 0 != ossStrncmp( dbpath, options->getReplLogPath(), pathLen ) )
      {
         _nodePaths.push_back( options->getReplLogPath() ) ;
      }
      if ( 0 != ossStrncmp( dbpath, options->getBkupPath(), pathLen ) )
      {
         _nodePaths.push_back( options->getBkupPath() ) ;
      }
      if ( 0 != ossStrncmp( dbpath, options->getTmpPath(), pathLen ) )
      {
         _nodePaths.push_back( options->getTmpPath() ) ;
      }
      if ( 0 != ossStrncmp( dbpath, options->getArchivePath(), pathLen ) )
      {
         _nodePaths.push_back( options->getArchivePath() ) ;
      }
   }

   BOOLEAN _omaNodePathGuard::muteXOn( _omaNodePathGuard *pOther )
   {
      BOOLEAN ret = FALSE ;
      std::vector< std::string > *paths = NULL ;

      if ( 0 == ossStrcmp( name(), pOther->name() ) )
      {
         goto done ;
      }

      paths = pOther->getPaths() ;
      for ( UINT32 i = 0 ; i < _nodePaths.size() ; ++i )
      {
         string &path1 = _nodePaths[ i ] ;

         for ( UINT32 j = 0 ; j < paths->size() ; ++j )
         {
            string &path2 = (*paths)[ j ] ;

            if ( 0 == ossStrncmp( path1.c_str(), path2.c_str(),
                                  ossStrlen( path1.c_str() ) ) ||
                 0 == ossStrncmp( path1.c_str(), path2.c_str(),
                                  ossStrlen( path2.c_str() ) ) )
            {
               PD_LOG( PDERROR, "The node[%s]'s path[%s] is conflict with "
                       "an other node[%s]'s path[%s]", name(),
                       path1.c_str(), pOther->name(), path2.c_str() ) ;
               ret = TRUE ;
               break ;
            }
         }
      }

   done:
      return ret ;
   }

   INT32 _omaNodePathGuard::checkValid( pmdOptionsCB *options )
   {
      INT32 rc = SDB_OK ;

      rc = _checkExistedFiles( options->getDbPath(), "*.data", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _checkExistedFiles( options->getDbPath(), "sdb.conf", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _checkExistedFiles( options->getIndexPath(), "*.idx", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _checkExistedFiles( options->getLobMetaPath(), "*.lobm", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _checkExistedFiles( options->getLobPath(), "*.lobd", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _checkExistedFiles( options->getReplLogPath(),
                               "sequoiadbLog.*", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _checkExistedFiles( options->getArchivePath(),
                               "archivelog.*", 1 );
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _omaNodePathGuard::~_omaNodePathGuard()
   {
   }

   INT32 _omaNodePathGuard::_checkExistedFiles( const CHAR* path,
                                                const CHAR *filter,
                                                UINT32 deep )
   {
      SDB_ASSERT( NULL != path, "path cannot be NULL" ) ;

      INT32 rc = SDB_OK ;

      rc = ossAccess( path ) ;
      if ( SDB_OK == rc )
      {
         std::multimap<std::string, std::string> subFiles ;
         rc = ossEnumFiles( path, subFiles, filter, deep ) ;
         if ( rc )
         {
            goto error ;
         }
         else if ( !subFiles.empty() )
         {
            PD_LOG( PDERROR, "The node[%s]'s path: %s is not empty",
                    name(), path ) ;
            rc = SDB_DIR_NOT_EMPTY ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_OK ;
      }

   done:
      return rc ;
   error:
      goto done;
   }

}

