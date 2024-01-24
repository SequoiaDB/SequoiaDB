/******************************************************************************
 * @Description   : seqDB-23196:重命名序列接口参数校验
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

   var coordNodes = getCoordNodeNames( db );
   if( coordNodes.length < 3 )
   {
      return;
   };

   var sequenceName = 's23196';
   var newSequenceName = 's23196_new';
   dropSequence( db, sequenceName );
   dropSequence( db, newSequenceName );

   //重命名不存在的序列
   assert.tryThrow( SDB_SEQUENCE_NOT_EXIST, function()
   {
      db.renameSequence( sequenceName, newSequenceName );
   } )

   //coord1创建序列
   var coord1 = new Sdb( coordNodes[0] );
   coord1.createSequence( sequenceName );

   //重命名时，新名字的序列已存在
   db.createSequence( newSequenceName );
   assert.tryThrow( SDB_SEQUENCE_EXIST, function()
   {
      db.renameSequence( sequenceName, newSequenceName );
   } )
   db.dropSequence( newSequenceName );

   //coord2获取序列缓存
   var coord2 = new Sdb( coordNodes[1] );
   var s = coord2.getSequence( sequenceName );
   s.getNextValue();

   //coord3重命名序列
   var coord3 = new Sdb( coordNodes[2] );
   coord3.renameSequence( sequenceName, newSequenceName );

   //coord2获取序列
   assert.tryThrow( SDB_SEQUENCE_NOT_EXIST, function()
   {
      s.getNextValue();
   } )

   dropSequence( db, sequenceName );
   dropSequence( db, newSequenceName );

   //seqDB-23197:删除序列接口参数校验
   assert.tryThrow( SDB_SEQUENCE_NOT_EXIST, function()
   {
      db.dropSequence( sequenceName );
   } )

   coord1.close();
   coord2.close();
   coord3.close();
}