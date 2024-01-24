/*******************************************************************************
*@Description : test selector: $include nest 6 layer array
*@Example: record: {a:{b:{c:["d", "e"}}}
*          query: db.cs.cl.find({},{"a.b.c":{$include:1/0, "a.b":{$include:1/0}}}})
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/


main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var recordNum = 100;
   var nestLayer = 31;
   var totalLayer = 31;
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // gen
   var nestRecord1 = genNestedObject( nestLayer, totalLayer, "obj" )
   var nestRecord2 = genNestedObject( nestLayer, totalLayer, "arr" )
   var nestRecord3 = genNestedObject( nestLayer, totalLayer, "arr_obj" )
   var nestRecord4 = genNestedObject( nestLayer, totalLayer, "obj_arr" )
   var addRecord1 = JSON.parse( nestRecord1 );
   var addRecord2 = JSON.parse( nestRecord2 );
   var addRecord3 = JSON.parse( nestRecord3 );
   var addRecord4 = JSON.parse( nestRecord4 );

   // auto generate data
   selAutoGenData( cl, recordNum, addRecord1,
      addRecord2, addRecord3, addRecord4 );

   /*Test Point 1 $include=1 different field name do $include quering*/
   var selobj = autoGenSelector( "ExtraField1", 31 );
   var selArr = autoGenSelector( "ExtraField2", 31 );
   var selArrObj = autoGenSelector( "ExtraField3", 31 );
   var selObjArr = autoGenSelector( "ExtraField4", 31 );
   var condObj = {};
   var selStr = '{"' + selobj + '": {"$include": 1}, "' +
      selArr + '": { "$include": 1 }, "' +
      selArrObj + '": { "$include": 1 }, "' +
      selObjArr + '": { "$include": 1 } }';
   var selObj = JSON.parse( selStr );
   var ret = selMainQuery( cl, condObj, selObj );
   /*Test Point 1 $include:1/0 */
   var condObj = {};
   var selStr = '{"' + selobj + '": {"$include": 0}, "' +
      selArr + '": { "$include": 0 }, "' +
      selArrObj + '": { "$include": 0 }, "' +
      selObjArr + '": { "$include": 0 } }';
   var selObj = JSON.parse( selStr );
   var ret = selMainQuery( cl, condObj, selObj );

   /*Test Point 2 $default*/
   var selobj = autoGenSelector( "ExtraField1", 31 );
   var selArr = autoGenSelector( "ExtraField2", 31 );
   var selArrObj = autoGenSelector( "ExtraField3", 31 );
   var selObjArr = autoGenSelector( "ExtraField4", 31 );
   var condObj = {};
   var selStr = '{"' + selobj + '": {"$default": "object" }, "' +
      selArr + '": { "$default": "array" }, "' +
      selArrObj + '": { "$default": "array_object" }, "' +
      selObjArr + '": { "$default": "object_array" } }';
   var selObj = JSON.parse( selStr );
   var ret = selMainQuery( cl, condObj, selObj );

   /*Test Point 3 $slice*/
   var selobj = autoGenSelector( "ExtraField1", 31 );
   var selArr = autoGenSelector( "ExtraField2", 31 );
   var selArrObj = autoGenSelector( "ExtraField3", 31 );
   var selObjArr = autoGenSelector( "ExtraField4", 31 );
   var condObj = {};
   var selStr = '{"' + selobj + '": {"$slice": [2, 9] }, "' +
      selArr + '": { "$slice": [2, 9] }, "' +
      selArrObj + '": { "$slice": [2,9] }, "' +
      selObjArr + '": { "$slice": [2,9] } }';
   var selObj = JSON.parse( selStr );
   var ret = selMainQuery( cl, condObj, selObj );

   /*Test Point 4 $elemMatch*/
   var selobj = autoGenSelector( "ExtraField1", 31 );
   var selArr = autoGenSelector( "ExtraField2", 31 );
   var selArrObj = autoGenSelector( "ExtraField3", 31 );
   var selObjArr = autoGenSelector( "ExtraField4", 31 );
   var condObj = {};
   var selStr = '{"' + selobj + '": {"$elemMatch": {"age":19} }, "' +
      selArr + '": { "$elemMatch": {"age":19} }, "' +
      selArrObj + '": { "$elemMatch": {"age":19} }, "' +
      selObjArr + '": { "$elemMatch": {"age":19} } }';
   var selObj = JSON.parse( selStr );
   var ret = selMainQuery( cl, condObj, selObj );

   /*Test Point 5 $elemMatchOne*/
   var selobj = autoGenSelector( "ExtraField1", 31 );
   var selArr = autoGenSelector( "ExtraField2", 31 );
   var selArrObj = autoGenSelector( "ExtraField3", 31 );
   var selObjArr = autoGenSelector( "ExtraField4", 31 );
   var condObj = {};
   var selStr = '{"' + selobj + '": {"$elemMatchOne": {"age":19} }, "' +
      selArr + '": { "$elemMatchOne": {"age":19} }, "' +
      selArrObj + '": { "$elemMatchOne": {"age":19} }, "' +
      selObjArr + '": { "$elemMatchOne": {"age":19} } }';
   var selObj = JSON.parse( selStr );
   var ret = selMainQuery( cl, condObj, selObj );

}

// auto generate nested json object
function genNestedObject ( nestLayer, totalLayer, nestType )
{
   if( undefined == nestLayer )
   {
      nestLayer = 31;
      totalLayer = 31;
   }
   if( undefined == totalLayer )
      totalLayer = nestLayer;
   var nestRecord = "";
   var nestFieldVal = "Oh, My God !";
   // define the brace
   if( ( nestLayer != totalLayer && "arr" == nestType ) ||
      ( nestLayer != totalLayer && "arr_obj" == nestType ) )
   {
      var leftBrace = "[{";
      var rightBrace = "}]";
   }
   else
   {
      var leftBrace = "{";
      var rightBrace = "}";
   }
   // nested type
   if( "obj" == nestType || "arr_obj" == nestType )
      nestFieldVal = "Oh, My God !";
   else if( "arr" == nestType || "obj_arr" == nestType )
      nestFieldVal = '[ "Oh, My God !",257,1048,{"age":19},{"age":20},{"age":19}]';
   if( 0 == nestLayer )
   {
      if( "arr" == nestType || "obj_arr" == nestType )
         nestRecord = nestFieldVal;
      else
         nestRecord = '"' + nestFieldVal + '"';
      return nestRecord;
   }
   else
   {
      var nestFunc = genNestedObject( nestLayer - 1, totalLayer, nestType );
      var cnt = totalLayer - nestLayer;
      var nestFieldName = "nestField_" + cnt;
      nestRecord = leftBrace + '"' + nestFieldName + '": ' + nestFunc + rightBrace;
      return nestRecord;
   }
}

// auto generate selector for query
// such as : {"field_0":{$include:1},...,"field_n":{$include:1}}
function autoGenSelector ( selField, fieldNum )
{
   for( var i = 0; i < fieldNum; ++i )
   {
      selField = selField + "." + "nestField_" + i;
   }
   return selField;
}





