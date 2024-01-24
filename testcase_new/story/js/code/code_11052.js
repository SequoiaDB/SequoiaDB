/************************************************************************
*@Description:   seqDB-11052:支持code类型
*@Author:  2017/2/7  huangxiaoni init
*		     2019/11/18 zhaoyu modify
************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   //ready env
   var procedureName = "abc11052";

   //structural data
   var cmd = "db.createProcedure( function " + procedureName + "(x, y){return x+y;} )";
   db.eval( cmd );

   checkResult( procedureName );
   db.removeProcedure( procedureName );
}

function checkResult ( procedureName )
{
   //compare the returned records
   var rc = db.listProcedures( { name: procedureName } ).current().toObj().func;

   var expRlt = '{"$code":"function ' + procedureName + '(x, y) {\\n    return x + y;\\n}"}';
   var actRlt = JSON.stringify( rc );
   assert.equal( expRlt, actRlt );
}
