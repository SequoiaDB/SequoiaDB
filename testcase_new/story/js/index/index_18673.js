/******************************************************************************
*@Description : seqDB-18673:节点间index一致性检查（index、LSN） 
*@Author      : 2019-07-13  XiaoNi Huang init
******************************************************************************/


main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = "cs18673";
   var clName = "cl";

   commDropCS( db, csName, true, "Failed to drop cs in the pre-condition." );
   var cs = db.createCS( csName );
   var cl = cs.createCL( clName, { ReplSize: 0 } );

   cl.alter( { ShardingType: "hash", ShardingKey: { a: 1 } } );
   cl.insert( { a: 1 } );

   cl.truncate();

   db.sync( { CollectionSpace: csName } );

   var fullCLName = csName + "." + clName;
   var groupName = commGetCLGroups( db, fullCLName )[0];
   var rg = db.getRG( groupName );
   var sDB;
   var mDB;
   sDB = rg.getSlave().connect();
   mDB = rg.getMaster().connect();

   checkIndex( sDB, mDB, csName, clName );
   checkIndexLSN( sDB, mDB, fullCLName );

   commDropCS( db, csName, false, "Failed to drop cs in the end-condition." );
}

function checkIndex ( sDB, mDB, csName, clName )
{
   var sCL = sDB.getCS( csName ).getCL( clName );
   var mCL = mDB.getCS( csName ).getCL( clName );
   var sCursor = sCL.listIndexes();
   var mCursor = mCL.listIndexes();
   var sIdxNum = 0;
   var mIdxNum = 0;
   while( sCursor.next() )
   {
      sIdxNum++;
   }

   while( mCursor.next() )
   {
      mIdxNum++;
   }

   var expIdxNum = 2; //index: $id, $shard
   if( expIdxNum !== mIdxNum || sIdxNum !== mIdxNum )
   {
      throw new Error( "main fail,[check index]" +
         "[expIdxNum = " + expIdxNum + "]" +
         "[mIdxNum = " + mIdxNum + ", sIdxNum = " + sIdxNum + "]" );
   }

}

function checkIndexLSN ( sDB, mDB, fullCLName )
{
   var sLSN = sDB.snapshot( SDB_SNAP_COLLECTIONS, { Name: fullCLName } ).current().toObj().Details[0].IndexCommitLSN;
   var mLSN = mDB.snapshot( SDB_SNAP_COLLECTIONS, { Name: fullCLName } ).current().toObj().Details[0].IndexCommitLSN;
   assert.equal( sLSN, mLSN );
}
