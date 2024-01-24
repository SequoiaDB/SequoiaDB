/******************************************************************************
 * @Description   : seqDB-23207:获取多个序列值
 * @Author        : zhaoyu
 * @CreateTime    : 2021.01.11
 * @LastEditTime  : 2021.02.03
 * @LastEditors   : Lai Xuan
 ******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   };

   var sequenceName = 's23207';
   var cacheSize = 1000;
   var acquireSize = 100;
   dropSequence( db, sequenceName );

   var s = db.createSequence( sequenceName, { AcquireSize: acquireSize, CacheSize: cacheSize } );

   var coordNames = getCoordNodeNames( db );
   var coords = [];
   var sequences = [];
   for( var i = 0; i < coordNames.length; i++ )
   {
      var coord = new Sdb( coordNames[i] );
      coords.push( coord );
      println( "coord: " + coords[i] );

      var sequence = coord.getSequence( sequenceName );
      sequences.push( sequence );

      var fetchNum = 10;
      sequence.fetch( fetchNum );
      var nextValue = sequence.getNextValue();
      assert.equal( nextValue, acquireSize * i + fetchNum + 1 );
   }

   for( var i = 0; i < coords.length; i++ )
   {
      sequences[i].fetch( acquireSize );

      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, acquireSize * coords.length + acquireSize * i + 1 );
   }

   for( var i = 0; i < coords.length; i++ )
   {
      sequences[i].fetch( cacheSize );

      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, acquireSize * coords.length * 2 + acquireSize * i + 1 );
   }

   for( var i = 0; i < coords.length; i++ )
   {
      coords[i].close();
   }

   db.dropSequence( sequenceName );
}