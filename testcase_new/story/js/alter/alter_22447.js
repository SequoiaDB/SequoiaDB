/******************************************************************************
*@Description :   seqDB-22447:主表修改compress属性会修改到子表
*@author:      liyuanyue
*@createdate:  2020.07.10
******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "cl_22447";
main( test );

function test ()
{
   var testcaseID = 22447;
   var csName = COMMCSNAME;
   var dataGroupName = commGetDataGroupNames( db )[0];
   var mainclName = CHANGEDPREFIX + testcaseID + "_maincl";
   var sub1clName = CHANGEDPREFIX + testcaseID + "_sub1cl";
   var sub2clName = CHANGEDPREFIX + testcaseID + "_sub2cl";
   var sub3clName = CHANGEDPREFIX + testcaseID + "_sub3cl";

   commDropCL( db, csName, mainclName );
   commDropCL( db, csName, sub1clName );
   commDropCL( db, csName, sub2clName );
   commDropCL( db, csName, sub3clName );

   var maincl = commCreateCL( db, csName, mainclName, { "ShardingKey": { "a": 1 }, "IsMainCL": true } );
   var sub1cl = commCreateCL( db, csName, sub1clName, { "Group": dataGroupName, "Compressed": false } );
   var sub2cl = commCreateCL( db, csName, sub2clName, { "Group": dataGroupName, "Compressed": true, "CompressionType": 'snappy' } );
   var sub3cl = commCreateCL( db, csName, sub3clName, { "Group": dataGroupName, "Compressed": true, "CompressionType": 'lzw' } );

   maincl.attachCL( csName + "." + sub1clName, { "LowBound": { "a": 1 }, "UpBound": { "a": 10 } } );
   maincl.attachCL( csName + "." + sub2clName, { "LowBound": { a: 10 }, "UpBound": { a: 20 } } );
   maincl.attachCL( csName + "." + sub3clName, { "LowBound": { a: 20 }, "UpBound": { a: 30 } } );

   // alter Compressed:false
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      maincl.alter( { "Compressed": false } );
   });

   // 检查主备一致
   commCheckLSN( db, dataGroupName, 600 );

   //var cond = { Name: { $regex: csName + "." + CHANGEDPREFIX + testcaseID } };
   //checkCompressedFalse( db, cond, dataGroupName );

   // recovery attribute
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      sub1cl.alter( { "Compressed": false } );
   });
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      sub2cl.alter( { "Compressed": true, "CompressionType": 'snappy' } );
   });
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      sub3cl.alter( { "Compressed": true, "CompressionType": 'lzw' } );
   });

   // alter CompressionType:'lzw'
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      maincl.alter( { CompressionType: 'lzw' } );
   });

   // 检查主备一致
   commCheckLSN( db, dataGroupName, 600 );

   //checkCompressedTrue( db, cond, dataGroupName );

   commDropCL( db, csName, mainclName );
   commDropCL( db, csName, sub1clName );
   commDropCL( db, csName, sub2clName );
   commDropCL( db, csName, sub3clName );
}
function checkCompressedFalse ( db, cond, dataGroupName )
{
   var masterDB = db.getRG( dataGroupName ).getMaster().connect();
   var slaveDB = db.getRG( dataGroupName ).getSlave().connect();

   var expectAttr = "AttributeDesc";
   var expectValue = "";
   checkCATALOG( db, cond, expectAttr, expectValue );

   var expectAttr = "Attribute";
   var expectValue = "";
   checkCOLLECTIONS( masterDB, cond, expectAttr, expectValue )

   var expectAttr = "Attribute";
   var expectValue = "";
   checkCOLLECTIONS( slaveDB, cond, expectAttr, expectValue )
}
function checkCompressedTrue ( db, cond, dataGroupName )
{
   var masterDB = db.getRG( dataGroupName ).getMaster().connect();
   var slaveDB = db.getRG( dataGroupName ).getSlave().connect();

   var expectAttr = "CompressionTypeDesc";
   var expectValue = "lzw";
   checkCATALOG( db, cond, expectAttr, expectValue );

   var expectAttr = "Attribute";
   var expectValue = "Compressed";
   checkCOLLECTIONS( masterDB, cond, expectAttr, expectValue )

   var expectAttr = "Attribute";
   var expectValue = "Compressed";
   checkCOLLECTIONS( slaveDB, cond, expectAttr, expectValue )
}

function checkCATALOG ( db, cond, expectAttr, expectValue )
{
   var cur = db.snapshot( SDB_SNAP_CATALOG, cond );
   while( cur.next() )
   {
      var obj = cur.current().toObj();
      for( var att in obj )
      {
         if( att == expectAttr && obj[att] !== expectValue )
         {
            throw new Error( "expect " + expectAttr + " is " + expectValue + "\n"
               + "actually value is :" + obj[att] + "\n" );
         }
      }
   }
}

function checkCOLLECTIONS ( db, cond, expectAttr, expectValue )
{
   var cur = db.snapshot( SDB_SNAP_COLLECTIONS, cond );
   while( cur.next() )
   {
      var obj = cur.current().toObj();
      var details = obj["Details"][0];
      for( var att in details )
      {
         if( att == expectAttr && details[att] !== expectValue )
         {
            throw new Error( "expect " + expectAttr + " is " + expectValue + "\n"
               + "actually value is :" + details[att] + "\n" );
         }
      }
   }
}
