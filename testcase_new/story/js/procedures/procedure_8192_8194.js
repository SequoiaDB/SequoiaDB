/******************************************************************************
 * @Description   : seqDB-8192:创建执行存储过程
 *                  seqDB-8194:删除查询存储过程
 * @Author        : TingYU
 * @CreateTime    : 2016.09.11
 * @LastEditTime  : 2022.05.09
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
var csName = COMMCSNAME + "_8192";
var pcdName1 = COMMCLNAME + '_procedurename_8192';
var pcdName2 = 'sum_procedure_8192';

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   ready();
   createPcd();
   excutePcd();
   listPcd();
   removePcd();
   clean();
}

function ready ()
{
   fmpRemoveProcedures( [pcdName1, pcdName2], true );
   commDropCS( db, csName, true, "drop cs[" + csName + "] in ready" );
}

function createPcd ()
{
   var str = "db.createProcedure( function " + pcdName1 + "(" + ") {db.createCS('" + csName + "')} )";
   db.eval( str );
   db.createProcedure( function sum_procedure_8192 ( x, y, z ) { return x + y + z; } );
}

function excutePcd ()
{
   db.eval( pcdName1 + "()" );
   db.getCS( csName );

   var sumRst = db.eval( pcdName2 + "(1,2,3)" );
   assert.equal( sumRst, 6 );
}

function listPcd ()
{
   var rc = db.listProcedures();

   var expPcd1 = {};
   expPcd1["name"] = pcdName1;
   expPcd1["func"] = { "$code": "function " + pcdName1 + "() {\n    db.createCS(\"" + csName + "\");\n}" };
   expPcd1["funcType"] = 0;

   var rc = db.listProcedures( { name: pcdName1 } );
   checkResult( rc, [expPcd1] );

   var expPcd2 = {};
   expPcd2["name"] = pcdName2;
   expPcd2["func"] = { "$code": "function " + pcdName2 + "(x, y, z) {\n    return x + y + z;\n}" };
   expPcd2["funcType"] = 0;

   var rc = db.listProcedures( { name: pcdName2 } );
   checkResult( rc, [expPcd2] );
}

function removePcd ()
{
   db.removeProcedure( pcdName1 );

   var expPcd2 = {};
   expPcd2["name"] = pcdName2;
   expPcd2["func"] = { "$code": "function " + pcdName2 + "(x, y, z) {\n    return x + y + z;\n}" };
   expPcd2["funcType"] = 0;

   var rc = db.listProcedures( { name: pcdName1 } );
   checkResult( rc, [] );

   var rc = db.listProcedures( { name: pcdName2 } );
   checkResult( rc, [expPcd2] );

}

function clean ()
{
   fmpRemoveProcedures( [pcdName1, pcdName2], true );
   commDropCS( db, csName, true, "drop cs in clean()" )
}