/************************************
*@Description: 批量插入记录,_id超过整型范围
*@author:      luweikang
*@createdate:  2017.7.11
*@testlinkCase:seqDB-11772
**************************************/

main( test );
function test ()
{
   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "_11772";
   var optionObj = { Capped: true, Size: 4096, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   //insertData
   var randomNum = Math.floor( Math.random() * 100 );
   var bigStr = createBigStr( randomNum );
   for( var i = 0; i < 20; i++ )
   {
      bulkInsertData( dbcl, bigStr, 100 );
   }

   //check id
   checkId( dbcl, bigStr, 2000 );

   //clean environment after test  
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function bulkInsertData ( dbcl, bigStr, times )
{
   var doc = new Array();
   for( var i = 0; i < times; i++ )
   {
      var options = { a: bigStr };
      doc.push( options );
   }
   dbcl.insert( doc );
}

function checkId ( dbcl, bigStr, recordNum )
{
   //dbcl.insert( { a : 1 } );
   var cursor = dbcl.find( null, { '_id': "" } ).sort( { "_id": -1 } ).limit( 1 );
   var actID = cursor.next().toObj()._id;

   var len = fourByte( recordHeader + bigStr.length );
   var blank = 33554396 % len;
   var countLen = len * recordNum;
   var one = Math.floor( 33554396 / len );
   var blanks = Math.floor( recordNum / one ) * blank;
   var expID = countLen + blanks - len;

   if( actID <= 2147483647 )
   {
      throw new Error( "ERR_ID_MIN" );
   }
   assert.equal( actID, expID );
   cursor.close();

}

function fourByte ( len )
{
   if( len % 4 !== 0 )
   {
      len = len - len % 4 + 4;
   }
   return len;
}

function createBigStr ( randomNum )
{
   var size = 1024 * ( 1024 + randomNum );
   var str = "";
   for( var i = 0; i < size; i++ )
   {
      str = str + "a";
   }
   return str;
}
