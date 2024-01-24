/******************************************************************************
 * @Description   : seqDB-24437:访问计划指定Abbrev
 * @Author        : liuli
 * @CreateTime    : 2021.10.19
 * @LastEditTime  : 2021.10.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24437";

main( test );

function test ( args )
{
   var indexName = "index_24437";
   var dbcl = args.testCL;

   dbcl.createIndex( indexName, { a: 1 } );

   // 构造访问的字符串,str1 100个字符，str2 101个字符，str3 16M相同字符，str4 16M不同字符
   var basic = "iuytredfgh";
   var str1 = "";
   var str2 = "";
   var str3 = "";
   var str4 = "";
   for( var i = 0; i < 10; i++ )
   {
      str1 += basic;
   }
   str2 = str1 + "a";

   var arr1 = new Array( 16 * 1024 * 1024 );
   str3 = arr1.join( "a" );

   var arr2 = new Array( 1024 * 1024 );
   str4 = arr2.join( basic + "test" );

   // 指定Abbrev为true，100个字符不进行忽略
   var obj = dbcl.find( { a: str1 } ).explain( { Abbrev: true } ).next().toObj();
   checkExplain( obj, str1 );

   // 101个字符进行截断
   // 字符截断后显示如下，"sdfghjklsdfghjklsdfghjklsdfghjklsdfghjklsdfghjklsdfghjklsdfg ...<16777148 characters more>..."
   // 只校验截断后内容前60个字符与原内容相同，后面19个字符显示为 characters more>...
   // 截断字符时字符不同显示的后缀
   var suffix = "characters more>...";
   var obj = dbcl.find( { a: str2 } ).explain( { Abbrev: true } ).next().toObj();
   var actQuery = obj.Query["$and"][0]["a"]["$et"];
   assert.notEqual( actQuery, str2 );
   assert.equal( actQuery.slice( 0, 60 ), str2.slice( 0, 60 ) );
   assert.equal( actQuery.slice( -19 ), suffix );

   // 超长字符进行截断
   var obj = dbcl.find( { a: str4 } ).explain( { Abbrev: true } ).next().toObj();
   var actQuery = obj.Query["$and"][0]["a"]["$et"];
   assert.notEqual( actQuery, str4 );
   assert.equal( actQuery.slice( 0, 60 ), str4.slice( 0, 60 ) );
   assert.equal( actQuery.slice( -19 ), suffix );

   // 超长字符对重复字符进行统计
   // 字符截断后重复字符统计如下，"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa...<a repeat 963 times>..."
   // 只校验截断后内容前60个字符与原内容相同，后面9个字符显示为 times>...
   // 字符相同时截断字符显示的后缀
   var suffix = "times>...";
   var obj = dbcl.find( { a: str3 } ).explain( { Abbrev: true } ).next().toObj();
   var actQuery = obj.Query["$and"][0]["a"]["$et"];
   assert.notEqual( actQuery, str3 );
   assert.equal( actQuery.slice( 0, 60 ), str3.slice( 0, 60 ) );
   assert.equal( actQuery.slice( -9 ), suffix );

   // 指定Abbrev为false
   var obj = dbcl.find( { a: str1 } ).explain( { Abbrev: false } ).next().toObj();
   checkExplain( obj, str1 );

   var obj = dbcl.find( { a: str2 } ).explain( { Abbrev: false } ).next().toObj();
   checkExplain( obj, str2 );

   assert.tryThrow( SDB_SYS, function()
   {
      dbcl.find( { a: str3 } ).explain( { Abbrev: false } );
   } );

   assert.tryThrow( SDB_SYS, function()
   {
      dbcl.find( { a: str4 } ).explain( { Abbrev: false } );
   } );

   // 不指定Abbrev，默认值为false
   var obj = dbcl.find( { a: str1 } ).explain().next().toObj();
   checkExplain( obj, str1 );

   var obj = dbcl.find( { a: str2 } ).explain().next().toObj();
   checkExplain( obj, str2 );

   assert.tryThrow( SDB_SYS, function()
   {
      dbcl.find( { a: str3 } ).explain();
   } );

   assert.tryThrow( SDB_SYS, function()
   {
      dbcl.find( { a: str4 } ).explain();
   } );

}

function checkExplain ( obj, str )
{
   var actQuery = obj.Query["$and"][0]["a"]["$et"];
   assert.equal( actQuery, str );
}