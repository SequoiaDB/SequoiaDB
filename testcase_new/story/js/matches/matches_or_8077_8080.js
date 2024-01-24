/************************************************************************
*@Description:      seqDB-8077:使用$or查询，value取1个值
                    seqDB-8080:使用$or查询，给定值为空（如{$or:[]}）
*@Author:  2016/5/24  xiaoni huang
*@Mender:  2020/10/16 Zixian Yan
************************************************************************/
testConf.clName = COMMCLNAME + "cl_8077_8080";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   cl.insert( [ { a: 0 }, { a: 1, b: null } ] );

   var rc1 = cl.find( { $or: [{ a: 0 }] } ).sort( { a: 1 } );
   var rc2 = cl.find( { $or: [] } ).sort( { a: 1 } );
   var expRec1 = [ {a: 0} ];
   var expRec2 = [ {a: 0}, { a: 1, b: null } ];

   checkRec( rc1, expRec1 );
   checkRec( rc2, expRec2 );
}
