/******************************************************************************
*@Description : test CRUD with SdbDate function
*@Modify list :
*               2016-07-12   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true, "create CL in the begining..." );

   // 以SdbDate函数的方式插入数据
   cl.insert( { date: SdbDate( "2016-07-12" ) } );

   // 以Regex函数的方式查询数据
   var rc = cl.find( { date: SdbDate( "2016-07-12" ) }, { date: "" } );
   var expRecs = { date: { $date: "2016-07-12" } };
   checkRec( rc, [expRecs] );

   // 以普通方式查询数据
   rc = cl.find( { date: { "$date": "2016-07-12" } }, { date: "" } );
   expRecs = { date: { $date: "2016-07-12" } };
   checkRec( rc, [expRecs] );

   // 使用$type匹配符查询数据
   rc = cl.find( { date: { $type: 1, $et: 9 } }, { date: "" } );
   expRecs = { date: { $date: "2016-07-12" } };
   checkRec( rc, [expRecs] );

}
