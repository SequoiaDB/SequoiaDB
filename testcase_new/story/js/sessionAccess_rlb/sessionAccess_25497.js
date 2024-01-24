/******************************************************************************
 * @Description   : seqDB-25497 :: 版本: 1 :: 设置PerferredConstraint为PrimaryOnly，指定主节点M，该主降备 
 * @Author        : liuli
 * @CreateTime    : 2022.03.17
 * @LastEditTime  : 2022.03.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25497";
testConf.clName = COMMCLNAME + "_25497";
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var filePath = WORKDIR + "/lob25497/";
   var fileName = "filelob_25497";
   var fileSize = 1024 * 1024;
   var group = args.srcGroupName;

   var dbcl = args.testCL;
   insertBulkData( dbcl, 1000 );
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );

   // 修改会话属性，访问主节点，指定访问实例为M
   var options = { "PreferredConstraint": "PrimaryOnly", "PreferredInstance": "M" };
   db.setSessionAttr( options );

   // 查询lob
   dbcl.getLob( lobID, filePath + "checkputlob22856b", true );
   var actMD5 = File.md5( filePath + "checkputlob22856b" );
   assert.equal( fileMD5, actMD5 );

   // 重新选主
   masterChangToSlave( db, group );

   // 再次查询lob
   dbcl.getLob( lobID, filePath + "checkputlob22856b", true );
   var actMD5 = File.md5( filePath + "checkputlob22856b" );
   assert.equal( fileMD5, actMD5 );

   // 获取主节点
   var masterNodeName = getGroupMasterNodeName( db, group );
   // 查看访问计划
   explainAndCheckAccessNodes( dbcl, masterNodeName );

   deleteTmpFile( filePath );
}