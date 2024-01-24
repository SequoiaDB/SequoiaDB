/************************************************************************
*@Description:   seqDB-9107:导入date类型数据合法（支持三种格式）:SdbDate()/SdbDate(字符串:YYYY-MM-DD)/SdbDate(数字:秒数, 数字:微秒)
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9107";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9107.json";
   var srcDatas = "{date1:SdbDate('2015-06-05')}\n{date2:SdbDate(804334924130)}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"date1":{"$date":"2015-06-05"}},{"date2":{"$date":"1995-06-28"}}]';
   checkCLData( cl, expRecs );

   //import the data of {date:SdbDate()}
   var imprtFile1 = tmpFileDir + "9107a.json";
   var srcDatas1 = "{testdate:SdbDate()}"
   importData( COMMCSNAME, clName, imprtFile1, srcDatas1 );
   checkCLDateData( cl );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

function checkCLDateData ( cl )
{
   var actCnt = cl.find( { testdate: { $exists: 1 } } ).count();
   var expCnt = 1;

   if( Number( actCnt ) !== Number( expCnt ) )
   {
      throw new Error( "actCnt: " + actCnt + "\nexpCnt: " + expCnt );
   }

}
