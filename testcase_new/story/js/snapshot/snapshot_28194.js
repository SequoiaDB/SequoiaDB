/******************************************************************************
 * @Description   : seqDB-28194:listCS/CL新增创建/更新时间信息验证
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.10.19
 * @LastEditTime  : 2022.10.26
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var csName = "csName_28194";
   var clName = "clName_28194";
   var newClName = "newClName_28194";

   // 1.创建CS
   commCreateCS( db, csName, true );
   var cursor1 = db.exec( "select * from $LIST_CS where Name = 'csName_28194' " );
   var createTime1 = cursor1.current().toObj()["CreateTime"];
   var updateTime1 = cursor1.current().toObj()["UpdateTime"];

   // 2.创建CL
   db.getCS( csName ).createCL( clName );
   var cursor2 = db.exec( "select * from $LIST_CS where Name = 'csName_28194' " );
   var createTime2 = cursor2.current().toObj()["CreateTime"];
   var updateTime2 = cursor2.current().toObj()["UpdateTime"];
   assert.equal( createTime2, createTime1, "expected createTime equal" );
   if( updateTime2 <= updateTime1 )
   {
      throw new Error( "expected post-updateTime to be more than pre-updateTime" );
   }

   // 3.更新CL属性信息 
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      db.getCS( csName ).getCL( clName ).disableCompression();
   } );
   var cursor3 = db.exec( "select * from $LIST_CS where Name = 'csName_28194' " );
   var createTime3 = cursor3.current().toObj()["CreateTime"];
   var updateTime3 = cursor3.current().toObj()["UpdateTime"];
   assert.equal( createTime3, createTime2, "expected createTime equal" );
   assert.equal( updateTime3, updateTime2, "expected updateTime equal" );

   // 4.修改CL名
   db.getCS( csName ).renameCL( clName, newClName );
   var cursor4 = db.exec( "select * from $LIST_CS where Name = 'csName_28194' " );
   var createTime4 = cursor4.current().toObj()["CreateTime"];
   var updateTime4 = cursor4.current().toObj()["UpdateTime"];
   assert.equal( createTime4, createTime3, "expected createTime equal" );
   if( updateTime4 <= updateTime3 )
   {
      throw new Error( "expected post-updateTime to be more than pre-updateTime" );
   }

   // 5.修改元数据信息
   db.getCS( csName ).getCL( newClName ).createIndex( 'idIdx', { 'id': 1 } );
   var cursor5 = db.exec( "select * from $LIST_CS where Name = 'csName_28194' " );
   var createTime5 = cursor5.current().toObj()["CreateTime"];
   var updateTime5 = cursor5.current().toObj()["UpdateTime"];
   assert.equal( createTime5, createTime4, "expected createTime equal" );
   assert.equal( updateTime5, updateTime4, "expected updateTime equal" );

   // 6.删除CL
   db.getCS( csName ).dropCL( newClName );
   var cursor6 = db.exec( "select * from $LIST_CS  where Name = 'csName_28194' " );
   var createTime6 = cursor6.current().toObj()["CreateTime"];
   var updateTime6 = cursor6.current().toObj()["UpdateTime"];
   assert.equal( createTime6, createTime5, "expected createTime equal" );
   if( updateTime6 <= updateTime5 ) 
   {
      throw new Error( "expected post-updateTime to be more than pre-updateTime" );
   }
}
