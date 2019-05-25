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
@description: create relationship
@modify list:
   2017-10-16 JiaWen He  Init
*/

function main()
{
   var PD_LOGGER = new Logger( "createRelationship.js" ) ;

   PD_LOGGER.log( PDEVENT, "Begin to create relationship" ) ;

   var fromBuz      = BUS_JSON[FIELD_FROM] ;
   var fromBuzInfo  = fromBuz[FIELD_INFO] ;
   var businessType = fromBuzInfo[FIELD_BUSINESS_TYPE] ;
   var importFile   = '../conf/script/createRelationship/' + businessType + '.js' ;

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

   PD_LOGGER.log( PDEVENT, "Finish to create relationship" ) ;

   return result ;
}

main() ;