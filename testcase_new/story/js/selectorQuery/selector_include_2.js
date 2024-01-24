/*******************************************************************************
*@Description : $include: nest array
*@Example: record: {a:{b:{c:["d", "e"}}}/{a:[{b:[{c:["d", "e"]}]}]}/
*                  {a:[{b:[{c:["d": "e"]}]}]}
*          query: db.cs.cl.find({},{"a.b.c":{$include:1/0}}})
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var recordNum = 100;
   var addRecord1 = { "nest1": { "nest2": { "nest3": ["a", "b"] } } };
   var addRecord2 = [{ "nest1": [{ "nest2": [{ "nest3": ["a", "b"] }] }] }];
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data
   selAutoGenData( cl, recordNum, addRecord1, addRecord2 );

   /*【Test Point 1】 record: {a:{b:{c:["d", "e"}}}*/
   var condObj = {};
   var selObj = { "ExtraField1.nest1.nest2.nest3": { "$include": 1 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   selVerifyIncludeRet( ret, selObj, 1, "1" );
   var condObj = {};
   var selObj = { "ExtraField1.nest1.nest2.nest3": { "$include": 0 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   selVerifyIncludeRet( ret, selObj, 0, "0" );

   /*【Test Point 2】 {a:[{b:[{c:["d", "e"]}]}]}*/
   var condObj = {};
   var selObj = { "Group.Service.Name": { "$include": 1 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   selVerifyIncludeRet( ret, selObj, 1, "4" );
   var condObj = {};
   var selObj = { "Group.Service.Name": { "$include": 0 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   selVerifyIncludeRet( ret, selObj, 0, "0" );

   /*【Test Point 3】 {a:[{b:[{c:["d": "e"]}]}]}*/
   var condObj = {};
   var selObj = { "ExtraField2.nest1.nest2.nest3": { "$include": 1 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   selVerifyIncludeRet( ret, selObj, 1, "1" );
   var condObj = {};
   var selObj = { "ExtraField2.nest1.nest2.nest3": { "$include": 0 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   selVerifyIncludeRet( ret, selObj, 0, "0" );
}


