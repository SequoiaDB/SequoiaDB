/******************************************************************************
 * @Description   : seqDB-26596:rest接口insert/update/upsert/delete支持返回记录数
 * @Author        : liuli
 * @CreateTime    : 2022.05.31
 * @LastEditTime  : 2022.05.31
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26596";

// 由于问题单SEQUOIADBMAINSTREAM-8004，独立模式与集群模式返回结果不一致，待问题单处理后调整用例，将 expInfo 赋值if部分删除，只保留else部分
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_26596";

   // insert
   var word = "insert";
   var curlPara = ["cmd=" + word, "name=" + csName + '.' + clName, 'insertor={"age":12}'];
   var curlInfo = runCurlAndReturn( curlPara );
   if( commIsStandalone( db ) )
   {
      var expInfo = [{ "errno": 0, "InsertedNum": 1, "DuplicatedNum": 0, "ModifiedNum": 0 }];
   }
   else
   {
      var expInfo = [{ "errno": 0 }, { "InsertedNum": 1, "DuplicatedNum": 0, "ModifiedNum": 0 }];
   }
   assert.equal( curlInfo, expInfo );

   // update
   var word = "update";
   var curlPara = ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$set:{"age":100}}&filter={"age":12}'];
   var curlInfo = runCurlAndReturn( curlPara );
   if( commIsStandalone( db ) )
   {
      var expInfo = [{ "errno": 0, "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }];
   }
   else
   {
      var expInfo = [{ "errno": 0 }, { "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }];
   }
   assert.equal( curlInfo, expInfo );

   // upsert
   var word = "upsert";
   var curlPara = ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$set:{"age":200}}&filter={"age":12}&setoninsert={"name":"rest"}'];
   var curlInfo = runCurlAndReturn( curlPara );
   if( commIsStandalone( db ) )
   {
      var expInfo = [{ "errno": 0, "UpdatedNum": 0, "ModifiedNum": 0, "InsertedNum": 1 }];
   }
   else
   {
      var expInfo = [{ "errno": 0 }, { "UpdatedNum": 0, "ModifiedNum": 0, "InsertedNum": 1 }];
   }
   assert.equal( curlInfo, expInfo );

   // deletor
   var word = "delete";
   var curlPara = ["cmd=" + word, "name=" + csName + '.' + clName, 'deletor={"name":"rest"}'];
   var curlInfo = runCurlAndReturn( curlPara );
   if( commIsStandalone( db ) )
   {
      var expInfo = [{ "errno": 0, "DeletedNum": 1 }];
   }
   else
   {
      var expInfo = [{ "errno": 0 }, { "DeletedNum": 1 }];
   }
   assert.equal( curlInfo, expInfo );
}

function runCurlAndReturn ( curlPara )
{
   if( undefined == curlPara )
   {
      throw new Error( "curlPara can not be undefined " );
   }
   if( 'string' == typeof ( COORDSVCNAME ) ) { COORDSVCNAME = parseInt( COORDSVCNAME, 10 ); }
   if( 'number' != typeof ( COORDSVCNAME ) ) { throw new Error( "typeof ( COORDSVCNAME ): " + typeof ( COORDSVCNAME ) ); }
   str = "curl http://" + COORDHOSTNAME + ":" + ( COORDSVCNAME + 4 )
      + "/ -d '";
   for( var i = 0; i < curlPara.length; i++ )
   {
      str += curlPara[i];
      if( i < ( curlPara.length - 1 ) )
      {
         str += "&";
      }
   }
   str += "' 2>/dev/null"; // to get curl command
   var cmd = new Cmd();
   info = cmd.run( str ); // to get info
   infoSplit = info.replace( /\}\{/g, "\}\n\{" ).split( "\n" );

   var infoObj = [];
   for( var i in infoSplit )
   {
      var infoSplitObj = JSON.parse( infoSplit[i] );
      infoObj.push( infoSplitObj );
   }

   return infoObj;
}
