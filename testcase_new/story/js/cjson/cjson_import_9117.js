/************************************************************************
*@Description:   seqDB-9117:导入NumberLong类型数据合法（支持两种格式）:
                                          a、NumberLong(数字:值)，如{number:NumberLong(100)} 
                                          b、NumberLong(字符串:值)，如{number:NumberLong("100")}
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9117";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9117.json";
   var srcDatas = "{number:NumberLong(100)}\n{number:NumberLong('100')}}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"number":100},{"number":100}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

