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

   Source File Name = sptUsrSdbTool.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptUsrSdbTool.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "pmdOptions.h"
#include "utilCommon.hpp"
#include "ossProc.hpp"
#include "omagentDef.hpp"
#include "pmdDaemon.hpp"
#include "utilParam.hpp"
#include "ossSocket.hpp"
#include "pmdOptionsMgr.hpp"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{

   /*
      Function Define
   */
   JS_CONSTRUCT_FUNC_DEFINE(_sptUsrSdbTool, construct)
   JS_STATIC_FUNC_DEFINE(_sptUsrSdbTool, listNodes)

   /*
      Function Map
   */
   JS_BEGIN_MAPPING( _sptUsrSdbTool, "Sdbtool" )
      JS_ADD_CONSTRUCT_FUNC(construct)
      JS_ADD_STATIC_FUNC("listNodes", listNodes)
   JS_MAPPING_END()

   /*
      _sptUsrSdbTool Implement
   */
   _sptUsrSdbTool::_sptUsrSdbTool()
   {
   }

   _sptUsrSdbTool::~_sptUsrSdbTool()
   {
   }

   INT32 _sptUsrSdbTool::construct( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      detail = BSON( SPT_ERR << "Sdbtool can't new" ) ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptUsrSdbTool::listNodes( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj option ;
      BSONObj filterObj ;
      _sdbToolListParam optionParam ;
      UTIL_VEC_NODES nodes ;
      string strpath ;
      CHAR rootPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR localPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      vector< BSONObj > vecObj ;

      if ( arg.argc() > 0 )
      {
         rc = arg.getBsonobj( 0, option ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "option must be bson obj" ) ;
            goto error ;
         }
      }
      if ( arg.argc() > 1 )
      {
         rc = arg.getBsonobj( 1, filterObj ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "filter must be bson obj" ) ;
            goto error ;
         }
      }
      if ( arg.argc() > 2 )
      {
         rc = arg.getString( 2, strpath ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "rootPath must be string" ) ;
            goto error ;
         }
      }

      if ( strpath.empty() )
      {
         rc = ossGetEWD( rootPath, OSS_MAX_PATHSIZE ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "get current path failed" ) ;
            goto error ;
         }
      }
      else
      {
         ossStrncpy( rootPath, strpath.c_str(), OSS_MAX_PATHSIZE ) ;
      }

      rc = utilBuildFullPath( rootPath, SDBCM_LOCAL_PATH,
                              OSS_MAX_PATHSIZE, localPath ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "build local conf path failed" ) ;
         goto error ;
      }

      rc = _parseListParam( option, optionParam, detail ) ;
      if ( rc )
      {
         goto error ;
      }

      utilListNodes( nodes, optionParam._typeFilter, NULL,
                     OSS_INVALID_PID, optionParam._roleFilter,
                     optionParam._showAlone ) ;

      if ( RUN_MODE_RUN == optionParam._modeFilter )
      {
         BOOLEAN bFind = FALSE ;
         UTIL_VEC_NODES::iterator it = nodes.begin() ;
         while( it != nodes.end() && optionParam._svcnames.size() > 0 )
         {
            bFind = FALSE ;
            utilNodeInfo &info = *it ;
            for ( UINT32 j = 0 ; j < optionParam._svcnames.size() ; ++j )
            {
               if ( info._svcname == optionParam._svcnames[ j ] )
               {
                  bFind = TRUE ;
                  break ;
               }
            }
            if ( !bFind )
            {
               it = nodes.erase( it ) ;
               continue ;
            }
            ++it ;
         }
      }
      else
      {
         BOOLEAN bFind = FALSE ;
         UTIL_VEC_NODES tmpNodes = nodes ;
         nodes.clear() ;
         utilEnumNodes( localPath, nodes, optionParam._typeFilter, NULL,
                        optionParam._roleFilter ) ;
         if ( ( -1 == optionParam._typeFilter ||
                SDB_TYPE_OMA == optionParam._typeFilter ) &&
              ( -1 == optionParam._roleFilter ||
                SDB_ROLE_OMA == optionParam._roleFilter ) )
         {
            CHAR hostName[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
            CHAR confFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
            utilNodeInfo node ;
            node._orgname = "" ;
            node._pid = OSS_INVALID_PID ;
            node._role = SDB_ROLE_OMA ;
            node._type = SDB_TYPE_OMA ;
            ossGetHostName( hostName, OSS_MAX_HOSTNAME ) ;

            utilBuildFullPath( rootPath, SDBCM_CONF_PATH_FILE,
                               OSS_MAX_PATHSIZE, confFile ) ;
            // file exist
            if ( 0 == ossAccess( confFile ) )
            {
               utilGetCMService( rootPath, hostName, node._svcname, TRUE ) ;
               nodes.push_back( node ) ;
            }
         }

         UTIL_VEC_NODES::iterator it = nodes.begin() ;
         while ( it != nodes.end() && optionParam._svcnames.size() > 0 )
         {
            utilNodeInfo &info = *it ;
            bFind = FALSE ;
            for ( UINT32 j = 0 ; j < optionParam._svcnames.size() ; ++j )
            {
               if ( info._svcname == optionParam._svcnames[ j ] )
               {
                  bFind = TRUE ;
                  break ;
               }
            }
            if ( !bFind )
            {
               it = nodes.erase( it ) ;
               continue ;
            }
            ++it ;
         }

         for ( UINT32 i = 0 ; i < nodes.size() ; ++i )
         {
            for ( UINT32 k = 0 ; k < tmpNodes.size() ; ++k )
            {
               if ( nodes[ i ]._svcname == tmpNodes[ k ]._svcname )
               {
                  nodes[ i ] = tmpNodes[ k ] ;
                  break ;
               }
            }
         }
      }

      // filter
      for ( UINT32 k = 0 ; k < nodes.size() ; ++k )
      {
         BSONObj obj = _nodeInfo2Bson( nodes[ k ],
                                       optionParam._expand ?
                                       _getConfObj( rootPath, localPath,
                                       nodes[ k ]._svcname.c_str(),
                                       nodes[ k ]._type ) :
                                       BSONObj() ) ;
         if ( _match( obj, filterObj, SPT_MATCH_AND ) )
         {
            vecObj.push_back( obj ) ;
         }
      }

      // if no -p, and list all/list cm, need to show sdbcmd
      if ( optionParam._svcnames.size() == 0 &&
           ( SDB_TYPE_OMA == optionParam._typeFilter ||
             -1 == optionParam._typeFilter ) &&
           ( optionParam._roleFilter == -1 ||
             SDB_ROLE_OMA == optionParam._roleFilter ) )
      {
         vector < ossProcInfo > procs ;
         ossEnumProcesses( procs, PMDDMN_EXE_NAME, TRUE, FALSE ) ;

         for ( UINT32 i = 0 ; i < procs.size() ; ++i )
         {
            BSONObj obj = BSON( "type" << PMDDMN_SVCNAME_DEFAULT <<
                                "pid" << (UINT32)procs[ i ]._pid ) ;
            if ( _match( obj, filterObj, SPT_MATCH_AND ) )
            {
               vecObj.push_back( obj ) ;
            }
         }
      }

      // set result
      rval.getReturnVal().setValue( vecObj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSdbTool::_parseListParam( const  BSONObj &option,
                                          _sdbToolListParam &param,
                                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      BSONObjIterator it ( option ) ;
      while ( it.more() )
      {
         BSONElement e = it.next() ;
         if ( 0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_TYPE ) )
         {
            if ( String != e.type() )
            {
               detail = BSON( SPT_ERR << "type must be string(db/om/cm/all)" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            if ( 0 == ossStrcasecmp( e.valuestr(), SDBLIST_TYPE_DB_STR ) )
            {
               param._typeFilter = SDB_TYPE_DB ;
            }
            else if ( 0 == ossStrcasecmp( e.valuestr(), SDBLIST_TYPE_OM_STR ) )
            {
               param._typeFilter = SDB_TYPE_OM ;
            }
            else if ( 0 == ossStrcasecmp( e.valuestr(), SDBLIST_TYPE_OMA_STR ) )
            {
               param._typeFilter = SDB_TYPE_OMA ;
            }
            else if ( 0 == ossStrcasecmp( e.valuestr(), SDBLIST_TYPE_ALL_STR ) )
            {
               param._typeFilter = -1 ;
            }
            else
            {
               detail = BSON( SPT_ERR << "type must be db/om/cm/all" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( 0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_MODE ) )
         {
            if ( String != e.type() )
            {
               detail = BSON( SPT_ERR << "mode must be string[run/local]" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            if ( 0 == ossStrcasecmp( e.valuestr(),
                                     SDB_RUN_MODE_TYPE_LOCAL_STR ) )
            {
               param._modeFilter = RUN_MODE_LOCAL ;
            }
            else if ( 0 == ossStrcasecmp( e.valuestr(),
                                          SDB_RUN_MODE_TYPE_RUN_STR ) )
            {
               param._modeFilter = RUN_MODE_RUN ;
            }
            else
            {
               detail = BSON( SPT_ERR << "mode must be run/local" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( 0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_ROLE ) )
         {
            if ( String != e.type() )
            {
               detail = BSON( SPT_ERR << "role must be string[data/coord/"
                              "catalog/standalone/om/cm]" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            param._roleFilter = utilGetRoleEnum( e.valuestr() ) ;
            if ( SDB_ROLE_MAX == param._roleFilter )
            {
               detail = BSON( SPT_ERR << "role must be data/coord/catalog/"
                              "standalone/om/cm" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( 0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_SVCNAME ) )
         {
            if ( String != e.type() )
            {
               detail = BSON( SPT_ERR << "svcname must be string" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            rc = utilSplitStr( e.valuestr(), param._svcnames, ", \t" ) ;
            if ( rc )
            {
               detail = BSON( SPT_ERR << "svcname must be string, use "
                              "comma(,) to sperate") ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( 0 == ossStrcasecmp( e.fieldName(), "showalone" ) )
         {
            param._showAlone = e.booleanSafe() ? TRUE : FALSE ;
         }
         else if ( 0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_EXPAND ) )
         {
            param._expand = e.booleanSafe() ? TRUE : FALSE ;
         }
      }

      if ( param._roleFilter != -1 )
      {
         param._typeFilter = -1 ;
      }
      if ( param._svcnames.size() > 0 )
      {
         param._roleFilter = -1 ;
         param._typeFilter = -1 ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj _sptUsrSdbTool::_nodeInfo2Bson( const utilNodeInfo &info,
                                           const BSONObj &conf )
   {
      BSONObjBuilder builder ;
      struct tm otm ;
      time_t tt = info._startTime ;
      CHAR tmpTime[ 21 ] = { 0 } ;
#if defined (_WINDOWS)
         localtime_s( &otm, &tt ) ;
#else
         localtime_r( &tt, &otm ) ;
#endif
         ossSnprintf( tmpTime, sizeof( tmpTime ) - 1,
                      "%04d-%02d-%02d-%02d.%02d.%02d",
                      otm.tm_year+1900,
                      otm.tm_mon+1,
                      otm.tm_mday,
                      otm.tm_hour,
                      otm.tm_min,
                      otm.tm_sec ) ;

      builder.append( "svcname", info._svcname ) ;
      builder.append( "type", utilDBTypeStr( (SDB_TYPE)info._type ) ) ;
      builder.append( "role", utilDBRoleStr( (SDB_ROLE)info._role ) ) ;
      builder.append( "pid", (UINT32)info._pid ) ;
      builder.append( "groupid", info._groupID ) ;
      builder.append( "nodeid", info._nodeID ) ;
      builder.append( "primary", info._primary ) ;
      builder.append( "isalone", info._isAlone ) ;
      builder.append( "groupname", info._groupName ) ;
      builder.append( "starttime", tmpTime ) ;
      if ( !info._dbPath.empty() )
      {
         builder.append( "dbpath", info._dbPath ) ;
      }
      BSONObj infoObj = builder.obj() ;

      BSONObjBuilder retObjBuilder ;
      retObjBuilder.appendElements( infoObj ) ;
      BSONObjIterator it( conf ) ;
      while ( it.more() )
      {
         BSONElement e = it.next() ;
         if ( EOO == infoObj.getField( e.fieldName() ).type() )
         {
            retObjBuilder.append( e ) ;
         }
      }
      return retObjBuilder.obj() ;
   }

   BSONObj _sptUsrSdbTool::_getConfObj( const CHAR *rootPath,
                                        const CHAR *localPath,
                                        const CHAR *svcname,
                                        INT32 type )
   {
      BSONObj obj ;
      CHAR confFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      // not cm
      if ( type != SDB_TYPE_OMA )
      {
         pmdOptionsCB conf ;
         utilBuildFullPath( localPath, svcname, OSS_MAX_PATHSIZE, confFile ) ;
         if ( 0 != ossAccess( confFile, 0 ) )
         {
            goto done ;
         }
         utilCatPath( confFile, OSS_MAX_PATHSIZE, PMD_DFT_CONF ) ;
         if ( SDB_OK == conf.initFromFile( confFile, FALSE ) )
         {
            conf.toBSON( obj ) ;
         }
      }
      else
      {
         po::options_description desc ;
         po::variables_map vm ;
         BSONObjBuilder builder ;
         desc.add_options()
            ( "*", po::value<string>(), "" ) ;

         utilBuildFullPath( rootPath, SDBCM_CONF_PATH_FILE,
                            OSS_MAX_PATHSIZE, confFile ) ;
         if ( 0 != ossAccess( confFile, 0 ) )
         {
            goto done ;
         }
         utilReadConfigureFile( confFile, desc, vm ) ;
         po::variables_map::iterator it = vm.begin() ;
         while( it != vm.end() )
         {
            builder.append( it->first.data(), it->second.as<string>() ) ;
            ++it ;
         }
         obj = builder.obj() ;
      }

   done:
      return obj ;
   }

   BOOLEAN _sptUsrSdbTool::_match( const BSONObj &obj, const BSONObj &filter,
                                   SPT_MATCH_PRED pred )
   {
      BOOLEAN matched = ( SPT_MATCH_AND == pred ) ? TRUE : FALSE ;
      BSONObjIterator itFilter( filter ) ;
      while( itFilter.more() )
      {
         BOOLEAN subMatch = FALSE ;
         BSONElement e = itFilter.next() ;
         // $and
         if ( 0 == ossStrcmp( e.fieldName(), "$and" ) &&
              Array == e.type() )
         {
            subMatch = _match( obj, e.embeddedObject(), SPT_MATCH_AND ) ;
         }
         // $or
         else if ( 0 == ossStrcmp( e.fieldName(), "$or" ) &&
                   Array == e.type() )
         {
            subMatch = _match( obj, e.embeddedObject(), SPT_MATCH_OR ) ;
         }
         // $not
         else if ( 0 == ossStrcmp( e.fieldName(), "$not" ) &&
                   Array == e.type() )
         {
            subMatch = !_match( obj, e.embeddedObject(), SPT_MATCH_AND ) ;
         }
         else if ( Object == e.type() )
         {
            subMatch = _match( obj, e.embeddedObject(), pred ) ;
         }
         else
         {
            BSONElement e1 = obj.getField( e.fieldName() ) ;
            subMatch = ( 0 == e1.woCompare( e, false ) ) ? TRUE : FALSE ;
         }

         if ( SPT_MATCH_AND == pred && FALSE == subMatch )
         {
            matched = FALSE ;
            break ;
         }
         else if ( SPT_MATCH_OR == pred && TRUE == subMatch )
         {
            matched = TRUE ;
            break ;
         }
      }
      return matched ;
   }

}


