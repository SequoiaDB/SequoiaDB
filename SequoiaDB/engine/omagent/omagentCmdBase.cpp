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

   Source File Name = omagentCmdBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentCmdBase.hpp"
#include "omagentUtil.hpp"
#include "omagentHelper.hpp"
#include "ossProc.hpp"
#include "utilPath.hpp"
#include "omagentJob.hpp"
#include "omagentMgr.hpp"

using namespace bson ;

namespace engine
{

   /*
      _omaCommand
   */
   _omaCommand::_omaCommand ()
   {
      _scope      = NULL ;
      _fileBuff   = NULL ;
      _buffSize   = 0 ;
      _readSize   = 0 ;
      ossMemset( _jsFileName, 0, OSS_MAX_PATHSIZE + 1 ) ;
      prime() ;
   }

   _omaCommand::~_omaCommand ()
   {
      if ( _scope )
      {
         sdbGetOMAgentMgr()->releaseScope( _scope ) ;
         _scope = NULL ;
      }
      if ( _fileBuff )
      {
         SAFE_OSS_FREE ( _fileBuff ) ;
      }
   }

   INT32 _omaCommand::addUserDefineVar( const CHAR* pVariable )
   {
      INT32 rc = SDB_OK ;
      string variable ;

      if( pVariable == NULL )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Invalid variable value" ) ;
         goto error ;
      }

      variable = pVariable ;

      _userDefineVar.push_back( variable ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaCommand::setJsFile( const CHAR *fileName )
   {
      INT32 rc = SDB_OK ;
      const CHAR *tmp = NULL ;
      if ( NULL == fileName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Invalid js file name" ) ;
         goto error ;
      }
      tmp = sdbGetOMAgentOptions()->getScriptPath() ;
      ossStrncpy ( _jsFileName, tmp, OSS_MAX_PATHSIZE ) ;
      rc = utilCatPath ( _jsFileName, OSS_MAX_PATHSIZE, fileName ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to build js file full path, rc = %d",
                      rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaCommand::addJsFile( const CHAR *fileName,
                                 const CHAR *bus,
                                 const CHAR *sys,
                                 const CHAR *env,
                                 const CHAR *other )
   {
      INT32 rc = SDB_OK ;
      string name( fileName ) ;
      string para ;
      vector< pair<string, string> >::iterator it = _jsFiles.begin() ;

      for ( ; it != _jsFiles.end(); it++ )
      {
         /// caller need to deal with error, when js file had been add
         if ( it->first == name )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG ( PDWARNING, "Js file[%s] already exit", fileName ) ;
            goto error ;
         }
      }
      if ( bus ) para += bus ;
      if ( sys ) para += sys ;
      if ( env ) para += env ;
      if ( other ) para += other ;
      _jsFiles.push_back( pair<string, string>( name, para ) ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaCommand::getExcuteJsContent( string &content )
   {
      INT32 rc = SDB_OK ;
      vector<string>::iterator varIter ;
      vector< pair<string, string> >::iterator it = _jsFiles.begin() ;

      if ( it == _jsFiles.end() )
      {
         goto done ;
      }

      content.clear() ;

      for( varIter = _userDefineVar.begin(); varIter != _userDefineVar.end();
          ++varIter )
      {
         content += *varIter ;
         content += OSS_NEWLINE ;
      }

      for ( ; it != _jsFiles.end(); it++ )
      {
         rc = setJsFile( it->first.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to set js file[%s], rc = %d",
                         it->first.c_str(), rc ) ;
            goto error ;
         }
         rc = readFile ( _jsFileName, &_fileBuff,
                         &_buffSize, &_readSize ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to read js file[%s], rc = %d",
                         _jsFileName, rc ) ;
            goto error ;
         }
         content += it->second ;
         content += OSS_NEWLINE ;
         content += _fileBuff ;
         content += OSS_NEWLINE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaCommand::prime()
   {
      // add some common files
      addJsFile ( FILE_DEFINE ) ;
      addJsFile ( FILE_COMMON ) ;
      addJsFile ( FILE_LOG ) ;
      addJsFile ( FILE_FUNC ) ;
      return SDB_OK ;
   }

   INT32 _omaCommand::init ( const CHAR *pIndtallInfo )
   {
      INT32 rc = SDB_OK ;

      return rc ;
   }

   INT32 _omaCommand::doit ( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string errmsg ;
      BSONObjBuilder bob ;
      BSONObj detail ;
      const sptResultVal *pRval = NULL ;
      BSONObj rval ;

      rc = getExcuteJsContent( _content ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to get js file to "
                      "excute, rc = %d", rc ) ;
         goto error ;
      }
      // 1. get scope
      _scope = sdbGetOMAgentMgr()->getScope() ;
      if ( !_scope )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG ( PDERROR, "Failed to get scope, rc = %d", rc ) ;
         goto error ;
      }
      // 2. execute js
      rc = _scope->eval( _content.c_str(), _content.size(), "", 1,
                         SPT_EVAL_FLAG_NONE | SPT_EVAL_FLAG_IGNORE_ERR_PREFIX,
                         &pRval ) ;
      if ( rc )
      {
         // we come here for one of the follow reasons:
         // a. we throw exception out from js file
         // b. eval fail for js syntax error
         errmsg = _scope->getLastErrMsg() ;
         rc = _scope->getLastError() ;
         PD_LOG_MSG ( PDERROR, "%s", errmsg.c_str() ) ;
         PD_LOG ( PDDEBUG, "Failed to eval js file for command[%s]: "
                  "%s, rc = %d", name(), errmsg.c_str(), rc ) ;
         bob.append( OMA_FIELD_DETAIL, errmsg.c_str() ) ;
         retObj = bob.obj() ;
         goto error ;
      }
      rval = pRval->toBSON() ;
      // 3. adapt the result
      rc = final ( rval, retObj ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to extract result for command[%s], "
                  "rc = %d", name(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaCommand::final ( BSONObj &rval, BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele = rval.getField( "" ) ;
      BSONType type   = ele.type();
      BSONObjBuilder bob ;
      BSONObj subObj ;

      PD_LOG ( PDDEBUG, "Js return raw result for command[%s]: %s",
               name(), rval.toString(FALSE, TRUE).c_str() ) ;

      if ( Object == type || Array == type )
      {
         subObj = ele.embeddedObject() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to get the nameless field from the js"
                  "return object, rc: %d", rc ) ;
         goto error ;
      }

      bob.appendElements( subObj ) ;
      retObj = bob.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaCommand::setRuningStatus( const BSONObj& itemInfo,
                                       BSONObj& taskInfo )
   {
      return SDB_OK ;
   }

   INT32 _omaCommand::convertResult( const BSONObj& itemInfo,
                                     BSONObj& taskInfo )
   {
      return SDB_OK ;
   }

   /*
      _omaCmdAssit
   */
   _omaCmdAssit::_omaCmdAssit ( OA_NEW_FUNC pFunc )
   {
      if ( pFunc )
      {
         _omaCommand *pCommand = (*pFunc)() ;
         if ( pCommand )
         {
            getOmaCmdBuilder()->_register ( pCommand->name(), pFunc ) ;
            SDB_OSS_DEL pCommand ;
            pCommand = NULL ;
         }
      }
   }

   _omaCmdAssit::~_omaCmdAssit ()
   {
   }

   /*
      _omaCmdBuilder
   */
   _omaCmdBuilder::_omaCmdBuilder ()
   {
   }

   _omaCmdBuilder::~_omaCmdBuilder ()
   {
   }

   _omaCommand* _omaCmdBuilder::create ( const CHAR *command )
   {
      OA_NEW_FUNC pFunc = _find ( command ) ;
      if ( pFunc )
      {
         return (*pFunc)() ;
      }
      return NULL ;
   }

   void _omaCmdBuilder::release ( _omaCommand *&pCommand )
   {
      if ( pCommand )
      {
         SDB_OSS_DEL pCommand ;
         pCommand = NULL ;
      }
   }

   INT32 _omaCmdBuilder::_register ( const CHAR *name, OA_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;

      pair< MAP_OACMD_IT, BOOLEAN > ret ;
      ret = _cmdMap.insert( pair<const CHAR*, OA_NEW_FUNC>(name, pFunc) ) ;
      if ( FALSE == ret.second )
      {
         PD_LOG_MSG ( PDERROR, "Failed to register omagent command[%s], "
                      "already exist", name ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   OA_NEW_FUNC _omaCmdBuilder::_find ( const CHAR *name )
   {
      if ( name )
      {
         MAP_OACMD_IT it = _cmdMap.find( name ) ;
         if ( it != _cmdMap.end() )
         {
            return it->second ;
         }
      }
      return NULL ;
   }

   /*
      get omagent command builder
   */
   _omaCmdBuilder* getOmaCmdBuilder()
   {
      static _omaCmdBuilder cmdBuilder ;
      return &cmdBuilder ;
   }

} // namespace engine

