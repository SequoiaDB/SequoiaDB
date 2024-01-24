/************************************
*@Description:  指定其他模式执行统计
*@author:      liuxiaoxuan
*@createdate:  2017.11.11
*@testlinkCase: seqDB-11636
**************************************/
main( test );
function test ()
{
   var csName = COMMCSNAME + "11636";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create cl
   var clName = COMMCLNAME + "11636";
   var dbcl = commCreateCL( db, csName, clName );

   //insert
   var insertNums = 4000;
   insertDiffDatas( dbcl, insertNums );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   //check invalid analyze
   var options = [{ Mode: 6 },
   { Mode: "string" },
   { Mode: 123.456 },
   { Mode: true },
   { Mode: null }]

   for( var i in options )
   {
      checkAnalyzeMode( options[i] );
   }

   //analyze success
   var options = { Collection: csName + "." + clName };
   db.analyze( options );

   commDropCS( db, csName, true, "drop CS in the end" );
}

function checkAnalyzeMode ( options )
{
   try
   {
      db.analyze( options );
      throw new Error( "NEED ANALYZE FAILED" );
   }
   catch( e )
   {
      if( SDB_INVALIDARG != e.message )
      {
         throw e;
      }
   }
}

