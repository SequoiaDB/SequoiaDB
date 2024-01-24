/******************************************************************************
 * @Description   : seqDB-23203:重命名序列，清空coord上序列缓存
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

   var sequenceName = 's23203';
   var newSequenceName = 'new_s23203';
   var acquireSize = 10;
   var increment = 1;
   dropSequence( db, sequenceName );
   dropSequence( db, newSequenceName );

   var s = db.createSequence( sequenceName, { AcquireSize: acquireSize } );

   var coords = getCoordNodeNames( db );
   for( var i = 0; i < coords.length; i++ )
   {
      var coord = new Sdb( coords[i] );
      coord.getSequence( sequenceName ).getNextValue();
      coord.close();
   }

   db.renameSequence( sequenceName, newSequenceName );

   for( var i = 0; i < coords.length; i++ )
   {
      var coord = new Sdb( coords[i] );
      var nextValue = coord.getSequence( newSequenceName ).getNextValue();
      var discordValue = coords.length * acquireSize; //30
      var initialValue = 1;

      var expectValue = ( discordValue + initialValue ) + i * increment * acquireSize;
      assert.equal( expectValue, nextValue );
   }

   dropSequence( db, newSequenceName );
}