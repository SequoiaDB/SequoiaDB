/******************************************************************************
@Description : seqDB-13739:数组的排序（包括不带索引、带索引、升序）
@Modify list :
               2015-01-15 pusheng Ding  Init
               2020-08-14 Zixian YAn    Modify
               2020-10-12 Xiaoni Huang  Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13739";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   insertRecs( cl );
   sort_notIndex( cl );
   sort_index( cl );
}

function sort_notIndex ( cl )
{
   // order by
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { b: 1 } );
   var expRecs = [
      { "b": [null] },
      { "b": [1, 2] },
      { "b": [2, 1] },
      { "b": [2, 1] },
      { "b": [1] },
      { "b": [3, 2] },
      { "b": [2] },
      { "b": [3] },
      { "b": ["t"] },
      { "b": [{ "b1": 1 }, { "b1": 2 }] },
      { "b": [{ "b1": { "b2": 1 } }, { "b1": { "b2": 2 } }] },
      { "b": [] }];
   commCompareResults( cursor, expRecs );

   // order by desc
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { b: -1 } );
   var expRecs = [
      { "b": [] },
      { "b": [{ "b1": { "b2": 1 } }, { "b1": { "b2": 2 } }] },
      { "b": [{ "b1": 1 }, { "b1": 2 }] },
      { "b": ["t"] },
      { "b": [3, 2] },
      { "b": [3] },
      { "b": [1, 2] },
      { "b": [2, 1] },
      { "b": [2, 1] },
      { "b": [2] },
      { "b": [1] },
      { "b": [null] }];
   commCompareResults( cursor, expRecs );
}

function sort_index ( cl )
{
   var idxName = "idx";
   cl.createIndex( idxName, { "b": -1 } );

   // order by
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).hint( { "": idxName } ).sort( { b: 1 } );
   var expRecs = [
      { "b": [null] },
      { "b": [2, 1] },
      { "b": [2, 1] },
      { "b": [1, 2] },
      { "b": [1] },
      { "b": [3, 2] },
      { "b": [2] },
      { "b": [3] },
      { "b": ["t"] },
      { "b": [{ "b1": 1 }, { "b1": 2 }] },
      { "b": [{ "b1": { "b2": 1 } }, { "b1": { "b2": 2 } }] },
      { "b": [] }];
   commCompareResults( cursor, expRecs );

   // order by, not sort
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).hint( { "": idxName } );
   var expRecs = [
      { "b": [] },
      { "b": [{ "b1": { "b2": 1 } }, { "b1": { "b2": 2 } }] },
      { "b": [{ "b1": 1 }, { "b1": 2 }] },
      { "b": ["t"] },
      { "b": [3] },
      { "b": [3, 2] },
      { "b": [2] },
      { "b": [1, 2] },
      { "b": [2, 1] },
      { "b": [2, 1] },
      { "b": [1] },
      { "b": [null] }];
   commCompareResults( cursor, expRecs );
}

function insertRecs ( cl )
{
   var recs = [
      { "b": [1] },
      { "b": [3] },
      { "b": [2] },
      { "b": [1, 2] },
      { "b": [3, 2] },
      { "b": [2, 1] },
      { "b": [2, 1] },
      { "b": [{ "b1": 1 }, { "b1": 2 }] },
      { "b": [{ "b1": { "b2": 1 } }, { "b1": { "b2": 2 } }] },
      { "b": ["t"] },
      { "b": [null] },
      { "b": [] }
   ]
   cl.insert( recs );
}
