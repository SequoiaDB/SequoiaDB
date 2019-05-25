/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
/*
@description: check whether target host has installed valid db packet or not,
              then record the result to a file in /tmp/omatmp/tmp
@modify list:
   2015-1-6 Zhaobo Tan  Init
@parameter
   install_path[string]: the path of where we expect the sequoiadb programs in
@return void
*/

var FILE_NAME_ADD_HOST_CHECK_ENV = "addHostCheckEnv.js" ;
var rc                 = SDB_OK ;
var errMsg             = "" ;

var result_file        = OMA_FILE_TEMP_ADD_HOST_CHECK ;
var expect_programs    = null ;

function OMAOption()
{
   this.role   = "cm" ;
   this.mode   = "run" ;
   this.expand = true ;
}

function OMAFilter()
{
   this.type   = "sdbcm" ;
}

/* *****************************************************************************
@discretion: init
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _init()
{
   if ( SYS_LINUX == SYS_TYPE )
   {
      expect_programs = [ "sequoiadb", "sdb", "sdbcm", "sdbcmd", "sdbcmart", "sdbcmtop" ] ;
   }
   else
   {
      // TODO: windows
   }
   PD_LOG2( LOG_NONE, arguments, PDEVENT, FILE_NAME_ADD_HOST_CHECK_ENV,
            sprintf( "Begin to check add host[?]'s environment", System.getHostName() ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( LOG_NONE, arguments, PDEVENT, FILE_NAME_ADD_HOST_CHECK_ENV,
            sprintf( "Finish checking add host[?]'s environment", System.getHostName() ) ) ;
}

/* *****************************************************************************
@discretion: judge whether sequoiadb programs exist or not
@author: Tanzhaobo
@parameter
@return
   [bool]: true for exist, false for not
***************************************************************************** */
function _isProgramExist()
{
   var path = "" ;
   var program_name = "" ;
   
   try
   {
      // check
      if ( SYS_LINUX == SYS_TYPE )
      {
         // get program's path
         path = adaptPath( install_path ) + 'bin/' ;
         for( var i =1; i < expect_programs.length; i++ )
         {
            program_name = path + expect_programs[i] ;
            if( !File.exist( program_name ) )
            {
               PD_LOG2( LOG_NONE, arguments, PDWARNING, FILE_NAME_ADD_HOST_CHECK_ENV,
                        sprintf( "Program [?] does not exist in target host", program_name ) ) ;
               return false ;
            }
         }

         return true ;
      }
      else
      {
         // TODO: windows
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to judge whether programs exist or not" ;
      rc = GETLASTERROR() ;
      PD_LOG2( LOG_NONE, arguments, PDERROR, FILE_NAME_ADD_HOST_CHECK_ENV,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;

      return false ;
   }
}

function main()
{
   _init() ;

   var resultInfo     = new addHostCheckEnvResult() ;
   var installInfoObj = null ;
   var option         = null ;
   var filter         = null ;
   
   try
   {
      // get SequoiaDB install info
      try
      {
         installInfoObj = getInstallInfoObj() ;
         resultInfo[MD5]           = installInfoObj[MD5] ;
         resultInfo[SDBADMIN_USER] = installInfoObj[SDBADMIN_USER] ;
         resultInfo[INSTALL_DIR]   = installInfoObj[INSTALL_DIR] ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = sprintf( "Failed to get install info in host[?]", System.getHostName() ) ;
         PD_LOG2( LOG_NONE, arguments, PDWARNING, FILE_NAME_ADD_HOST_CHECK_ENV,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         resultInfo[MD5]           = "" ;
         resultInfo[SDBADMIN_USER] = "" ;
         resultInfo[INSTALL_DIR]   = "" ;
      }

      // get installed sdbcm's service
      try
      {
         var num = 0 ;
         option = new OMAOption() ;
         filter = new OMAFilter() ;
         if ( "undefined" != typeof(installInfoObj) && null != installInfoObj )
         {
            installPath =  adaptPath( installInfoObj[INSTALL_DIR] ) + OMA_PATH_BIN ;
            omaArr = Sdbtool.listNodes( option, filter, installPath ) ;
            num = omaArr.size() ;
            PD_LOG2( LOG_NONE, arguments, PDEVENT, FILE_NAME_ADD_HOST_CHECK_ENV,
                     sprintf( "The amount of running OM Agent in host[?] is [?]", System.getHostName(), num ) ) ;
            if ( 0 != num )
            {
               omaObj = eval( '(' + omaArr.pos() + ')' ) ;
               resultInfo[OMA_SERVICE] = omaObj[SvcName3] ;
            }
            else
            {
               option["mode"] = "local" ;
               // when no running sdbcm, get the amount of local sdbcm
               omaArr = Sdbtool.listNodes( option, filter, installPath ) ;
               num = omaArr.size() ;
               PD_LOG2( LOG_NONE, arguments, PDEVENT, FILE_NAME_ADD_HOST_CHECK_ENV,
                        sprintf( "The amount of local OM Agent in host[?] is [?]",
                                 System.getHostName(), num ) ) ;
               if ( 0 != num )
               {
                  omaObj = eval( '(' + omaArr.pos() + ')' ) ;
                  resultInfo[OMA_SERVICE] = omaObj[SvcName3] ;
               }
            }
         }
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = sprintf( "Failed to get installed OM Agent's service in host[?]", System.getHostName() ) ;
         PD_LOG2( LOG_NONE, arguments, PDWARNING, FILE_NAME_ADD_HOST_CHECK_ENV,
                 sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         resultInfo[OMA_SERVICE] = "" ;
      }

      // 2. judge programs exist or not
      resultInfo[ISPROGRAMEXIST] = _isProgramExist() ;
      
      PD_LOG2( LOG_NONE, arguments, PDEVENT, FILE_NAME_ADD_HOST_CHECK_ENV,
               sprintf( "Result is: ?", JSON.stringify(resultInfo) ) ) ;

      // 3. write the result to file
      try
      {
         if ( File.exist( result_file ) )
            File.remove( result_file ) ;
         file = new File( result_file ) ;
         file.close() ;
         Oma.setOmaConfigs( resultInfo, result_file )
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = sprintf( "Failed to write result to file[?] in host[?]",
                           result_file, System.getHostName() ) ;
         PD_LOG2( LOG_NONE, arguments, PDERROR, FILE_NAME_ADD_HOST_CHECK_ENV,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = sprintf( "Failed to check add host[?]'s environment", System.getHostName() ) ;
      PD_LOG2( LOG_NONE, arguments, PDERROR, FILE_NAME_ADD_HOST_CHECK_ENV,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   
   _final() ;
}

// execute
   main() ;

