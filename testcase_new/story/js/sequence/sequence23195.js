/******************************************************************************
 * @Description   : seqDB-23195:获取序列接口参数校验
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

   var sequenceName = 's23195';
   dropSequence( db, sequenceName );

   //获取不存在的序列
   assert.tryThrow( SDB_SEQUENCE_NOT_EXIST, function()
   {
      db.getSequence( sequenceName );
   } )

   //coord1创建序列
   var coord1 = new Sdb( coordNodes[0] );
   coord1.createSequence( sequenceName );

   //coord2获取序列缓存
   var coord2 = new Sdb( coordNodes[1] );
   var s = coord2.getSequence( sequenceName );
   s.getNextValue();

   //coord3删除序列
   var coord3 = new Sdb( coordNodes[2] );
   coord3.dropSequence( sequenceName );

   //coord2获取序列
   assert.tryThrow( SDB_SEQUENCE_NOT_EXIST, function()
   {
      s.getNextValue();
   } )

   coord1.close();
   coord2.close();
   coord3.close();
}