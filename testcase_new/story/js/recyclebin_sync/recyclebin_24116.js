/******************************************************************************
 * @Description   : 设置AutoDrop为true，回收站项目满
 * @Author        : liuli
 * @CreateTime    : 2021.04.21
 * @LastEditTime  : 2022.04.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   try
   {
      itmeFull();
   }
   finally
   {
      db.getRecycleBin().alter( { AutoDrop: false } );
      db.getRecycleBin().alter( { MaxVersionNum: 2 } );
   }
}

function itmeFull ()
{
   var csName = "cs_24116";
   var clName = "cl_24116";

   db.getRecycleBin().alter( { AutoDrop: true } );
   db.getRecycleBin().alter( { MaxVersionNum: -1 } );
   var autoDrop = db.getRecycleBin().getDetail().toObj().AutoDrop;
   assert.equal( autoDrop, true );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcl = commCreateCL( db, csName, clName );
   dbcl.insert( { "item": 1 } );
   dbcl.truncate();
   var recycleName = getRecycleName( db, csName + "." + clName, "Truncate" );
   var num = db.getRecycleBin().count();
   var maxItemNum = db.getRecycleBin().getDetail().toObj().MaxItemNum;

   if( maxItemNum > 100 || maxItemNum == -1 )
   {
      db.getRecycleBin.alter( { "MaxItemNum": 100 } );
   }

   for( var i = num; i < maxItemNum + num + 100; i++ )
   {
      dbcl.insert( { a: i } );
      dbcl.truncate();
   }

   // 校验第一个项目被删除，回收站项目数量等于MaxItemNum
   checkRecycleItem( recycleName );
   var itemNum = db.getRecycleBin().count();
   assert.equal( itemNum, maxItemNum );

   // 删除CS，检查最后一个项目被放入回收站中
   commDropCS( db, csName );
   var itemNum = db.getRecycleBin().count( { "OriginName": csName, "Type": "CollectionSpace", "OpType": "Drop" } );
   assert.equal( itemNum, 1 );

   db.getRecycleBin().dropAll();
}