/******************************************************************************
*@Description : test import js file with return and no return
*               seqDB-11902:使用import、importOnce导入js文件（无返回值）
*               seqDB-11903:使用import importOnce导入js文件（有返回值）
*@author      : Liang XueWang 
******************************************************************************/
main( test );

function test ()
{
   // create js file without ret and with ret
   createWithoutRetFile();
   createWithRetFile();

   // test import js file without return
   var ret = import( withoutRetFile );
   assert.equal( typeof ( ret ), "undefined" );
   var sum = add( 1, 2 );
   assert.equal( sum, 3 );

   // test import js file with return
   var ret = import( withRetFile );
   assert.equal( ret, 6 );
   assert.equal( tmp, 100 );

   var pro = mul( 10, 20 );
   assert.equal( pro, 200 );

   // remove js file without ret and with ret    
   removeFile( withoutRetFile );
   removeFile( withRetFile );
}