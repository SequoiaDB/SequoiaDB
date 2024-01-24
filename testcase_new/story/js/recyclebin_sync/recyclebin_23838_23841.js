/******************************************************************************
 * @Description   : seqDB-23838:SdbRecycle.list 不带条件匹配所有回收项目
 *                : seqDB-23841:SdbRecycle.snapshot 不带条件匹配所有回收项目
 * @Author        : Yang Qincheng
 * @CreateTime    : 2021.04.25
 * @LastEditTime  : 2023.02.07
 * @LastEditors   : liuli
 ******************************************************************************/

main( test );
function test ()
{
   var csName = "cs_23838_23841";
   var clName = "cl_23838_23841";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var recycleBin = db.getRecycleBin();
   recycleBin.dropAll();
   assert.equal( recycleBin.getDetail().toObj().Enable, true );

   var clFullName = csName + "." + clName;
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );

   cleanRecycleBin( db, testConf.csName );
   dbcl.insert( { "a": 1 } );

   // 构造回收站truncate项目
   dbcl.truncate();

   // 构造回收站dropCL项目
   dbcs.dropCL( clName );

   // 构造回收站dropCS项目
   db.dropCS( csName );

   // list列取回收站项目
   var expItemInfo = ["Truncate", "Drop", "Drop", "Collection", "Collection", "CollectionSpace", clFullName, clFullName, csName];
   var listItemInfo = [];
   var cursor = recycleBin.list();
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      listItemInfo.push( obj.OriginName );
      listItemInfo.push( obj.Type );
      listItemInfo.push( obj.OpType );
   }
   cursor.close();
   assert.equal( listItemInfo.sort(), expItemInfo.sort() );

   // 等待LSN同步后进行校验
   commCheckBusinessStatus( db );

   // snapshot查看回收站项目快照
   var expItemInfo = ["Truncate", "Drop", "Drop", "Collection", "Collection", "CollectionSpace", clFullName, clFullName, csName];
   var listItemInfo = [];
   var cursor = recycleBin.snapshot();
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      listItemInfo.push( obj.OriginName );
      listItemInfo.push( obj.Type );
      listItemInfo.push( obj.OpType );
   }
   cursor.close();
   assert.equal( listItemInfo.sort(), expItemInfo.sort() );

   recycleBin.dropAll();
}