/******************************************************
 * @Description: seqDB-10444:主子表使用匹配符查询数据（补充测试逆序情况）
 * @Author: linsuqiang 
 * @Date: 2016-11-29
 ******************************************************/

main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   //splitting need two groups at least
   allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }

   var csName = COMMCSNAME;
   var mainCLName = COMMCLNAME + "_mcl";

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the beginning" );
   // create mainCL and subCLs
   db.setSessionAttr( { PreferedInstance: "M" } );
   var mainCL = createMainCL( csName, mainCLName );

   // left subCL, occupying [ 0, 100 )
   var lSubCLName = COMMCLNAME + "_scl_l";
   var lLowBound = 200;
   var lUpBound = 100;
   createSubCL( csName, lSubCLName );
   attachCL( csName, mainCL, lSubCLName, lLowBound, lUpBound );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, lSubCLName, startCondition, null );

   // right subCL, occupying [ 100, 200 )
   var rSubCLName = COMMCLNAME + "_scl_r";
   var rLowBound = 100;
   var rUpBound = 0;
   createSubCL( csName, rSubCLName );
   attachCL( csName, mainCL, rSubCLName, rLowBound, rUpBound );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, rSubCLName, startCondition, null );

   // intRecs means "integer records".
   // range is [ 0, 200 ).
   var intRecs = [];
   var intLowBound = 200;
   var intUpBound = 0;
   intRecs = buildIntRecs( intLowBound, intUpBound );
   mainCL.insert( intRecs );
   // test $et
   var etOption_1 = { MCLKEY: { $et: 200 } };
   var etExpRes_1 = [{ MCLKEY: 200 }];
   testMatch( mainCL, etOption_1, etExpRes_1, "$et no.1" );

   var etOption_2 = { MCLKEY: { $et: 100 } };
   var etExpRes_2 = [{ MCLKEY: 100 }];
   testMatch( mainCL, etOption_2, etExpRes_2, "$et no.2" );
   // test $gte
   var gteOption_1 = { MCLKEY: { $gte: 200 } };
   var gteExpRes_1 = [{ MCLKEY: 200 }];
   testMatch( mainCL, gteOption_1, gteExpRes_1, "$gte no.1" );

   var gteOption_2 = { MCLKEY: { $gte: 100 } };
   var gteExpRes_2 = buildIntRecs( 200, 99 );
   testMatch( mainCL, gteOption_2, gteExpRes_2, "$gte no.2" );
   // test $lte
   var lteOption_1 = { MCLKEY: { $lte: 200 } };
   var lteExpRes_1 = buildIntRecs( 200, 0 );
   testMatch( mainCL, lteOption_1, lteExpRes_1, "$lte no.1" );

   var lteOption_2 = { MCLKEY: { $lte: 100 } };
   var lteExpRes_2 = buildIntRecs( 100, 0 );
   testMatch( mainCL, lteOption_2, lteExpRes_2, "$lte no.2" );

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the end" );
}

function createMainCL ( csName, mainCLName )
{
   var options = { ShardingKey: { MCLKEY: -1 }, ShardingType: "range", IsMainCL: true };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false, true );
   return mainCL;
}

function createSubCL ( csName, subCLName )
{
   var options = { ShardingKey: { SCLKEY: 1 }, ShardingType: "hash" };
   subCL = commCreateCL( db, csName, subCLName, options, false, true, "Failed to create subCL" );
   return subCL;
}

function attachCL ( csName, mainCL, subCLName, lowBound, upBound )
{
   var options = {
      LowBound: { MCLKEY: lowBound },
      UpBound: { MCLKEY: upBound }
   };
   mainCL.attachCL( csName + "." + subCLName, options );
}
// in invert sequence! 
function buildIntRecs ( lowBound, upBound )
{
   var intRecs = [];
   for( var i = lowBound; i > upBound; i-- )
   {
      intRecs.push( { MCLKEY: i } );
   }
   return intRecs;
}

function testMatch ( mainCL, option, expRes, matchName )
{
   actRes = mainCL.find( option ).sort( { _id: 1 } );
   lsqCheckRec( actRes, expRes );
}