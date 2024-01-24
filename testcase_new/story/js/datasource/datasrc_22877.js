/******************************************************************************
 * @Description   : seqDB-22877:源集群创建cs和数据源上cs属性不同 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22877";
   var csName = "cs_22877";
   var clName = "cl_22877";
   var srcCSName = "datasrcCS_22877";
   var filePath = WORKDIR + "/lob22877/";
   var fileName = "filelob_22877";
   var fileSize = 1024 * 1024;

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.createCS( srcCSName, { LobPageSize: 131072 } );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = db.createCS( csName, { LobPageSize: 65536 } );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );
   dbcl.getLob( lobID, filePath + "putlob22877", true );
   var actMD5 = File.md5( filePath + "putlob22877" );
   assert.equal( fileMD5, actMD5 );

   var expectRes = datasrcDB.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: srcCSName } ).current().toObj().LobPageSize;
   assert.equal( expectRes, 131072 );
   deleteTmpFile( filePath );
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}