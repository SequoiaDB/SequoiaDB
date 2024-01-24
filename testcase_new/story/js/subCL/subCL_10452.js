/************************************
*@Description: 分区范围正序，查询条件带主表分区键，查询条件被主表的LowBond-UpBound不完全包含，进行查询/更新/删除
*@author:      zengxianquan
*@createDate:  2016.11.25
*@testlinkCase: seqDB-10452
**************************************/

main( test );

function test ()
{
   mainCL_Name = CHANGEDPREFIX + "_maincl";
   subCL_Name1 = CHANGEDPREFIX + "_subcl1";
   subCL_Name2 = CHANGEDPREFIX + "_subcl2";

   //检查环境，是否可以拆分
   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //少于两个数据组，不适合测试该用例
   var allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }
   //在测试前清除环境中冲突的表
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true );
   commDropCL( db, COMMCSNAME, subCL_Name1, true, true );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true );
   prepareByPositiveSequence( mainCL_Name, subCL_Name1, subCL_Name2 );
   db.setSessionAttr( { PreferedInstance: "M" } );
   var maincl = db.getCS( COMMCSNAME ).getCL( mainCL_Name );
   //检测查询
   checkFind( maincl );
   //检测更新
   checkUpdate( maincl );
   //检测删除
   checkRemove( maincl );

   //清除环境
   db.getCS( COMMCSNAME ).dropCL( mainCL_Name );
}
//检测查询
function checkFind ( maincl )
{
   var realData = maincl.find( { a: { $gte: 9 } } ).sort( { a: 1, b: 1 } );
   var expectDataArray = [];
   for( var i = 9; i < 20; i++ )
   {
      for( var j = 0; j < 100; j++ )
      {
         expectDataArray.push( { a: i, b: j, test: "testData" + j } );
      }

   }
   //检查插入的数据与预期的数据是否一致
   zxqCheckRec( realData, expectDataArray );
}
//检测更新
function checkUpdate ( maincl )
{
   //更新数据
   maincl.update( { $set: { "test": "update" } }, { a: { $gte: 9 } } );
   var expectDataArray = [];
   for( var i = 9; i < 20; i++ )
   {
      for( var j = 0; j < 100; j++ )
      {
         expectDataArray.push( { a: i, b: j, test: "update" } );
      }
   }
   var realData = maincl.find( { a: { $gte: 9 } } ).sort( { a: 1, b: 1 } );
   //更新后对数据的检验
   zxqCheckRec( realData, expectDataArray );
}
//检测删除
function checkRemove ( maincl )
{
   //删除数据
   maincl.remove( { a: { $lt: 17 } } );
   var expectDataArray = [];
   for( var i = 17; i < 20; i++ )
   {
      for( var j = 0; j < 100; j++ )
      {
         expectDataArray.push( { a: i, b: j, test: "update" } );
      }
   }
   var realData = maincl.find( { a: { $gte: 17 } } ).sort( { a: 1, b: 1 } );
   //删除后对数据的检验
   zxqCheckRec( realData, expectDataArray );
}
