/******************************************************************************
*@Description : seqDB-19695:检查检查list( SDB_LIST_USERS )列表信息
*@author      : wangkexin
*@Date        : 2019-09-27
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var userName1 = "user_19695_1";
   var userName2 = "user_19695_2";
   var options1 = {};
   var options2 = { AuditMask: "DDL|DML|!DQL" };

   try
   {
      db.createUsr( userName1, userName1, options1 );
      db.createUsr( userName2, userName2, options2 );

      checkListUsers( userName1, options1 );
      checkListUsers( userName2, options2 );
   }
   finally
   {
      db.dropUsr( userName1, userName1 );
      db.dropUsr( userName2, userName2 );
   }
}

function checkListUsers ( user, options )
{
   var cursor = db.list( SDB_LIST_USERS, { "User": user } );
   var count = 0;
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      assert.equal( obj.User, user );

      assert.equal( obj.Options, options );

      count++;
   }
   cursor.close();
   assert.equal( count, 1 );
}
