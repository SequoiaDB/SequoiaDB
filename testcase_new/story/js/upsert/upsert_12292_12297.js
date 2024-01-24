/*******************************************************************************
*@Description: JavaScript common function library
*@Modify list:
*   2014-4-21 wenjing wang Init
*******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_upsertsetoninsert";
   var cl = commCreateCL( db, COMMCSNAME, clName );
   db.setSessionAttr( { PreferedInstance: "M" } );

   upsertandmergerWithHint( cl, { $set: { c: 1, d: 1 } }, {}, {}, { _id: 1 },
      { c: 1, d: 1, _id: 1 } );
   upsertandmergerWithHint( cl, { $set: { a: 3 } }, { a: 2 }, {}, { a: 1 },
      { a: 1 } );
   cl.insert( { _id: 1, a: 1 } );
   upsertandmergerWithHint( cl, { $set: { a: 2 } }, { _id: 1 }, {}, { msg: "test" },
      { _id: 1, a: 2 }, { _id: 1, a: 2, msg: "test" } );
   upsertandmergerWithHint( cl, { $set: { a: 1 } }, { b: 1 }, {}, { c: { d: 1 } }, { a: 1, b: 1, c: { d: 1 } } );
   upsertandmergerWithHint( cl, { $set: { a: 1 } }, { b: 1 }, {}, { "c.d": 1 },
      { a: 1, b: 1, c: { d: 1 } } );
   upsertandmergerWithHint( cl, { $set: { a: 1 } }, { b: 1 }, {}, { c: [1] },
      { a: 1, b: 1, c: [1] } );
   cl.createIndex( 'aidx', { a: 1 }, false );
   upsertandmergerWithHint( cl, { $set: { a: 1 } }, {}, { "": 'aidx' }, { _id: 1 },
      { a: 1, _id: 1 } );
   //upsertandmergerWithHint( cl, {$set:{a:1}}, {"_id.b":1}, {}, {"_id.a": new Date()}, 
   //               {a:1, "_id.b":1} ); 
   upsertandmergerWithHint( cl, { $set: { "_id.a": 1 } }, { "_id.b": 1 }, {}, { "_id.c": 1 },
      { _id: { a: 1, b: 1, c: 1 } } );

   commDropCL( db, COMMCSNAME, clName );
}