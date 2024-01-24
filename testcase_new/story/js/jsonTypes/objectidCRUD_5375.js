/******************************************************************************
*@Description : test CRUD with ObjectId function
*@Modify list :
*               2016-07-12   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true, "create CL in the begining..." );

   // 以ObjectId函数的方式插入数据
   cl.insert( { id: ObjectId( "55713f7953e6769804000001" ) } );

   // 以ObjectId函数的方式查询数据
   var rc = cl.find( { id: ObjectId( "55713f7953e6769804000001" ) }, { id: "" } );
   var expRecs = { id: { $oid: "55713f7953e6769804000001" } };
   checkRec( rc, [expRecs] );

   // 以普通方式查询数据
   rc = cl.find( { id: { "$oid": "55713f7953e6769804000001" } }, { id: "" } );
   expRecs = { id: { $oid: "55713f7953e6769804000001" } };
   checkRec( rc, [expRecs] );

   // 使用$type匹配符查询数据
   rc = cl.find( { id: { $type: 1, $et: 7 } }, { id: "" } );
   expRecs = { id: { $oid: "55713f7953e6769804000001" } };
   checkRec( rc, [expRecs] );

}
