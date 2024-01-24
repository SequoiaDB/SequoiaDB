/******************************************************************************
@Description : seqDB-8193:创建存储过程（异常）
@Modify list :
               2014-7-30   xiaojun Hu  Init
               2016-9-11   TingYU      modify
******************************************************************************/
var pcdName = COMMCLNAME + '_procedurename_8193';
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   ready();
   createExistPcd();
   lackName();
   wrongParameterType();
   clean();
}

function createExistPcd ()
{
   var cmd = "db.createProcedure( function " + pcdName + "(x, y){return x-y;} )";
   db.eval( cmd );

   assert.tryThrow( SDB_FMP_FUNC_EXIST, function()
   {
      db.eval( cmd );
   } );

   var cmd = "db.createProcedure( function " + pcdName + "(x){return x;} )";

   assert.tryThrow( SDB_FMP_FUNC_EXIST, function()
   {
      db.eval( cmd );
   } );
}

function lackName ()
{
   db.eval( cmd );
   var cmd = "db.createProcedure( function " + "(x, y){return x-y;} )";

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.eval( cmd );
   } );
}

function wrongParameterType ()
{
   db.eval( cmd );
   var cmd = "db.createProcedure( function " + pcdName + "{return x-y;} )";

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createProcedure( "" );
   } );
}

function ready ()
{
   fmpRemoveProcedures( [pcdName], true );
}

function clean ()
{
   fmpRemoveProcedures( [pcdName], true );
}
