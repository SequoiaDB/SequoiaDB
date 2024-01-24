/*******************************************************************************
*@Description : when the number of fields greater than 65, we use selector to
*               query. such as: $include/$default/$slice/$elemMatchOne/$elemMatch
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var recordNum = 100;
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data
   autoGenData( cl, recordNum );

   /*【Test Point 1】$include=1/0*/
   var selField = "$include";
   var selValue = 1;
   var genSelector = autoGenSelector( selField, selValue );
   var condObj = {};
   var selObj = JSON.parse( genSelector );
   var ret = selMainQuery( cl, condObj, selObj );
   selValue = 0;
   genSelector = autoGenSelector( selField, selValue );
   condObj = {};
   selObj = JSON.parse( genSelector );
   var ret = selMainQuery( cl, condObj, selObj );

   /*【Test Point 2】$default*/
   var selField = "$default";
   var selValue = '{ "$oid" : "123abcd00ef12358902300ef" }';
   var genSelector = autoGenSelector( selField, selValue );
   var condObj = {};
   var selObj = JSON.parse( genSelector );
   var ret = selMainQuery( cl, condObj, selObj );

   /*【Test Point 3】$slice*/
   var selField = "$slice";
   var selValue = '[2, 4]';
   var genSelector = autoGenSelector( selField, selValue );
   var condObj = {};
   var selObj = JSON.parse( genSelector );
   var ret = selMainQuery( cl, condObj, selObj );

   /*【Test Point 4】$elemMatch*/
   var selField = "$elemMatch";
   var selValue = '{"co.no":80}';
   var genSelector = autoGenSelector( selField, selValue );
   var condObj = {};
   var selObj = JSON.parse( genSelector );
   var ret = selMainQuery( cl, condObj, selObj );

   /*【Test Point 5】$elemMatchOne*/
   var selField = "$elemMatch";
   var selValue = '{"NO":80}';
   var genSelector = autoGenSelector( selField, selValue );
   var condObj = {};
   var selObj = JSON.parse( genSelector );
   var ret = selMainQuery( cl, condObj, selObj );
}

// auto generate data, which have 65 fields
var fieldNum = 70;
function autoGenData ( cl, recordNum )
{
   for( var i = 0; i < recordNum; ++i )
   {
      var record = "{";
      var field = "";
      for( var j = 0; j < fieldNum; ++j )
      {
         if( 0 == j )
            field = "\"field_" + j + "\":[ \"a\",\"b\",{\"NO\":80},\"d\",{\"NO\":88}, \"e\",\"f\",{\"NO\":80},\"h\",{\"NO\":80},\"j\",\"k\"]";
         else
            field = ", \"field_" + j + "\":[ \"a\",\"b\",{\"NO\":80},\"d\",{\"NO\":88}, \"e\",\"f\",{\"NO\":80},\"h\",{\"NO\":80},\"j\",\"k\"]";
         record = record + field;
      }
      record = record + "}";
      cl.insert( JSON.parse( record ) );
   }
   var cnt = 0;
   while( recordNum != cl.count() && 1000 > cnt )
   {
      cnt++;
      sleep( 3 );
   }
   assert.equal( recordNum, cl.count() );
}
// auto generate selector for query
// such as : {"field_0":{$include:1},...,"field_n":{$include:1}}
function autoGenSelector ( selField, selValue )
{
   var selQuery = "{";
   for( var i = 0; i < ( fieldNum - 2 ); ++i )
   {
      if( 0 == i )
         selVar = "\"field_" + i + "\":{\"" + selField + "\":" + selValue + "}";
      else
         selVar = ", \"field_" + i + "\":{\"" + selField + "\":" + selValue + "}";
      selQuery = selQuery + selVar;
   }
   selQuery = selQuery + "}";
   return selQuery;
}


