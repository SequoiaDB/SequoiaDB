/***************************************************************************
@Description :seqDB-16024 :同时修改自增字段及其他属性 
@Modify list :
              2018-10-26  zhaoyu  Create
****************************************************************************/

main( test );
function test ()
{
   var sortField = 0;
   if( commIsStandalone( db ) )
   {
      return;
   };

   var clName = COMMCLNAME + "_16024";
   var fieldName = "id";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var increment = 12;
   var acquireSize = 11;

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName, Increment: increment, AcquireSize: acquireSize } } );

   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   var expR = [];
   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      var doc = [];
      for( var i = 0; i < 100; i++ )
      {
         doc.push( { a: sortField } );
         expR.push( { a: sortField, id: 1 + Math.ceil( 100 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   var cacheSize = 32;
   var acquireSize = 11;
   var generated = "strict";
   var currentValue = 999 * increment + 1;
   dbcl.setAttributes( { AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize, Generated: generated }, ShardingKey: { a: 1 }, ReplSize: -1 } );
   var clID = getCLID( db, COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: clSequenceName, Generated: generated }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expIncrementArr );

   var clExpSequenceObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, CurrentValue: currentValue };
   checkSequence( db, clSequenceName, clExpSequenceObj );

   checkSnapshot8onCL( COMMCSNAME, clName );

   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      //alter操作会变更集合版本号，插入时会取2次seqence值，SEQUOIADBMAINSTREAM-3895,通过find操作更新版本号
      var cursor = cl.find();
      while( cursor.next() ) { }
      var doc = [];
      for( var i = 0; i < 100; i++ )
      {
         doc.push( { a: sortField } );
         expR.push( { a: sortField, id: 1 + Math.ceil( 100 / acquireSize ) * acquireSize * increment * coordNum + Math.ceil( 100 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
   }
   var actR = dbcl.find().sort( { _id: 1 } );
   checkRec( actR, expR );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.insert( { id: "a" } );
   } );

   commDropCL( db, COMMCSNAME, clName, true, true );
}


function checkSnapshot8onCL ( csName, clName )
{
   var obj = db.snapshot( 8, { Name: csName + "." + clName } ).next().toObj();
   var shardingType = obj.ShardingType;
   if( shardingType !== "hash" )
   {
      throw new Error( "ALTER_CL_ERR" );
   }
}
