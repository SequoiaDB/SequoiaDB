/************************************
*@Description: 创建固定集合空间集合，find查询数据 
*@author:      liuxiaoxuan
*@createdate:  2018.4.18
*@testlinkCase:seqDB-11799
**************************************/

var currentLastLogicalID = 0;
var blockCounts = 1;
main( test );

function test ()
{
   var csName = CHANGEDPREFIX + "_11799_large_CS";
   var clName = CHANGEDPREFIX + "_11799_large_CL";

   //clean and createCS CL before test
   commDropCS( db, csName, true, "drop CS at begin" );

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   //create cappedCL
   var options = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, csName, clName, options, false, false, "create capped cl" )

   //insert 100 records at first extend
   var doc = [];
   var insertNum = 100;
   for( var i = 0; i < insertNum; i++ )
   {
      doc.push( { a: i } );
   }
   dbcl.insert( doc );

   //insert 10w records at middle, each record lengths 968 
   var insertNum = 100000;
   var stringLength = 968;
   insertFixedLengthDatas( dbcl, insertNum, stringLength, 'a' );

   //insert 100 records at the last extend
   var doc = [];
   var insertNum = 100;
   for( var i = 100000; i < 100000 + insertNum; i++ )
   {
      doc.push( { a: i } );
   }
   dbcl.insert( doc );

   //get size of each record
   var recordSize = recordHeader;
   if ( recordSize % 4 != 0 ) 
   {   
       recordSize = recordSize + ( 4 - recordSize % 4 );
   }   

   //check count
   checkCount( dbcl, {}, 100200 );

   //get all logicalIDs
   var expLogicalIDs = getExpectLogicalIDs( 1, 100 );
   expLogicalIDs = expLogicalIDs.concat( getExpectLogicalIDs( 968, 100000 ) ).concat( getExpectLogicalIDs( 1, 100 ) );

   //get all records
   var allResults = [];
   for( var i = 0; i < 100; i++ )
   {
      allResults.push( { a: i } );
   }
   for( var i = 0; i < 100000; i++ )
   {
      allResults.push( { a: 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa' } );
   }
   for( var i = 100000; i < 100100; i++ )
   {
      allResults.push( { a: i } );
   }

   //check query without options   
   checkLogicalID( dbcl, null, null, { _id: 1 }, null, null, expLogicalIDs );

   //$gt、$lt
   var gtObj = { a: { $gt: 0, $lt: 100 } };
   checkQueryResult( dbcl, gtObj, null, { _id: 1 }, allResults.slice( 1, 100 ) )
   checkLogicalID( dbcl, gtObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 1, 100 ) );

   //$exists
   var existsObj = { _id: { $exists: 1 } };
   checkQueryResult( dbcl, existsObj, null, { _id: 1 }, allResults );
   checkLogicalID( dbcl, existsObj, null, { _id: 1 }, null, null, expLogicalIDs );

   //$or
   var orObj = { $or: [{ _id: { $lt: recordSize } }, { a: { $gt: 100000 } }] }
   var results = allResults.slice( 0, 1 ).concat( allResults.slice( 100101 ) );
   var expectIDs = expLogicalIDs.slice( 0, 1 ).concat( expLogicalIDs.slice( 100101 ) );
   checkQueryResult( dbcl, orObj, null, { _id: 1 }, results );
   checkLogicalID( dbcl, orObj, null, { _id: 1 }, null, null, expectIDs );

   //$type(function)
   var etObj = { a: { $et: 100000 } }
   var typeObj = { "a": { "$type": 1 } }
   checkQueryResult( dbcl, etObj, typeObj, { _id: 1 }, [{ "a": 16 }] );
   checkLogicalID( dbcl, etObj, typeObj, { _id: 1 }, null, null, expLogicalIDs.slice( 100100, 100101 ) );

   commDropCS( db, csName, true, "drop CS in the end" );
}

function checkQueryResult ( dbcl, FindOptions, selectorOptions, sortOptions, results )
{
   var rc = dbcl.find( FindOptions, selectorOptions ).sort( sortOptions );
   checkRec( rc, results );
}

function getExpectLogicalIDs ( stringLength, recordNums )
{
   var expLogicalIDs = [];

   var recordLength = stringLength + recordHeader;
   if( recordLength % 4 !== 0 )
   {
      recordLength = recordLength + ( 4 - recordLength % 4 );
   }

   for( var i = 0; i < recordNums; i++ )
   {
      expLogicalIDs.push( currentLastLogicalID );
      var nextLogicalID = currentLastLogicalID + recordLength;

      currentLastLogicalID = nextLogicalID;
      if( currentLastLogicalID > ( blockCounts * 33554396 - recordLength ) )
      { //if the next record length is up to current block size,it will be put to the next block
         currentLastLogicalID = blockCounts * 33554396;
         ++blockCounts;
      }
   }

   return expLogicalIDs;
}

