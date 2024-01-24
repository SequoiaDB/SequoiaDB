/*******************************************************************************
*@Description : test selector: $include nest array
*@Example: record: {a:{b:{c:["d", "e"}},x:[{y:[{z:["m", "n"]}]}]}/
*          query: db.cs.cl.find({},{"a.b.c":{$include:1}}})
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/
main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );
   var recordNum = 1;
   var addRecord1 = { "nest1": { "nest2": { "nest3": ["a", "b"] } } };
   var addRecord2 = [{ "nest1": [{ "nest2": [{ "nest3": ["a", "b"] }] }] }];
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data
   selAutoGenData( cl, recordNum, addRecord1, addRecord2 );

   /*Test Point 1 $include=1/0, different field name*/
   var condObj = {};
   var selObj = {
      "ExtraField1.nest1.nest2.nest3": { "$include": 1 },
      "GroupID": { "$include": 1 },
      "PrimariNode": { "$include": 1 },
      "Group.Service.Name": { "$include": 1 },
      "ExtraField2.nest1.nest2.nest3": { "$include": 0 }
   };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var ret = selMainQuery( cl, condObj, selObj );
   } );
}



