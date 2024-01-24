/******************************************************************************
 * @Description   : seqDB-25513:设置PerferredConstraint为SecondaryOnly，指定主节点M，该主降备 
 * @Author        : liuli
 * @CreateTime    : 2022.03.16
 * @LastEditTime  : 2022.03.16
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25513";
testConf.clName = COMMCLNAME + "_25513";
testConf.useSrcGroup = true;

// SEQUOIADBMAINSTREAM-8179
// main( test );
function test ( args )
{
   var filePath = WORKDIR + "/lob25513/";
   var fileName = "filelob_25513";
   var fileSize = 1024 * 1024;
   var group = args.srcGroupName;

   var dbcl = args.testCL;
   insertBulkData( dbcl, 1 );
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );

   // 修改会话属性，访问备节点，指定访问实例为M
   var options = { "PreferredConstraint": "SecondaryOnly", "PreferredInstance": "M" };
   db.setSessionAttr( options );

   // 查询lob
   assert.tryThrow( SDB_CLS_NOT_SECONDARY, function()
   {
      dbcl.getLob( lobID, filePath + "checkputlob25513", true );
   } );

   // 重新选主
   masterChangToSlave( db, group );

   // 再次查询lob
   assert.tryThrow( SDB_CLS_NOT_SECONDARY, function()
   {
      dbcl.getLob( lobID, filePath + "checkputlob25513", true );
   } );

   deleteTmpFile( filePath );
}
