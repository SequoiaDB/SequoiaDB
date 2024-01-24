/*******************************************************************************
@Description : 创建复合索引，指定索引查询（查询字段包括所有索引键）
@Modify list :
               2016-3-16  yan Wu  init
*******************************************************************************/
var clName = CHANGEDPREFIX + "_duplicateIndex7399";
main( test );

function test ()
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, {}, true, false, "create collection" );

   // insert data to SDB
   idxCL.insert( { no: 001, name: "A", score: [60, 70, 80], coutry: { china: { guangdong: "guanzhou" } }, age: 15, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.1" } } );
   idxCL.insert( { no: 002, name: "B", score: [60, 71, 80], coutry: { china: { guangdong: "shenzhen" } }, age: 17, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.2" } } );
   idxCL.insert( { no: 003, name: "C", score: [62, 70, 80], coutry: { china: { guangdong: "huizhou" } }, age: 18, major: ["English", "History", "Physics"], "class": { grade: "NO.3" } } );
   idxCL.insert( { no: 004, name: "D", score: [60, 70, 85], coutry: { china: { guangdong: "foshan" } }, age: 17, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.4" } } );
   idxCL.insert( { no: 005, name: "E", score: [65, 75, 85], coutry: { china: { guangdong: "zhuhai" } }, age: 18, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.5" } } );

   // create index
   idxCL.createIndex( "comIndex1", { no: 1, name: 1, "score": 1, "coutry.china.guangdong": 1 }, false, false );

   // inspect index
   try
   {
      inspecIndex( idxCL, "comIndex1", "no", 1, false, false );
   }
   catch( e )
   {
      if( "ErrIdxName" != e.message )
      {
         throw e;
      }
   }

   //check the result of find  
   var rc = idxCL.find( { no: 002, name: "B", score: [60, 71, 80], "coutry.china.guangdong": "shenzhen" } );
   var expRecs = [];
   expRecs.push( { no: 002, name: "B", score: [60, 71, 80], coutry: { china: { guangdong: "shenzhen" } } } );
   checkRec( rc, expRecs );

   // drop collection in clean
   commDropCL( db, csName, clName, false, false );
}
