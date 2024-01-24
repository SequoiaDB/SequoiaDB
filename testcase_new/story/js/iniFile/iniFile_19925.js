/************************************
*@Description: seqDB-19925 已有结尾注释，新增item
*@author:      yinzhen
*@createDate:  2019.10.09
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19925/";
   var fileName = "file19925";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";

   var iniFile = new IniFile( fileFullPath );
   var lastComment = "This is last comment...";
   iniFile.setLastComment( lastComment );

   // 指定新的section和item设置item值
   iniFile.setValue( "auto", key1, value1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( "auto", key1 );
   compareValue( checkValue1, value1 );

   // 新增section和item在结尾注释之上
   var checkValue2 = checkFile.toString();
   var value2 = "[auto]\n" + key1 + "=" + value1 + "\n; " + lastComment;
   compareValue( checkValue2, value2 );

   deleteIniFile( filePath );
}
