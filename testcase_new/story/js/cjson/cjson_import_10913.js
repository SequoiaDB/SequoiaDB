/************************************************************************
*@Description:   seqDB-10913:导入json文件，其中numberlong类型数据超过long类型数据最大值                                 
*@Author:        2017-1-5  wuyan
************************************************************************/
var clName = COMMCLNAME + "_10913";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //-------------test a：--------------------------
   //import datas          
   var imprtFile = tmpFileDir + "10913.json";
   var srcDatas = "{a:9223372036854775808}\n{b:-9223372036854775809}\n"
      + "{c:9223372036854775807}\n{d:-9223372036854775808}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"a":{"$decimal":"9223372036854775808"}},{"b":{"$decimal":"-9223372036854775809"}},'
      + '{"c":{"$numberLong":"9223372036854775807"}},{"d":{"$numberLong":"-9223372036854775808"}}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}