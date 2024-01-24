/************************************
*@Description: seqDB-19542 指定section设置item注释
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19542/";
   var fileName = "file19542";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section = "auto";

   var key = "server-uuid";
   var value = "428df49d-7ad1-11e9-b432-000c292210af";

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
   iniFile.save();

   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.setComment( "notSection", key, itemComment );
      iniFile.save();
   } );

   var checkFile = new IniFile( fileFullPath );
   var checkComment1 = checkFile.getComment( section, key );
   compareValue( itemComment, checkComment1 );

   deleteIniFile( filePath );
}