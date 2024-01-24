/************************************
*@Description: seqDB-19548 指定section为item添加多个注释 
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19548/";
   var fileName = "file19548";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section = "auto";

   var key = "server-uuid";
   var value = "428df49d-7ad1-11e9-b432-000c292210af";

   var oldComment = "test item comment";
   var comment1 = "front annotation 1";
   var comment2 = "front annotation 2";
   var comment3 = "post annotation 1";
   var comment4 = "post annotation 2";

   var content = key + "=" + value + "\n" +
      "; test section comment\n" +
      "[" + section + "]\n" +
      "; " + oldComment + "\n" +
      key + "=" + value + " ; " + oldComment + "\n" +
      "; test last comment";

   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );

   //添加注释
   iniFile.addComment( section, key, comment1, true );
   iniFile.addComment( section, key, comment2, true );
   iniFile.addComment( section, key, comment3, false );
   iniFile.addComment( section, key, comment4, false );
   iniFile.save();

   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.addComment( "notsection", key, comment1, true );
      iniFile.save();
   } );

   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.addComment( "notsection", key, comment3, false );
      iniFile.save();
   } );

   var checkFile = new IniFile( fileFullPath );

   var checkComment1 = checkFile.getComment( section, key, true );
   compareValue( oldComment + "\n" + comment1 + "\n" + comment2, checkComment1 );
   var checkComment2 = checkFile.getComment( section, key, false );
   compareValue( oldComment + " " + comment3 + " " + comment4, checkComment2 );

   deleteIniFile( filePath );
}