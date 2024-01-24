/******************************************************************************
 * @Description   : seqDB-23202:设置序列属性，清空coord上序列缓存
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

   var sequenceName = 's23202';
   var acquireSize = 10;
   var increment = -1;

   dropSequence( db, sequenceName );

   var s = db.createSequence( sequenceName, { AcquireSize: acquireSize } );

   var coords = getCoordNodeNames( db );
   for( var i = 0; i < coords.length; i++ )
   {
      var coord = new Sdb( coords[i] );
      coord.getSequence( sequenceName ).getNextValue();
      coord.close();
   }

   s.setAttributes( { Increment: increment } );

   for( var i = 0; i < coords.length; i++ )
   {
      var coord = new Sdb( coords[i] );
      var nextValue = coord.getSequence( sequenceName ).getNextValue();
      var discordValue = coords.length * acquireSize; //30
      var initialValue = 1;

      var expectValue = ( discordValue - initialValue ) + i * increment * acquireSize;
      assert.equal( expectValue, nextValue );
   }

   db.dropSequence( sequenceName );
}