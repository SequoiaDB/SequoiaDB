/******************************************************************************
*@Description : seqDB-20150 插入数据不冲突 
*@Author      : 2020-01-09  Zhao xiaoni Init
******************************************************************************/
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_20150";

main( test );

function test ()
{
   var cl = db.getCS( testConf.csName ).getCL( testConf.clName );

   //单条插入唯一索引不重复的数据，flag为不指定 
   var expRecs = { "InsertedNum": 1, "DuplicatedNum": 0, "ModifiedNum": 0 };
   var actRecs = cl.insert( { "_id": 0, "a": 0 } ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //单条插入唯一索引不重复的数据，flag为0
   actRecs = cl.insert( { "_id": 1, "a": 1 }, 0 ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //单条插入唯一索引不重复的数据，flag为SDB_INSERT_CONTONDUP 
   actRecs = cl.insert( { "_id": 2, "a": 2 }, SDB_INSERT_CONTONDUP ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //单条插入唯一索引不重复的数据，flag为SDB_INSERT_REPLACEONDUP
   actRecs = cl.insert( { "_id": 3, "a": 3 }, SDB_INSERT_REPLACEONDUP ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //单条插入唯一索引不重复的数据，flag为SDB_INSERT_RETURN_ID
   expRecs = { "_id": 4, "InsertedNum": 1, "DuplicatedNum": 0, "ModifiedNum": 0 };
   actRecs = cl.insert( { "_id": 4, "a": 4 }, SDB_INSERT_RETURN_ID ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //批量插入唯一索引不重复的数据、flag为不指定
   idStartData = 10;
   aStartData = 10
   dataNum = 10;
   doc = getBulkData( dataNum, idStartData, aStartData );
   expRecs = { "InsertedNum": dataNum, "DuplicatedNum": 0, "ModifiedNum": 0 };
   actRecs = cl.insert( doc ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //批量插入唯一索引不重复的数据、flag为0
   idStartData = 20;
   aStartData = 20;
   dataNum = 10;
   doc = getBulkData( dataNum, idStartData, aStartData );
   expRecs = { "InsertedNum": dataNum, "DuplicatedNum": 0, "ModifiedNum": 0 };
   actRecs = cl.insert( doc, 0 ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //批量插入唯一索引不重复的数据、flag为SDB_INSERT_CONTONDUP
   idStartData = 30;
   aStartData = 30;
   dataNum = 10;
   doc = getBulkData( dataNum, idStartData, aStartData );
   expRecs = { "InsertedNum": dataNum, "DuplicatedNum": 0, "ModifiedNum": 0 };
   actRecs = cl.insert( doc, SDB_INSERT_CONTONDUP ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //批量插入唯一索引不重复的数据、flag为SDB_INSERT_REPLACEONDUP
   idStartData = 40;
   aStartData = 40;
   dataNum = 10;
   doc = getBulkData( dataNum, idStartData, aStartData );
   expRecs = { "InsertedNum": dataNum, "DuplicatedNum": 0, "ModifiedNum": 0 };
   actRecs = cl.insert( doc, SDB_INSERT_REPLACEONDUP ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //批量插入唯一索引不重复的数据、flag为SDB_INSERT_RETURN_ID
   idStartData = 50;
   aStartData = 50;
   dataNum = 10;
   doc = getBulkData( dataNum, idStartData, aStartData );
   expRecs = { "_id": [50, 51, 52, 53, 54, 55, 56, 57, 58, 59], "InsertedNum": dataNum, "DuplicatedNum": 0, "ModifiedNum": 0 };
   actRecs = cl.insert( doc, SDB_INSERT_RETURN_ID ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }
}

