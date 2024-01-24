/******************************************************************************
*@Description : test SdbQueryOption
*               TestLink :    seqDB-15750:使用SdbOptionBase查询记录
*                             seqDB-15729:使用SdbOptionBase查询快照信息
*@auhor       : CSQ   2018-09-20 
*               liuli 2020-10-15
******************************************************************************/
testConf.clName = COMMCLNAME + "_15750";

main( test );

function test ( args )
{
   var cl = args.testCL;
   cl.createIndex( "bindex", { b: 1 }, false );
   insertRecord( cl );
   test15750( cl );
   test15751( cl );
}

function test15750 ( varCL )
{
   var option = new SdbOptionBase();
   option.cond( { b: { $lt: 5 } } ).sel( { _id: { $include: 1 }, a: { $include: 1 }, b: { $include: 1 } } ).sort( { _id: 1 } ).hint( { "": "bindex" } ).limit( 3 ).skip( 1 ).flags( 1 );
   var cur = varCL.find( option );
   var expFindResult = [{ "_id": 1, "a": 1, "b": 1 },
   { "_id": 2, "a": 2, "b": 2 },
   { "_id": 3, "a": 3, "b": 3 }];
   commCompareResults( cur, expFindResult, false );
}

function test15751 ( varCL )
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   db.createProcedure( function test15750 () { return new SdbQueryOption().cond( { b: { $lt: 5 } } ).sel( { _id: { $include: 0 } } ).sort( { b: -1 } ).limit( 7 ).skip( 2 ).update( { $inc: { c: 1 } }, true, { KeepShardingKey: true } ); } )
   var a = db.eval( 'test15750()' );
   var cur = varCL.find( a );
   var expFindResult = [{ "a": 2, "b": 2, "c": -1 },
   { "a": 1, "b": 1, "c": 0 },
   { "a": 0, "b": 0, "c": 1 }
   ];
   commCompareResults( cur, expFindResult );
   db.removeProcedure( "test15750" );
}

function insertRecord ( varCL )
{
   for( var i = 0; i <= 100; i++ )
   {
      varCL.insert( { _id: i, a: i, b: i, c: -i } );
   }
}
