/********************************************************************************
@Description : abnormal test : db.CS.CL.createIndex("",{a:1},false)
@Modify list :
               2014-5-20  xiaojun Hu  Modify
********************************************************************************/

main( test );

function test ()
{
   var clName = "cl7373";
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );
   var index = "";
   for( var i = 0; i < 1024; i++ )
   {
      index += 't';
   }

   // create collection
   var idxCL = commCreateCL( db, csName, clName, {}, true, false, "create collection" );

   // insert data to SDB
   idxCL.insert( { a: 1 } );

   // create index
   createIndex( idxCL, "" );
   createIndex( idxCL, index );

   // drop collection in clean
   commDropCL( db, csName, clName, false, false, "drop collection in the end" );
}

function createIndex ( cl, idxName )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.createIndex( idxName, { a: 1 } );
   } );
}

