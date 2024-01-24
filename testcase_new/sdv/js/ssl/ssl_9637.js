/***************************************************************************
@Description :seqDB-9637:SSL功能开启，使用SSL连接执行数据操作（增删改查）
@Modify list :
              2016-8-31  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_9637";

function main ( dbs )
{
   // drop collection in the beginning
   commDropCL( dbs, csName, clName, true, true, "drop collection in the beginning" );

   // create cs /cl
   var dbCL = commCreateCL( dbs, csName, clName, {}, true, true );

   //insert data 
   dbCL.insert( { "_id": 1, "a": "test", "b": 2 } );
   //find data and check the insert result
   var expRecs = '[{"_id":1,"a":"test","b":2}]';
   checkCLData( dbCL, expRecs );

   //update the data
   try
   {
      dbCL.update( { $inc: { b: 2 } }, { a: "test" } );
   }
   catch( e )
   {
      println( "failed to update record, rc= " + e );
      throw e;
   }
   //check the data result
   var expRecUpdate = '[{"_id":1,"a":"test","b":4}]';
   checkCLData( dbCL, expRecUpdate );

   //remove the data
   try
   {
      dbCL.remove();
   }
   catch( e )
   {
      println( "failed to remove record, rc= " + e );
      throw e;
   }
   //check the remove result
   checkRemoveData( dbCL );
   //dropCS
   commDropCS( dbs, csName, false, "seqDB-9636: dropCS failed" );
}

try
{
   main( dbs );
   dbs.close();
}
catch( e )
{
   throw e;
}

function checkRemoveData ( dbcl )
{
   var num = dbcl.find().count();
   if( 0 !== Number( num ) )
   {
      throw buildException( "checkRemoveData()", e );
   }
}