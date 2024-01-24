/************************************
*@Description: 指定cs创建存储过程，修改cs名 
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16107
**************************************/

main( test );

function test ()
{
   //@ clean before
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var oldcsName = CHANGEDPREFIX + "_16107_oldcs";
   var newcsName = CHANGEDPREFIX + "_16107_newcs";
   var clName = CHANGEDPREFIX + "_16107_cl1";
   var procedureName = CHANGEDPREFIX + "_pro_16107";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine" );
   var cl = commCreateCL( db, oldcsName, clName, {}, false, false, "create CL in the begin" );

   var str = "db.createProcedure(function " + procedureName + "(){var cl = db.getCS('" + oldcsName + "').getCL('" + clName + "'); " +
      "cl.insert({'no':10086, 'customerName':'testTrans', 'phone':13700010086, 'openDate':1402990912105}) })";

   db.eval( str );
   db.eval( procedureName + "()" );

   var num = cl.count();
   assert.equal( num, 1 );

   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.eval( procedureName + "()" );
   } );

   db.removeProcedure( procedureName );

   var cur = db.listProcedures( { name: procedureName } );
   assert.equal( cur.next(), null );

   commDropCS( db, newcsName, true, "clean cs---" );
}