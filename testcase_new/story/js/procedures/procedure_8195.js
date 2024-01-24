/******************************************************************************
@Description : seqDB-8195:执行存储过程db.eval（异常）
@Modify list :               
               2016-9-11   TingYU      modify
******************************************************************************/
var csName = COMMCSNAME + 'procedure_8195';
var pcdName = COMMCLNAME + '_procedurename_8195';

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   ready();
   excuteWrongPcd();
   excuteNotExistPcd();
   clean();
}

function excuteWrongPcd ()
{

   var cmd = "db.createProcedure( function " + pcdName + "(x){return db.getCS(x);} )";
   db.eval( cmd );

   var cmd = pcdName + '("' + csName + '")';
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.eval( cmd );
   } );
}

function excuteNotExistPcd ()
{

   fmpRemoveProcedures( [pcdName], true );

   var cmd = pcdName + "()";
   assert.tryThrow( SDB_SPT_EVAL_FAIL, function()
   {
      db.eval( cmd );
   } );
}

function ready ()
{
   fmpRemoveProcedures( [pcdName], true );
   commDropCS( db, csName, true, "drop cs[" + csName + "] in ready" );
}

function clean ()
{
   fmpRemoveProcedures( [pcdName], true );
   commDropCS( db, csName, true, "drop cs[" + csName + "] in clean" );
}