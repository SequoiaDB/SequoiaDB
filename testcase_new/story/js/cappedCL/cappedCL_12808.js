/************************************
*@Description: connect data group to drop CL
*@author:      liuxiaoxuan
*@createdate:  2017.10.09
*@testlinkCase: seqDB-12808
**************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCAPPEDCLNAME + "12808";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, true, true );

   //get primary node
   var dataGroup = commGetCLGroups( db, COMMCAPPEDCSNAME + "." + clName );

   var priNodeAddr = getMasterNodeName( dataGroup[0] );

   //connect to primary node
   var dataDB = getDataDB( priNodeAddr );
   //drop cl at priNode
   commDropCL( dataDB, COMMCAPPEDCSNAME, clName );

   //insert data with coord
   var doc = [];
   var insertNums = 10;
   for( var i = 0; i < insertNums; i++ )
   {
      doc.push( { a: i } );
   }
   dbcl.insert( doc );

   //check cl
   checkCappedCL( dataDB, COMMCAPPEDCSNAME, clName );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function getMasterNodeName ( groupName )
{
   var priNode = null;
   var rg = db.getRG( groupName );
   priNode = rg.getMaster();
   return priNode;
}

function getDataDB ( nodeAddr )
{
   var dataDB = new Sdb( nodeAddr );
   return dataDB;
}

function checkCappedCL ( dataDB, csName, clName )
{
   var expectName = csName + "." + clName;
   var isCappedCLExist = false;
   var cursor = dataDB.listCollections();
   while( cursor.next() )
   {
      var actName = cursor.current().toObj().Name;
      if( expectName == actName )
      {
         isCappedCLExist = true;
      }
   }

   if( !isCappedCLExist )
   {
      throw new Error( 'CHECK CAPPED CL FAIL' );
   }
}