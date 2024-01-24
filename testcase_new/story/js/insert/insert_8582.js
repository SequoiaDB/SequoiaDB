/******************************************************************************
*@Description : seqDB-8582:插入超出double类型表示范围的数据并读取
*@Author      : 2019-5-29  wuyan modify
******************************************************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_8582";
   var cl = readyCL( clName );

   var obj = [
      { "a": -1.797693134862315e+308 }, { "a": 1.797693134862315e+308 },
      { "a": -1.7976931348623157e+308 }, { "a": 1.7976931348623157e+308 },
      { "a": -1.78E+309 }, { "a": 1.78E+309 }];
   cl.insert( obj );

   var expRecords = [
      { "a": -1.797693134862315e+308 }, { "a": 1.797693134862315e+308 },
      { "a": -Infinity }, { "a": Infinity },
      { "a": -Infinity }, { "a": Infinity }]
   var actRecords = cl.find();
   commCompareResults( actRecords, expRecords );

   commDropCL( db, COMMCSNAME, clName );
}
