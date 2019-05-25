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

   Source File Name = omagentNodeCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/08/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentNodeCmd.hpp"
#include "rtnCommandDef.hpp"
#include "omagentMgr.hpp"
#include "pmdOptions.h"
#include "msgDef.h"
#include "pmd.hpp"

using namespace bson ;

namespace engine
{

   /*
      _omaShutdownCmd implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaShutdownCmd )

   _omaShutdownCmd::_omaShutdownCmd()
   {
   }

   _omaShutdownCmd::~_omaShutdownCmd()
   {
   }

   const CHAR* _omaShutdownCmd::name()
   {
      return NAME_SHUTDOWN ;
   }

   INT32 _omaShutdownCmd::init( const CHAR * pInfomation )
   {
      return SDB_OK ;
   }

   INT32 _omaShutdownCmd::doit( BSONObj & retObj )
   {
      PMD_SHUTDOWN_DB( SDB_OK ) ;
      return SDB_OK ;
   }

   /*
      _omaSetPDLevelCmd implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaSetPDLevelCmd )

   _omaSetPDLevelCmd::_omaSetPDLevelCmd()
   {
      _pdLevel = PDWARNING ;
   }

   _omaSetPDLevelCmd::~_omaSetPDLevelCmd()
   {
   }

   const CHAR* _omaSetPDLevelCmd::name()
   {
      return NAME_SET_PDLEVEL ;
   }

   INT32 _omaSetPDLevelCmd::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj match ( pInfomation ) ;
         BSONElement e = match.getField( FIELD_NAME_PDLEVEL ) ;
         if ( !e.isNumber() )
         {
            PD_LOG( PDERROR, "Failed to get field[%s], field: %s",
                    FIELD_NAME_PDLEVEL, e.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _pdLevel = e.numberInt() ;

         if ( _pdLevel < PDSEVERE || _pdLevel > PDDEBUG )
         {
            PD_LOG ( PDWARNING, "PDLevel[%d] error, set to default[%d]",
                     _pdLevel, PDWARNING ) ;
            _pdLevel = PDWARNING ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaSetPDLevelCmd::doit( BSONObj & retObj )
   {
      setPDLevel( (PDLEVEL)_pdLevel ) ;
      PD_LOG ( getPDLevel(), "Set PDLEVEL to [%s]",
               getPDLevelDesp( getPDLevel() ) ) ;
      return SDB_OK ;
   }

   /*
      _omaCreateNodeCmd implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaCreateNodeCmd )

   _omaCreateNodeCmd::_omaCreateNodeCmd()
   {
   }

   _omaCreateNodeCmd::~_omaCreateNodeCmd()
   {
   }

   const CHAR* _omaCreateNodeCmd::name()
   {
      return NAME_CREATE_NODE ;
   }

   INT32 _omaCreateNodeCmd::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      _roleStr = SDB_ROLE_COORD_STR ;

      if ( sdbGetOMAgentOptions()->isStandAlone() )
      {
         rc = SDB_PERM ;
         goto error ;
      }

      try
      {
         BSONObj obj( pInfomation ) ;
         BSONObjIterator it( obj ) ;

         while ( it.more() )
         {
            BSONElement e = it.next() ;

            if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_GROUPNAME ) )
            {
               if ( 0 != ossStrcmp( e.valuestrsafe(), COORD_GROUPNAME ) )
               {
                  PD_LOG( PDERROR, "Group[%s] is not %s in command[%s]",
                          e.valuestrsafe(), COORD_GROUPNAME, name() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               continue ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_HOST ) )
            {
               continue ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), PMD_OPTION_ROLE ) )
            {
               if ( 0 == ossStrcmp( e.valuestrsafe(),
                                    SDB_ROLE_STANDALONE_STR ) )
               {
                  _roleStr = SDB_ROLE_STANDALONE_STR ;
               }
               else if ( 0 == ossStrcmp( e.valuestrsafe(),
                                         SDB_ROLE_OM_STR ) )
               {
                  _roleStr = SDB_ROLE_OM_STR ;
               }
               continue ;
            }

            builder.append( e ) ;
         }

         builder.append( PMD_OPTION_ROLE, _roleStr ) ;
         _config = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Ocurr exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaCreateNodeCmd::doit( BSONObj & retObj )
   {
      BSONObj dummy ;
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      string omsvc ;

      if ( 0 == ossStrcmp( _roleStr.c_str(), SDB_ROLE_OM_STR ) )
      {
         sdbGetOMAgentOptions()->lock( EXCLUSIVE ) ;
         locked = TRUE ;

         if ( sdbGetOMAgentOptions()->omAddrs().size() > 0 )
         {
            PD_LOG( PDERROR, "OM node[%s] has already exist, can't create "
                    "another", sdbGetOMAgentOptions()->getOMAddress() ) ;
            rc = SDBCM_NODE_EXISTED ;
            goto error ;
         }
      }

      rc = sdbGetOMAgentMgr()->getNodeMgr()->addANode( _config.objdata(),
                                                       dummy.objdata(),
                                                       &omsvc ) ;
      if ( ( SDB_OK == rc || SDBCM_NODE_EXISTED == rc ) &&
           0 == ossStrcmp( _roleStr.c_str(), SDB_ROLE_OM_STR ) )
      {
         sdbGetOMAgentOptions()->addOMAddr( pmdGetKRCB()->getHostName(),
                                            omsvc.c_str() ) ;
         sdbGetOMAgentOptions()->save() ;
         sdbGetOMAgentMgr()->onConfigChange() ;
      }

   done:
      if ( locked )
      {
         sdbGetOMAgentOptions()->unLock( EXCLUSIVE ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaRemoveNodeCmd implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaRemoveNodeCmd )

   _omaRemoveNodeCmd::_omaRemoveNodeCmd()
   {
   }

   _omaRemoveNodeCmd::~_omaRemoveNodeCmd()
   {
   }

   const CHAR* _omaRemoveNodeCmd::name()
   {
      return NAME_REMOVE_NODE ;
   }

   INT32 _omaRemoveNodeCmd::doit( BSONObj & retObj )
   {
      BSONObj dummy ;
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      string omsvc ;

      if ( 0 == ossStrcmp( _roleStr.c_str(), SDB_ROLE_OM_STR ) )
      {
         sdbGetOMAgentOptions()->lock( EXCLUSIVE ) ;
         locked = TRUE ;
      }

      rc = sdbGetOMAgentMgr()->getNodeMgr()->rmANode( _config.objdata(),
                                                      dummy.objdata(),
                                                      _roleStr.c_str(),
                                                      &omsvc ) ;
      if ( SDB_OK == rc &&
           0 == ossStrcmp( _roleStr.c_str(), SDB_ROLE_OM_STR ) )
      {
         sdbGetOMAgentOptions()->delOMAddr( pmdGetKRCB()->getHostName(),
                                            omsvc.c_str() ) ;
         sdbGetOMAgentOptions()->save() ;
         sdbGetOMAgentMgr()->onConfigChange() ;
      }

      if ( locked )
      {
         sdbGetOMAgentOptions()->unLock( EXCLUSIVE ) ;
      }

      return rc ;
   }

   /*
      _omaStartNodeCmd implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaStartNodeCmd )

   _omaStartNodeCmd::_omaStartNodeCmd()
   {
      _pData = NULL ;
   }

   _omaStartNodeCmd::~_omaStartNodeCmd()
   {
   }

   const CHAR* _omaStartNodeCmd::name()
   {
      return NAME_START_NODE ;
  }

   INT32 _omaStartNodeCmd::init( const CHAR * pInfomation )
   {
      if ( sdbGetOMAgentOptions()->isStandAlone() )
      {
         return SDB_PERM ;
      }

      _pData = pInfomation ;
      return SDB_OK ;
   }

   INT32 _omaStartNodeCmd::doit( BSONObj & retObj )
   {
      return sdbGetOMAgentMgr()->getNodeMgr()->startANode( _pData ) ;
   }

   /*
      _omaStopNodeCmd implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaStopNodeCmd )

   _omaStopNodeCmd::_omaStopNodeCmd()
   {
   }

   _omaStopNodeCmd::~_omaStopNodeCmd()
   {
   }

   const CHAR* _omaStopNodeCmd::name()
   {
      return NAME_SHUTDOWN_NODE ;
   }

   INT32 _omaStopNodeCmd::doit( BSONObj & retObj )
   {
      return sdbGetOMAgentMgr()->getNodeMgr()->stopANode( _pData ) ;
   }

}

