/**************************************
 * @Description: 测试分区范围为多分区键，包含部分子表分区键字段进行批量查询/更新/删除
 * @author: ouyangzhongnan 
 * @Date: 2016-11-22
 * @RunDemo:
 * /opt/sequoiadb/bin/sdb -f "func.js,commlib.js,subCL_containPart_subclSK_CRUD_10457.js" -e "var CHANGEDPREFIX='prefix';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/

main( test );
function test ()
{
   //设置变量初始化值
   var mainCl = null;
   var subCls = [];
   var data = [];
   var mainClName = CHANGEDPREFIX + "_maincl_10457";
   var subClNames = [
      CHANGEDPREFIX + "_subcl_10457_0",
      CHANGEDPREFIX + "_subcl_10457_1",
   ];

   //check test environment before split
   if( true == commIsStandalone( db ) ) 
   {
      return;
   }
   //less two groups,can not split
   if( 1 === getGroupName( db ).length ) 
   {
      return;
   }

   //clean the environment, drop subcl and maincl
   commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
   //create maincl and subcl, attach subcl
   db.setSessionAttr( { PreferedInstance: "M" } );
   var mainCLOption = { IsMainCL: true, ShardingKey: { a: -1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   var subCLOption = { ShardingKey: { b: 1, c: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   mainCl = commCreateCL( db, COMMCSNAME, mainClName, mainCLOption, true, true );
   subCls.push( commCreateCL( db, COMMCSNAME, subClNames[0] ) );
   subCls.push( commCreateCL( db, COMMCSNAME, subClNames[1], subCLOption, true, true ) );
   attachCL( mainCl, COMMCSNAME + "." + subClNames[0], { LowBound: { a: 20 }, UpBound: { a: 10 } } );
   attachCL( mainCl, COMMCSNAME + "." + subClNames[1], { LowBound: { a: 10 }, UpBound: { a: 0 } } );
   //init data
   var query_B15 = [];  //保存数据供批量查询对比结果用的, 查询条件为{a:5}
   for( var i = 0; i < 100; i++ ) 
   {
      var mainSkValue = i % 20 + 1;
      data.push( { a: mainSkValue, b: mainSkValue, c: mainSkValue } );
      if( mainSkValue === 15 ) 
      {
         query_B15.push( { a: mainSkValue, b: mainSkValue, c: mainSkValue } );
      }
   }
   mainCl.insert( data );
   ClSplitOneTimes( COMMCSNAME, subClNames[1], 50, null );

   //query test
   assert.equal( mainCl.find( { b: 15 } ).count(), query_B15.length );

   //update test
   mainCl.update( { $set: { name: "update_name" } }, { c: 5 } );
   var cursor = mainCl.find( { c: 5 } );
   for( var i = 0; i < cursor.count(); i++ ) 
   {
      assert.equal( cursor.current().toObj().name, "update_name" );
   }

   //delete test
   mainCl.remove( { b: 13 } );
   assert.equal( mainCl.find( { b: 13 } ).count(), 0 );

   // clean the environment, drop subcl and maincl
   commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
}