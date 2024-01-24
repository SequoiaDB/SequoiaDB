/******************************************************
 * @Description: seqDB-10444:主子表使用匹配符查询数据
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

   //splitting needs two groups at least
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
   var lLowBound = 0;
   var lUpBound = 100;
   createSubCL( csName, lSubCLName );
   attachCL( csName, mainCL, lSubCLName, lLowBound, lUpBound );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, lSubCLName, startCondition, null );

   // right subCL, occupying [ 100, 200 )
   var rSubCLName = COMMCLNAME + "_scl_r";
   var rLowBound = 100;
   var rUpBound = 200;
   createSubCL( csName, rSubCLName );
   attachCL( csName, mainCL, rSubCLName, rLowBound, rUpBound );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, rSubCLName, startCondition, null );

   // intRecs means "integer records".
   // range is [ 0, 200 ).
   // for testing $gt, $gte, $lt, $lte, $and, $in, $ne, $et, $mod, $nin, $not, $isnull.
   var intRecs = [];
   var intLowBound = 0;
   var intUpBound = 200;
   intRecs = buildIntRecs( intLowBound, intUpBound );
   mainCL.insert( intRecs );

   // test $gt
   var gtOption = { MCLKEY: { $gt: 0 } };
   var gtExpRes = buildIntRecs( 1, 200 );
   testMatch( mainCL, gtOption, gtExpRes, "$gt" );

   // test $gte
   var gteOption = { MCLKEY: { $gte: 100 } };
   var gteExpRes = buildIntRecs( 100, 200 );
   testMatch( mainCL, gteOption, gteExpRes, "$gte" );

   // test $lt
   var ltOption = { MCLKEY: { $lt: 100 } };
   var ltExpRes = buildIntRecs( 0, 100 );
   testMatch( mainCL, ltOption, ltExpRes, "$lt" );

   var lteOption = { MCLKEY: { $lte: 100 } };
   var lteExpRes = buildIntRecs( 0, 101 );
   testMatch( mainCL, lteOption, lteExpRes, "$lte" );

   // test $and, $in, $ne
   var andInNeOptions = {
      $and: [{ MCLKEY: { $in: [20, 21, 22, 23, 24, 25, 300] } },
      { MCLKEY: { $ne: 22 } }]
   };
   var andInNeExpRes = [{ MCLKEY: 20 },
   { MCLKEY: 21 },
   { MCLKEY: 23 },
   { MCLKEY: 24 },
   { MCLKEY: 25 }];
   testMatch( mainCL, andInNeOptions, andInNeExpRes, "$and $in $ne" );

   // test $et
   var etOption = { MCLKEY: { $et: 100 } };
   var etExpRes = [{ MCLKEY: 100 }];
   testMatch( mainCL, etOption, etExpRes, "$et" );

   // test $mod
   var modOption = { MCLKEY: { $mod: [50, 0] } };
   var modExpRes = [{ MCLKEY: 0 },
   { MCLKEY: 50 },
   { MCLKEY: 100 },
   { MCLKEY: 150 }];
   testMatch( mainCL, modOption, modExpRes, "$mod" );

   // test $nin
   var ninOption = { MCLKEY: { $nin: [300, 100, 0] } };
   var ninExpRes = buildIntRecs( 1, 100 ).concat( buildIntRecs( 101, 200 ) );
   testMatch( mainCL, ninOption, ninExpRes, "$nin" );

   // test $not, $isnull
   var notIsnullOptions = { $not: [{ MCLKEY: { $isnull: 1 } }] };
   var notIsnullExpRes = buildIntRecs( 0, 200 );
   testMatch( mainCL, notIsnullOptions, notIsnullExpRes, "$not $isnull" );

   // unset variable
   commDropCL( db, csName, mainCLName, true, true, "Fail to drop CL in the end" );
}

function createMainCL ( csName, mainCLName )
{
   var options = { ShardingKey: { MCLKEY: 1 }, ShardingType: "range", IsMainCL: true };
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

function buildIntRecs ( lowBound, upBound )
{
   var intRecs = [];
   for( var i = lowBound; i < upBound; i++ )
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
