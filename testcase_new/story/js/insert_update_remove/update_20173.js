/******************************************************************************
*@Description : seqDB-20173: 更新记录与原记录冲突 
*@Author      : 2020-01-09  Zhao xiaoni Init
******************************************************************************/
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_20173";

main( test );

function test ()
{
   var cl = db.getCS( testConf.csName ).getCL( testConf.clName );

   var idStartData = 0;
   var aStartData = 0;
   var dataNum = 10;
   var doc = getBulkData( dataNum, idStartData, aStartData );
   cl.insert( doc );

   //更新一条记录，与原记录冲突
   var updatedRecord = { "_id": 9, "a": 9 };
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.update( { "$set": updatedRecord }, doc[8] );
   } )

   //批量更新记录，与原记录冲突
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.update( { "$set": updatedRecord } );
   } )
}
