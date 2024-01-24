/******************************************************************************
 * @Description   : seqDB-23825:分区表且被切分到多个数据组，读写数据并snapshot查看集合统计信息，truncate后恢复 
 * @Author        : liuli
 * @CreateTime    : 2022.03.04
 * @LastEditTime  : 2022.10.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23825";
   var clName = "cl_23825";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true, ReplSize: 0 } );

   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   commCheckLSN( db );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName },
      { "Details.Group.UpdateTime": { "$include": 0 } }, { "Details.Group.NodeName": 1 } );
   var expSnapshot = cursor.current().toObj();

   dbcl.truncate();

   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );

   commCheckLSN( db );
   var cursorNew = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName },
      { "Details.Group.UpdateTime": { "$include": 0 } }, { "Details.Group.NodeName": 1 } );
   var actSnapshot = cursorNew.current().toObj();
   assert.equal( expSnapshot, actSnapshot );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}