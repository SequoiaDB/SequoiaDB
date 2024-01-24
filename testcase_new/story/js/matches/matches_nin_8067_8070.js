/************************************************************************
*@Description:   seqDB-8067:使用$nin查询，value取1个值
                 seqDB-8070:使用$nin查询，给定值为空（如{$nin:[]}）
                     dataType: array
*@Author:  2016/5/20  xiaoni huang
*@Mender:  2020/10/16 Zixian Yan
************************************************************************/
testConf.clName = COMMCLNAME + "cl_8067_8070";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   cl.insert( { a: [{ b: 2147483648 }, { c: 1.7E+308 }] } );

   var rc1 = cl.find( { a: { $nin: [] } } );
   var rc2 = cl.find( { a: { $nin: [{ b: 2147483648 }] } } );
   var expRec1 = [ { a: [{ b: 2147483648 }, { c: 1.7E+308 }] } ];
   var expRec2 = [];

   checkRec( rc1, expRec1 );
   checkRec( rc2, expRec2 );
}
