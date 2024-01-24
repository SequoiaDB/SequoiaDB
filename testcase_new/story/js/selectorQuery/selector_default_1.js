/*******************************************************************************
*@Description : $default
*@Test Point: 1. {"GroupID":{"$default":255}}   [normal record, non-nested]
*             2. {"ExtraField1.nest1.nest2.nest3":{"$default":
*                 "when nest test,use$defalut"}} [nested field]
*             3. 13 kinds of data types as default value
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var recordNum = 1;
   var addRecord = { "nest1": { "nest2": { "nest3": "when nest test, use $defalut" } } };
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data
   selAutoGenData( cl, recordNum, addRecord );

   /*【Test Point 1】 normal record, non-nest*/
   var condObj = {};
   var selObj = { "GroupID": { "$default": 255 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( 1 == recordNum && 1 != retObj["GroupID"] )
   {
      throw new Error( "ErrQueryRecord1" );
   }

   /*【Test Point 2】 nested field*/
   var condObj = {};
   var selObj = { "ExtraField1.nest1.nest2.nest3": { "$default": "when nest test, use $defalut" } };
   var ret = selMainQuery( cl, condObj, selObj );
   var retObj = JSON.parse( ret );
   if( 1 == recordNum && "when nest test, use $defalut" !=
      retObj["ExtraField1"]["nest1"]["nest2"]["nest3"] )
   {
      throw new Error( "ErrQueryRecord2" );
   }

   /*【Test Point 3】 13 types record*/
   var condObj = {};
   var defaultArr = new Array( 123, 3000000000, 123.456, 123e+50, "value",
      { "$oid": "123abcd00ef12358902300ef" }, true,
      { "$date": "2012-01-01" },
      { "$timestamp": "2012-01-01-13.14.26.124233" },
      { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
      { "$regex": "^张", "$options": "i" },
      { "subobj": "value" }, ["abc", 0, "def"], null );
   for( var i = 0; i < defaultArr.length; ++i )
   {
      var selObj = { "ExtraField1.nest1.nest2.nest4.NotExist": { "$default": defaultArr[i] } };
      var ret = selMainQuery( cl, condObj, selObj );
   }
}



