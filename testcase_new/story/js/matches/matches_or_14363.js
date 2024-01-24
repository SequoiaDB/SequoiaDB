/************************************
*@Description: hash表使用$or查询，$or指定多个子条件  
*@author:      liuxiaoxuan
*@createdate:  2018.02.02
*@testlinkCase: seqDB-14363
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   if( 2 > commGetGroupsNum( db ) )
   {
      return;
   }

   // get src and dest group      
   var groups = commGetGroups( db );
   var srcGroupName = groups[0][0].GroupName
   var destGroupName = groups[1][0].GroupName

   var csName = COMMCSNAME + "14363";
   commDropCS( db, csName, true, "drop CS in the beginning" );
   commCreateCS( db, csName, false, "" );

   //create cl
   var hashCLName = COMMCLNAME + "14363";
   var options = { Group: srcGroupName, ShardingKey: { a: 1, b: 1, c: 1 }, ShardingType: "hash", ReplSize: 0 };
   var hashCL = commCreateCL( db, csName, hashCLName, options, true );

   //split class
   hashCL.split( srcGroupName, destGroupName, 50 )

   //insert datas
   insertDatas( hashCL );

   //find with $or
   var findConf = { $or: [{ a: { $gt: 1 }, b: { $lte: 108 } }, { c: { $et: 1009 } }, { c: { $et: 1010 } }] }
   var actFindResult = getQueryResult( hashCL, findConf );
   var expFindResult = [{ a: 2, b: 102, c: 1002 },
   { a: 3, b: 103, c: 1003 },
   { a: 4, b: 104, c: 1004 },
   { a: 5, b: 105, c: 1005 },
   { a: 6, b: 106, c: 1006 },
   { a: 7, b: 107, c: 1007 },
   { a: 8, b: 108, c: 1008 },
   { a: 9, b: 109, c: 1009 },
   { a: 10, b: 110, c: 1010 }];
   //check find result
   checkResult( expFindResult, actFindResult );

   //explain with $or
   var actExplainResult = getExplainResult( hashCL, findConf );
   var expExplainResult = [srcGroupName, destGroupName];
   //check explain result
   checkResult( expExplainResult, actExplainResult );

   commDropCS( db, csName, true, "drop CS in the end" );
}
function insertDatas ( dbcl )
{
   var doc = [];
   for( var i = 0; i <= 10; i++ )
   {
      doc.push( { a: i, b: ( i + 100 ), c: ( i + 1000 ) } );
   }
   dbcl.insert( doc );

}
function getQueryResult ( dbcl, findConf, hintConf, sortConf )
{
   if( findConf == undefined ) var findConf = null;
   if( hintConf == undefined ) var hintConf = null;
   if( sortConf == undefined ) var sortConf = null;

   var newArray = new Array();
   var rc = dbcl.find( findConf ).hint( hintConf ).sort( sortConf ).toArray();

   for( var i = 0; i < rc.length; i++ )
   {
      var obj = eval( "(" + rc[i] + ")" );
      // delete _id
      delete obj._id;
      // sort by json key
      var newObj = sortByKey( obj );
      newArray.push( newObj );
   }
   return newArray;
}
function getExplainResult ( dbcl, findConf, hintConf, sortConf )
{
   if( findConf == undefined ) var findConf = null;
   if( hintConf == undefined ) var hintConf = null;
   if( sortConf == undefined ) var sortConf = null;

   var newArray = new Array();
   var rc = dbcl.find( findConf ).hint( hintConf ).sort( sortConf ).explain().toArray();
   for( var i = 0; i < rc.length; i++ )
   {
      var obj = eval( "(" + rc[i] + ")" );
      newArray.push( obj['GroupName'] );
   }
   return newArray;
}
function checkResult ( expResult, actResult )
{
   //check length
   assert.equal( expResult.length, actResult.length );

   //check records
   for( var i = 0; i < expResult.length; i++ )
   {
      if( JSON.stringify( actResult ).indexOf( JSON.stringify( expResult[i] ) ) === -1 )  
      {
         throw new Error( "check result fail" + JSON.stringify( actResult ) + JSON.stringify( expResult[i] ) );
      }
   }

   for( var i = 0; i < expResult.length; i++ )
   {
      if( JSON.stringify( expResult ).indexOf( JSON.stringify( actResult[i] ) ) === -1 )  
      {
         throw new Error( "check result fail" + JSON.stringify( actResult ) + JSON.stringify( expResult[i] ) );
      }
   }
}
function sortByKey ( obj )
{
   var newKey = Object.keys( obj ).sort();
   var newObj = {};
   for( var i = 0; i < newKey.length; i++ )
   {
      newObj[newKey[i]] = obj[newKey[i]];
   }
   return newObj;
}
