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
@description: add business
@modify list:
   2017-09-25 JiaWen He  Init
*/

function main()
{
   var PD_LOGGER = new Logger( "addBusiness.js" ) ;

   PD_LOGGER.logComm( PDEVENT, sprintf( "Begin to add business, Step [?]",
                                        SYS_STEP ) ) ;

   var taskInfo     = BUS_JSON[FIELD_INFO] ;
   var businessType = taskInfo[FIELD_BUSINESS_TYPE] ;
   var importFile   = '../conf/script/addBusiness/' + businessType + '.js' ;

   if ( false == File.exist( importFile ) )
   {
      var error = new SdbError( SDB_INVALIDARG,
                                sprintf( "Invalid business name [?]",
                                         businessType ) ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      throw error ;
   }

   import( importFile ) ;

   return run() ;
}

main() ;