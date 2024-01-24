/*******************************************************************************
*@Description:   seqDB-11133:查询时间戳，闰年的2月29号
*@Author:        2019-2-25  wangkexin
********************************************************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = "cl11133";

   var clOpt = { ReplSize: 0, ShardingKey: { a: 1 }, ShardingType: "hash" };
   var cl = commCreateCL( db, csName, clName, clOpt );

   //test several sets of data
   var obj1 = { "a": { "$timestamp": "1984-02-29-13.14.26.124233" } };
   checkRsult( cl, obj1 );

   var obj2 = { "a": { "$timestamp": "2004-02-29-01.01.01.123456" } };
   checkRsult( cl, obj2 );

   var obj3 = { "a": { "$timestamp": "2000-02-29-12.23.34.111111" } };
   checkRsult( cl, obj3 );

   commDropCL( db, csName, clName, true, true, "drop cl in the end." )
}

function checkRsult ( cl, insert_timestamp_rec )
{
   cl.insert( insert_timestamp_rec );
   var countNum = cl.count( insert_timestamp_rec );
   assert.equal( countNum, 1 );
}