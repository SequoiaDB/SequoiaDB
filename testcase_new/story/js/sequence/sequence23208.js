/******************************************************************************
 * @Description   : seqDB-23208:重置序列值
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

   var sequenceName = 's23208';
   var cacheSize = 1000;
   var acquireSize = 100;
   var maxValue = 10000;
   var minValue = -10000;
   dropSequence( db, sequenceName );

   var s = db.createSequence( sequenceName, { AcquireSize: acquireSize, CacheSize: cacheSize, MaxValue: maxValue, MinValue: minValue, Cycled: false } );

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

      var fetchNum = 50;
      sequence.fetch( fetchNum );
   }


   //restart为本coord已使用值
   var restartNum = 120;
   sequences[0].restart( restartNum );

   for( var i = 0; i < coords.length; i++ )
   {
      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, i * acquireSize + restartNum );
   }

   //restart为本coord缓存内未使用值
   var restartNum = 180;
   sequences[0].restart( restartNum );
   for( var i = 0; i < coords.length; i++ )
   {
      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, i * acquireSize + restartNum );
   }

   //restart为其他coord缓存内值
   var restartNum = 290;
   sequences[0].restart( restartNum );
   for( var i = 0; i < coords.length; i++ )
   {
      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, i * acquireSize + restartNum );
   }

   //restart为所有coord缓存内值
   var restartNum = 800;
   sequences[0].restart( restartNum );
   for( var i = 0; i < coords.length; i++ )
   {
      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, i * acquireSize + restartNum );
   }

   //restart为CacheSize外的值
   var restartNum = 5000;
   sequences[0].restart( restartNum );
   for( var i = 0; i < coords.length; i++ )
   {
      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, i * acquireSize + restartNum );
   }

   //restart为初始值
   var restartNum = 1;
   sequences[0].restart( restartNum );
   for( var i = 0; i < coords.length; i++ )
   {
      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, i * acquireSize + restartNum );
   }

   //序列超过最大值后restart
   sequences[0].setCurrentValue( 10000 );
   assert.tryThrow( SDB_SEQUENCE_EXCEEDED, function()
   {
      sequences[0].getNextValue();
   } )
   var restartNum = 1;
   sequences[0].restart( restartNum );
   for( var i = 0; i < coords.length; i++ )
   {
      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, i * acquireSize + restartNum );
   }

   //设置序列为最小值
   sequences[0].restart( minValue );
   for( var i = 0; i < coords.length; i++ )
   {
      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, i * acquireSize + minValue );
   }

   //重置序列为最大值
   sequences[0].setAttributes( { Increment: -1 } )
   sequences[0].restart( maxValue );
   for( var i = 0; i < coords.length; i++ )
   {
      var nextValue = sequences[i].getNextValue();
      assert.equal( nextValue, maxValue - i * acquireSize );
   }

   //重置序列小于最小值
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      sequences[0].restart( minValue - 1 );
   } )

   //重置序列大于最大值
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      sequences[0].restart( maxValue + 1 );
   } )

   for( var i = 0; i < coords.length; i++ )
   {
      coords[i].close();
   }

   db.dropSequence( sequenceName );

}