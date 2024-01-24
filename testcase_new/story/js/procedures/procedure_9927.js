/******************************************************************************
@Description : seqDB-9927:删除存储过程（异常）
@Modify list :
               2016-9-11   TingYU      Init
******************************************************************************/
var pcdName = COMMCLNAME + '_procedurename_9927';
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   ready();
   assert.tryThrow( SDB_FMP_FUNC_NOT_EXIST, function()
   {
      db.removeProcedure( pcdName );
   } );
   parameterCheck();
   clean();
}


function parameterCheck ()
{
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      db.removeProcedure();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.removeProcedure( 123 );
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
