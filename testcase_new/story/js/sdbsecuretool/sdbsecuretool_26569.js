/******************************************************************************
 * @Description   : seqDB-26569:执行-d/--decrypt 
 *                : seqDB-26572:sdbsecuretool工具参数校验
 * @Author        : liuli
 * @CreateTime    : 2022.05.18
 * @LastEditTime  : 2022.06.02
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );

function test ()
{
   // 正常字符解密
   var ciphertext = '"SDBSECURE0000(hfIcY+zrCqbohfIcQO2uMJC1CJC+Sqo/FXC+BNSaMATsTsTzT+C+SNjfSqocCP/wCJQpCqboSWaoCsCcBcImCP/=)"';
   var planitext = '{ "_id": { "$oid": "6284a26830ecf6ce7b607228" }, "a": 1, "b": 3 }\n';

   var command = installDir + "bin/sdbsecuretool" + " --decrypt " + ciphertext;
   var rc = cmd.run( command );
   assert.equal( rc, planitext );

   // 解密数据不包含开头部分
   var ciphertext = '"(hfIcY+zrCqbohfIcQO2uMJC1CJC+Sqo/FXC+BNSaMATsTsTzT+C+SNjfSqocCP/wCJQpCqboSWaoCsCcBcImCP/=)"';
   var command = installDir + "bin/sdbsecuretool" + " -d " + ciphertext;
   assert.tryThrow( COMMAND_NOT_FOUND, function()
   {
      cmd.run( command );
   } );

   // 解密数据开头部分包含多余内容
   var ciphertext = '"SDBSECURE000011(hfIcY+zrCqbohfIcQO2uMJC1CJC+Sqo/FXC+BNSaMATsTsTzT+C+SNjfSqocCP/wCJQpCqboSWaoCsCcBcImCP/=)"';
   var command = installDir + "bin/sdbsecuretool" + " --decrypt " + ciphertext;
   assert.tryThrow( COMMAND_NOT_FOUND, function()
   {
      cmd.run( command );
   } );

   // 解密数据中间数据不正确
   var ciphertext = '"SDBSECURE0000(hfIcY+zrcY+zrCqbohfIcQCqbohfIcQO2uMJC1CJC+Sqo/FXC+BNSJC+Sqo/FXC+BNSaMATsTsTzT+C+SNjfSqocCP/wCJQpCqboSWaoCsCcBcImCP/=)"';

   var command = installDir + "bin/sdbsecuretool" + " --decrypt " + ciphertext;
   var rc = cmd.run( command );
   assert.notEqual( rc, planitext );

   // 超长字符解密
   var ciphertext = '"SDBSECURE0000(hfIcY+zrCqbohfIcQO2uMJC1CJC+Sqo/FXC9SmSaMATsTsTzT+C+SNjfSqrcCP/wCJQpCqboCsGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFAGpFW9eZcK2)"';
   var planitext = '{ "_id": { "$oid": "6284a28330ecf6ce7b607229" }, "a": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa... }\n';

   var command = installDir + "bin/sdbsecuretool" + " -d " + ciphertext;
   var rc = cmd.run( command );
   assert.equal( rc, planitext );

   // 指定工具参数有误
   var command = installDir + "bin/sdbsecuretool" + " --encryption " + ciphertext;
   assert.tryThrow( COMMAND_NOT_FOUND, function()
   {
      cmd.run( command );
   } );
}