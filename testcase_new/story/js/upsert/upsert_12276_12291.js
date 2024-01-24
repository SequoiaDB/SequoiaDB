/*******************************************************************************
*@Description: JavaScript common function library
*@Modify list:
*   2014-4-20 wenjing wang Init
*******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_upsert";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   db.setSessionAttr( { PreferedInstance: "M" } );
   upsertandmerger( cl, { $set: { a: 1 } }, { a: 1 }, { a: 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { _id: 1, a: 1 }, { _id: 1, a: 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { a: 2 }, { a: 1 } );
   upsertandmerger( cl, { $set: { b: 1, a: 1 } }, { b: 2 }, { a: 1, b: 1 } );
   upsertandmerger( cl, { $set: { b: 1, a: 1 } }, {}, { a: 1, b: 1 } );
   upsertandmerger( cl, { $set: { a: { b: 1 } } }, { a: { b: 1 } }, { a: { b: 1 } } );
   upsertandmerger( cl, { $inc: { c: 1 } }, { c: 1 }, { c: 2 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { x: { $et: 1 } }, { x: 1, a: 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { x: { $all: [1] } }, { a: 1, x: [1] } );
   upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ x: { $et: 1 } }] }, { x: 1, a: 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ x: { $all: [1] } }] }, { x: [1], a: 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ x: { $all: [1] } }, { b: { $et: 1 } }] }, { x: [1], a: 1, b: 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ d: { $gte: 1 } }, { $and: [{ x: { $all: [1] } }] }] }, { x: [1], a: 1 }, { x: [1], a: 1, d: 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ d: { $gte: 1 } }, { $and: [{ x: { $in: [1] } }] }] }, { a: 1 }, { x: [1], a: 1, d: 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { x: { $all: ["Tom", "Mike"] } }, { x: ["Tom", "Mike"], a: 1 } );
   //upsertandmerger( cl, { $set : { a : 1 } }, { $or : [{ x : { $et : 1 } }] }, { x:1, a:1} ); 
   upsertandmerger( cl, { $set: { a: 1 } }, { $or: [{ x: { $et: 1 } }, { $and: [{ b: { $et: 1 } }] }] }, { a: 1 } );
   upsertandmerger( cl, { $set: { a: 1, x: 1 } }, { c: { $gte: 1 } }, { x: 1, a: 1 }, { x: 1, a: 1, c: 1 } );
   upsertandmerger( cl, { $set: { a: 1, x: 1 } }, { c: { $in: [1] } }, { a: 1, x: 1 }, { x: 1, a: 1, c: 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { x: [1, 2] }, { "x": [1, 2], "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { x: { $et: /abc/ } }, { "x": /abc/, "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": 1 }, { "x": { "x": 1 }, "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": { $et: 1 } }, { "x": { "x": 1 }, "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": { $all: [1] } }, { "x": { "x": [1] }, "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { $and: [{ "x.x": { $et: 1 } }] }, { "x": { "x": 1 }, "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": 1, "x.y": 1 }, { "x": { "x": 1, "y": 1 }, "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": 1, "x.y.z": 1 }, { "x": { "x": 1, "y": { "z": 1 } }, "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { "x.x": [] }, { "x": { "x": [] }, "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { x: { x: [] } }, { "x": { "x": [] }, "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { x: [{ x: 1 }] }, { "x": [{ "x": 1 }], "a": 1 } );
   upsertandmerger( cl, { $set: { a: 1 } }, { "x.x.x": { $et: 1 } }, { "x": { "x": { "x": 1 } }, "a": 1 } );

   commDropCL( db, COMMCSNAME, clName );
}