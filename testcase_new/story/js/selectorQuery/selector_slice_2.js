/*******************************************************************************
*@Description : $slice
*               record: ["a","b","c","d","e","f", ... , "v","w","x","y","z"]
*               query: {"$slice": 0}  [ get nothing from array ]
*               query: {"$slice": 30}  [ countValue(30) larger than array lengh]
*               query: {"$slice": -50}  [ countValue(-50) larger than array lengh]
*               query: {"$slice": [100,5]} [ countValue(100) larger than array lengh]
*               query: {"$slice": [-1000,20]}
*                                     [ countValue(100) larger than array lengh]
*               query: {"$slice": [0,10]} [ countValue(0) the first location of array]
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

   /*【Test Point 1】 {"$slice": 0}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": 0 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "" != retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify1-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );


   /*【Test Point 2.1】 {"$slice": 30}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": 30 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var verifyObj = JSON.parse( cl.find().toArray() );
   if( "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z" != retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify2.1-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );
   /*【Test Point 2.2】 {"$slice": -50}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": -50 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var verifyObj = JSON.parse( cl.find().toArray() );
   if( "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z" !=
      retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify2.2-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );


   /*【Test Point 3.1】 {"$slice": [100, 5]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": [100, 5] } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "" != retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify3.1-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );
   /*【Test Point 3.2】 {"$slice": [-1000, 20]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": -50 } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   var verifyObj = JSON.parse( cl.find().toArray() );
   if( "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z" !=
      retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify3.2-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );


   /*【Test Point 4 】 {"$slice": [0,10]}*/
   var condObj = {};
   var selObj = { "ExtraField1": { "$slice": [0, 10] } };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "a,b,c,d,e,f,g,h,i,j" != retObj["ExtraField1"] && 1 == recordNum )
   {
      throw new Error( "ErrVerify4-$Slice" );
   }
   selVerifyNonSelectorObj( cl, ret, condObj, selObj );
}


