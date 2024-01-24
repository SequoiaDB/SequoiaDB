/************************************
*@Description: seqDB-19928 IniFile类SDB_INIFILE_HASHMARK测试
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19928/";
   var fileName = "file19928";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section = "auto";

   var key = "server-uuid";
   var value = "428df49d-7ad1-11e9-b432-000c292210af";

   var sectionComment = "This comment is intended to illustrate the purpose of the section";
   var itemComment = "This comment is intended to illustrate the purpose of the item";
   var lastComment = "This comment is intended to illustrate the purpose in the end";

   var content = key + "=" + value + "\n" +
      "[" + section + "]\n" +
      key + "=" + value + "\n";

   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath, SDB_INIFILE_HASHMARK );

   //设置注释
   iniFile.setSectionComment( section, sectionComment );
   iniFile.setComment( key, itemComment );
   iniFile.setLastComment( lastComment );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath, SDB_INIFILE_HASHMARK );
   var checkComment1 = checkFile.getSectionComment( section );
   compareValue( sectionComment, checkComment1 );

   var checkComment2 = checkFile.getComment( key );
   compareValue( itemComment, checkComment2 );

   var checkComment3 = checkFile.getLastComment();
   compareValue( lastComment, checkComment3 );

   var expContent = "# " + itemComment + "\n" +
      key + "=" + value + "\n" +
      "# " + sectionComment + "\n" +
      "[" + section + "]\n" +
      key + "=" + value + "\n" +
      "# " + lastComment;
   var checkContent = checkFile.toString();
   compareValue( expContent, checkContent );

   deleteIniFile( filePath );
}