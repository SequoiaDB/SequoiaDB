/*******************************************************************************

   Copyright (C) 2012-2018 SequoiaDB Ltd.

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
@description: deploy package
@modify list:
   2017-09-12 JiaWen He  Init
*/

function main()
{
   var PD_LOGGER = new Logger( "deployPackage.js" ) ;

   PD_LOGGER.logComm( PDEVENT, sprintf( "Begin to deploy package, Step [?]",
                                        SYS_STEP ) ) ;

   var taskInfo    = BUS_JSON[FIELD_INFO] ;
   var packageName = taskInfo[FIELD_PACKAGE_NAME] ;
   var importFile  = '../conf/script/deployPackage/' + packageName + '.js' ;

   if ( false == File.exist( importFile ) )
   {
      var error = new SdbError( SDB_INVALIDARG,
                                sprintf( "Invalid package name [?]",
                                         packageName ) ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      throw error ;
   }

   import( importFile ) ;

   return run() ;
}

main() ;