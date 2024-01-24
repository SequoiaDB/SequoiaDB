/******************************************************************************
 * @Description   : seqDB-23205:获取序列当前值
 * @Author        : zhaoyu
 * @CreateTime    : 2021.01.11
 * @LastEditTime  : 2021.01.11
 * @LastEditors   : zhaoyu
 ******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   };

   var coordNames = getCoordNodeNames( db );
   if( coordNames.length < 3 )
   {
      return;
   };

   var sequenceName = 's23205';
   var acquireSize = 100;
   var cacheSize = 1000;
   dropSequence( db, sequenceName );
   var s = db.createSequence( sequenceName, { AcquireSize: acquireSize, CacheSize: cacheSize } );

   var coords = [];
   var sequences = [];
   for( var i = 0; i < coordNames.length; i++ )
   {
      var coord = new Sdb( coordNames[i] );
      coords.push( coord );
      println( "coord: " + coords[i] );

      var sequence = coord.getSequence( sequenceName );
      sequences.push( sequence );
      println( "sequence: " + sequences[i] );

      //获取序列当前值
      assert.tryThrow( SDB_SEQUENCE_NEVER_USED, function()
      {
         sequences[i].getCurrentValue();
      } )
   }

   //设置序列当前值，获取当前值
   var currentValue = 500;
   sequences[1].setCurrentValue( 500 );

   //获取序列当前值，为CacheSize的当前值
   for( var i = 0; i < coords.length; i++ )
   {
      var currentValue1 = sequences[i].getCurrentValue();
      assert.equal( currentValue1, cacheSize );
   }

   //fetch序列后，获取当前值
   for( var i = 0; i < coords.length; i++ )
   {
      var fetchNum = 50;
      sequences[i].fetch( fetchNum );
      var currentValue1 = sequences[i].getCurrentValue();
      assert.equal( currentValue1, currentValue + fetchNum + acquireSize * i );
   }

   //restart序列后，获取当前值
   var restartValue = 50;
   sequences[1].restart( restartValue );
   for( var i = 0; i < coords.length; i++ )
   {
      assert.tryThrow( SDB_SEQUENCE_NEVER_USED, function()
      {
         sequences[i].getCurrentValue();
      } )
   }


   db.dropSequence( sequenceName );
}