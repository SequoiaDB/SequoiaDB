/***************************************************************************
@Description : The array have 5 layer nest, and the create index .
               such:[db.CS.CL.createIndex("arrLay5Index",
                    {"array1.array2.array3.array4.1":1???}, true, true )]
@Modify list :
               2014-5-21  xiaojun Hu  Init
****************************************************************************/
main( test );

function test ()
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, {}, true, false, "create collection" );

   // insert data to SDB
   idxCL.insert( {
      no: 001, name: "A", age: 2, array1: [{
         "array2": [{
            "array3": [{
               "array4": ["array5",
                  "temp4"]
            }, "temp3"]
         }, "temp2"]
      }, "temp1"]
   } );
   var i = 0;
   do
   {
      var count = idxCL.count( {
         no: 001, name: "A", age: 2, array1: [{
            "array2": [{
               "array3": [{
                  "array4": ["array5",
                     "temp4"]
               }, "temp3"]
            }, "temp2"]
         }, "temp1"]
      } );
      ++i;
   } while( i < 10 )
   assert.equal( 1, count );

   // create index
   createIndex( idxCL, "arrLay5Index", { "array1.array2.array3.array4": 1 }, true, true );

   // inspect index
   try
   {
      inspecIndex( idxCL, "arrLay5Index", "array1.array2.array3.array4", 1, true, true );
   }
   catch( e )
   {
      if( "ErrIdxName" != e.message )
      {
         throw e;
      }
   }

   //test find by index 
   checkExplain( idxCL, { "array1.array2.array3.array4": "temp4" }, "ixscan", "arrLay5Index" );

   //check the result of find  
   checkResult( idxCL, { array1: [{ "array2": [{ "array3": [{ "array4": ["array5", "temp4"] }, "temp3"] }, "temp2"] }, "temp1"] } );

   // drop collection in clean
   commDropCL( db, csName, clName, false, false, "drop colleciton in the end" );
}
