/************************************
*@Description: seqDB-19939 IniFile类flags全部填写测试
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19939/";
   var fileName = "file19939";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 指定所有flags获取IniFile对象


   var iniFile = new IniFile( fileFullPath, SDB_INIFILE_NOTCASE |
      SDB_INIFILE_SEMICOLON |
      SDB_INIFILE_HASHMARK |
      SDB_INIFILE_ESCAPE |
      SDB_INIFILE_DOUBLE_QUOMARK |
      SDB_INIFILE_SINGLE_QUOMARK |
      SDB_INIFILE_EQUALSIGN |
      SDB_INIFILE_COLON |
      SDB_INIFILE_UNICODE |
      SDB_INIFILE_STRICTMODE );

   var newValue1 = "newValue1";
   var comment1 = "comment1"
   iniFile.setValue( section1, key1, newValue1 );
   iniFile.setComment( section1, key1, comment1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath, SDB_INIFILE_NOTCASE |
      SDB_INIFILE_SEMICOLON |
      SDB_INIFILE_HASHMARK |
      SDB_INIFILE_ESCAPE |
      SDB_INIFILE_DOUBLE_QUOMARK |
      SDB_INIFILE_SINGLE_QUOMARK |
      SDB_INIFILE_EQUALSIGN |
      SDB_INIFILE_COLON |
      SDB_INIFILE_UNICODE |
      SDB_INIFILE_STRICTMODE );
   var checkValue1 = checkFile.getValue( section1, key1 );
   compareValue( newValue1, checkValue1 );
   var checkValue1 = checkFile.getComment( section1, key1 );
   compareValue( comment1, checkValue1 );

   deleteIniFile( filePath );
}
