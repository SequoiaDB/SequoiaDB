/* *****************************************************************************
@discretion: seqDB-20546:reset接口支持创建/删除多个自增字段，覆盖所有参数
@author：2018-11-04 zhao xiaoni
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME;
testConf.clName = "cl_" + 20546;

main( test );

function test ()
{
   var curlPara = ["cmd=create autoincrement",
      "name=" + testConf.csName + "." + testConf.clName,
      "options={ AutoIncrement: [ { Field: \"field1\", Increment: 10, StartValue: 100, MinValue: 100, MaxValue: 10000, CacheSize: 100, AcquireSize: 100, Cycled: true, Generated: \"always\" },{ Field: \"field2\", Increment: 10, StartValue: 100, MinValue: 100, MaxValue: 10000, CacheSize: 100, AcquireSize: 100, Cycled: false, Generated: \"strict\" } ] }"];
   runCurl( curlPara );

   var clID = getCLID( db, testConf.csName, testConf.clName );
   var sequenceNames = ["SYS_" + clID + "_field1_SEQ", "SYS_" + clID + "_field2_SEQ"];
   var expArr = [{ Field: "field1", SequenceName: sequenceNames[0], Generated: "always" },
   { Field: "field2", SequenceName: sequenceNames[1], Generated: "strict" }];
   checkAutoIncrementonCL( db, testConf.csName, testConf.clName, expArr );

   expArr = [{ Increment: 10, StartValue: 100, CurrentValue: 100, MinValue: 100, MaxValue: 10000, CacheSize: 100, AcquireSize: 100, Cycled: true },
   { Increment: 10, StartValue: 100, CurrentValue: 100, MinValue: 100, MaxValue: 10000, CacheSize: 100, AcquireSize: 100, Cycled: false }];
   for( var i in sequenceNames )
   {
      checkSequence( db, sequenceNames[i], expArr[i] );
   }

   curlPara = ["cmd=drop autoincrement",
      "name=" + testConf.csName + "." + testConf.clName,
      "options={ Field: [ \"field1\", \"field2\" ] }"];
   runCurl( curlPara );

   var cursor = db.snapshot( 8, { Name: testConf.csName + "." + testConf.clName } );
   if( cursor.current().toObj().AutoIncrement.length !== 0 )
   {
      throw new Error( "drop autoIncrement failed!" );
   }
}

