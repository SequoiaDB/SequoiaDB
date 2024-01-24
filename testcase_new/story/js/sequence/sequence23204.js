/******************************************************************************
 * @Description   : seqDB-23204:删除序列，清空coord上序列缓存
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

   var sequenceName = 's23204';
   dropSequence( db, sequenceName );

   var s = db.createSequence( sequenceName, { AcquireSize: 10 } );

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
      sequence.getNextValue();
   }

   db.dropSequence( sequenceName );

   for( var i = 0; i < coords.length; i++ )
   {
      assert.tryThrow( SDB_SEQUENCE_NOT_EXIST, function()
      {
         sequences[i].getNextValue();
      } )

   }

   for( var i = 0; i < coords.length; i++ )
   {
      coords[i].close();
   }

}