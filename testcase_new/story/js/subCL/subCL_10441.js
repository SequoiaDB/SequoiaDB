/************************************
*@Description: 数据落在相同组上的不同子表中,批量插入数据
*@author:      zengxianquan
*@createDate:  2016.11.24
*@testlinkCase: seqDB-10441
**************************************/
main( test );
function test ()
{
   //check test environment before split
   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups,can not split
   var allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }

   mainCL_Name = CHANGEDPREFIX + "_maincl";
   subCL_Name1 = CHANGEDPREFIX + "_subcl1";
   subCL_Name2 = CHANGEDPREFIX + "_subcl2";

   //在测试前清除环境中冲突的表
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true );
   commDropCL( db, COMMCSNAME, subCL_Name1, true, true );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true );

   //获取所有的数据组
   db.setSessionAttr( { PreferedInstance: "M" } );
   var groupsArray = commGetGroups( db, false, "", false, true, true );
   //创建主表
   var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
   var mainCL = commCreateCL( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );
   //创建普通子表
   var groupName = groupsArray[1][0].GroupName;
   var subClOption1 = { Group: groupName };
   commCreateCL( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );
   var subClOption2 = { Group: groupName };
   commCreateCL( db, COMMCSNAME, subCL_Name2, subClOption2, true, true );
   //attach 普通的表
   mainCL.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   //检验分区范围的顺序与插入数据的顺序相同
   checkInsertSameSequence( db, groupName, mainCL_Name, COMMCSNAME, subCL_Name1, subCL_Name2 );
   //检验分区范围的顺序与插入数据的顺序不相同
   checkInsertDiffSequence( db, groupName, mainCL_Name, COMMCSNAME, subCL_Name1, subCL_Name2 );
   //清除环境
   db.getCS( COMMCSNAME ).dropCL( mainCL_Name );
}

function checkInsertSameSequence ( db, groupName, mainCL_Name, csName, subCL_Name1, subCL_Name2 )
{
   var doc1 = [];
   for( var j = 0; j < 200; j++ )
   {
      for( var k = 0; k < 10; k++ )
      {
         doc1.push( { a: j, b: k, test: "testData" + k } );
      }
   }
   //批量插入数据，分区范围的顺序与插入数据的顺序相同
   db.getCS( csName ).getCL( mainCL_Name ).insert( doc1 );
   //检测插入的数据是否一致
   var expectDataArray1 = [];
   for( var i = 0; i < 100; i++ )
   {
      for( var k = 0; k < 10; k++ )
      {
         expectDataArray1.push( { a: i, b: k, test: "testData" + k } );
      }
   }
   var rg = db.getRG( groupName );
   var dataStr = rg.getMaster();
   var data1 = new Sdb( dataStr );
   var realData1 = data1.getCS( csName ).getCL( subCL_Name1 ).find().sort( { a: 1, b: 1 } );
   zxqCheckRec( realData1, expectDataArray1 );
   var expectDataArray2 = [];
   for( var i = 100; i < 200; i++ )
   {
      for( var k = 0; k < 10; k++ )
      {
         expectDataArray2.push( { a: i, b: k, test: "testData" + k } );
      }
   }
   var rg = db.getRG( groupName );
   var dataStr = rg.getMaster();
   var data2 = new Sdb( dataStr );
   var realData2 = data2.getCS( csName ).getCL( subCL_Name2 ).find().sort( { a: 1, b: 1 } );
   zxqCheckRec( realData2, expectDataArray2 );
   db.getCS( csName ).getCL( mainCL_Name ).remove();
}
function checkInsertDiffSequence ( db, groupName, mainCL_Name, csName, subCL_Name1, subCL_Name2 )
{

   //批量插入数据，分区范围的顺序与插入数据的顺序不相同
   var doc2 = [];
   for( var i = 0; i < 100; i++ )
   {
      for( var k = 0; k < 10; k++ )
      {
         doc2.push( { a: 100 * 1 + i, b: k, test: "testData" + k } );
         doc2.push( { a: 100 * 0 + i, b: k, test: "testData" + k } );
      }
   }
   db.getCS( csName ).getCL( mainCL_Name ).insert( doc2 );
   //检测插入的数据是否一致
   var expectDataArray1 = [];
   for( var i = 0; i < 100; i++ )
   {
      for( var k = 0; k < 10; k++ )
      {
         expectDataArray1.push( { a: i, b: k, test: "testData" + k } );
      }
   }
   var rg = db.getRG( groupName );
   var dataStr = rg.getMaster();
   var data1 = new Sdb( dataStr );
   var realData1 = data1.getCS( csName ).getCL( subCL_Name1 ).find().sort( { a: 1, b: 1 } );
   zxqCheckRec( realData1, expectDataArray1 );
   var expectDataArray2 = [];
   for( var i = 100; i < 200; i++ )
   {
      for( var k = 0; k < 10; k++ )
      {
         expectDataArray2.push( { a: i, b: k, test: "testData" + k } );
      }
   }
   var rg = db.getRG( groupName );
   var dataStr = rg.getMaster();
   var data2 = new Sdb( dataStr );
   var realData2 = data2.getCS( csName ).getCL( subCL_Name2 ).find().sort( { a: 1, b: 1 } );
   zxqCheckRec( realData2, expectDataArray2 );
   db.getCS( csName ).getCL( mainCL_Name ).remove();
}