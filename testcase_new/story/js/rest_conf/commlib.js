/****************************************************
@description:	commlib of basic		
@modify list:
                     2015-4-3 Ting YU init
****************************************************/

import( "../lib/rest_commlib.js" );

/*****************************************************************
@description:	open ssl, use https
@pre-condition: usessl=true in node config
@modify list:	
				2017-11-22 XiaoNi Huang init 
 *****************************************************************/
function runCurlByHttps ( curlPara )
{
   if( undefined == curlPara ) 
   {
      throw new Error( "curlPara can not be undefined" );
   }
   if( 'string' == typeof ( COORDSVCNAME ) ) { COORDSVCNAME = parseInt( COORDSVCNAME, 10 ); }
   if( 'number' != typeof ( COORDSVCNAME ) ) { throw new Error( "COORDSVCNAME type, expect: number, actual: " + typeof ( COORDSVCNAME ) ); }
   str = "curl -k https://" + COORDHOSTNAME + ":" + ( COORDSVCNAME + 4 ) + "/ -d '";
   for( var i = 0; i < curlPara.length; i++ )
   {
      str += curlPara[i];
      if( i < ( curlPara.length - 1 ) )
      {
         str += "&";
      }
   }
   str += "' 2>>" + WORKDIR + "runCurlByHttps.txt"; // to get curl command
   com.run( "date >> " + WORKDIR + "runCurlByHttps.txt" );
   com.run( "echo '" + curlPara + "' >>" + WORKDIR + "runCurlByHttps.txt" );
   info = com.run( str ); // to get info
   infoSplit = info.replace( /\}\{/g, "\}\n\{" ).split( "\n" );
   var tem = JSON.parse( infoSplit[0] );
   errno = tem.errno; // to get errno [int]
}

/***************************************************************
@description:	to get current function name, use https
@modify list:	
				2017-11-22 XiaoNi Huang init 
***************************************************************/
function tryCatchByHttps ( curlPara, throwCond, throwOutput )
{
   if( undefined == curlPara )
   {
      throw new Error( "tryCatchByHttps message:" + throwOutput );
   }
   if( undefined == throwCond ) { throwCond = []; }
   if( undefined == throwOutput ) { throwOutput = ""; }
   var condFlag = true;
   runCurlByHttps( curlPara );

   for( var i = 0; i < throwCond.length; i++ )
   {
      condFlag = condFlag && errno != throwCond[i];
   }
   if( condFlag ) { throw new Error( "command: " + str + "\n" + "return: " + info ); }
}
