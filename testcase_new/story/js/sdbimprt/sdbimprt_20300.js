/************************************************************************
*@Description: seqDB-20300:导入字段值为特殊字符的json类型数据
*@Author: 2019-11-26 Zhao xiaoni Init
************************************************************************/
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_20300";

main( test );

function test ( testPara )
{
   var imprtFile = tmpFileDir + "20300.json";
   readyData( imprtFile );
   var rcResults = importData( testConf.csName, testConf.clName, imprtFile, "json" );
   checkImportRC( rcResults, 4, 4, 0 );
   //由于不可见字符导入到集合后的记录不一定以“?”的形式显示，因此不校验最后一条记录
   var expResult = [{ "a": "&" }, { "b": "$" }, { "c": "^" }];
   checkCLData( testPara.testCL, expResult );
}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( "{_id: 1, a: \"&\"}" );
   file.write( "{_id: 2, b: \"$\"}" );
   file.write( "{_id: 3, c: \"^\"}" );
   var str = String.fromCharCode( 0x1B );
   file.write( "{_id: 4, d: \"" + str + "\"}" );
   file.close();
}

function checkCLData ( cl, expResult )
{
   var actResult = [];
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "_id": 1 } );
   while( cursor.next() )
   {
      actResult.push( cursor.current().toObj() );
   }
   for( var i in expResult )
   {
      if( JSON.stringify( expResult[i] ) !== JSON.stringify( actResult[i] ) )
      {
         throw new Error( "actResult: " + JSON.stringify( actResult[i] ) + ", expResult: " + JSON.stringify( expResult[i] ) );
      }
   }
}
