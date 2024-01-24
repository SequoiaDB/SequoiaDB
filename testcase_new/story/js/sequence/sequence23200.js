/******************************************************************************
 * @Description   : seqDB-23200:系统序列值不可修改
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

   var clName = COMMCLNAME + "_23200";
   commDropCL( db, COMMCSNAME, clName, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id" } } );

   //获取自增字段名
   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_id_SEQ";

   //删除系统sequence
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.dropSequence( sequenceName );
   } )

   //获取系统sequence
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getSequence( sequenceName );
   } )

   //重命名系统sequence
   var newSequenceName = "s23200";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.renameSequence( sequenceName, newSequenceName );
   } )

   commDropCL( db, COMMCSNAME, clName, true, true );

}