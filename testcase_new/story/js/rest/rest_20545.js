/* *****************************************************************************
@discretion: seqDB-20545:reset接口支持创建/删除一个自增字段，覆盖所有参数
@author：2018-11-04 zhao xiaoni
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME;
testConf.clName = "cl_" + 20545;

main( test );

function test ()
{
   var field = "field_" + 20545;
   var curlPara = ["cmd=create autoincrement",
      "name=" + testConf.csName + "." + testConf.clName,
      "options={ AutoIncrement: { Field: \"" + field + "\", Increment: 10, StartValue: 100, MinValue: 100, MaxValue: 10000, CacheSize: 100, AcquireSize: 100, Cycled: true, Generated: \"always\" } }"];
   runCurl( curlPara );

   var clID = getCLID( db, testConf.csName, testConf.clName );
   var sequenceName = "SYS_" + clID + "_" + field + "_SEQ";
   var expArr = [{ Field: field, SequenceName: sequenceName, Generated: "always" }];
   checkAutoIncrementonCL( db, testConf.csName, testConf.clName, expArr );
   var expObj = { Increment: 10, StartValue: 100, CurrentValue: 100, MinValue: 100, MaxValue: 10000, CacheSize: 100, AcquireSize: 100, Cycled: true };
   checkSequence( db, sequenceName, expObj );

   curlPara = ["cmd=drop autoincrement",
      "name=" + testConf.csName + "." + testConf.clName,
      "options={ Field: \"" + field + "\" }"];
   runCurl( curlPara );

   var cursor = db.snapshot( 8, { Name: testConf.csName + "." + testConf.clName } );
   if( cursor.current().toObj().AutoIncrement.length !== 0 )
   {
      throw new Error( "drop autoIncrement failed!" );
   }
}
