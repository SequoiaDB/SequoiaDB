/************************************
*@Description: 修改cs名后，执行数据增删改查操作
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16099
**************************************/

main( test );

function test ()
{
   var oldcsName = CHANGEDPREFIX + "_16099_oldcs";
   var newcsName = CHANGEDPREFIX + "_16099_newcs";
   var clName = CHANGEDPREFIX + "_16099_cl";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine" );
   var cl = commCreateCL( db, oldcsName, clName, {}, false, false, "create CL in the begin" );

   //insert 1000 data
   insertData( cl, 1000, 1000 );

   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   cl = db.getCS( newcsName ).getCL( clName );

   //insert 1000 data, and check data
   insertData( cl, 1000, 2000 );

   //update ($set: {a:10086}) 2000 data, and check data
   updateData( cl );

   //delete no < 500 data, and check data
   deleteData( cl );

   commDropCS( db, newcsName, true, "clean cs---" );
}

function insertData ( cl, insertNum, expNum )
{
   var docs = [];
   for( var i = 0; i < insertNum; ++i )
   {
      var no = i;
      var a = i;
      var user = "test" + i;
      var phone = 13700000000 + i;
      var time = new Date().getTime();
      var doc = { no: no, a: a, customerName: user, phone: phone, openDate: time };
      //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105

      docs.push( doc );
   }
   cl.insert( docs );

   var recordNum = cl.count();
   assert.equal( recordNum, expNum );
}

function updateData ( cl )
{
   cl.update( { $set: { a: 10086 } } );
   var recordNum = cl.count( { a: 10086 } );
   assert.equal( recordNum, 2000 );
}

function deleteData ( cl )
{
   cl.remove( { no: { $lt: 500 } } );
   var recordNum = cl.count();
   assert.equal( recordNum, 1000 );
}