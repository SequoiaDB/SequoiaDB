/******************************************************************************
*@Description : test CRUD with MinKey MaxKey function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true);
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true, "create CL in the begining..." );

   // 以MinKey, MaxKey函数的方式插入数据
   cl.insert( { _id: 1, key: MinKey() } );
   cl.insert( { _id: 2, key: MaxKey() } );

   // 以MinKey, MaxKey函数的方式查询更新数据
   var rc = cl.find();
   var expRecs = [{ _id: 1, key: { $minKey: 1 } }, { _id: 2, key: { $maxKey: 1 } }];
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true);
   // 不能以find( {key:Minkey()} ) find( {key:MaxKey()} )的方式查询数据 不支持  报错-6
   // 不能以find( {key:{$type: }} )的方式查询数据 MinKey MaxKey没有type值
}
