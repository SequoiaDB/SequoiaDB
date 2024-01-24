/******************************************************************************
 * @Description   : seqDB-16102 修改cs名后，执行数据增删改查操作---//review 1:描述和实际执行步骤不相符
 * @Author        : luweikang
 * @CreateTime    : 2018.10.12
 * @LastEditTime  : 2021.02.02
 * @LastEditors   : Yi Pan
 ******************************************************************************/

main( test );

function test ()
{
   var oldcsName = CHANGEDPREFIX + "_16102_oldcs";
   var newcsName = CHANGEDPREFIX + "_16102_newcs";
   var clName = CHANGEDPREFIX + "_16102_cl";

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine" );
   var cl = commCreateCL( db, oldcsName, clName, {}, false, false, "create CL in the begin" );

   cl.createIndex( "aIndex", { a: 1 }, true );
   cl.createIndex( "noIndex", { no: 1 }, false );

   //insert 1000 data
   insertData( cl, 1000 );

   var oldcl = db.getCS( oldcsName ).getCL( clName );

   db.renameCS( oldcsName, newcsName );

   //SEQUOIADBMAINSTREAM-6431补充测试点
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      oldcl.createIndex( "test", { a: 1 } );
   } );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   cl = db.getCS( newcsName ).getCL( clName );

   cl.dropIndex( "aIndex" );
   cl.dropIndex( "noIndex" );

   cl.createIndex( "aIndex", { a: 1 }, true );
   cl.createIndex( "noIndex", { no: 1 }, false );

   var indexArr = ['$id', 'aIndex', 'noIndex'];

   var cur = cl.listIndexes();
   while( cur.next() )
   {
      var index = cur.current().toObj();
      var name = index.IndexDef.name;
      assert.notEqual( indexArr.indexOf( name ), -1 );
   }

   checkRenameCSResult( oldcsName, newcsName, 1 );

   commDropCS( db, newcsName, true, "clean cs---" );
}