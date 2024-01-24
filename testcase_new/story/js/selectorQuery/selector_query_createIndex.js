/*******************************************************************************
*@Description : When we create index for querying, test query use selector:
*               $include/$default/$slice/$elemMatchOne/$elemMatch
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var recordNum = 100;
   var idxName = "rgIdIndex";
   var idxDef = { "GroupID": -1, "PrimaryNode": 1, "Version": 1 };
   var addRecord1 = { "nest1": { "nest2": { "nest3": "when nest test, use $include" } } };
   var addRecord2 = { "array0": [{ "array1": [{ "array2": ["a", "b", "c", "d", "e", "f", "g", "h", "i"] }] }] };
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data and create Index
   commCreateIndex( cl, idxName, idxDef );
   selAutoGenData( cl, recordNum, addRecord1, addRecord2 );

   /*Test Point 1 $include */
   var condObj = {};
   var selObj = {
      "GroupID": { "$include": 1 },
      "Group.Service.Name": { "$include": 1 },
      "Group.HostName": { "$include": 1 },
      "Version": { "$include": 1 }
   };
   var ret = selMainQuery( cl, condObj, selObj );
   var condObj = {};
   var selObj = {
      "GroupID": { "$include": 0 },
      "Group.Service.Name": { "$include": 0 },
      "Group.HostName": { "$include": 0 },
      "PrimaryNode": { "$include": 0 }
   };
   var ret = selMainQuery( cl, condObj, selObj );

   /*Test Point 2 $default*/
   var condObj = {};
   var selObj = {
      "GroupID": { "$default": [1, 2] },
      "Group.Service.Name": { "$default": [1, 2] },
      "Group.HostName": { "$default": [1, 2] },
      "Version": { "$default": [1, 2] }
   };
   var ret = selMainQuery( cl, condObj, selObj );

   /*Test Point 3 $slice*/
   var condObj = {};
   var selObj = {
      "Group.Service": { "$slice": [1, 2] },
      "ExtraField2.array0.array1.array2": { "$slice": [-7, 4] }
   };
   var ret = selMainQuery( cl, condObj, selObj );
}


