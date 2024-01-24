/***************************************************************************
@Description :seqDB-16022 :修改单个自增字段的所有属性 
@Modify list :
              2018-10-25  zhaoyu  Create
****************************************************************************/

main( test );
function test ()
{
   var sortField = 0;
   if( commIsStandalone( db ) )
   {
      return;
   };

   var clName = COMMCLNAME + "_16022";
   var fieldName = "id";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName } } );

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
         expR.push( { a: sortField, id: 1 + k * 1000 + i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   var increment = 10;
   var startValue = 100;
   var minValue = -100;
   var currentValue = -10
   var maxValue = 1000000;
   var cacheSize = 32;
   var acquireSize = 10;
   var cycled = true;
   var generated = "strict";
   dbcl.setAttributes( {
      AutoIncrement: {
         Field: fieldName, Increment: increment, StartValue: startValue, MinValue: minValue, MaxValue: maxValue,
         CacheSize: cacheSize, AcquireSize: acquireSize, Cycled: cycled, Generated: generated, CurrentValue: currentValue
      }
   } );
   var clID = getCLID( db,  COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: clSequenceName, Generated: generated }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expIncrementArr );

   var clExpSequenceObj = {
      Increment: increment, StartValue: startValue, MinValue: minValue, MaxValue: maxValue, CacheSize: cacheSize,
      AcquireSize: acquireSize, Cycled: cycled, CurrentValue: currentValue
   };
   checkSequence( db, clSequenceName, clExpSequenceObj );

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
         expR.push( { a: sortField, id: currentValue + increment + Math.ceil( 100 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   commDropCL( db, COMMCSNAME, clName, true, true );
}
