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

   Source File Name = sdblobtool.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pd.hpp"
#include "migLobTool.hpp"
#include "utilParam.hpp"
#include "pmdDef.hpp"
#include "ossVer.hpp"
#include "utilPasswdTool.hpp"
#include "utilCommon.hpp"
#include <iostream>

using namespace std ;

#define LOG_FILE "sdblobtool.log"

vector<string> passwdVec ;

#define COMMANDS_OPTIONS \
        ( PMD_COMMANDS_STRING("help", ",h"), "Help" )\
        ( PMD_COMMANDS_STRING("version", ",v"), "Version" )\
        ( MIG_HOSTNAME, boost::program_options::value<string>(), "The host name of coord. Default value is localhost." )\
        ( MIG_SERVICE, boost::program_options::value<string>(), "The service name of coord. Default value is \"11810\"." )\
        ( MIG_USRNAME, boost::program_options::value<string>(), "Username" )\
        ( MIG_PASSWD, boost::program_options::value< vector<string> >(&passwdVec)->multitoken()->zero_tokens(), "Password" )\
        ( MIG_CIPHERFILE, boost::program_options::value<string>(), "cipher file location, default ~/sequoiadb/passwd" )\
        ( MIG_CIPHER, boost::program_options::value<bool>(), "input password using a cipher file" )\
        ( MIG_TOKEN, boost::program_options::value<string>(), "password encryption token" )\
        ( MIG_OP, boost::program_options::value<string>(), "import/export/migration" )\
        ( MIG_CL, boost::program_options::value<string>(), "Full name of collection, eg:\"foo.bar\"" )\
        ( MIG_FILE, boost::program_options::value<string>(), "Full path of file" )\
        ( MIG_IGNOREFE, "When operation is \"import\" or \"migration\", skip the lob which exists in collection" )\
        ( MIG_DST_HOST, boost::program_options::value<string>(), "The hostname of destination coord. Default value is localhost.(Specify it when use migration)" )\
        ( MIG_DST_SERVICE, boost::program_options::value<string>(), "The service name of destination coord. Default value is localhost.(Specify it when use migration)" )\
        ( MIG_DST_USRNAME, boost::program_options::value<string>(), "Destination username(Specify it when use migration)" )\
        ( MIG_DST_PASSWD, boost::program_options::value<string>(), "Destination password(Specify it when use migration)" )\
        ( MIG_DST_CL, boost::program_options::value<string>(), "Destination collection(Specify it when use migration)" )\
        ( MIG_SESSION_PREFER, boost::program_options::value<string>(), "Indicate which instance to respond export request in current session. (\"m\"/\"M\"/\"s\"/\"S\"/\"a\"/\"A\"/1-7 default is \"M\")" ) \

static void initDesc( po::options_description &desc )
{
   PMD_ADD_PARAM_OPTIONS_BEGIN ( desc )
      COMMANDS_OPTIONS
#ifdef SDB_SSL
      ( MIG_SSL, "use SSL connection" )
#endif
   PMD_ADD_PARAM_OPTIONS_END
}

static INT32 parseCmdLine( const po::options_description &desc,
                           const po::variables_map &vm,
                           bson::BSONObj &obj,
                           BOOLEAN &doNothing,
                           INT32 argc )
{
   INT32 rc = SDB_OK ;
   bson::BSONObjBuilder builder ;
   doNothing = FALSE ;
   std::string optype ;
   BOOLEAN isMig = FALSE ;
   std::string cipherfile ;
   std::string token;

   if ( vm.count( "help" ) )
   {
      cout << desc << endl ;
      doNothing = TRUE ;
      goto done ;
   }

   if ( vm.count( "version" ) )
   {
      ossPrintVersion("sdblobtool version") ;
      doNothing = TRUE ;
      goto done ;
   }

   if ( !vm.count( MIG_HOSTNAME ) )
   {
      builder.append( MIG_HOSTNAME, "localhost" ) ;
   }
   else
   {
      builder.append( MIG_HOSTNAME, vm[MIG_HOSTNAME].as<string>() ) ;
   }

   if ( !vm.count( MIG_SERVICE ) )
   {
      builder.append( MIG_SERVICE, "11810" ) ;
   }
   else
   {
      builder.append( MIG_SERVICE, vm[MIG_SERVICE].as<string>() ) ;
   }

   if ( vm.count( MIG_CIPHERFILE ) )
   {
      cipherfile = vm[MIG_CIPHERFILE].as<string>() ;
   }

   if ( vm.count( MIG_TOKEN ) )
   {
      token = vm[MIG_TOKEN].as<string>() ;
   }

   if ( vm.count( MIG_USRNAME ) )
   {
      string user = vm[MIG_USRNAME].as<string>();

      if ( vm.count( MIG_PASSWD ) )
      {
         string  passwd ;
         BOOLEAN isNormalInput = FALSE ;

         if ( 0 == passwdVec.size() )
         {
            isNormalInput = utilPasswordTool::interactivePasswdInput( passwd ) ;
         }
         else
         {
            isNormalInput = TRUE ;
            passwd = passwdVec[0] ;
         }

         if ( !isNormalInput )
         {
            rc = SDB_APP_INTERRUPT ;
            std::cerr << getErrDesp( rc ) << ", rc: " << rc << std::endl ;
            goto error ;
         }

         builder.append( MIG_USRNAME, user ) ;
         builder.append( MIG_PASSWD, passwd ) ;
      }
      else
      {
         utilPasswordTool passwdTool ;
         string passwd ;

         if ( vm.count(MIG_CIPHER) && vm[MIG_CIPHER].as<bool>() )
         {
            string connectionUserName ;

            rc = passwdTool.getPasswdByCipherFile( user, token,
                                                   cipherfile,
                                                   passwd ) ;
            if ( SDB_OK != rc )
            {
               std::cerr << "Failed to get user[" << user.c_str()
                         << "] password from cipher file"
                         << "[" << cipherfile.c_str() << "], rc: " << rc
                         << std::endl ;
               PD_LOG( PDERROR, "Failed to get user[%s] password from cipher"
                       " file[%s], rc: %d", user.c_str(),
                       cipherfile.c_str(), rc ) ;
               goto error ;
            }
            connectionUserName = utilGetUserShortNameFromUserFullName( user ) ;
            builder.append( MIG_USRNAME, connectionUserName ) ;
            builder.append( MIG_PASSWD, passwd ) ;
         }
         else
         {
            builder.append( MIG_USRNAME, user ) ;
            if ( vm.count(MIG_TOKEN) || vm.count(MIG_CIPHERFILE) )
            {
               std::cout << "If you want to use cipher text, you should use"
                         << " \"--cipher true\"" << std::endl ;
            }
         }
      }
   }
   else
   {
      builder.append( MIG_USRNAME, "" ) ;
      vm.count( MIG_PASSWD )?
      builder.append( MIG_PASSWD, vm[MIG_PASSWD].as<string>() ):
      builder.append( MIG_PASSWD, "");
   }

   if ( !vm.count( MIG_OP ) )
   {
      PD_LOG( PDERROR, "operation type is not specified" ) ;
      cerr << "Error: operation type must be specified" << endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   else
   {
      optype = vm[MIG_OP].as<string>() ;
      builder.append( MIG_OP, optype ) ;
      isMig = ( 0 == optype.compare( MIG_OP_MIGRATION ) ) ;
   }

   if ( !vm.count( MIG_CL ) )
   {
      PD_LOG( PDERROR, "collection is not specified" ) ;
      cerr << "Error: collection must be specified" << endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   builder.append( MIG_CL, vm[MIG_CL].as<string>() ) ;

   if ( !isMig )
   {
      if ( !vm.count( MIG_FILE ) )
      {
         PD_LOG( PDERROR, "local file is not specified" ) ;
         cerr << "Error: local file must be specified" << endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      builder.append( MIG_FILE, vm[MIG_FILE].as<string>() ) ;
   }

   builder.appendBool( MIG_IGNOREFE, vm.count( MIG_IGNOREFE ) ) ;

   if ( !vm.count( MIG_DST_HOST ) )
   {
      builder.append( MIG_DST_HOST, "localhost" ) ;
   }
   else
   {
      builder.append( MIG_DST_HOST, vm[MIG_DST_HOST].as<string>() ) ;
   }

   if ( !vm.count( MIG_DST_SERVICE ) )
   {
      builder.append( MIG_DST_SERVICE, "11810" ) ;
   }
   else
   {
      builder.append( MIG_DST_SERVICE, vm[MIG_DST_SERVICE].as<string>() ) ;
   }

   if ( vm.count( MIG_DST_USRNAME ) )
   {
      builder.append( MIG_DST_USRNAME, vm[MIG_DST_USRNAME].as<string>() ) ;
   }
   else
   {
      builder.append( MIG_DST_USRNAME, "" ) ;
   }

   if ( vm.count( MIG_DST_PASSWD ) )
   {
      builder.append( MIG_DST_PASSWD, vm[MIG_DST_PASSWD].as<string>() ) ;
   }
   else
   {
      builder.append( MIG_DST_PASSWD, "" ) ;
   }

   if ( vm.count( MIG_SESSION_PREFER ) )
   {
      std::string prefer = vm[MIG_SESSION_PREFER].as<string>() ;
      UINT32 preferNum = 0 ;
      try
      {
         preferNum = boost::lexical_cast<UINT32>( prefer ) ;
         builder.append( FIELD_NAME_PREFERRED_INSTANCE_LEGACY, preferNum ) ;
      }
      catch ( boost::bad_lexical_cast & )
      {
         builder.append( FIELD_NAME_PREFERRED_INSTANCE_LEGACY, prefer ) ;
      }
   }
   else
   {
       builder.append( FIELD_NAME_PREFERRED_INSTANCE_LEGACY, "M" ) ;
   }

   if ( isMig )
   {
      if ( vm.count( MIG_DST_CL ) )
      {
         builder.append( MIG_DST_CL, vm[MIG_DST_CL].as<string>() ) ;
      }
      else
      {
         cout << "Error: destination collection must be specified" << endl ;
         PD_LOG( PDERROR, "destination collection must be specified" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

#ifdef SDB_SSL
   builder.appendBool( MIG_SSL, vm.count( MIG_SSL ) ) ;
#endif

   obj = builder.obj() ;
done:
   return rc ;
error:
   goto done ;
}

INT32 main( INT32 argc, CHAR *argv[] )
{
   INT32 rc = SDB_OK ;
   INT32 errcode = SDB_OK ;
   po::options_description desc ( "Command options" ) ;
   po::variables_map vm ;
   bson::BSONObj options ;
   BOOLEAN doNothing = FALSE ;
   lobtool::migLobTool tool ;

   sdbEnablePD( LOG_FILE ) ;

   PD_LOG ( PDEVENT ,
            "Start sdblobtool [Ver: %d.%d, Release: %d, Build: %s]...",
            SDB_ENGINE_VERISON_CURRENT,
            SDB_ENGINE_SUBVERSION_CURRENT,
            SDB_ENGINE_RELEASE_CURRENT,
            SDB_ENGINE_BUILD_TIME ) ;

   initDesc( desc ) ;

   rc = engine::utilReadCommandLine( argc, argv, desc, vm, FALSE ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "invalid arguments" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = parseCmdLine( desc, vm, options, doNothing, argc ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "invalid arguments" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   PD_LOG( PDEVENT, "options:%s",
           options.toString( FALSE, TRUE ).c_str() ) ;

   if ( doNothing )
   {
      goto done ;
   }

   rc = tool.exec( options ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to execute operation:%d", rc ) ;
      goto error ;
   }
done:
   if ( SDB_OK != rc )
   {
      errcode = rc ;
      rc = engine::utilRC2ShellRC( rc ) ;
   }

   PD_LOG( PDEVENT, "sdblobtool quit. rc: %d, shell rc: %d.",
           errcode, rc ) ;
   return rc ;
error:
   cerr << "Error: failed to complete operation, rc:" << rc
        << " Error Desc: " << getErrDesp(rc ) << endl ;
   cerr << "Error: get details in log file: " << LOG_FILE << endl ;
   goto done ;
}

