/******************************************************************************
 * @Description   : seqDB-24125:SdbRecyclebin.snapshot 使用 SdbSnapshotOption 参数 
 * @Author        : liuli
 * @CreateTime    : 2022.03.01
 * @LastEditTime  : 2022.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var csName = "cs_24125";
   var clName = "cl_24125_";
   var limitNum = 10;
   var skipNum = 5;
   var fullName = csName + "." + clName + "+";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );

   var recycle = db.getRecycleBin();
   recycle.dropAll();
   try
   {
      recycle.alter( { MaxVersionNum: -1 } );
      var dbcl = dbcs.createCL( clName );

      // 多次创建CL执行truncate
      var clNum = 5;
      var truncateNum = 5;
      for( var i = 0; i < clNum; i++ )
      {
         var dbcl = dbcs.createCL( clName + i );
         for( var j = 0; j < truncateNum; j++ )
         {
            dbcl.insert( { a: j } );
            dbcl.truncate();
         }
         dbcs.dropCL( clName + i );
      }

      db.dropCS( csName );

      // 所有查询全部带sel，防止TotalDataSize等字段两次查询不一致
      // cond、sort、sel
      var option = new SdbSnapshotOption().cond( { OriginName: { $regex: fullName }, OpType: "Drop" } ).sel( { OriginID: 1 } ).sort( { OriginID: -1 } );
      var actualResults = db.snapshot( SDB_SNAP_RECYCLEBIN, { OriginName: { $regex: fullName }, OpType: "Drop" }, { OriginID: 1 }, { OriginID: -1 } );
      snapshotAndCheck( option, actualResults );

      // cond、sel、sort、skip
      var option = new SdbSnapshotOption().cond( { OriginName: { $regex: fullName } } ).sel( { RecycleID: "" } ).sort( { RecycleID: -1 } ).skip( skipNum );
      var actualResults = db.snapshot( SDB_SNAP_RECYCLEBIN, { OriginName: { $regex: fullName } }, { RecycleID: "" }, { RecycleID: -1 } );
      snapshotAndCheck( option, actualResults, skipNum );

      // cond、sel、sort、limit
      var option = new SdbSnapshotOption().cond( { OriginName: { $regex: fullName } } ).sel( { RecycleID: "" } ).sort( { OriginID: -1 } ).limit( limitNum );
      var actualResults = db.snapshot( SDB_SNAP_RECYCLEBIN, { OriginName: { $regex: fullName } }, { RecycleID: "" }, { OriginID: -1 } );
      snapshotAndCheck( option, actualResults, 0, limitNum + 1 );

      // cond、sel、sort、limit、skip
      var option = new SdbSnapshotOption().cond( { OriginName: { $regex: fullName } } ).sel( { OriginID: "" } ).sort( { RecycleID: -1 } ).limit( limitNum ).skip( skipNum );
      var actualResults = db.snapshot( SDB_SNAP_RECYCLEBIN, { OriginName: { $regex: fullName } }, { OriginID: "" }, { RecycleID: -1 } );
      snapshotAndCheck( option, actualResults, skipNum, limitNum + skipNum + 1 );

      recycle.dropAll();
   }
   finally
   {
      recycle.alter( { MaxVersionNum: 2 } );
   }
}

function snapshotAndCheck ( option, actualResults, minnum, maxnum )
{
   var expectResult = [];
   var num = 0;
   var cursor = db.getRecycleBin().snapshot( option );
   while( actualResults.next() )
   {
      num++;
      if( isRangeIn( num, minnum, maxnum ) )
      {
         var actualResultsInfo = actualResults.current().toObj();
         expectResult.push( actualResultsInfo );
      }
   }
   actualResults.close();
   commCompareResults( cursor, expectResult, false );
}

function isRangeIn ( num, minnum, maxnum )
{
   if( minnum == undefined ) { minnum = 0; }
   if( maxnum == undefined ) { maxnum = 31; }// maxnum = clNum * truncateNum + clNum + 1;
   if( num < maxnum && num > minnum )
   {
      return true;
   }
   return false;
}