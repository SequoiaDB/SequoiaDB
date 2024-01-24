/***************************************************************************
@Description :create a index which tne key is more than 1000B
@Modify list :
              2014-5-21  xiaojun Hu  Init
              2016-3-4   yan wu Modify(增加预置条件和结果检测（插入字段对应的values的大小超过1000b，检查创建索引结果)
****************************************************************************/
main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );
   var varCL = commCreateCL( db, csName, clName );

   varCL.insert( { a: 1, longint: 2147483647000, floatNum: 12345.456 } );
   varCL.insert( { a: 2, b: "abcdgasdgasdgadgadgadgasdgadsgasdgadgasdgasdgasdgasdgasdgetetetetetetetasdgasdgasdgasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggasdgasdgasdgasdgadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggadgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdgasdggas12" } );
   createIndex( varCL, "testindex", { b: 1 }, false, false, SDB_IXM_KEY_TOO_LARGE );

   commDropCL( db, csName, clName, false, false );
}
