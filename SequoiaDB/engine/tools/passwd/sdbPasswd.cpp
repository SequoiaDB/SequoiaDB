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

   Source File Name = sdbPasswd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who            Description
   ====== =========== ========== ==========================================
          11/26/2018  ZWB        Initial Draft
          13/12/2019  fangjiabin

   Last Changed =

*******************************************************************************/

#include "ossErr.h"
#include "utilPasswdTool.hpp"
#include "utilParam.hpp"
#include "pd.hpp"
#include <iostream>
#include "msgDef.h"

using namespace passwd ;

#define PASSWD_OPTIONS_HELP         ( "help" )
#define PASSWD_OPTIONS_ADDUSER      ( "adduser" )
#define PASSWD_OPTIONS_RMUSER       ( "removeuser" )
#define PASSWD_OPTIONS_CLUSTER      ( "cluster" )
#define PASSWD_OPTIONS_TOKEN        ( "token" )
#define PASSWD_OPTIONS_FILE         ( "file" )
#define PASSWD_OPTIONS_PASSWD       ( "password" )

#define COMMANDS_ADD_PARAM_OPTIONS_BEGIN(des)  des.add_options()
#define COMMANDS_ADD_PARAM_OPTIONS_END ;
#define COMMANDS_STRING(a, b) (string(a) + string(b)).c_str()

#define PASSWD_GENERAL_OPTIONS \
   (COMMANDS_STRING(PASSWD_OPTIONS_HELP, ",h"), "help") \
   (COMMANDS_STRING(PASSWD_OPTIONS_ADDUSER,",a"), po::value<string>(), "add a user")\
   (COMMANDS_STRING(PASSWD_OPTIONS_RMUSER,",r"), po::value<string>(), "remove a user")\
   (COMMANDS_STRING(PASSWD_OPTIONS_TOKEN,",t"), po::value<string>(), "password encryption token")\
   (COMMANDS_STRING(PASSWD_OPTIONS_FILE,",f"), po::value<string>(), "cipher file location, default ~/sequoiadb/passwd")\
   (COMMANDS_STRING(PASSWD_OPTIONS_PASSWD,",p"), implicit_value<string>(""), "password")

enum OP_MODE
{
   OP_ADD_USER = 0,
   OP_REMOVE_USER
} ;

#define SDBPASSWD_LOG "sdbpasswd.log"

void displayUsage()
{
   std::cout << "Usage:" << std::endl ;
   std::cout << "  sdbpasswd -a sdbadmin -p sequoiadb "
             << " # Add username and password" << std::endl;
   std::cout << "  sdbpasswd -r sdbadmin              "
             << " # Remove username and password" << std::endl << std::endl ;
}

void displayArg ( po::options_description &desc )
{
   displayUsage() ;
   std::cout << desc << std::endl ;
}

INT32 resolveArgument( INT32 argc, CHAR* argv[],
                       po::options_description &desc,
                       po::variables_map &vm,
                       string &passwd,
                       string &userFullName,
                       string &token,
                       INT32 &mode,
                       string &filePath )
{
   INT32 rc = SDB_OK ;

   COMMANDS_ADD_PARAM_OPTIONS_BEGIN ( desc )
        PASSWD_GENERAL_OPTIONS
   COMMANDS_ADD_PARAM_OPTIONS_END

   rc = engine::utilReadCommandLine( argc, argv, desc, vm, FALSE ) ;
   if ( SDB_OK != rc )
   {
      std::cerr << "Failed to read command line, rc: " << rc << std::endl ;
      goto error ;
   }

   if ( vm.count( PASSWD_OPTIONS_HELP ) )
   {
      displayArg( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if ( !vm.count( PASSWD_OPTIONS_ADDUSER ) &&
        !vm.count( PASSWD_OPTIONS_RMUSER ) )
   {
      rc = SDB_INVALIDARG ;
      std::cerr << "sdbpasswd must specify adduser or removeuser"
                << std::endl ;
      goto error ;
   }

   if ( vm.count( PASSWD_OPTIONS_ADDUSER ) &&
        vm.count( PASSWD_OPTIONS_RMUSER ) )
   {
      rc = SDB_INVALIDARG ;
      std::cerr << "sdbpasswd can't specify adduser and removeuser "
                << "at the same time" << std::endl ;
      goto error ;
   }

   if ( vm.count( PASSWD_OPTIONS_ADDUSER ) )
   {
      mode = OP_ADD_USER ;
      userFullName = vm[PASSWD_OPTIONS_ADDUSER].as<string>() ;
   }
   else if ( vm.count( PASSWD_OPTIONS_RMUSER ) )
   {
      mode = OP_REMOVE_USER ;
      userFullName = vm[PASSWD_OPTIONS_RMUSER].as<string>() ;
   }

   if ( userFullName.empty() )
   {
      rc = SDB_INVALIDARG ;
      std::cerr << "Username can't be empty" << std::endl ;
      goto error ;
   }

   if ( SDB_MAX_USERNAME_LENGTH < userFullName.size() )
   {
      rc = SDB_INVALIDARG ;
      std::cerr << "The username is too long. Its maximum length is "
                << SDB_MAX_USERNAME_LENGTH << std::endl ;
      goto error ;
   }

   if ( OP_ADD_USER == mode )
   {
      string tmpPasswd ;
      if ( vm.count( PASSWD_OPTIONS_PASSWD ) )
      {
         tmpPasswd = vm[PASSWD_OPTIONS_PASSWD].as<string>() ;
         if ( tmpPasswd.empty() )
         {
            tmpPasswd = passwd::utilPasswordTool::interactivePasswdInput() ;
         }
      }
      passwd = tmpPasswd ;

      if ( SDB_MAX_PASSWORD_LENGTH < passwd.size() )
      {
         rc = SDB_INVALIDARG ;
         std::cerr << "The password is too long. Its maximum length is "
                   << SDB_MAX_PASSWORD_LENGTH << std::endl ;
         goto error ;
      }
   }

   if ( vm.count( PASSWD_OPTIONS_TOKEN ) )
   {
      token = vm[PASSWD_OPTIONS_TOKEN].as<string>() ;

      if ( SDB_MAX_TOKEN_LENGTH < token.size() )
      {
         rc = SDB_INVALIDARG ;
         std::cerr << "The token is too long. Its maximum length is "
                   << SDB_MAX_TOKEN_LENGTH << std::endl ;
         goto error ;
      }
   }

   if ( vm.count( PASSWD_OPTIONS_FILE ) )
   {
      filePath = vm[PASSWD_OPTIONS_FILE].as<string>() ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 mainEntry( INT32 argc, CHAR **argv )
{
   INT32             rc      = SDB_OK ;
   INT32             mode    = 0 ;
   INT32             retCode = SDB_OK ;
   string            passwd ;
   string            userFullName ;
   string            token ;
   string            filePath ;
   utilCipherMgr     mgr ;
   utilCipherFile    cipherfile ;
   utilPasswordTool  passwrdTool ;
   po::variables_map vm ;
   po::options_description desc( "Command options" ) ;

   sdbEnablePD( SDBPASSWD_LOG ) ;
   setPDLevel( PDINFO ) ;

   rc = resolveArgument( argc, argv, desc, vm, passwd, userFullName, token,
                         mode, filePath ) ;
   if ( rc )
   {
      if ( SDB_PMD_HELP_ONLY == rc )
      {
         rc = SDB_OK;
      }
      else
      {
         displayArg( desc ) ;
      }
      goto done;
   }

   if ( OP_ADD_USER == mode )
   {
      rc = cipherfile.init( filePath, W_ROLE ) ;
   }
   else if ( OP_REMOVE_USER == mode )
   {
      rc = cipherfile.init( filePath, R_ROLE ) ;
   }
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDERROR, "Failed to init cipher file[%s], rc: %d",
               filePath.c_str(), rc ) ;
      goto error;
   }

   rc = mgr.init( &cipherfile ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDERROR, "Failed to get cipher text, rc: %d", rc ) ;
      goto error;
   }

   if ( OP_ADD_USER == mode )
   {
      rc = mgr.addUser( userFullName, token, passwd ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to add user[%s], rc: %d",
                  userFullName.c_str(), rc ) ;
         goto error ;
      }
   }
   else if ( OP_REMOVE_USER == mode )
   {
      rc = mgr.removeUser( userFullName, retCode ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to remove user[%s], rc: %d",
                  userFullName.c_str(), rc ) ;
         goto error ;
      }
   }

done:
   if ( SDB_OK == rc )
   {
      if ( OP_ADD_USER == mode )
      {
         PD_LOG ( PDDEBUG, "Add user[%s] to file[%s] successfuly",
                  userFullName.c_str(), filePath.c_str() ) ;
      }
      else if ( OP_REMOVE_USER == mode && SDB_OK == retCode )
      {
         PD_LOG ( PDDEBUG, "Remove user[%s] from file[%s] "
                  "successfuly", userFullName.c_str(), filePath.c_str() ) ;
      }
   }
   return rc ;
error:
   goto done ;
}

INT32 main ( INT32 argc, CHAR **argv )
{
   return mainEntry( argc, argv ) ;
}
