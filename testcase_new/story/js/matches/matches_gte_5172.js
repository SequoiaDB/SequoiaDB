/*******************************************************************************
*@Description: find操作，匹配符$gte，匹配的字段值跟记录的字段值数据类型不一致_ST.basicOperate.find.gte.010
*@Author:  2019-5-31  wangkexin
*@testlinkCase: seqDB-5172
********************************************************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_5172";

   //clean environment before test
   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );

   var cl = commCreateCL( db, csName, clName );
   cl.insert( { "key": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } );
   var count = cl.find( { a: { "$gte": 1 } } ).count();
   assert.equal( count, 0 );

   //drop cl
   commDropCL( db, csName, clName, true, true, "drop cl in the end" );
}