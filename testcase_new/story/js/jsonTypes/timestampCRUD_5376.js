/******************************************************************************
*@Description : test CRUD with Timestamp function
*@Modify list :
*               2016-07-12   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true, "create CL in the begining..." );

   // 以Timestamp函数的方式插入数据
   cl.insert( { time: Timestamp( "2015-06-05-16.10.33.000000" ) } );

   // 以Timestamp函数的方式查询数据
   var rc = cl.find( { time: Timestamp( "2015-06-05-16.10.33.000000" ) }, { time: "" } );
   var expRecs = { time: { $timestamp: "2015-06-05-16.10.33.000000" } };
   checkRec( rc, [expRecs] );

   // 以普通方式查询数据
   rc = cl.find( { time: { "$timestamp": "2015-06-05-16.10.33.000000" } }, { time: "" } );
   expRecs = { time: { $timestamp: "2015-06-05-16.10.33.000000" } };
   checkRec( rc, [expRecs] );

   // 使用$type匹配符查询数据
   rc = cl.find( { time: { $type: 1, $et: 17 } }, { time: "" } );
   expRecs = { time: { $timestamp: "2015-06-05-16.10.33.000000" } };
   checkRec( rc, [expRecs] );

}
