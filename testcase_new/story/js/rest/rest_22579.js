/************************************
*@Description: insert接口支持传入flag和options参数
*@author:      zhaohailin
**************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_22579";
   commDropCL( db, COMMCSNAME, clName );
   var restcl = commCreateCL( db, COMMCSNAME, clName, {}, true, false, "create cl in begin" );

   restcl.createIndex( "nameIndex", { name: 1 }, { Unique: true, NotNull: true } );

   var record = [{ 'age': 1, 'name': 'user1' }, { 'age': 2, 'name': 'user2' }, { 'age': 3, 'name': 'user3' }, { 'age': 4, 'name': 'user4' }];
   restcl.insert( record );

   //REST接口flag参数兼容SDB_INSERT_CONTONDUP,当发生索引键冲突时，跳过该记录并插入其他记录
   checkFlagType( restcl, COMMCSNAME, clName, 11, "user1", "SDB_INSERT_CONTONDUP", [{ 'age': 1, 'name': 'user1' }], 0 );
   checkFlagType( restcl, COMMCSNAME, clName, 11, "user11", "SDB_INSERT_CONTONDUP", [{ 'age': 11, 'name': 'user11' }], 0 );

   //REST接口flag参数兼容SDB_INSERT_REPLACEONDUP,当发生索引冲突时，将已存在的记录更新为待插入的新记录(替换)，并继续插入其他记录
   checkFlagType( restcl, COMMCSNAME, clName, 22, "user2", "SDB_INSERT_REPLACEONDUP", [{ 'age': 22, 'name': 'user2' }], 0 );
   checkFlagType( restcl, COMMCSNAME, clName, 22, "user22", "SDB_INSERT_REPLACEONDUP", [{ 'age': 22, 'name': 'user22' }], 0 );

   //REST接口flag参数不兼容SDB_INSERT_RETURN_ID,报错显示为-6：输入非法参数
   checkFlagType( restcl, COMMCSNAME, clName, 33, "user3", "SDB_INSERT_RETURN_ID", [], -6 );

   //REST接口flag参数兼容SDB_INSERT_RETURNNUM带回单条插入成功的返回值,插入数据出现唯一索引冲突时：报错-38
   checkFlagType( restcl, COMMCSNAME, clName, 44, "user4", "SDB_INSERT_RETURNNUM", [{ 'age': 44, 'name': 'user4' }], -38 );
   checkFlagType( restcl, COMMCSNAME, clName, 44, "user44", "SDB_INSERT_RETURNNUM", [{ 'age': 44, 'name': 'user44' }], 0 );

   restcl.truncate()
   restcl.insert( { '_id': 1, 'age': 1, 'name': 'user1' }, { '_id': 2, 'age': 2, 'name': 'user2' } );
   //REST接口flag参数兼容SDB_INSERT_CONTONDUP_ID,当发生_id键冲突时，跳过该记录并插入其他记录
   checkFlagType( restcl, COMMCSNAME, clName, 11, "user11", "SDB_INSERT_CONTONDUP_ID", [{ '_id': 1, 'age': 1, 'name': 'user1' }], 0, 1 );
   checkFlagType( restcl, COMMCSNAME, clName, 11, "user11", "SDB_INSERT_CONTONDUP_ID", [{ '_id': 11, 'age': 11, 'name': 'user11' }], 0, 11 );

   //REST接口flag参数兼容SDB_INSERT_REPLACEONDUP_ID,当发生_id冲突时，将已存在的记录更新为待插入的新记录(替换)，并继续插入其他记录
   checkFlagType( restcl, COMMCSNAME, clName, 22, "user22", "SDB_INSERT_REPLACEONDUP_ID", [{ '_id': 2, 'age': 22, 'name': 'user22' }], 0, 2 );
   checkFlagType( restcl, COMMCSNAME, clName, 33, "user33", "SDB_INSERT_REPLACEONDUP_ID", [{ '_id': 3, 'age': 33, 'name': 'user33' }], 0, 3 );

   commDropCL( db, COMMCSNAME, clName );
}

function sendRequest ( cmd, content )
{
   var port = parseInt( COORDSVCNAME, 10 ) + 4;
   var url = "http://" + COORDHOSTNAME + ":" + port;
   var curl = "curl " + "-X  POST \"" + url + "\" -d \"" + content + "\" -H \"Accept: application/json\" 2>/dev/null";
   var rc = cmd.run( curl );
   var rcObj = JSON.parse( rc );
   return rcObj;
}

function checkFlagType ( cl, COMMCSNAME, clName, userage, username, flagType, expData, expError, id )
{
   if( expError === undefined ) { expError = null; }
   if( expData === undefined ) { expData = []; }
   var cmd = new Cmd();
   var data = "{'name':'" + username + "','age': " + userage + "}";
   if( id != undefined )
   {
      data = "{'_id': " + id + ",'name': '" + username + "','age': " + userage + "}";
   }
   var content = "cmd=insert&name=" + COMMCSNAME + "." + clName + "&insertor=" + data + "&flag=" + flagType;
   var rc = sendRequest( cmd, content );
   var errno = rc[0]["errno"];
   if( expError != errno )
   {
      throw new Error( " check flag error! actual errno is " + errno );
   }
   else
   {
      switch( errno )
      {
         case -6:
            if( flagType != "SDB_INSERT_RETURN_ID" )
            {
               throw new Error( "check SDB_INSERT_RETURN_ID errno !" );
            }
            break;
         case -38:
            if( flagType != "SDB_INSERT_RETURNNUM" || username != "user4" )
            {
               throw new Error( "check SDB_INSERT_RETURNNUM errno !" );
            }
            break;
         case 0:
            if( id == undefined )
            {
               var cur = cl.find( { 'name': username } );
               commCompareResults( cur, expData );
            }
            if( id != undefined )
            {
               cur = cl.find( { '_id': id } );
               commCompareResults( cur, expData, false );
            }
            //校验兼容SDB_INSERT_RETURNNUM带回单条插入成功的返回值{ "InsertedNum": 1, "DuplicatedNum": 0 }
            if( flagType == "SDB_INSERT_RETURNNUM" )
            {
               for( let i = 0; i < rc.length; i++ )
               {
                  if( rc[i]["InsertedNum"] !== undefined && rc[i]["InsertedNum"] != null )
                  {
                     assert.equal( rc[i]["InsertedNum"], 1 );
                     assert.equal( rc[i]["DuplicatedNum"], 0 );
                  }
               }
            }
            break;
      }
   }
}