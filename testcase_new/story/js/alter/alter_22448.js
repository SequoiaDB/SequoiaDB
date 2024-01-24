/******************************************************************************
*@Description :   seqDB-22448:主表修改strictDataMode属性会修改到子表
*@author:      liyuanyue
*@createdate:  2020.07.10
******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var testcaseID = 22448;
   var csName = COMMCSNAME;
   var dataGroupName = commGetDataGroupNames( db )[0];
   var mainclName = CHANGEDPREFIX + testcaseID + "_maincl";
   var subclName = CHANGEDPREFIX + testcaseID + "_sub1cl";

   commDropCL( db, csName, mainclName );
   commDropCL( db, csName, subclName );

   var maincl = commCreateCL( db, csName, mainclName, { "ShardingKey": { "a": 1 }, "IsMainCL": true, "Compressed": false } );
   commCreateCL( db, csName, subclName, { "Group": dataGroupName, "Compressed": false } );
   maincl.attachCL( csName + "." + subclName, { "LowBound": { "a": 1 }, "UpBound": { "a": 10 } } );
   var cond = { Name: { $regex: csName + "." + CHANGEDPREFIX + testcaseID } };

   maincl.alter( { "StrictDataMode": true } );
   // 检查主备一致
   commCheckLSN( db, dataGroupName, 600 );

   checkStrictDataModeTrue( db, cond, dataGroupName );

   maincl.alter( { "StrictDataMode": false } );

   // 检查主备一致
   commCheckLSN( db, dataGroupName, 600 );

   checkStrictDataModeFalse( db, cond, dataGroupName );

   commDropCL( db, csName, mainclName );
}
function checkStrictDataModeTrue ( db, cond, dataGroupName )
{
   var masterDB = db.getRG( dataGroupName ).getMaster().connect();
   var slaveDB = db.getRG( dataGroupName ).getSlave().connect();

   var expectAttr = "AttributeDesc";
   var expectValue = "StrictDataMode";
   checkCATALOG( db, cond, expectAttr, expectValue );

   var expectAttr = "Attribute";
   var expectValue = "StrictDataMode";
   checkCOLLECTIONS( masterDB, cond, expectAttr, expectValue )

   var expectAttr = "Attribute";
   var expectValue = "StrictDataMode";
   checkCOLLECTIONS( slaveDB, cond, expectAttr, expectValue )
}
function checkStrictDataModeFalse ( db, cond, dataGroupName )
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