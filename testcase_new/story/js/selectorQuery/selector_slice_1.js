/*******************************************************************************
*@Description : $slice
*               record: ["a","b","c","d","e","f", ... , "v","w","x","y","z"]
*               query: {"$slice": 6}  [ get the previous six array elements ]
*               query: {"$slice": -7}  [ get the last seven array elements ]
*               query: {"$slice": [10,10]}
*               query: {"$slice": [-20,19]}
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

   /*【Test Point 1.1】 {"$slice": 6}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": 6 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "a,b,c,d,e,f" != retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify1-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );
   /*【Test Point 1.2】 {"$slice": -7}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": -7 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "t,u,v,w,x,y,z" != retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify2-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );

   /*【Test Point 2.1】 {"$slice": [10,10]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": [10, 10] } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "k,l,m,n,o,p,q,r,s,t" != retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify2-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );
   /*【Test Point 2.2】 {"$slice": [-20,19]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": [-20, 19] } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y" != retObj["ExtraField1"] &&
      1 == recordNum )
   {
      throw new Error( "ErrVerify2-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );
}


