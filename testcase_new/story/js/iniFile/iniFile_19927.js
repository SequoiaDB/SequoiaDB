/************************************
*@Description: seqDB-19927 IniFile类SDB_INIFILE_SEMICOLON测试
*@author:      yinzhen
*@createDate:  2019.10.09
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19927/";
   var fileName = "file19927";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";

   // 指定SDB_INIFILE_SEMICOLON获取IniFile对象
   var iniFile = new IniFile( fileFullPath, SDB_INIFILE_SEMICOLON );
   iniFile.setValue( "auto", key1, value1 );
   iniFile.save();

   // 设置item/section/结尾注释
   // 获取item/section/结尾注释
   var itemComment1 = "this is comment1";
   var itemComment2 = "this is comment2";
   iniFile.setComment( "auto", key1, itemComment1 );
   iniFile.setComment( "auto", key1, itemComment2, false );
   iniFile.save();
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getComment( "auto", key1 );
   compareValue( checkValue1, itemComment1 );
   var checkValue2 = checkFile.getComment( "auto", key1, false );
   compareValue( checkValue2, itemComment2 );

   var sectionComment = "this is section comment";
   iniFile.setSectionComment( "auto", sectionComment );
   iniFile.save();
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getSectionComment( "auto" );
   compareValue( checkValue1, sectionComment );

   var lastComment = "this is last comment";
   iniFile.setLastComment( lastComment );
   iniFile.save();
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getLastComment();
   compareValue( checkValue1, lastComment );

   // 检查注释符为分号
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.toString();
   var fileContent = "; " + sectionComment + "\n" +
      "[auto]\n" +
      "; " + itemComment1 + "\n" +
      key1 + "=" + value1 + " ; " + itemComment2 + "\n" +
      "; " + lastComment;
   compareValue( fileContent, checkValue1 );

   deleteIniFile( filePath );
}
