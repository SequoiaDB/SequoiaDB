/*******************************************************************************
*@Description : $default
*@Test Point: 1. {"ExtraField1.nest1.nest2.nest3":{"$default":"Nest Array"}}
*                 [ nest object record, query by using selector $default]
*             2. {"Group.Service.Name": {"$default":"Nest Array"}}
*                 [nested field]
*             3. 13 kinds of data types as default value
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

   /*【Test Point 1】 nest object:*
    * {"ExtraField1.nest1.nest2.nest3":{"$default":"Nest Array"}}*/
   var condObj = {};
   var selObj = { "ExtraField1.nest1.nest2.nest3": { "$default": "Nest Array" } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "a,b" != retObj["ExtraField1"]["nest1"]["nest2"]["nest3"] )
   {
      throw new Error( "ErrQueryRecord1" );
   }

   /*【Test Point 2】 nest array, array element is object*/
   var condObj = {};
   var selObj = { "Group.Service.Name": { "$default": "Nest Array" } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   retObj = retObj["Group"][0]["Service"];
   if( 20000 != retObj[0]["Name"] && 20001 != retObj[1]["Name"] &&
      20002 != retObj[2]["Name"] && 20003 != retObj[3]["Name"] && 1 == recordNum )
   {
      throw new Error( "ErrQueryRecord1" );
   }

   /*【Test Point 3】 {a:[{b:[{c:{"d": "e"}}]}]}*/
   var condObj = {};
   var selObj = { "ExtraField2.nest1.nest2.nest3": { "$default": "Nest Array" } };
   var ret = selMainQuery( cl, condObj, selObj );
}
