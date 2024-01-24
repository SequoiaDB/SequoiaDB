/******************************************************************************
 * @Description   : seqDB-27839 创建/删除CS后检查快照
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.09.25
 * @LastEditTime  : 2023.05.06
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName1 = "cs_27839_1";
   var csName2 = "cs_27839_2";
   var csName3 = "cs_27839_3";
   var clName1 = "cl_27839_1";
   var clName2 = "cl_27839_2";
   var clName3 = "cl_27839_3";
   var filePath = WORKDIR + "/lob27839/";
   var fileName = "filelob_27839";
   var fileSize = 1024 * 2;
   var lobPageSize = 262144;

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   commDropCS( db, csName3 );
   var dbcl1 = commCreateCL( db, csName1, clName1, { ReplSize: 0 } );
   var dbcl2 = commCreateCL( db, csName2, clName2, { ReplSize: 0 } );
   var dbcl3 = commCreateCL( db, csName3, clName3, { ReplSize: 0 } );

   // putLob
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   dbcl1.putLob( filePath + fileName );
   dbcl2.putLob( filePath + fileName );
   dbcl3.putLob( filePath + fileName );

   // 获取集合空间快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName1 } );
   var csInfo1 = getSnapshotLobStat( cursor );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName2 } );
   var csInfo2 = getSnapshotLobStat( cursor );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName3 } );

   // 获取集合空间快照信息，非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: csName1, RawData: true } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   var csInfoRawData1 = getSnapshotLobStat( cursor );
   var option = new SdbSnapshotOption().cond( { Name: csName2, RawData: true } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   var csInfoRawData2 = getSnapshotLobStat( cursor );
   var option = new SdbSnapshotOption().cond( { Name: csName3, RawData: true } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, option );

   // 删除一个cs
   db.dropCS( csName3 );

   // 等待LSN同步后进行校验
   commCheckBusinessStatus( db );

   // 集合空间聚合结果检验
   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;
   var cursor1 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName1 } );
   checkSnapshot( cursor1, csInfo1, lobPages );
   var cursor2 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName2 } );
   checkSnapshot( cursor2, csInfo2, lobPages );
   var cursor3 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName3 } );
   while( cursor3.next() )
   {
      throw new Error( "游标中有返回结果！cursor3值为:" + JSON.stringify( cursor3.current().toObj() ) );
   }
   cursor3.close();
   // 集合空间非聚合结果检验
   var option = new SdbSnapshotOption().cond( { Name: csName1, RawData: true } ).sort( { NodeName: 1 } );
   var cursor4 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor4, csInfoRawData1, lobPages );

   var option = new SdbSnapshotOption().cond( { Name: csName2, RawData: true } ).sort( { NodeName: 1 } );
   var cursor5 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor5, csInfoRawData2, lobPages );

   var option = new SdbSnapshotOption().cond( { Name: csName3, RawData: true } ).sort( { NodeName: 1 } );
   var cursor6 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   while( cursor6.next() )
   {
      throw new Error( "游标中有返回结果！cursor6值为:" + JSON.stringify( cursor6.current().toObj() ) );
   }
   cursor6.close();

   db.dropCS( csName1 );
   db.dropCS( csName2 );
   deleteTmpFile( filePath + fileName )
}