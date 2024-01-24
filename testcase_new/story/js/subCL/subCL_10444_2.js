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
   // special subCL for testing some special data( arrRecs & strRecs )
   var subCLName = COMMCLNAME + "_scl";
   var lowBound = 500;
   var upBound = 1000;
   createSubCL( csName, subCLName );
   attachCL( csName, mainCL, subCLName, lowBound, upBound );
   var startCondition = { Partition: 2048 };
   ClSplitOneTimes( csName, subCLName, startCondition, null );

   // for testing $all, $size, $ + tag, $or
   var arrRecs = [{ arr: ["Tom", "Mike", "Jhon"], MCLKEY: 555 },
   { arr: ["Mike", "Tom", "Jerry"], MCLKEY: 555 },
   { arr: ["Mike", "Tom", "Tim", "Jerry"], MCLKEY: 555 }];
   mainCL.insert( arrRecs );
   // test $all
   var allOption = { arr: { $all: ["Tom", "Mike"] } };
   var allExpRes = [{ arr: ["Tom", "Mike", "Jhon"], MCLKEY: 555 },
   { arr: ["Mike", "Tom", "Jerry"], MCLKEY: 555 },
   { arr: ["Mike", "Tom", "Tim", "Jerry"], MCLKEY: 555 }];
   testMatch( mainCL, allOption, allExpRes, "$all" );

   // test $size
   var sizeOption = { arr: { $size: 1, $et: 3 } };
   var sizeExpRes = [{ arr: ["Tom", "Mike", "Jhon"], MCLKEY: 555 },
   { arr: ["Mike", "Tom", "Jerry"], MCLKEY: 555 }];
   testMatch( mainCL, sizeOption, sizeExpRes, "$size" );

   // test $or, $+tag
   var orTagOptions = { $or: [{ "arr.$1": "Tim" }, { "arr.$1": "Jerry" }] };
   var orTagExpRes = [{ arr: ["Mike", "Tom", "Jerry"], MCLKEY: 555 },
   { arr: ["Mike", "Tom", "Tim", "Jerry"], MCLKEY: 555 }];
   testMatch( mainCL, orTagOptions, orTagExpRes, "$or $+tag" );

   cleanUpData( mainCL );

   // for testing $type, $exists, $elemMatch, $regex, field, isnull
   var strRecs = [{ str_1: "Hello World", str_2: "Hello World", MCLKEY: 555 },
   { str_1: "hello friends", str_2: "Hi friends", MCLKEY: 555 },
   { str_1: "Hell oworld", MCLKEY: 555 },
   { str_1: { str_3: "Hello Hello" }, str_2: null, MCLKEY: 555 }];
   mainCL.insert( strRecs );

   // test $type
   var typeOption = { str_1: { $type: 1, $et: 2 } };
   var typeExpRes = [{ str_1: "Hello World", str_2: "Hello World", MCLKEY: 555 },
   { str_1: "hello friends", str_2: "Hi friends", MCLKEY: 555 },
   { str_1: "Hell oworld", MCLKEY: 555 }];
   testMatch( mainCL, typeOption, typeExpRes, "$type" );

   // test $exist
   var existsOption = { str_2: { $exists: 0 } };
   var existsExpRes = [{ str_1: "Hell oworld", MCLKEY: 555 }];
   testMatch( mainCL, existsOption, existsExpRes, "$exists" );

   // test $elemMatch
   var elemOption = { str_1: { $elemMatch: { str_3: "Hello Hello" } } };
   var elemExpRes = [{ str_1: { str_3: "Hello Hello" }, str_2: null, MCLKEY: 555 }];
   testMatch( mainCL, elemOption, elemExpRes, "$elemMatch" );

   // test $regex
   var regexOption = { str_1: { $regex: 'hello .*', $options: 'i' } };
   var regexExpRes = [{ str_1: "Hello World", str_2: "Hello World", MCLKEY: 555 },
   { str_1: "hello friends", str_2: "Hi friends", MCLKEY: 555 }];
   testMatch( mainCL, regexOption, regexExpRes, "$regex" );

   // test $field
   var fieldOption = { str_1: { $field: "str_2" } };
   var fieldExpRes = [{ str_1: "Hello World", str_2: "Hello World", MCLKEY: 555 }];
   testMatch( mainCL, fieldOption, fieldExpRes, "$field" );

   // test isnull
   var isnullOption = { str_2: { $isnull: 1 } };
   var isnullExpRes = [{ str_1: "Hell oworld", MCLKEY: 555 },
   { str_1: { str_3: "Hello Hello" }, str_2: null, MCLKEY: 555 }];
   testMatch( mainCL, isnullOption, isnullExpRes, "$isnull" );

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

function cleanUpData ( mainCL )
{
   mainCL.remove();
}

function testMatch ( mainCL, option, expRes, matchName )
{
   actRes = mainCL.find( option ).sort( { _id: 1 } );
   lsqCheckRec( actRes, expRes );
}
