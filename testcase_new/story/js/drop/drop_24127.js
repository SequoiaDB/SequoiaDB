/******************************************************************************
 * @Description   : seqDB-24127:dropCS可以通过参数注入指定其他CS
 * @Author        : Yi Pan
 * @CreateTime    : 2021.04.26
 * @LastEditTime  : 2022.03.23
 * @LastEditors   : liuli
 ******************************************************************************/

main( test );

function test ()
{
   var csname1 = "cs_24127a";
   var csname2 = "cs_24127b";
   var clname = "cl_24127";

   //直连coord节点创建删除
   commDropCS( db, csname1 );
   commDropCS( db, csname2 );
   commCreateCL( db, csname1, clname );
   commCreateCL( db, csname2, clname );
   db.dropCS( csname1, { "Name": csname2 } );
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getCS( csname1 );
   } );
   db.getCS( csname2 );
   commDropCS( db, csname1, true );
   commDropCS( db, csname2, true );
}