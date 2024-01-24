/******************************************************************************
 * @Description   :seqDB-31012:删除集合空间部分集合，集合空间快照字段验证
 *                :seqDB-31017:回收站恢复部分集合，集合空间快照验证
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.03
 * @LastEditTime  : 2023.04.11
 * @LastEditors   : Bi Qin
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_31012_31017";
   var clName1 = "cl_31012_31017_1";
   var clName2 = "cl_31012_31017_2";
   var otherClName = "cl_31012_31017_2_2";
   var filePath = WORKDIR + "/lob31012_31017/";
   deleteTmpFile( filePath );
   var fileName = "file31012_31017";
   var fileFullPath = filePath + fileName;

   makeTmpFile( filePath, fileName );
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dataGroup = commGetDataGroupNames( db );

   var dbcs = commCreateCS( db, csName );
   var dbcl1 = dbcs.createCL( clName1, { Group: dataGroup[0], ReplSize: -1 } );
   var dbcl2 = dbcs.createCL( clName2, { Group: dataGroup[0], ReplSize: -1 } );

   dbcl1.insert( { a: 1 } );
   dbcl1.putLob( fileFullPath );
   dbcl2.insert( { a: 1 } );
   dbcl2.putLob( fileFullPath );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   checkRecycleBinSnapEqualCsSnap( cursor );
   var freeSize = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize;

   dbcl1.truncate();

   var recycleName = getOneRecycleName( db, csName + "." + clName1 );
   assert.notEqual( db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize, freeSize );

   cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   var recycleCursor = db.getRecycleBin().snapshot( { OriginName: csName + "." + clName1 } );
   checkRecycleBinSnapEqualCsSnap( cursor, recycleCursor );

   db.getRecycleBin().returnItem( recycleName + "" );
   //等待节点同步
   commCheckLSN( db, dataGroup );
   assert.equal( db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize, freeSize );
   cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   checkRecycleBinSnapEqualCsSnap( cursor );

   dbcs.dropCL( clName2 );

   assert.notEqual( db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize, freeSize );
   cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   recycleCursor = db.getRecycleBin().snapshot( { OriginName: csName + "." + clName2 } );
   checkRecycleBinSnapEqualCsSnap( cursor, recycleCursor );

   recycleName = getOneRecycleName( db, csName + "." + clName2 );
   db.getRecycleBin().returnItemToName( recycleName + "", csName + "." + otherClName );
   commCheckLSN( db, dataGroup );

   assert.equal( db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize, freeSize );
   cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   checkRecycleBinSnapEqualCsSnap( cursor );
   //31017 恢复回收站部分集合，集合空间快照验证
   dbcs.dropCL( otherClName );
   dbcl1.truncate();

   recycleName = getOneRecycleName( db, csName + "." + clName1 );
   assert.notEqual( db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj().FreeSize, freeSize );

   db.getRecycleBin().returnItem( recycleName + "" );
   //等待节点同步
   commCheckLSN( db, dataGroup );

   cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   recycleCursor = db.getRecycleBin().snapshot( { OriginName: csName + "." + otherClName } );
   checkRecycleBinSnapEqualCsSnap( cursor, recycleCursor );

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