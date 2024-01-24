/************************************************************************
*@Description:   seqDB-9090: 导入json文件数据（包括所有数据类型）
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9090";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9090.json";
   var srcDatas = "{ true : true,null:null,int :12345,int64:21474836470,double:123456789.123,string:'test'}\n{int:12345,numberlong:{'$numberLong':'300000000000'},array:[1,true,123.11,'abc def'],decimal:{$decimal:'123.456'},oid:{'$oid':'123abcd00ef12358902300ef'},date:{'$date':'2012-01-01'},time:{'$timestamp':'2012-01-01-13.14.26.124233'},binary:{'$binary':'aGVsbG8gd29ybGQ=','$type':'1'},regex:{'$regex':'^张','$options':'i'},obj:{'a':1}}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"true":true,"null":null,"int":12345,"int64":21474836470,"double":123456789.123,"string":"test"},{"int":12345,"numberlong":300000000000,"array":[1,true,123.11,"abc def"],"decimal":{"$decimal":"123.456"},"oid":{"$oid":"123abcd00ef12358902300ef"},"date":{"$date":"2012-01-01"},"time":{"$timestamp":"2012-01-01-13.14.26.124233"},"binary":{"$binary":"aGVsbG8gd29ybGQ=","$type":"1"},"regex":{"$regex":"^张","$options":"i"},"obj":{"a":1}}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName )
   removeTmpDir();
}
