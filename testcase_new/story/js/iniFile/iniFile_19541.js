/************************************
*@Description: seqDB-19541 指定item设置注释 
*@author:      yinzhen
*@createDate:  2019.10.10
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19541/";
   var fileName = "file19541";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var fileContent = "[" + section1 + "]\n" + key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 指定item设置注释
   // 设置前置注释，设置后置注释 
   var iniFile = new IniFile( fileFullPath );
   var comment1 = "comment1";
   var comment2 = "comment2";
   iniFile.setComment( section1, key1, comment1 );
   iniFile.setComment( section1, key1, comment2, false );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkComment1 = checkFile.getComment( section1, key1 );
   compareValue( comment1, checkComment1 );
   var checkComment2 = checkFile.getComment( section1, key1, false );
   compareValue( comment2, checkComment2 );
   deleteIniFile( fileFullPath );

   // 已有多个注释，设置前置注释和后置注释
   makeIniFile( filePath, fileName );
   var fileContent = "[" + section1 + "]\n" +
      "; new comment1\n" +
      "; new comment2\n" +
      "; new comment3\n" +
      key1 + "=" + value1 + " ; new comment1 new comment2 new comment3";
   initFile( fileFullPath, fileContent );
   var iniFile = new IniFile( fileFullPath );
   iniFile.setComment( section1, key1, comment1 );
   iniFile.setComment( section1, key1, comment2, false );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkComment1 = checkFile.getComment( section1, key1 );
   compareValue( comment1, checkComment1 );
   var checkComment2 = checkFile.getComment( section1, key1, false );
   compareValue( comment2, checkComment2 );

   deleteIniFile( filePath );
}
