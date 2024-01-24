/******************************************************************************
 * @Description   : seqDB-26590:指定-s/--source
 *                : seqDB-26591:指定-o/--output
 * @Author        : liuli
 * @CreateTime    : 2022.05.18
 * @LastEditTime  : 2022.06.02
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );

function test ()
{
   var logFile = testCaseDir + "logFile/sdbdiag.log_26590";
   var logPath = testCaseDir + "logFile";
   var actDecryptFile = testCaseDir + "logFile/sdbdiag_26590.log.decrypt";

   // -s 指定文件解密，-o 指定输出文件
   var decryptFile = tmpFileDir + "decrypt_26590.log"
   var command = installDir + "bin/sdbsecuretool" + " -s " + logFile + " --output " + decryptFile;
   cmd.run( command );
   checkFileMD5( decryptFile, actDecryptFile );
   cmd.run( "rm -rf " + decryptFile );

   // -s指定解密文件路径，-o指定输出文件路径
   var command = installDir + "bin/sdbsecuretool" + " --source " + logPath + " --output " + tmpFileDir;
   cmd.run( command );
   var decryptFile = tmpFileDir + "sdbdiag.log_26590.decrypt";
   checkFileMD5( decryptFile, actDecryptFile );
   cmd.run( "rm -rf " + decryptFile );
   rc = cmd.run( "ls " + tmpFileDir )

   // -s指定解密文件，-o指定输出路径
   var command = installDir + "bin/sdbsecuretool" + " -s " + logFile + " -o " + tmpFileDir;
   cmd.run( command );
   var decryptFile = tmpFileDir + "sdbdiag.log_26590.decrypt";
   checkFileMD5( decryptFile, actDecryptFile );
   cmd.run( "rm -rf " + decryptFile );

   // -s指定解密文件，不指定 -o
   var decryptFile = testCaseDir + "logFile/sdbdiag.log_26590.decrypt";
   cmd.run( "rm -rf " + decryptFile );
   var command = installDir + "bin/sdbsecuretool" + " --source " + logFile;
   cmd.run( command );
   checkFileMD5( decryptFile, actDecryptFile );
   cmd.run( "rm -rf " + decryptFile );

   // -s指定解密文件路径，不指定 -o
   var decryptFile = testCaseDir + "logFile/sdbdiag.log_26590.decrypt";
   cmd.run( "rm -rf " + decryptFile );
   var command = installDir + "bin/sdbsecuretool" + " -s " + logPath;
   cmd.run( command );
   checkFileMD5( decryptFile, actDecryptFile );

   // 指定解密文件或路径不存在
   var command = installDir + "bin/sdbsecuretool" + " --source " + tmpFileDir + "none_26590";
   assert.tryThrow( FILE_NOT_EXIST, function()
   {
      cmd.run( command );
   } );

   // 指定 -d同时指定 -s
   var ciphertext = '"(hfIcY+zrCqbohfIcQO2uMJC1CJC+Sqo/FXC+BNSaMATsTsTzT+C+SNjfSqocCP/wCJQpCqboSWaoCsCcBcImCP/=)"';
   var command = installDir + "bin/sdbsecuretool" + " -d " + ciphertext + " -s " + logPath;
   assert.tryThrow( COMMAND_NOT_FOUND, function()
   {
      cmd.run( command );
   } );

   // 指定 -d同时指定 -o
   var command = installDir + "bin/sdbsecuretool" + " -d " + ciphertext + " -o " + testCaseDir;
   assert.tryThrow( COMMAND_NOT_FOUND, function()
   {
      cmd.run( command );
   } );

   // 指定 -d同时指定 -o / -s
   var command = installDir + "bin/sdbsecuretool" + " -d " + ciphertext + " --source " + logPath + " -o " + testCaseDir;
   assert.tryThrow( COMMAND_NOT_FOUND, function()
   {
      cmd.run( command );
   } );

   // -o指定文件，文件已存在
   var command = installDir + "bin/sdbsecuretool" + " -s " + logFile + " --output " + actDecryptFile;
   assert.tryThrow( FILE_ALREADY_EXIST, function()
   {
      cmd.run( command );
   } );

   // -o指定路径，路径下部分文件已经存在
   var command = installDir + "bin/sdbsecuretool" + " -s " + logFile + " -o " + decryptFile;
   assert.tryThrow( FILE_ALREADY_EXIST, function()
   {
      cmd.run( command );
   } );

   cmd.run( "rm -rf " + decryptFile );
}

function checkFileMD5 ( actFile, expFile )
{
   actMD5 = File.md5( actFile );
   expMD5 = File.md5( expFile );
   assert.equal( actMD5, expMD5 );
}
