/***************************************************************************
@Description : seqDB-15726:
modify list : 2019-11-14  Chen siqin  Create
****************************************************************************/
main( test );

function test ()
{
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   //cond指定一个条件
   var count = 0;
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().cond( { role: "catalog" } ) );
   while( cursor.next() )
   {
      var role = cursor.current().toObj().role;
      if( role !== "catalog" )
      {
         throw new Error( "Expect role is catalog, but act role is " + ret.toObj().role );
      }
      count++;
   }
   if( count <= 0 )
   {
      throw new Error( "count: " + count );
   }

   //cond指定多个条件
   count = 0;
   cursor = db.snapshot( SDB_SNAP_HEALTH, new SdbSnapshotOption().cond( { $and: [{ IsPrimary: true }, { ServiceStatus: true }] } ) );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      if( !obj.IsPrimary )
      {
         throw new Error( "IsPrimary is false!" );
      }
      if( !obj.ServiceStatus )
      {
         throw new Error( "ServiceStatus is false!" );
      }
      count++;
   }
   if( count <= 0 )
   {
      throw new Error( "count: " + count );
   }

   //cond条件匹配不到记录
   cursor = db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().cond( { "key": "value" } ) );
   while( cursor.next() )
   {
      throw new Error( "Matched record!" );
   }

   //sel指定多个字段名（包含不存在的字段名）
   count = 0;
   cursor = db.snapshot( SDB_SNAP_HEALTH, new SdbSnapshotOption().sel( { IsPrimary: 1, ServiceStatus: 1, ZXN: 1 } ) );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      if( !obj.hasOwnProperty( "IsPrimary" ) )
      {
         throw new Error( "IsPrimary does not exist in the own properties!" );
      }
      if( !obj.hasOwnProperty( "ServiceStatus" ) )
      {
         throw new Error( "ServiceStatus does not exist in the own properties!" );
      }
      if( !obj.hasOwnProperty( "ZXN" ) )
      {
         throw new Error( "ZXN exists in the own properties!" );
      }
      count++;
   }
   if( count <= 0 )
   {
      throw new Error( "count: " + count );
   }

   //sort指定降序
   count = 0;
   var tmp = 66660;
   cursor = db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().sort( { svcname: -1 } ) );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      if( obj.svcname > tmp )
      {
         throw new Error( "Sort failed!" );
      }
      tmp = obj.svcname;
      count++;
   }
   if( count <= 0 )
   {
      throw new Error( "count: " + count );
   }

   //sort升序
   count = 0;
   tmp = 0;
   cursor = db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().sort( { svcname: 1 } ) );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      if( obj.svcname < tmp )
      {
         throw new Error( "Sort failed" );
      }
      tmp = obj.svcname;
      count++;
   }
   if( count <= 0 )
   {
      throw new Error( "count: " + count );
   }

   //skip为0
   count = 0;
   cursor = db.snapshot( SDB_SNAP_SYSTEM, new SdbSnapshotOption().skip( 0 ) );
   while( cursor.next() )
   {
      count++;
   }
   if( count !== 1 )
   {
      throw new Error( "count: " + count );
   }

   cursor = db.snapshot( SDB_SNAP_SYSTEM, new SdbSnapshotOption().skip( 1 ) );
   while( cursor.next() )
   {
      throw new Error( "Skip failed!" );
   }

   count = 0;
   cursor = db.snapshot( SDB_SNAP_SYSTEM, new SdbSnapshotOption().skip( 2 ) );
   while( cursor.next() )
   {
      count++;
   }
   if( count !== 0 )
   {
      throw new Error( "count: " + count );
   }

   //limit0
   cursor = db.snapshot( SDB_SNAP_SYSTEM, new SdbSnapshotOption().limit( 0 ) );
   while( cursor.next() )
   {
      throw new Error( "Limit failed!" );
   }

   //limit1
   count = 0;
   cursor = db.snapshot( SDB_SNAP_SYSTEM, new SdbSnapshotOption().limit( 1 ) );
   while( cursor.next() )
   {
      count++;
   }
   if( count !== 1 )
   {
      throw new Error( "count: " + count );
   }

   //limit2
   count = 0;
   cursor = db.snapshot( SDB_SNAP_SYSTEM, new SdbSnapshotOption().limit( 2 ) );
   while( cursor.next() )
   {
      count++;
   }
   if( count !== 1 )
   {
      throw new Error( "count: " + count );
   }

   //options
   cursor = db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().options( { "expand": false } ) );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      if( obj.hasOwnProperty( "archiveon" ) )
      {
         throw new Error( "archiveon exists in properties!" );
      }
   }
}
