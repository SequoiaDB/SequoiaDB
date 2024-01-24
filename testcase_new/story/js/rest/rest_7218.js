/****************************************************
@description:   get count, normal case
                testlink cases: seqDB-7218
@input:         insert 1 rec
@expectation:   varCL.count() result == get count by rest
@modify list:
                2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7218";

main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var varCL = commCreateCL( db, csName, clName, {}, true, false, "create cl in begin" );
   varCL.insert( [
      { age: 11, name: "Tom", male: true }
   ] );
   getcountAndCheck( varCL );
   commDropCL( db, csName, clName, false, false, "drop cl in clean" );
}

function getcountAndCheck ( varCL )
{
   var word = "get count";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName], [0] );
   var arr = infoSplit;

   if( JSON.parse( arr[1] ).Total != varCL.count() )
   {
      throw new Error( "rest command  " + word + "result is " + JSON.parse( arr[1] ).Total + ", but cl.count() result is " + 1 );
   }
}




