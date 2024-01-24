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
@description: sync business configure ( sequoiadb )
@modify list:
   2017-07-14 JiaWen He  Init
   2019-05-28 JiaWen He  Modify
*/

function main()
{
   var PD_LOGGER = new Logger( "syncBusinessConf.js" ) ;

   PD_LOGGER.log( PDEVENT, "Begin to sync business configure" ) ;

   var businessType = BUS_JSON[FIELD_BUSINESS_TYPE] ;
   var importFile   = '../conf/script/syncBusinessConf/' + businessType + '.js' ;

   if ( false == File.exist( importFile ) )
   {
      var error = new SdbError( SDB_INVALIDARG,
                                sprintf( "Invalid business type [?]",
                                         businessType ) ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   import( importFile ) ;

   var result = run() ;

   PD_LOGGER.log( PDEVENT, "finish sync business configure" ) ;

   return result ;
}

main() ;
 