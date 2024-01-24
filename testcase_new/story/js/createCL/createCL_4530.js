/************************************
*@Description:  createCL，覆盖name有效字符和边界_st.verify.CL.001
*@author:      wangkexin
*@createDate:  2019.6.6
*@testlinkCase: seqDB-4530
**************************************/
main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = groupsArray[0][0].GroupName;

   var csName = COMMCSNAME;
   var clNames = ["a", "123-test-中文。~!@#%^&*()_+-=\][|,/<>?~·！@#￥%……&*~!-%^*()_-+=|\/<'?*[];:/#（）——+《》{}|【】、，。~_127B"];
   var clNames2 = ["b", "123-test-中文。another_test-=\][|）——,/<>?~·！@#￥%……&*~!-%^*()_-+=|\/<'?*[];:/#（+《:{}|//、，。~_127B"];
   for( var i = 0; i < clNames.length; i++ )
   {
      commDropCL( db, csName, clNames[i], true, true, "drop cl " + clNames[i] + " in the beginning." );
      commDropCL( db, csName, clNames2[i], true, true, "drop cl " + clNames[i] + " in the beginning." );
   }

   //name 覆盖1字节有效字符
   var optionObj = { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, Compressed: true, CompressionType: "lzw", AutoSplit: true };
   var cl1 = commCreateCL( db, csName, clNames[0], optionObj, true );

   checkResult( clNames[0], "ShardingKey", { a: 1 } );
   checkResult( clNames[0], "ShardingType", "hash" );
   checkResult( clNames[0], "Partition", 1024 );
   checkResult( clNames[0], "AttributeDesc", "Compressed" );
   checkResult( clNames[0], "CompressionTypeDesc", "lzw" );
   checkResult( clNames[0], "AutoSplit", true );
   checkCLByInsertData( cl1 );

   //name 覆盖127字节有效字符
   var optionObj = { ShardingKey: { a: 1 }, ShardingType: "range", Group: groupName };
   var cl2 = commCreateCL( db, csName, clNames[1], optionObj, true );

   checkResult( clNames[1], "ShardingKey", { a: 1 } );
   checkResult( clNames[1], "ShardingType", "range" );
   checkGroup( clNames[1], groupName );
   checkCLByInsertData( cl2 );

   //检测 IsMainCL  覆盖边界值
   var optionObj = { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true };
   var cl3 = commCreateCL( db, csName, clNames2[0], optionObj, true );

   checkResult( clNames2[0], "ShardingKey", { a: 1 } );
   checkResult( clNames2[0], "ShardingType", "range" );
   checkResult( clNames2[0], "IsMainCL", true );
   commCreateCL( db, csName, "subcl4530_1" );
   cl3.attachCL( csName + ".subcl4530_1", { LowBound: { a: 0 }, UpBound: { a: 2000 } } );
   checkCLByInsertData( cl3 );

   var optionObj = { ShardingKey: { a: 1 }, IsMainCL: true };
   var cl4 = commCreateCL( db, csName, clNames2[1], optionObj, true );

   checkResult( clNames2[1], "ShardingKey", { a: 1 } );
   checkResult( clNames2[1], "IsMainCL", true );
   commCreateCL( db, csName, "subcl4530_2" );
   cl4.attachCL( csName + ".subcl4530_2", { LowBound: { a: 0 }, UpBound: { a: 2000 } } );
   checkCLByInsertData( cl4 );

   for( var i = 0; i < clNames.length; i++ )
   {
      commDropCL( db, csName, clNames[i], true, true, "drop cl " + clNames[i] + " in the end." );
      commDropCL( db, csName, clNames2[i], true, true, "drop cl " + clNames2[i] + " in the end." );
   }
}

function checkResult ( clName, fieldName, expFieldValue )
{
   var clFullName = COMMCSNAME + "." + clName;
   var cur = db.snapshot( 8, { "Name": clFullName } );
   var actualFieldValue = cur.current().toObj()[fieldName];
   assert.equal( expFieldValue, actualFieldValue );

}

function checkGroup ( clName, groupName )
{
   var actGroups = [];
   var expGroups = [groupName]
   var clFullName = COMMCSNAME + "." + clName;
   var cur = db.snapshot( 4, { "Name": clFullName } );
   if( cur.next() )
   {
      var groups = cur.current().toObj()["Details"];
      for( var i = 0; i < groups.length; i++ )
      {
         actGroups.push( groups[i].GroupName );
      }
   }
   assert.equal( expGroups, actGroups );
}

function checkCLByInsertData ( cl )
{
   var dataArray = new Array();
   for( var i = 0; i < 1000; i++ )
   {
      var data = { a: i };
      dataArray.push( data );
   }
   cl.insert( dataArray );

   var actCount = cl.count();
   assert.equal( actCount, 1000 );

}