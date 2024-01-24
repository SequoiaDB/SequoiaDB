/***************************************************************************
/***************************************************************************
@Description :seqDB-9638:SSL功能开启，使用SSL连接创建/删除索引
@Modify list :
              2016-8-31  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_9638";

function main ( dbs )
{
   // drop collection in the beginning
   commDropCL( dbs, csName, clName, true, true, "drop collection in the beginning" );

   // create cs /cl
   var dbCL = commCreateCL( dbs, csName, clName, {}, true, true );

   //insert data 
   insertData( dbCL );

   //create index
   createIndex( dbCL, "testIndex", { a: 1 } );
   //inspect the index
   inspecIndex( dbCL, "testIndex", "a", 1, false, false );
   //test find by index    
   var rc = dbCL.find( { a: 3 } ).hint( { "": "testIndex" } )
   var expRecs = [];
   expRecs.push( { _id: 3, a: 3, b: "test3" } );
   checkCLData( rc, expRecs );

   //drop index
   dbCL.dropIndex( "testIndex" );
   //test the index
   try
   {
      dbCL.getIndex( "testIndex" );
   }
   catch( e )
   {
      if( -47 !== e )
      {
         throw buildException( "getIndex", e );
      }
   }

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


/****************************************************
@description: check the result of query
@modify list:
              2016-8-10 yan WU init
****************************************************/
function checkCLData ( rc, expRecs )
{
   println( "\n---Begin to check cl data." );
   var recsArray = [];
   while( rc.next() )
   {
      recsArray.push( rc.current().toObj() );
   }
   var actRecs = JSON.stringify( recsArray );
   var expRec = JSON.stringify( expRecs )
   if( actRecs !== expRec )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[recs:" + expRec + "]",
         "[recs:" + actRecs + "]" );
   }
   println( "cl records: " + actRecs );
}

function insertData ( dbcl )
{
   var doc = [];
   try
   {
      for( i = 0; i < 5; i++ )
      {
         doc.push( { _id: i, a: i, b: "test" + i } );
      }
      dbcl.insert( doc );
   }
   catch( e )
   {
      println( "failed to insert record, rc= " + e );
      throw e;
   }
}

function createIndex ( cl, idxName, idxKeygen, unique, enforced, errno )
{
   if( undefined == unique ) { unique = false; }
   if( undefined == enforced ) { enforced = false; }
   if( undefined == errno ) { errno = ""; }
   try
   {
      if( undefined == cl || undefined == idxName || undefined == idxKeygen )
      {
         println( "please check the argument of createIndex" );
         throw "ErrArg";
      }
      cl.createIndex( idxName, idxKeygen, unique, enforced );
      // inspect the index we created
   }
   catch( e )
   {
      if( errno != e )
      {
         println( "failed to create index, rc = " + e );
         throw e;
      }
   }
}