/******************************************************************************
*@Description : test CRUD with BinData function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true, "create CL in the begining..." );

   // 以BinData函数的方式插入数据
   cl.insert( { bin: BinData( "aGVsbG8gd29ybGQ=", "1" ) } );

   // 以BinData函数的方式查询更新数据
   var rc = cl.find( { bin: BinData( "aGVsbG8gd29ybGQ=", "1" ) }, { bin: "" } );
   var expRecs = { bin: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } };
   checkRec( rc, [expRecs] );

   // 以普通方式查询数据
   rc = cl.find( { bin: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } }, { bin: "" } );
   expRecs = { bin: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } };
   checkRec( rc, [expRecs] );

   // 使用$type匹配符查询数据
   rc = cl.find( { bin: { $type: 1, $et: 5 } }, { bin: "" } );
   expRecs = { bin: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } };
   checkRec( rc, [expRecs] );

}
