/******************************************************************************
@Description : seqDB-13738:查询数据指定对象类型字段排序
@Modify list :
               2015-01-15 pusheng Ding  Init
               2020-10-12 Xiaoni Huang  Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13738";

main( test );
function test ( arg )
{
   var cl = arg.testCL;
   insertRecs( cl );
   sort_notIndex( cl );
   sort_index( cl );
}

function sort_notIndex ( cl )
{
   // 排序字段为"a.a1"，正序
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a.a1": 1 } );
   var expRecs = [
      { "a": { "a2": 3 } },
      { "a": { "a1": null } },
      { "a": { "a1": 1 } },
      { "a": { "a1": 2, "a2": 1 } },
      { "a": { "a1": 3, "a2": 1 } },
      { "a": { "a1": 4, "a2": 2 } },
      { "a": { "a1": 4, "a2": 1 } },
      { "a": { "a1": "" } }];
   commCompareResults( cursor, expRecs );

   // 排序字段为"a.a1"（逆序）和"a.a2"（正序）
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a.a1": -1, "a.a2": 1 } );
   var expRecs = [
      { "a": { "a1": "" } },
      { "a": { "a1": 4, "a2": 1 } },
      { "a": { "a1": 4, "a2": 2 } },
      { "a": { "a1": 3, "a2": 1 } },
      { "a": { "a1": 2, "a2": 1 } },
      { "a": { "a1": 1 } },
      { "a": { "a1": null } },
      { "a": { "a2": 3 } }];
   commCompareResults( cursor, expRecs );
}

function sort_index ( cl )
{
   // create index
   cl.createIndex( "idx1", { "a": 1 } );
   cl.createIndex( "idx2", { "a.a1": 1 } );

   // sortKey: a.a1, indexKey: a
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a.a1": 1 } ).hint( { "": "idx1" } );
   var expRecs = [
      { "a": { "a2": 3 } },
      { "a": { "a1": null } },
      { "a": { "a1": 1 } },
      { "a": { "a1": 2, "a2": 1 } },
      { "a": { "a1": 3, "a2": 1 } },
      { "a": { "a1": 4, "a2": 1 } },
      { "a": { "a1": 4, "a2": 2 } },
      { "a": { "a1": "" } }];
   commCompareResults( cursor, expRecs );

   // sortKey: a.a1, indexKey: a.a1
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a.a1": 1 } ).hint( { "": "idx2" } );
   var expRecs = [
      { "a": { "a2": 3 } },
      { "a": { "a1": null } },
      { "a": { "a1": 1 } },
      { "a": { "a1": 2, "a2": 1 } },
      { "a": { "a1": 3, "a2": 1 } },
      { "a": { "a1": 4, "a2": 2 } },
      { "a": { "a1": 4, "a2": 1 } },
      { "a": { "a1": "" } }];
   commCompareResults( cursor, expRecs );

   // sortKey: a, indexKey: a.a1
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } ).hint( { "": "idx2" } );
   var expRecs = [
      { "a": { "a1": null } },
      { "a": { "a1": 1 } },
      { "a": { "a1": 2, "a2": 1 } },
      { "a": { "a1": 3, "a2": 1 } },
      { "a": { "a1": 4, "a2": 1 } },
      { "a": { "a1": 4, "a2": 2 } },
      { "a": { "a2": 3 } },
      { "a": { "a1": "" } }];
   commCompareResults( cursor, expRecs );
}

function insertRecs ( cl )
{
   var recs = [
      { "a": { "a1": "" } },
      { "a": { "a1": null } },
      { "a": { "a1": 1 } },
      { "a": { "a1": 2, "a2": 1 } },
      { "a": { "a1": 3, "a2": 1 } },
      { "a": { "a1": 4, "a2": 2 } },
      { "a": { "a1": 4, "a2": 1 } },
      { "a": { "a2": 3 } }
   ];
   cl.insert( recs );
}