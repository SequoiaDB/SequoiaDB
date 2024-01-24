/************************************
*@Description: seqDB-19931 IniFile类SDB_INIFILE_SINGLE_QUOMARK测试
*@author:      yinzhen
*@createDate:  2019.10.10
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19931/";
   var fileName = "file19931";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var initContent = "[" + section1 + "]\n" + key1 + "=" + value1;
   initFile( fileFullPath, initContent );

   // 指定SDB_INIFILE_SINGLE_QUOM获取IniFile对象
   // 设置item的值带有单引号 
   var iniFile = new IniFile( fileFullPath, SDB_INIFILE_SINGLE_QUOMARK );
   var newValue1 = "newValue1";
   iniFile.setValue( section1, key1, newValue1 );
   iniFile.save();

   // 设置item值成功，检查结果带有单引号
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( section1, key1 );
   var newValue1 = "'newValue1'";
   compareValue( checkValue1, newValue1 );

   var checkFile = new IniFile( fileFullPath, SDB_INIFILE_SINGLE_QUOMARK );
   var checkValue1 = checkFile.getValue( section1, key1 );
   var newValue1 = "newValue1";
   compareValue( checkValue1, newValue1 );

   deleteIniFile( filePath );
}
