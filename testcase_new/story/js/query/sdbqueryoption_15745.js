/******************************************************************************
*@Description : test SdbQueryOption
*               TestLink :   seqDB-15745:主子表上执行sort/limit/skip参数混合查询
*                            seqDB-15746:主子表上执行cond/update参数混合查询
*                            seqDB-15747:主子表上执行cond/sel/sort/hint参数混合查询
*                            seqDB-15748:主子表上执行cond/remove参数混合查询
*@auhor       : CSQ   2018-09-20 
*               liuli 2020-10-19
******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var clName = COMMCLNAME + "main_15745";
   var subclName1 = COMMCLNAME + "subcl1_15745";
   var subclName2 = COMMCLNAME + "subcl2_15745";
   commDropCL( db, COMMCSNAME, clName );
   commDropCL( db, COMMCSNAME, subclName1 );
   commDropCL( db, COMMCSNAME, subclName2 );
   var cl = commCreateCL( db, COMMCSNAME, clName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   var subcl1 = commCreateCL( db, COMMCSNAME, subclName1, { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024 } );
   var subcl2 = commCreateCL( db, COMMCSNAME, subclName2, { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024 } );

   cl.attachCL( COMMCSNAME + "." + subclName1, { LowBound: { a: 0 }, UpBound: { a: 50 } } );
   cl.attachCL( COMMCSNAME + "." + subclName2, { LowBound: { a: 50 }, UpBound: { a: 100 } } );

   cl.createIndex( "bindex", { b: 1 }, false );
   insertRecord( cl );
   testcombination15745( cl );
   testcombination15746( cl );
   testcombination15747( cl );
   testcombination15748( cl );
   commDropCL( db, COMMCSNAME, clName );
   commDropCL( db, COMMCSNAME, subclName1 );
   commDropCL( db, COMMCSNAME, subclName2 );

}

// sort/limit/skip参数混合查询
function testcombination15745 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { a: 1 } ).skip( 45 ).limit( 10 ) );
   var expFindResult = [{ "_id": 45, "a": 45, "b": 45, "c": -45 },
   { "_id": 46, "a": 46, "b": 46, "c": -46 },
   { "_id": 47, "a": 47, "b": 47, "c": -47 },
   { "_id": 48, "a": 48, "b": 48, "c": -48 },
   { "_id": 49, "a": 49, "b": 49, "c": -49 },
   { "_id": 50, "a": 50, "b": 50, "c": -50 },
   { "_id": 51, "a": 51, "b": 51, "c": -51 },
   { "_id": 52, "a": 52, "b": 52, "c": -52 },
   { "_id": 53, "a": 53, "b": 53, "c": -53 },
   { "_id": 54, "a": 54, "b": 54, "c": -54 }
   ];
   commCompareResults( cur, expFindResult, false );
}

// cond/update参数混合查询
function testcombination15746 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { a: 1 } ).cond( { $and: [{ a: { $gte: 45 } }, { b: { $lte: 54 } }] } ).update( { $set: { c: 1 } } ) );
   while( cur.next() )
   {
      cur.current();
   }
   var expFindResult = [{ "_id": 45, "a": 45, "b": 45, "c": 1 },
   { "_id": 46, "a": 46, "b": 46, "c": 1 },
   { "_id": 47, "a": 47, "b": 47, "c": 1 },
   { "_id": 48, "a": 48, "b": 48, "c": 1 },
   { "_id": 49, "a": 49, "b": 49, "c": 1 },
   { "_id": 50, "a": 50, "b": 50, "c": 1 },
   { "_id": 51, "a": 51, "b": 51, "c": 1 },
   { "_id": 52, "a": 52, "b": 52, "c": 1 },
   { "_id": 53, "a": 53, "b": 53, "c": 1 },
   { "_id": 54, "a": 54, "b": 54, "c": 1 }
   ];
   var cur = varCL.find( new SdbQueryOption().sort( { a: 1 } ).cond( { $and: [{ a: { $gte: 45 } }, { b: { $lte: 54 } }] } ) );
   commCompareResults( cur, expFindResult, false );
}

// cond/sel/sort/hint参数混合查询
function testcombination15747 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().cond( { $and: [{ a: { $gte: 45 } }, { b: { $lt: 55 } }] } ).sel( { _id: { $include: 1 }, a: { $include: 1 }, b: { $include: 1 } } ).sort( { b: 1 } ).hint( { "": "bindex" } ) );
   var expFindResult = [{ "_id": 45, "a": 45, "b": 45 },
   { "_id": 46, "a": 46, "b": 46 },
   { "_id": 47, "a": 47, "b": 47 },
   { "_id": 48, "a": 48, "b": 48 },
   { "_id": 49, "a": 49, "b": 49 },
   { "_id": 50, "a": 50, "b": 50 },
   { "_id": 51, "a": 51, "b": 51 },
   { "_id": 52, "a": 52, "b": 52 },
   { "_id": 53, "a": 53, "b": 53 },
   { "_id": 54, "a": 54, "b": 54 }
   ];
   commCompareResults( cur, expFindResult, false );
}

// cond/remove参数混合查询
function testcombination15748 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().cond( { $or: [{ a: { $lt: 45 } }, { b: { $gte: 55 } }] } ).remove() );
   while( cur.next() )
   {
      var ret = cur.current();
   }
   var cur = varCL.find( new SdbQueryOption().sort( { _id: 1 } ) );
   var expFindResult = [{ "_id": 45, "a": 45, "b": 45, "c": 1 },
   { "_id": 46, "a": 46, "b": 46, "c": 1 },
   { "_id": 47, "a": 47, "b": 47, "c": 1 },
   { "_id": 48, "a": 48, "b": 48, "c": 1 },
   { "_id": 49, "a": 49, "b": 49, "c": 1 },
   { "_id": 50, "a": 50, "b": 50, "c": 1 },
   { "_id": 51, "a": 51, "b": 51, "c": 1 },
   { "_id": 52, "a": 52, "b": 52, "c": 1 },
   { "_id": 53, "a": 53, "b": 53, "c": 1 },
   { "_id": 54, "a": 54, "b": 54, "c": 1 }
   ];
   commCompareResults( cur, expFindResult, false );
}

function insertRecord ( varCL )
{
   for( var i = 0; i < 100; i++ )
   {
      varCL.insert( { _id: i, a: i, b: i, c: -i } );
   }
}
