/******************************************************************************
 * @Description : test getSlave operation
 *                seqDB-13792:getSlave参数校验 
 * @auhor       : Liang XueWang
 ******************************************************************************/
var rgName = "testGetSlaveRg13792";

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   testIllegalPos();
}

function testIllegalPos ()
{
   var rg = db.createRG( rgName );

   var errorPos = ["a", 0, 8, 1.2, -10];
   try
   {
      for( var i = 0; i < errorPos.length; i++ )
      {
         assert.tryThrow( SDB_INVALIDARG, function()
         {
            rg.getSlave( errorPos[i] );
         } );
      }

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         rg.getSlave( 1, 2, 0, 5, 8 );
      } );

      assert.tryThrow( SDB_CLS_EMPTY_GROUP, function()
      {
         rg.getSlave( 1 );
      } );

   }
   finally
   {
      db.removeRG( rgName );
   }
}
