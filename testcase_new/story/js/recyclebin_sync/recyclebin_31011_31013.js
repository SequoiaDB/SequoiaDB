/******************************************************************************
 * @Description   : seqDB-31011:删除集合空间所有集合，集合空间快照字段验证
 *                : seqDB-31013:恢复回收站，集合空间快照验证
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.03
 * @LastEditTime  : 2023.04.11
 * @LastEditors   : Bi Qin
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_31011_31013";
   var clName = "cl_31011_31013";
   var otherClName = "cl_31011_31013_1";
   var filePath = WORKDIR + "/lob31011_31013/";
   deleteTmpFile( filePath );
   var fileName = "file31011_31013";
   var fileFullPath = filePath + fileName;

   makeTmpFile( filePath, fileName );
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dataGroup = commGetDataGroupNames( db );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { Group: dataGroup[0], ReplSize: -1 } );
   //插入记录和Lob
   dbcl.insert( { a: 1 } );
   dbcl.putLob( fileFullPath );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   checkRecycleBinSnapEqualCsSnap( cursor );
   var freeSize = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize;

   dbcl.truncate();

   var recycleName = getOneRecycleName( db, csName + "." + clName );

   assert.notEqual( db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize, freeSize );
   cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   var recycleCursor = db.getRecycleBin().snapshot( { OriginName: csName + "." + clName } );
   checkRecycleBinSnapEqualCsSnap( cursor, recycleCursor );
   // seqDB-31013:恢复回收站，集合空间快照验证
   db.getRecycleBin().returnItem( recycleName + "" );
   //等待节点同步
   commCheckLSN( db, dataGroup );

   assert.equal( db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize, freeSize );
   cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   checkRecycleBinSnapEqualCsSnap( cursor );

   dbcs.dropCL( clName );

   assert.notEqual( db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize, freeSize );
   cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   recycleCursor = db.getRecycleBin().snapshot( { OriginName: csName + "." + clName } );
   checkRecycleBinSnapEqualCsSnap( cursor, recycleCursor );

   recycleName = getOneRecycleName( db, csName + "." + clName );

   db.getRecycleBin().returnItemToName( recycleName + "", csName + "." + otherClName );
   commCheckLSN( db, dataGroup );

   assert.equal( db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize, freeSize );
   cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   checkRecycleBinSnapEqualCsSnap( cursor );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   deleteTmpFile( filePath );
}

function checkRecycleBinSnapEqualCsSnap ( csCursor, recycleBinCursor )
{
   var csObj = csCursor.current().toObj();
   if( recycleBinCursor == undefined )
   {
      assert.equal( csObj.RecycleDataSize, 0, "check RecycleDataSize error!\nCsSnapshot=" + JSON.stringify( csObj ) );
      assert.equal( csObj.RecycleIndexSize, 0, "check RecycleIndexSize error!\nCsSnapshot=" + JSON.stringify( csObj ) );
      assert.equal( csObj.RecycleLobSize, 0, "check RecycleLobSize error!\nCsSnapshot=" + JSON.stringify( csObj ) );
      csCursor.close();
   } else
   {
      var obj = recycleBinCursor.current().toObj();
      var errorInfo = "check snapshot error!\nRecycleBinSnapshot=" + JSON.stringify( obj ) + "\nCsSnapshot=" + JSON.stringify( csObj );
      assert.equal( csObj.RecycleDataSize, obj.TotalDataSize, errorInfo );
      assert.equal( csObj.RecycleIndexSize, obj.TotalIndexSize, errorInfo );
      assert.equal( csObj.RecycleLobSize, obj.TotalLobSize, errorInfo );

      csCursor.close();
      recycleBinCursor.close();
   }
}

