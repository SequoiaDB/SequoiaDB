1 /************************************
*@Description: 创建固定集合，使用_id字段执行组合查询、_id字段与其他字段执行组合查询 
*@author:      liuxiaoxuan
*@createdate:  2018.4.18
*@testlinkCase:seqDB-15126/seqDB-15127
**************************************/

var currentLastLogicalID = 0;
var blockCounts = 1;
main( test );
function test ()
{
   var csName = CHANGEDPREFIX + "_15126_15127_large_CS";
   var clName = CHANGEDPREFIX + "_15126_15127_large_CL";

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

   //insert 10w records, each record lengths 968 
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

   //only _id, and   
   var andObj = { $and: [{ _id: 0 }] }
   checkQueryResult( dbcl, andObj, null, { _id: 1 }, [{ "a": 0 }] );
   checkLogicalID( dbcl, andObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //only _id, or   
   var orObj = { $or: [{ _id: 0 }, { _id: expLogicalIDs[100199] }] }
   checkQueryResult( dbcl, orObj, null, { _id: 1 }, [{ "a": 0 }, { "a": 100099 }] );
   checkLogicalID( dbcl, orObj, null, { _id: 1 }, null, null, [expLogicalIDs[0], expLogicalIDs[100199]] );

   //only _id, not   
   var notObj = { $not: [{ _id: { $ne: 0 } }, { _id: { $gt: 0 } }] }
   checkQueryResult( dbcl, notObj, null, { _id: 1 }, allResults.slice( 0, 1 ) );
   checkLogicalID( dbcl, notObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //only _id, and-and  
   var andandObj = { $and: [{ $and: [{ _id: 0 }, { _id: { $in: [0, expLogicalIDs.slice( 1, 2 )] } }] }, { _id: { $lt: 5600 } }] }
   checkQueryResult( dbcl, andandObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, andandObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //only _id, and-or   
   var andorObj = { $and: [{ $or: [{ _id: 0 }, { _id: { $et: expLogicalIDs[1] } }] }, { _id: { $in: [0, expLogicalIDs[1]] } }] }
   checkQueryResult( dbcl, andorObj, null, { _id: 1 }, allResults.slice( 0, 2 ) );
   checkLogicalID( dbcl, andorObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 2 ) );

   //only _id, or-or   
   var ororObj = { $or: [{ $or: [{ _id: 0 }, { _id: { $gt: 1000000000 } }] }, { _id: { $et: expLogicalIDs[1] } }] }
   checkQueryResult( dbcl, ororObj, null, { _id: 1 }, allResults.slice( 0, 2 ) );
   checkLogicalID( dbcl, ororObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 2 ) );

   //only _id, or-not
   var ornotObj = { $or: [{ $not: [{ _id: { $gt: 0 } }, { _id: { $lt: 1000000000 } }] }, { _id: { $et: expLogicalIDs[1] } }] }
   checkQueryResult( dbcl, ornotObj, null, { _id: 1 }, allResults.slice( 0, 2 ) );
   checkLogicalID( dbcl, ornotObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 2 ) );

   //only _id, not-and
   var notandObj = { $not: [{ $and: [{ _id: { $gt: 1 } }, { _id: { $lt: 1000000000 } }] }, { _id: { $gt: 0 } }] }
   checkQueryResult( dbcl, notandObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, notandObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //only _id, and-and-or
   var andandorObj = { $and: [{ $and: [{ _id: { $gte: 0 } }, { _id: { $in: [0, 100000] } }] }, { $or: [{ _id: 0 }, { _id: expLogicalIDs[1] }] }] }
   checkQueryResult( dbcl, andandorObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, andandorObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //only _id, or-and-not   
   var orandnotObj = { $or: [{ $and: [{ _id: 0 }, { _id: { $et: 100000 } }] }, { $not: [{ _id: { $gt: 0 } }] }] }
   checkQueryResult( dbcl, orandnotObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, orandnotObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //only _id, not-and-or   
   var notandorObj = { $not: [{ $and: [{ _id: { $gt: 0 } }, { _id: { $lt: 1000000000 } }] }, { $or: [{ _id: { $gte: expLogicalIDs[1] } }] }] }
   checkQueryResult( dbcl, notandorObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, notandorObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //_id and other field, and   
   var andObj = { $and: [{ _id: 0 }, { a: { $in: [0, 100000] } }] }
   checkQueryResult( dbcl, andObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, andObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //_id and other field, or   
   var orObj = { $or: [{ _id: 0 }, { a: { $gt: 100000 } }] }
   checkQueryResult( dbcl, orObj, null, { _id: 1 }, allResults.slice( 0, 1 ).concat( allResults.slice( 100101 ) ) );
   checkLogicalID( dbcl, orObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ).concat( expLogicalIDs.slice( 100101 ) ) );

   //_id and other field, not   
   var notObj = { $not: [{ _id: { $gt: 0 } }, { a: { $in: [0, 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'] } }] };
   checkQueryResult( dbcl, notObj, null, { _id: 1 }, allResults.slice( 0, 100 ).concat( allResults.slice( 100100 ) ) );
   checkLogicalID( dbcl, notObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 100 ).concat( expLogicalIDs.slice( 100100 ) ) );

   //_id and other field, and-and   
   var andandObj = { $and: [{ $and: [{ _id: 0 }, { a: { $in: [0, 100000] } }] }, { a: 0 }] }
   checkQueryResult( dbcl, andandObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, andandObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //_id and other field, and-or   
   var andorObj = { $and: [{ $or: [{ _id: 0 }, { a: { $gte: 100000 } }] }, { a: { $lte: 100100 } }] }
   checkQueryResult( dbcl, andorObj, null, { _id: 1 }, allResults.slice( 0, 1 ).concat( allResults.slice( 100100 ) ) );
   checkLogicalID( dbcl, andorObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ).concat( expLogicalIDs.slice( 100100 ) ) );

   //_id and other field, or-or   
   var ororObj = { $or: [{ $or: [{ _id: 0 }, { a: { $gt: 100000 } }] }, { a: { $et: 100000 } }] }
   checkQueryResult( dbcl, ororObj, null, { _id: 1 }, allResults.slice( 0, 1 ).concat( allResults.slice( 100100 ) ) );
   checkLogicalID( dbcl, ororObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ).concat( expLogicalIDs.slice( 100100 ) ) );

   //_id and other field, or-not   
   var ornotObj = { $or: [{ $not: [{ _id: { $gte: expLogicalIDs[1] * 100 } }, { a: { $isnull: 0 } }] }, { a: { $et: 100000 } }] }
   checkQueryResult( dbcl, ornotObj, null, { _id: 1 }, allResults.slice( 0, 100 ).concat( allResults.slice( 100100, 100101 ) ) );
   checkLogicalID( dbcl, ornotObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 100 ).concat( expLogicalIDs.slice( 100100, 100101 ) ) );

   //_id and other field, not-and
   var notandObj = { $not: [{ $and: [{ _id: { $gt: 0 } }, { a: { $ne: 0 } }] }, { a: { $et: 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa' } }] };
   checkQueryResult( dbcl, notandObj, null, { _id: 1 }, allResults.slice( 0, 100 ).concat( allResults.slice( 100100 ) ) );
   checkLogicalID( dbcl, notandObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 100 ).concat( expLogicalIDs.slice( 100100 ) ) );

   //_id and other field, and-and-or
   var andandorObj = { $and: [{ $and: [{ _id: 0 }, { a: { $in: [0, 100000] } }] }, { $or: [{ _id: 0 }, { a: { $gt: 100000 } }] }] }
   checkQueryResult( dbcl, andandorObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, andandorObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //_id and other field, or-and-not
   var orandnotObj = { $or: [{ $and: [{ _id: 0 }, { a: { $et: 100000 } }] }, { $not: [{ _id: { $ne: 0 } }, { a: { $ne: 0 } }] }] }
   checkQueryResult( dbcl, orandnotObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, orandnotObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

   //_id and other field, not-and-or   
   var notandorObj = { $not: [{ $and: [{ _id: { $ne: 0 } }, { a: { $ne: 0 } }] }, { $or: [{ _id: { $gt: 0 } }, { a: { $isnull: 1 } }] }] }
   checkQueryResult( dbcl, notandorObj, null, { _id: 1 }, [{ a: 0 }] );
   checkLogicalID( dbcl, notandorObj, null, { _id: 1 }, null, null, expLogicalIDs.slice( 0, 1 ) );

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
   }

   return expLogicalIDs;
}
