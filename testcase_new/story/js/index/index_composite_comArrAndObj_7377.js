/***************************************************************************
@Description : The Object have 5 layer nest, and the create composite index .
               There are one nest array and nest object .
@Modify list :
               2014-5-21  xiaojun Hu  Init
               2016-3-4   yan wu Modify(增加结果检测（查看访问计划是否走索引、走索引查询数据是否正确）)
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
      no: 001, name: "A", age: 2,
      "object1": { "object2": { "object3": { "object4": { "object5": "5LayerObject" } } } }
   } );
   idxCL.insert( { no: 002, name: "a", age: 3, "a1": { "a2": { "a3": { "a4": { "a5": "5LayerObject_a" } } } } } );
   idxCL.insert( { no: 003, name: "B", age: 3, "b1": { "b2": { "b3": { "b4": { "b5": "5LayerObject_b" } } } } } );
   idxCL.insert( { no: 004, name: "C", age: 3, "c1": { "c2": { "c3": { "c4": { "c5": "5LayerObject_c" } } } } } );
   idxCL.insert( { no: 005, name: "D", age: 3, "d1": { "d2": { "d3": { "d4": { "d5": "5LayerObject_d" } } } } } );
   idxCL.insert( {
      no: 006, name: "E", age: 2, array1: [{
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
      var count = idxCL.count();
      ++i;
   } while( i < 10 )
   assert.equal( count, 6 );

   // create index
   createIndex( idxCL, "ObjLay5Index",
      {
         "object1.object2.object3.object4.object5": 1,
         "a1.a2.a3.a4.a5": -1, "b1.b2.b3.b4.b5": 1,
         "c1.c2.c3.c4.c5": 1, "d1.d2.d3.d4.d5": -1,
         "array1.array2.array3.array4": 1
      }, true, false );

   // inspect index
   try
   {
      inspecIndex( idxCL, "ObjLay5Index", "object1.object2.object3.object4.object5", 1, true, false );
   }
   catch( e )
   {
      if( "ErrIdxName" != e.message )
      {
         throw e;
      }
   }

   //test find by index
   checkExplain( idxCL, { "object1.object2.object3.object4.object5": "5LayerObject" }, "ixscan", "ObjLay5Index" );

   //check the result of find  
   var rc = idxCL.find( { "object1.object2.object3.object4.object5": "5LayerObject" } );
   var expRecs = [];
   expRecs.push( { "object1": { "object2": { "object3": { "object4": { "object5": "5LayerObject" } } } } } );
   checkRec( rc, expRecs );

   // drop collection in clean
   commDropCL( db, csName, clName, false, false );
}
