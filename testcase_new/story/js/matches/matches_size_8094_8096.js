/************************************************************************
*@Description:   seqDB-8094:使用$size查询，目标字段为嵌套数组
                 seqDB-8096:使用$size查询，目标字段为数组且数组元素为嵌套对象
*@Author:  2016/5/23  xiaoni huang
*@Mender: 2020/10/26 Zixian Yan
************************************************************************/
testConf.clName = COMMCLNAME + "cl_8094_8096";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var rawData = [ { a: 0, b: [] },
                   { a: 1, b: [1] },
                   { a: 2, b: [2, { c: "" }] },
                   { a: 3, b: [3, null, ""] },
                   { a: 4, b: [0, [{ c: null }], [{ d: [{ e: "" }, { f: [] }] }], { g: { h: { i: [] } } }] } ];
   cl.insert( rawData );

   var sizeNum = [-1, 0, 1, 2, 3, 4];
   for( var i in sizeNum )
   {
      var rc = cl.find( { b: { $size: 1, $et: sizeNum[i] } } ).sort( { a: 1 } );
      var expRec = [];
      if ( sizeNum[i] != -1 )
      {
         expRec = [rawData[ i+1 ]];
      }
      checkRec( rc, expRec );
   }

}
