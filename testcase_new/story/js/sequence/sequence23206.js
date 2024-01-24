/******************************************************************************
 * @Description   : seqDB-23206:设置当前值，并获取下一个序列值
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
   var coordNum = 3;
   if( coordNames.length != coordNum )
   {
      return;
   }

   var sequenceName = 's23206';
   var acquireSize = 100;
   var increment = 1;

   dropSequence( db, sequenceName );
   var s = db.createSequence( sequenceName, { AcquireSize: acquireSize } );

   var coords = [];
   var sequences = [];
   var currentValues = [];

   //各coord节点分别获取50个sequence值
   for( var i = 0; i < coordNames.length; i++ )
   {
      println( "i:" + i );
      var coord = new Sdb( coordNames[i] );
      coords.push( coord );
      var sequence = coord.getSequence( sequenceName );
      sequences.push( sequence );
      for( var j = 0; j < 50; j++ )
      {
         sequence.getNextValue();
      }
      var currentValue = sequence.getCurrentValue();
      currentValues.push( currentValue );
   }

   //设置currentValue在本coord缓存范围内且已被使用
   assert.tryThrow( SDB_SEQUENCE_VALUE_USED, function()
   {
      sequences[1].setCurrentValue( 120 );
   } )
   for( var i = 0; i < coordNum; i++ )
   {
      var nextVaule = sequences[i].getNextValue();
      assert.equal( nextVaule, currentValues[i] + increment );
   }

   //设置currentValue在其他coord缓存范围内，比本coord缓存小且已被使用
   assert.tryThrow( SDB_SEQUENCE_VALUE_USED, function()
   {
      sequences[1].setCurrentValue( 20 );
   } )
   for( var i = 0; i < coordNum; i++ )
   {
      var nextVaule = sequences[i].getNextValue();
      assert.equal( nextVaule, currentValues[i] + increment * 2 );
   }

   //设置currentValue在其他coord缓存范围内，比本coord缓存小且未被使用
   assert.tryThrow( SDB_SEQUENCE_VALUE_USED, function()
   {
      sequences[1].setCurrentValue( 80 );
   } )
   for( var i = 0; i < coordNum; i++ )
   {
      var nextVaule = sequences[i].getNextValue();
      assert.equal( nextVaule, currentValues[i] + increment * 3 );
   }

   //设置currentValue在其他coord缓存范围内，比本coord缓存大,且已被使用
   assert.tryThrow( SDB_SEQUENCE_VALUE_USED, function()
   {
      sequences[1].setCurrentValue( 220 );
   } )
   for( var i = 0; i < coordNum; i++ )
   {
      var nextVaule = sequences[i].getNextValue();
      assert.equal( nextVaule, currentValues[i] + increment * 4 );
   }

   //设置currentValue在其他coord缓存范围内，比本coord缓存大，且未被使用
   assert.tryThrow( SDB_SEQUENCE_VALUE_USED, function()
   {
      sequences[1].setCurrentValue( 280 );
   } )
   for( var i = 0; i < coordNum; i++ )
   {
      var nextVaule = sequences[i].getNextValue();
      println( "nextVaule:" + nextVaule );
      assert.equal( nextVaule, currentValues[i] + increment * 5 );
   }

   //设置currentValue在本coord缓存范围内且未被使用
   var expectValue = sequences[2].getCurrentValue();
   var currentValue = 180;
   sequences[1].setCurrentValue( currentValue );
   var nextValue1 = sequences[1].getNextValue();
   assert.equal( nextValue1, currentValue + increment );

   var nextValue0 = sequences[0].getNextValue();
   assert.equal( nextValue0, acquireSize * coords.length + increment );

   var nextValue2 = sequences[2].getNextValue();
   assert.equal( nextValue2, expectValue + increment );

   //获取各coord节点的当前值
   var currentValues = [];
   for( var i = 0; i < coordNum; i++ )
   {
      var currentVaule = sequences[i].getCurrentValue();
      currentValues.push( currentVaule );
   }
   println( "currentValues:" + currentValues );

   //设置当前超过acquireSize，在cacheSize内
   var currentValue1 = 401;
   sequences[1].setCurrentValue( currentValue1 );

   var nextValue0 = sequences[0].getNextValue();
   assert.equal( nextValue0, currentValue1 + increment );

   var nextValue1 = sequences[1].getNextValue();
   assert.equal( nextValue1, currentValue1 + acquireSize * 1 + increment );

   var nextValue2 = sequences[2].getNextValue();
   assert.equal( nextValue2, currentValue1 + acquireSize * 2 + increment );

   //获取各coord节点的当前值
   var currentValues = [];
   for( var i = 0; i < coordNum; i++ )
   {
      var currentVaule = sequences[i].getCurrentValue();
      currentValues.push( currentVaule );
   }
   println( "currentValues:" + currentValues );

   //设置超过cacheSize的值
   var currentValue1 = 1500;
   sequences[1].setCurrentValue( currentValue1 );

   var nextValue0 = sequences[0].getNextValue();
   assert.equal( nextValue0, currentValue1 + increment );

   var nextValue1 = sequences[1].getNextValue();
   assert.equal( nextValue1, currentValue1 + acquireSize * 1 + increment );

   var nextValue2 = sequences[2].getNextValue();
   assert.equal( nextValue2, currentValue1 + acquireSize * 2 + increment );

   var currentValue1 = 10000;
   sequences[1].setCurrentValue( currentValue1 );

   var nextValue0 = sequences[0].getNextValue();
   assert.equal( nextValue0, currentValue1 + increment );

   var nextValue1 = sequences[1].getNextValue();
   assert.equal( nextValue1, currentValue1 + acquireSize * 1 + increment );

   var nextValue2 = sequences[2].getNextValue();
   assert.equal( nextValue2, currentValue1 + acquireSize * 2 + increment );

   //设置当前值比所有coord缓存小
   assert.tryThrow( SDB_SEQUENCE_VALUE_USED, function()
   {
      sequences[1].setCurrentValue( 30 );
   } )
   var nextValue0 = sequences[0].getNextValue();
   assert.equal( nextValue0, currentValue1 + increment * 2 );

   var nextValue1 = sequences[1].getNextValue();
   assert.equal( nextValue1, currentValue1 + acquireSize * 1 + increment * 2 );

   var nextValue2 = sequences[2].getNextValue();
   assert.equal( nextValue2, currentValue1 + acquireSize * 2 + increment * 2 );

   db.dropSequence( sequenceName );
}