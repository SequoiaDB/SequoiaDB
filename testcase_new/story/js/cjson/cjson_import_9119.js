/************************************************************************
*@Description:   seqDB-9119:一条记录中包含所有函数类型数据                                  
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9119";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //import datas          
   var imprtFile = tmpFileDir + "9119.json";
   var srcDatas = "{id:ObjectId('55713f7953e6769804000001'),time:Timestamp(1433492413, 0),date:SdbDate('2015-12-31'),reg:Regex('^W', 'i'),bin:BinData('aGVsbG8gd29ybGQ=', '1' ),key:MinKey(),key2:MaxKey(),number:NumberLong(123)}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 0;
   var importRes = 1;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check the import result 
   var expRecs = '[{"id":{"$oid":"55713f7953e6769804000001"},"time":{"$timestamp":"2015-06-05-16.20.13.000000"},"date":{"$date":"2015-12-31"},"reg":{"$regex":"^W","$options":"i"},"bin":{"$binary":"aGVsbG8gd29ybGQ=","$type":"1"},"key":{"$minKey":1},"key2":{"$maxKey":1},"number":123}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

