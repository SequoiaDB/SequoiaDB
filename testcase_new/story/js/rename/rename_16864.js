/************************************
*@Description: access plan should be cleared when the old mainCS name be rename
*@author:      luweikang
*@createDate:  2019.04.09
*@testlinkCase:seqDB-16864
**************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainCSName = CHANGEDPREFIX + "maincs16864";
   var newMainCSName = CHANGEDPREFIX + "newmaincs16864";
   var subCSName = CHANGEDPREFIX + "subcs16864";
   var mainCLName = CHANGEDPREFIX + "maincl16864";
   var subCLName = CHANGEDPREFIX + "subcl16864";

   //create mainCSCL and subCSCL, query the main cs to generate an access plan
   var mainCL = db.createCS( mainCSName ).createCL( mainCLName, { "IsMainCL": true, "ReplSize": 0, "ShardingKey": { "a": 1 } } );
   var subCL = db.createCS( subCSName ).createCL( subCLName, { "ShardingKey": { b: 1 } } );

   mainCL.attachCL( subCSName + "." + subCLName, { "LowBound": { "a": 0 }, "UpBound": { "a": 10 } } );
   mainCL.insert( { "_id": 1, "a": 1, "b": 1 } );
   var cur = mainCL.find();
   while( cur.next() )
   {
      var record = cur.current();
   }
   cur.close();

   //check the old mainCS access plan is exist
   var matcher = { "CollectionSpace": mainCSName };
   var selector = { "Collection": 1 };
   var snapshotCur = db.snapshot( 11, matcher, selector );
   if( snapshotCur.next() )
   {
      var clName = snapshotCur.current().toObj().Collection;
      assert.equal( clName, mainCSName + "." + mainCLName );
      snapshotCur.close();
   }

   //rename the mainCS
   db.renameCS( mainCSName, newMainCSName );
   var newMainCL = db.getCS( newMainCSName ).getCL( mainCLName );
   cur = newMainCL.find();
   while( cur.next() )
   {
      var record = cur.current();
   }
   cur.close();

   //check the old mainCS access plan shuold be not exist
   snapshotCur = db.snapshot( 11, matcher, selector );
   assert.equal( snapshotCur.next(), null );
   snapshotCur.close();

   //check the new mainCS access plan shuold be not exist
   var newMatcher = { "CollectionSpace": newMainCSName };
   snapshotCur = db.snapshot( 11, newMatcher, selector );
   if( snapshotCur.next() )
   {
      var clName = snapshotCur.current().toObj().Collection;
      assert.equal( clName, newMainCSName + "." + mainCLName );
      snapshotCur.close();
   }

   db.dropCS( newMainCSName );
   db.dropCS( subCSName );
}