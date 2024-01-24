/************************************************************************
*@Description:   seqDB-9105:导入timestamp类型数据合法（支持三种格式）:Timestamp()/Timestamp(字符串:YYYY-MM-DD-HH.mm.ss.ffffff)/Timestamp(数字:秒数, 数字:微秒)
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9105";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9105.json";
   var srcDatas = "{time:Timestamp('2015-06-05-16.10.33.000000')}\n{'time':Timestamp(1433492413, 0)}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"time":{"$timestamp":"2015-06-05-16.10.33.000000"}},{"time":{"$timestamp":"2015-06-05-16.20.13.000000"}}]';
   checkCLData( cl, expRecs );

   //import the data of {time:Timestamp()}
   var imprtFile1 = tmpFileDir + "9105a.json";
   var srcDatas1 = "{testtime:Timestamp()}"
   importData( COMMCSNAME, clName, imprtFile1, srcDatas1 );
   checkCLTimeData( cl );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

function checkCLTimeData ( cl )
{
   var actCnt = cl.find( { testtime: { $exists: 1 } } ).count();
   var expCnt = 1;

   if( Number( actCnt ) !== Number( expCnt ) )
   {
      throw new Error( "actCnt: " + actCnt + "\nexpCnt: " + expCnt );
   }

}
