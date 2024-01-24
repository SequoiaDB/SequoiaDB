/*******************************************************************
*@Description : test mix import importOnce js file
*               seqDB-11911:混合使用import importOnce导入文件
*@author      : Liang XueWang 
*******************************************************************/
// js file to mix, first import then importOnce
var mixFile1 = WORKDIR + "/mix1_11911.js";
// js file to mix, first importOnce then import
var mixFile2 = WORKDIR + "/mix2_11911.js";
var a = 0;
var b = 0;
main( test );

function test ()
{





   // create mix use import importOnce file
   createMixFile();

   // test mix use import and importOnce
   // first import then importOnce

   import( mixFile1 );
   importOnce( mixFile1 );
   assert.equal( a, 1 );

   // test mix use import and importOnce
   // first importOnce then import    

   importOnce( mixFile2 );
   import( mixFile2 );
   assert.equal( b, 2 );

   // remove mix use import importOnce file
   removeFile( mixFile1 );
   removeFile( mixFile2 );
}
function createMixFile ()
{
   var file = new File( mixFile1 );
   file.write( "a++;" );
   file.close();
   file = new File( mixFile2 );
   file.write( "b++;" );
   file.close();
}