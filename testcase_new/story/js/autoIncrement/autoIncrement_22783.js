/******************************************************************************
 * @Description   : seqDB-22783:独立模式创建自增字段报错
 * @Author        : Li Yuanyue
 * @CreateData    : 2021.01.07
 * @LastEditTime  : 2021.01.07
 * @LastEditors   : Li Yuanyue
 ******************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_22783";
   if( commIsStandalone( db ) )
   {
      var cs = db.getCS( csName );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cs.createCL( clName, ( { AutoIncrement: { Field: "a" } } ) );
      } );

      commDropCL( db, csName, clName );
      var cl = cs.createCL( clName );
      assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
      {
         cl.setAttributes( { AutoIncrement: { Field: "id" } } );
      } );
      commDropCL( db, csName, clName );
   }
}