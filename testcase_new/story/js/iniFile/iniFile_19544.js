/************************************
*@Description: seqDB-19544 设置item的注释，item key和section名相同
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19544/";
   var fileName = "file19544";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section = "mysql";

   var key = "mysql";
   var value = "sequoiasql-mysql";

   var itemComment = "This comment is intended to illustrate the purpose of the item";

   var content = "; test item comment\n" +
      key + "=" + value + "\n" +
      "; test section comment\n" +
      "[" + section + "]\n" +
      key + "=" + value + "\n" +
      "; test last comment";

   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );

   //设置注释
   iniFile.setComment( section, key, itemComment );
   iniFile.setComment( key, itemComment );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkComment1 = checkFile.getComment( key );
   compareValue( itemComment, checkComment1 );
   var checkComment2 = checkFile.getComment( section, key );
   compareValue( itemComment, checkComment2 );
   var checkComment3 = checkFile.getSectionComment( section );
   compareValue( "test section comment", checkComment3 );

   deleteIniFile( filePath );
}