import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

var com = new Cmd();              //com.run("command");
var info;                         //returned information after curl command
var errno;                        //errno in returned information
var infoSplit = new Array();      //split info to array
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
                2023-01-06 Yang Qincheng init
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
                2023-01-06 Yang Qincheng init
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
   if( condFlag ) { throw new Error( throwOutput + ", command: " + str + "\n" + "return: " + info ); }
}