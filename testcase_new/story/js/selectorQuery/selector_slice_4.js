/*******************************************************************************
*@Description : $slice
*               record: ["a","b", ... , {"n":"N"},{"o":"O"},{"p":"P"} , ... ,
*                        "w","x","y","z"] (array have normal element and object)
*               query: {"$slice": [12, 13]} [ get normal element and object from array]
*               query: {"$slice": [19, -7]} [abnormal test]
*               query: {"$slice": [19, 0]} [abnormal test]
*               query: {"$slice": [9,70]}
*               query: {"$slice": {"HostName": "Host_1"}}    [abnormal test]
*               query: {"$slice": [-20,19,5]}} [abnormal test]
*@Modify list :
*               2015-01-26  xiaojun Hu  Init
*******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var recordNum = 1;
   var addRecord1 = ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
      { "n": "N" }, { "o": "O" }, { "p": "P" }, { "q": "Q" }, { "r": "R" },
      "s", "S", "t", "T", { "u": "U" }, { "v": "V" }, "w", "x", "y", "z"];
   var addRecord2 = [{ "nest1": [{ "nest2": [{ "nest3": [1, 2, 3, 4, 5, 6, 7, 8, 9] }] }] }];
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data
   selAutoGenData( cl, recordNum, addRecord1, addRecord2 );

   /*【Test Point 1】 {"$slice": [12, 13]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": [12, 13] } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( '["m",{"n":"N"},{"o":"O"},{"p":"P"},{"q":"Q"},' +
      '{"r":"R"},"s","S","t","T",{"u":"U"},{"v":"V"},"w"]' !=
      JSON.stringify( retObj["ExtraField1"] ) && 1 == recordNum )
   {
      throw new Error( "ErrVerify1-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );

   /*【Test Point 2】 abnormal: {"$slice": {"HostName": "Host_1"}]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": { "HostName": "Host_1" } } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var ret = selMainQuery( cl, condObj, selObj );
   } );

   /*【Test Point 3】 abnormal: {"$slice": [19, -7]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": [19, -7] } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( '["S","t","T",{"u":"U"},{"v":"V"},"w","x","y","z"]' !=
      JSON.stringify( retObj["ExtraField1"] ) && 1 == recordNum )
   {
      throw new Error( "ErrVerify2-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );

   /*【Test Point 4】 {"$slice": [19, 0]}*/
   var condObj = {};
   var selObj = {
      "Group.Service": { "$slice": [1, 3] },
      "Group.NodeID": { "$slice": -2 },
      "ExtraField1": { "$slice": [-19, 10] },
      "ExtraField2": { "$slice": 5 }
   };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( '["j","k","l","m",{"n":"N"},{"o":"O"},{"p":"P"},{"q":"Q"},' +
      '{"r":"R"},"s"]' != JSON.stringify( retObj["ExtraField1"] ) &&
      1 == recordNum )
   {
      throw new Error( "ErrVerify4-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );


   /*【Test Point 5】 Field isn't array.{"Group.HostName":{"$slice": [-20,19,5]}}*/
   var condObj = {};
   var selObj = { "Group.HostName": { "$slice": [-20, 19, 5] } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var ret = selMainQuery( cl, condObj, selObj );
   } );
}
