/*******************************************************************************
*@Description: find().sort({a:1|-1}).skip(number).limit(number)
*@Modify list:
*   2014-5-5 wenjing wang  Init
*   2019-08-23 wangkexin Modified
*******************************************************************************/
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_13756";
testConf.clOpt = { "ShardingType": "range", "IsMainCL": true, "ReplSize": 0, "ShardingKey": { "a": 1 } };
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

var csName = testConf.csName;

main( test );

function test ( testPara )
{
   var mainCL = testPara.testCL;
   var groupInfo = testPara.groups;

   var subCLName_1 = "subCL_13756_1";
   var subCLName_2 = "subCL_13756_2";

   var subCLOpt_1 = {};
   var subCLOpt_2 = { "ShardingType": "range", "ReplSize": 0, "ShardingKey": { "a": 1 } };

   var subCL_1 = commCreateCL( db, csName, subCLName_1, subCLOpt_1 );
   var subCL_2 = commCreateCL( db, csName, subCLName_2, subCLOpt_2 );

   var groupNum = groupInfo.length;
   var totalNum = 5 * groupNum * 2;
   mainCL.attachCL( csName + "." + subCLName_1, { LowBound: { a: 0 }, UpBound: { a: totalNum / 2 } } );
   mainCL.attachCL( csName + "." + subCLName_2, { LowBound: { a: totalNum / 2 }, UpBound: { a: totalNum } } );

   splitTable( db, subCL_2, subCLName_2, groupInfo, "a", 5 * groupNum );

   var data = [];
   var sub_data_1 = [];
   var sub_data_2 = [];
   for( var i = 0; i < totalNum; i++ )
   {
      data.push( { _id: i, a: i } );

      if( i < ( totalNum / 2 ) )
      {
         sub_data_1.push( { _id: i, a: i } );
      }
      else
      {
         sub_data_2.push( { _id: i, a: i } );
      }
   }
   mainCL.insert( data );

   // 普通表上测试sort({a:1|-1}).skip(1).limit(1)
   findAndCheck( subCL_1, { a: 1 }, 1, 1, sub_data_1 );
   findAndCheck( subCL_1, { a: -1 }, 1, 1, sub_data_1 );

   // 普通表上测试sort({a:1|-1}).skip(10).limit(1)
   findAndCheck( subCL_1, { a: 1 }, 10, 1, sub_data_1 );
   // 普通表上测试sort({a:1|-1}).skip(1).limit(10)
   findAndCheck( subCL_1, { a: -1 }, 1, 10, sub_data_1 );

   // skip一个组的记录
   // 水平分区表上测试sort({a:1|-1}).skip(5).limit(1)
   findAndCheck( subCL_2, { a: 1 }, 5, 1, sub_data_2 );
   findAndCheck( subCL_2, { a: -1 }, 5, 1, sub_data_2 );

   // limit一个组的记录
   // 水平分区表上测试sort({a:1|-1}).skip(0).limit(5)
   findAndCheck( subCL_2, { a: 1 }, 0, 5, sub_data_2 );
   findAndCheck( subCL_2, { a: -1 }, 0, 5, sub_data_2 );

   // skip一个组的记录后，limit一个组的记录
   // 水平分区表上测试sort({a:1|-1}).skip(5).limit(5)
   findAndCheck( subCL_2, { a: 1 }, 5, 5, sub_data_2 );
   findAndCheck( subCL_2, { a: -1 }, 5, 5, sub_data_2 );

   // skip一个组的记录
   // 垂直分区表上测试sort({a:1|-1}).limit(1)
   findAndCheck( mainCL, { a: 1 }, 5, 1, data );
   findAndCheck( mainCL, { a: -1 }, 5, 1, data );

   // limit一个组的记录
   // 垂直分区表上测试sort({a:1|-1}).limit(5)
   findAndCheck( mainCL, { a: 1 }, 0, 5, data );
   findAndCheck( mainCL, { a: -1 }, 0, 5, data );

   // skip一个组的记录后，limit一个组的记录
   // 垂直分区表上测试sort({a:1|-1}).skip(5).limit(5)
   findAndCheck( mainCL, { a: 1 }, 5, 5, data );
   findAndCheck( mainCL, { a: -1 }, 5, 5, data );

   // skip一个子表的记录
   // 垂直分区表上测试sort({a:1|-1}).skip(totalNum/2).limit(1)
   findAndCheck( mainCL, { a: 1 }, totalNum / 2, 1, data );
   findAndCheck( mainCL, { a: -1 }, totalNum / 2, 1, data );

   // skip一个子表的记录后,limit一个子表的记录
   // 垂直分区表上测试sort({a:1|-1}).skip(totalNum/2).limit(totalNum/2)
   findAndCheck( mainCL, { a: 1 }, totalNum / 2, totalNum / 2, data );
   findAndCheck( mainCL, { a: -1 }, totalNum / 2, totalNum / 2, data );

   commDropCL( db, csName, subCLName_1 );
   commDropCL( db, csName, subCLName_2 );
}

function splitTable ( db, cl, clName, dataGroups, keyField, startVal )
{
   var fullName = csName + "." + clName;
   var srcGroups = commGetCLGroups( db, fullName );
   if( srcGroups.length > 1 )
   {
      return srcGroups.length;
   }

   var startID = startVal;
   var endID = startVal + 5;
   var startObj = new Object();
   var endObj = new Object();
   for( var i = 0; i < dataGroups.length; ++i )
   {
      if( dataGroups[i][0].GroupName != srcGroups[0] )
      {
         startObj[keyField] = startID;
         endObj[keyField] = endID;
         cl.split( srcGroups[0], dataGroups[i][0].GroupName, startObj, endObj );
         startID = endID;
         endID += 5;
      }
   }

}

function getExpRec ( data, sortObj, skipNum, limitNum )
{
   var expectation = [];

   if( sortObj["a"] == -1 )
   {
      for( var i = data.length; i > 0; i-- ) { expectation.push( data[i - 1] ); }
   }
   else
   {
      expectation = data;
   }

   if( skipNum < 0 ) { skipNum = 0 }
   if( skipNum >= expectation.length ) { return []; }

   var tmp = [];
   for( var i = skipNum; i < expectation.length; i++ ) { tmp.push( expectation[i] ); }
   expectation = tmp;

   if( limitNum < 0 || limitNum > expectation.length ) { limitNum = expectation.length }
   var tmp = [];
   for( var i = 0; i < limitNum; i++ ) { tmp.push( expectation[i] ); }
   expectation = tmp;

   return expectation;

}

function findAndCheck ( cl, sortCondition, skipNum, limitNum, data )
{
   var records = cl.find().sort( sortCondition ).skip( skipNum ).limit( limitNum );
   var expectation = getExpRec( data, sortCondition, skipNum, limitNum );
   checkRec( records, expectation );
}
