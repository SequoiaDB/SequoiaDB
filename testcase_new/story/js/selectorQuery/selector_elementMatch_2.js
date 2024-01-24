/*******************************************************************************
*@Description : $elemMatch/$elemMatchOne
*@Example: record: {"field":[{a:"b"},{a:"c"},{a:"d"},{a:"b"},{a:"f"}]
*          query: db.cs.cl.find({},{"field":{$elemMatch:{a:"b"}}})
*          query: db.cs.cl.find({},{"field":{$elemMatchOne:{a:"b"}}})
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var recordNum = 1;
   var addRecord1 = [{ name: "ZhangSan", age: 18 },
   { name: "WangErmazi", age: 19 },
   { name: "lucy", age: 20 },
   { name: "alex", age: 18 },
   { name: "shanven", age: 18 }];
   //var addRecord2 = {"nest1":{"nest2":{"nest3":{"nest4":"element match query"}}}} ;
   var addRecord2 = { "nestObj": "element match query" };
   var addRecord3 = [{ "nestArr1": [{ "nestArr2": [{ "nestArr3": ["abc", 158, "elementMatch", "中文"] }] }] }];
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data
   selAutoGenData( cl, recordNum, addRecord1, addRecord2, addRecord3 );

   /*【Test Point 1】 $elemMatch: query field is object[normal]*/
   var condObj = {};
   var selObj = { "ExtraField2": { "$elemMatch": { "nestObj": "element match query" } } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var cnt = retObj["ExtraField2"]["nestObj"];
   var exp = "element match query";
   assert.equal( exp, cnt );

   /*【Test Point 2】 $elemMatchOne: query field is object[normal]*/
   var condObj = {};
   var selObj = { "ExtraField2": { "$elemMatchOne": { "nestObj": "element match query" } } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var cnt = retObj["ExtraField2"]["nestObj"];
   assert.equal( exp, cnt );

   /*【Test Point 3】 $elemMatch: nest array, array element isn't object*/
   var condObj = {};
   var selObj = { "ExtraField3.nestArr1.nestArr2.nestArr3": { "$elemMatch": 158 } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var ret = selMainQuery( cl, condObj, selObj );
   } );

   /*【Test Point 4】 $elemMatchOne: nest array, array element isn't object*/
   var condObj = {};
   var selObj = { "ExtraField3.nestArr1.nestArr2.nestArr3": { "$elemMatch": 158 } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var ret = selMainQuery( cl, condObj, selObj );
   } );
}


