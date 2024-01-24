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
   var addRecord2 = {
      "nest1": {
         "nest2": {
            "nest3":
               [{ "nest4": "element match query" },
               { "nest4": "element match query" },
               { "nest4": "element match query" },
               { "nest4": "element match query" }]
         }
      }
   };
   var addRecord3 = [{
      "nestArr1": [{
         "nestArr2": [{
            "nestArr3":
               [{ "abc": "中文" }, { "abc": "中文" }, { "abc": "中文" }]
         }]
      }]
   }];
   var addRecord4 = { "nest1": { "nest2": { "nest3": ["nest4", 123, "element match query"] } } };
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data
   selAutoGenData( cl, recordNum, addRecord1, addRecord2, addRecord3 );

   /*【Test Point 1】 $elemMatch: nest array, element is object*/
   var condObj = {};
   var selObj = { "ExtraField3.nestArr1.nestArr2.nestArr3": { "$elemMatch": { "abc": "中文" } } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var cnt = retObj["ExtraField3"][0]["nestArr1"][0]["nestArr2"][0]["nestArr3"];
   for( var i = 0; i < cnt.length; ++i )
   {
      assert.equal( "中文", cnt[0]["abc"] );
   }
   assert.equal( 3, cnt.length );
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );

   /*【Test Point 2】 $elemMatchOne: nest array, element is object*/
   var condObj = {};
   var selObj = { "ExtraField3.nestArr1.nestArr2.nestArr3": { "$elemMatchOne": { "abc": "中文" } } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var cnt = retObj["ExtraField3"][0]["nestArr1"][0]["nestArr2"][0]["nestArr3"];
   for( var i = 0; i < cnt.length; ++i )
   {
      assert.equal( "中文", cnt[0]["abc"] );
   }
   assert.equal( 1, cnt.length );
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );


   /*【Test Point 3】 $elemMatch: select query from {a:{b:{c:{d:["1","2","3"]}}}}*/
   var condObj = {};
   var selObj = { "ExtraField4.nest1.nest2.nest3": { "$elemMatch": "nest4" } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var ret = selMainQuery( cl, condObj, selObj );
   } );
   /*【Test Point 4】 $elemMatchOne: select query from {a:{b:["1","2","3"]}}*/
   var condObj = {};
   var selObj = { "ExtraField4.nest1.nest2.nest3": { "$elemMatchOne": "nest4" } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var ret = selMainQuery( cl, condObj, selObj );
   } );

   /*【Test Point 5】 $elemMatch: nest array, array elements is object*/
   var condObj = {};
   var selObj = {
      "ExtraField2.nest1.nest2.nest3":
         { "$elemMatch": { "nest4": "element match query" } }
   };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var cnt = retObj["ExtraField2"]["nest1"]["nest2"]["nest3"];
   for( var i = 0; i < cnt.length; ++i )
   {
      assert.equal( "element match query", cnt[0]["nest4"] );
   }
   assert.equal( 4, cnt.length );

   selVerifyNonSelectorObj( cl, ret, condObj, selObj );

   /*【Test Point 6】 $elemMatchOne: nest array, array elements is object*/
   var condObj = {};
   var selObj = {
      "ExtraField2.nest1.nest2.nest3":
         { "$elemMatchOne": { "nest4": "element match query" } }
   };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var cnt = retObj["ExtraField2"]["nest1"]["nest2"]["nest3"];
   for( var i = 0; i < cnt.length; ++i )
   {
      assert.equal( "element match query", cnt[0]["nest4"] );
   }
   assert.equal( 1, cnt.length );

   selVerifyNonSelectorObj( cl, ret, condObj, selObj );
}


