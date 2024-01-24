/*******************************************************************************
*@Description : $elemMatch/$elemMatchOne
*@Example: record: {"field":[{a:"b"},{a:"c"},{a:"d"},{a:"b"},{a:"f"}]
*          query: db.cs.cl.find({},{"field":{$elemMatch:{a:"b"}}})
*          query: db.cs.cl.find({},{"field":{$elemMatchOne:{a:"b"}}})
*@Modify list :
*               2015-02-04  xiaojun Hu  Change
*******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var recordNum = 1;
   var addRecord = [{ name: "ZhangSan", age: 18 },
   { name: "WangErmazi", age: 19 },
   { name: "lucy", age: 20 },
   { name: "alex", age: 18 },
   { name: "shanven", age: 18 }];
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data
   selAutoGenData( cl, recordNum, addRecord );

   /*【Test Point 1】 $elemMatch: normal operation*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$elemMatch": { age: 18 } } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var cnt = retObj["ExtraField1"].length;
   for( var i = 0; i < cnt; ++i )
   {
      assert.equal( selObj["ExtraField1"]["$elemMatch"]["age"], retObj["ExtraField1"][i]["age"] );
   }
   assert.equal( 3, cnt );
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );

   /*【Test Point 2】 $elemMatchOne: normal operation*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$elemMatchOne": { age: 18 } } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var cnt = retObj["ExtraField1"].length;
   for( var i = 0; i < cnt; ++i )
   {
      assert.equal( selObj["ExtraField1"]["$elemMatchOne"]["age"], retObj["ExtraField1"][i]["age"] );
   }
   assert.equal( 1, cnt );
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );
}


