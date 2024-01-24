/*******************************************************************************
*@Description : $slice
*               record: ["a","b","c","d","e","f","g","h", ... ,"w","x","y","z"]
*               query: {"$slice": [19, 0]}
*               query: {"$slice": [19, -7]}    [abnormal test]
*               query: {"$slice": [9,70]}
*               query: {"$slice": {"HostName": "Host_1"}}    [abnormal test]
*@Modify list :
*               2015-01-26  xiaojun Hu  Init
*******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var recordNum = 1;
   var addRecord = ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
      "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"];
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data
   selAutoGenData( cl, recordNum, addRecord );

   /*【Test Point 1】 {"$slice": [19, 0]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": [19, 0] } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "" != retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify1-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );

   /*【Test Point 2】 abnormal: {"$slice": [19, -7]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": [19, -7] } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "t,u,v,w,x,y,z" != retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify2-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );

   /*【Test Point 3】 {"ExtraField1":{"$slice": [9,70]}}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": [9, 70] } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z" != retObj["ExtraField1"]
      && 1 == recordNum )
   {
      throw new Error( "ErrVerify3-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );

   /*【Test Point 4】 abnormal: {"$slice": {"HostName": "Host_1"}]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": { "HostName": "Host_1" } } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var ret = selMainQuery( cl, condObj, selObj );
   } );

}


