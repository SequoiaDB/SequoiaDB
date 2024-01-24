/******************************************************************************
@Description : 1. Test create domain, specify Name "SYSDOMAIN" [Error].
               2. Test create domain specify nothing[NoArg].
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domName = csName + "_8116";

   // Drop domain in the begnning
   clearDomain( db, domName );

   db.createDomain( domName );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createDomain( "SYSDOMAIN" );
   } );

   clearDomain( db, domName );
}