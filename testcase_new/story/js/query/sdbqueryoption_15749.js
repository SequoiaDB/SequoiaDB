/******************************************************************************
*@Description : test SdbQueryOption
*               TestLink :   seqDB-15749:指定sdbQueryOption参数多次查询记录
*@auhor       : CSQ   2018-09-20 
*               liuli 2020-10-15
******************************************************************************/
testConf.clName = COMMCLNAME + "_15749";

main( test );

function test ( args )
{
   var cl = args.testCL;
   insertRecord( cl );
   test15749( cl );
}

function test15749 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { _id: 1 } ).cond( { $and: [{ a: { $gte: 45 } }, { b: { $lte: 54 } }] } ).update( { $inc: { c: 1 } }, true, { KeepShardingKey: true } ) );
   while( cur.next() )
   {
      cur.current();
   }
   var expFindResult = [{ "_id": 45, "a": 45, "b": 45, "c": -44 },
   { "_id": 46, "a": 46, "b": 46, "c": -45 },
   { "_id": 47, "a": 47, "b": 47, "c": -46 },
   { "_id": 48, "a": 48, "b": 48, "c": -47 },
   { "_id": 49, "a": 49, "b": 49, "c": -48 },
   { "_id": 50, "a": 50, "b": 50, "c": -49 },
   { "_id": 51, "a": 51, "b": 51, "c": -50 },
   { "_id": 52, "a": 52, "b": 52, "c": -51 },
   { "_id": 53, "a": 53, "b": 53, "c": -52 },
   { "_id": 54, "a": 54, "b": 54, "c": -53 }
   ];
   var cur = varCL.find( new SdbQueryOption().sort( { a: 1 } ).cond( { $and: [{ a: { $gte: 45 } }, { b: { $lte: 54 } }] } ) );
   commCompareResults( cur, expFindResult, false );

   var cur = varCL.find( new SdbQueryOption().sort( { _id: 1 } ).cond( { $and: [{ a: { $gt: 90 } }, { b: { $lte: 100 } }] } ) );
   var expFindResult = [{ "_id": 91, "a": 91, "b": 91, "c": -91 },
   { "_id": 92, "a": 92, "b": 92, "c": -92 },
   { "_id": 93, "a": 93, "b": 93, "c": -93 },
   { "_id": 94, "a": 94, "b": 94, "c": -94 },
   { "_id": 95, "a": 95, "b": 95, "c": -95 },
   { "_id": 96, "a": 96, "b": 96, "c": -96 },
   { "_id": 97, "a": 97, "b": 97, "c": -97 },
   { "_id": 98, "a": 98, "b": 98, "c": -98 },
   { "_id": 99, "a": 99, "b": 99, "c": -99 },
   { "_id": 100, "a": 100, "b": 100, "c": -100 }
   ];
   commCompareResults( cur, expFindResult, false );

   var cur = varCL.find( new SdbQueryOption().sort( { b: 1 } ).cond( { b: { $lt: 5 } } ) );
   var expFindResult = [{ "_id": 0, "a": 0, "b": 0, "c": 0 },
   { "_id": 1, "a": 1, "b": 1, "c": -1 },
   { "_id": 2, "a": 2, "b": 2, "c": -2 },
   { "_id": 3, "a": 3, "b": 3, "c": -3 },
   { "_id": 4, "a": 4, "b": 4, "c": -4 }];
   commCompareResults( cur, expFindResult, false );
}

function insertRecord ( varCL )
{
   for( var i = 0; i <= 100; i++ )
   {
      varCL.insert( { _id: i, a: i, b: i, c: -i } );
   }
}
