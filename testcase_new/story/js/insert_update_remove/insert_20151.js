/******************************************************************************
*@Description : seqDB-20151: 插入数据，不指定flag，插入重复数据  
                seqDB-20152: 单条插入，指定FLG_INSERT_CONTONDUP，插入重复数据
                seqDB-20153: 批量插入，指定FLG_INSERT_CONTONDUP，插入重复数据 
                seqDB-20154: 单条插入，指定FLG_INSERT_REPLACEONDUP，插入重复数据
                seqDB-20155: 批量插入，指定FLG_INSERT_REPLACEONDUP，插入重复数据  
*@Author      : 2020-01-09  Zhao xiaoni Init
******************************************************************************/
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_20151";

main( test );

function test ()
{
   var cl = db.getCS( testConf.csName ).getCL( testConf.clName );

   var idStartData = 0;
   var aStartData = 0;
   var dataNum = 10;
   var doc = getBulkData( dataNum, idStartData, aStartData );
   cl.insert( doc );

   //单条插入唯一索引重复的数据，flag为不指定 
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( { "_id": 0, "a": 0 } );
   } );

   //单条插入唯一索引重复的数据，flag为SDB_INSERT_CONTONDUP
   var expRecs = { "InsertedNum": 0, "DuplicatedNum": 1, "ModifiedNum": 0 };
   var actRecs = cl.insert( { "_id": 0, "a": 1 }, SDB_INSERT_CONTONDUP ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   var cursor = cl.find();
   commCompareResults( cursor, doc, false );

   //单条插入唯一索引重复的数据，flag为SDB_INSERT_REPLACEONDUP
   var duplicateRecord = { "_id": 0, "a": 1 };
   expRecs = { "InsertedNum": 0, "DuplicatedNum": 1, "ModifiedNum": 1 };
   actRecs = cl.insert( duplicateRecord, SDB_INSERT_REPLACEONDUP ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   doc.shift();
   doc.unshift( duplicateRecord );
   var cursor = cl.find();
   commCompareResults( cursor, doc, false );

   //批量插入唯一索引重复的数据，flag为不指定
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( doc );
   } );

   //批量插入唯一索引重复的数据，flag为SDB_INSERT_CONTONDUP
   dataNum = 20;
   idStartData = 0;
   aStartData = 10;
   var insertRecords = getBulkData( dataNum, idStartData, aStartData );
   expRecs = { "InsertedNum": dataNum / 2, "DuplicatedNum": dataNum / 2, "ModifiedNum": 0 };
   actRecs = cl.insert( insertRecords, SDB_INSERT_CONTONDUP ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   insertRecords.splice( 0, dataNum / 2 );
   doc = doc.concat( insertRecords );
   var cursor = cl.find();
   commCompareResults( cursor, doc, false );

   //批量插入唯一索引重复的数据，flag为SDB_INSERT_REPLACEONDUP
   idStartData = 10;
   aStartData = 30;
   insertRecords = getBulkData( dataNum, idStartData, aStartData );
   actRecs = cl.insert( insertRecords, SDB_INSERT_REPLACEONDUP ).toObj();
   expRecs = { "InsertedNum": dataNum / 2, "DuplicatedNum": dataNum / 2, "ModifiedNum": dataNum / 2 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   doc.splice( dataNum / 2, dataNum );
   doc = doc.concat( insertRecords );
   var cursor = cl.find();
   commCompareResults( cursor, doc, false );
}
