/******************************************************************************
 * @Description   : seqDB-7496:插入数据后对多个hash子表做范围切分
 * @Author        : Lan Tian
 * @CreateTime    : 2021.12.01
 * @LastEditTime  : 2021.12.02
 * @LastEditors   : Lan Tian
 ******************************************************************************/

testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var mclName = "mcl_7496";
   var sclName1 = "scl_7496_1";
   var sclName2 = "scl_7496_2";
   var groups = commGetGroups( db, false, "", false, true, true );
   var srcRG = groups[1][0].GroupName;
   var trgRG = groups[2][0].GroupName;

   commDropCL( db, COMMCSNAME, mclName, true, true, "clean main cl" );
   commDropCL( db, COMMCSNAME, sclName1, true, true, "clean sub cl1" );
   commDropCL( db, COMMCSNAME, sclName2, true, true, "clean sub cl2" );

   // create main cl
   var mOpt = { ShardingKey: { a: 1, b: -1 }, IsMainCL: true };
   var mainCL = commCreateCL( db, COMMCSNAME, mclName, mOpt, true, true );
   // create sub cl
   var sOpt = { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, Group: srcRG };
   var subCL1 = commCreateCL( db, COMMCSNAME, sclName1, sOpt, true, true );
   var sOpt = { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, Group: srcRG };
   var subCL2 = commCreateCL( db, COMMCSNAME, sclName2, sOpt, true, true );
   // attach cl
   mainCL.attachCL( COMMCSNAME + "." + sclName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( COMMCSNAME + "." + sclName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // insert
   var recordsNum = 2000;
   var insertTimes = 3;
   var docs = [];
   for( var i = 0; i < recordsNum; i++ )
   {
      docs.push( { a: i } );
      docs.push( { a: i + 1, b: i + 1 } );
   }
   for( var i = 0; i < insertTimes; i++ )
   {
      mainCL.insert( docs );
   }

   // split
   let taskid1 = subCL1.splitAsync( srcRG, trgRG, { Partition: 500 }, { Partition: 1000 } );
   let taskid2 = subCL2.splitAsync( srcRG, trgRG, { a: 1500 }, { a: 2000 } );
   db.waitTasks( taskid1, taskid2 );

   // check min/maxValue
   var expMainCnt = recordsNum * insertTimes * 2;
   var mainCnt = mainCL.count();
   var maxValue1 = mainCL.find().sort( { a: 1 } ).limit( 1 ).current().toObj()["a"];
   var minValue1 = mainCL.find().sort( { a: -1 } ).limit( 1 ).current().toObj()["a"];
   if( maxValue1 !== 0 || minValue1 !== 2000 || Number( mainCnt ) !== expMainCnt )
   {
      throw new Error( "[check result for mainCL]" +
         "[maxValue1: 0, minValue1: 2000, mainCnt: " + expMainCnt + "]" +
         "[maxValue1: " + maxValue1 + ", minValue1: " + minValue1 + ", mainCnt: " + Number( mainCnt ) + "]" );
   }

   var expSubCnt = recordsNum * insertTimes;
   var subCnt1 = subCL1.count();
   var maxValue1 = subCL1.find().sort( { a: 1 } ).limit( 1 ).current().toObj()["a"];
   var minValue1 = subCL1.find().sort( { a: -1 } ).limit( 1 ).current().toObj()["a"];
   if( maxValue1 !== 0 || minValue1 !== 1000 || Number( subCnt1 ) !== expSubCnt )
   {
      throw new Error( "", null, "[check result for subCL1]" +
         "[maxValue1: 0, minValue1: 1000, subCnt1: " + expSubCnt + "]" +
         "[maxValue1: " + maxValue1 + ", minValue1: " + minValue1 + ", subCnt1: " + Number( subCnt1 ) + "]" );
   }

   var maxValue2 = subCL2.find().sort( { a: 1 } ).limit( 1 ).current().toObj()["a"];
   var minValue2 = subCL2.find().sort( { a: -1 } ).limit( 1 ).current().toObj()["a"];
   var subCnt2 = subCL2.count();
   if( maxValue2 !== 1000 || minValue2 !== 2000 || Number( subCnt2 ) !== expSubCnt )
   {
      throw new Error( "", null, "[check result for subCL1]" +
         "[maxValue2: 0, minValue2: 200, subCnt2: " + expSubCnt + "]" +
         "[maxValue2: " + maxValue2 + ", minValue2: " + minValue2 + ", subCnt2: " + Number( subCnt2 ) + "]" );
   }

   commDropCL( db, COMMCSNAME, mclName, true, true, "clean main cl in the end" );
}