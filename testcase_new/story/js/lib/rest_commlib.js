/****************************************************
@description:   commlib of basic		
@modify list:
                2015-4-3 Ting YU init
****************************************************/

import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

var com = new Cmd();              //com.run("command");
var info;                             //returned information after curl command
var errno;                            //errno in returned information
var infoSplit = new Array();          //split info to array 
var str;

/*****************************************************************
@description:   run CURL command, to get return errno by rest	
@input:         eg:curlPara=["cmd=query","name=foo.bar"]
                run("curl http://127.0.0.1:11814/ -d "cmd=query&name=foo.bar" 2>/dev/nu")
@expectation:   rest return message:{ "errno": 0 }{ "_id": { "$oid": "551d0486e9c0d31814000009" }, "age": 2 }
                info={ "errno": 0 }{ "_id": { "$oid": "551d0486e9c0d31814000009" }, "age": 2 }
                errno=0
                infosplit=[{ "errno": 0 },{ "_id": { "$oid": "551d0486e9c0d31814000009" }, "age": 2 }]				
@modify list:
                2015-3-30 Ting YU init
******************************************************************/
function runCurl ( curlPara )
{
   if( undefined == curlPara ) 
   {
      throw new Error( "curlPara can not be undefined " );
   }
   if( 'string' == typeof ( COORDSVCNAME ) ) { COORDSVCNAME = parseInt( COORDSVCNAME, 10 ); }
   if( 'number' != typeof ( COORDSVCNAME ) ) { throw new Error( "typeof ( COORDSVCNAME ): " + typeof ( COORDSVCNAME ) ); }
   str = "curl http://" + COORDHOSTNAME + ":" + ( COORDSVCNAME + 4 )
      + "/ -d '";
   for( var i = 0; i < curlPara.length; i++ )
   {
      str += curlPara[i];
      if( i < ( curlPara.length - 1 ) )
      {
         str += "&";
      }
   }
   str += "' 2>/dev/null"; // to get curl command
   info = com.run( str ); // to get info
   infoSplit = info.replace( /\}\{/g, "\}\n\{" ).split( "\n" );
   var tem = JSON.parse( infoSplit[0] );
   errno = tem.errno; // to get errno [int]
}
/*****************************************************************
@description:   run CURL command, if return errno != throwCond, throw out error	
@input:         curlPara=["cmd=create collectionspace","name=foo"]
                throwCond=[0,-33]
                throwOutput="Failed to create cs!"
@expectation:   if return errno == 0 or -33, NO error to be throwed out.
                if return errno !=0 and -33, throw "Faied to create cs!".
@modify list:	
                2015-3-30 Ting YU init
******************************************************************/
function tryCatch ( curlPara, throwCond, throwOutput )
{
   if( undefined == throwCond ) { throwCond = []; }
   if( undefined == throwOutput ) { throwOutput = ""; }
   var condFlag = true;
   runCurl( curlPara );

   for( var i = 0; i < throwCond.length; i++ )
   {
      condFlag = condFlag && errno != throwCond[i];
   }
   if( condFlag ) { throw new Error( "command: " + str + "\n" + "return: " + info ); }
}

/***************************************************************
@description:   to get current function name
@modify list:
                2015-4-6 Ting YU init 
***************************************************************/
function getFuncName ()
{
   var func = getFuncName.caller.toString();
   var re = /function\s*(\w*)/i;
   var funcName = re.exec( func );
   return funcName[1] + "()";
}

function insertDataWithIndex ( cl )
{
   var recs = [];
   var recNum = 2000;
   var randomStr = getString( 4096 );
   for( var i = 0; i < recNum; i++ )
   {
      recs.push( { a: 0, b: randomStr } );
   }
   for( var i = 0; i < recNum; i++ )
   {
      var value = parseInt( Math.random() * 100 );
      recs.push( { a: value } );
   }
   cl.insert( recs );
   cl.createIndex( 'aIndex', { a: 1 } );
}

function checkScanTypeByExplain ( cl, expScanType, cond )
{
   if( cond === undefined ) { cond = { a: 0 }; }
   var cursor = cl.find( cond ).explain( { Run: true } );
   var actScanType = cursor.next().toObj().ScanType;
   cursor.close();
   if( expScanType !== actScanType )
   {
      throw new Error( "expect: " + expScanType + ", actual: " + actScanType );
   }
}

function getString ( length )
{
   var str = '';
   var baseStr = "adcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
   var count = parseInt( length / baseStr.length );
   var remainder = length % baseStr.length;
   for( var i = 0; i < count; i++ )
   {
      str += baseStr;
   }
   return str + baseStr.substring( 0, remainder );
}

