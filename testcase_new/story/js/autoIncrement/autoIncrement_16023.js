/***************************************************************************
@Description :seqDB-16023 :同时修改多个自增字段的属性值 
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

   var clName = COMMCLNAME + "_16023";
   var fieldName1 = "id1";
   var fieldName2 = "id2";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: [{ Field: fieldName1 }, { Field: fieldName2 }] } );

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
         expR.push( { a: sortField, id1: 1 + k * 1000 + i, id2: 1 + k * 1000 + i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
   }
   var actR = dbcl.find().sort( { a: 1 } );

   checkRec( actR, expR );

   var increment1 = 10;
   var cacheSize1 = 32;
   var acquireSize1 = 10;
   var generated1 = "strict";
   var increment2 = 11;
   var cacheSize2 = 33;
   var acquireSize2 = 11;
   dbcl.setAttributes( {
      AutoIncrement: [{ Field: fieldName1, Increment: increment1, CacheSize: cacheSize1, AcquireSize: acquireSize1, Generated: generated1 },
      { Field: fieldName2, Increment: increment2, CacheSize: cacheSize2, AcquireSize: acquireSize2 }]
   } );
   var clID = getCLID( db, COMMCSNAME, clName );
   var clSequenceName1 = "SYS_" + clID + "_" + fieldName1 + "_SEQ";
   var clSequenceName2 = "SYS_" + clID + "_" + fieldName2 + "_SEQ";
   var expIncrementArr = [{ Field: fieldName1, SequenceName: clSequenceName1, Generated: generated1 },
   { Field: fieldName2, SequenceName: clSequenceName2 }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expIncrementArr );

   var currentValue1 = coordNum * 1000;
   var clExpSequenceObj = { Increment: increment1, CacheSize: cacheSize1, AcquireSize: acquireSize1, CurrentValue: currentValue1 };
   checkSequence( db, clSequenceName1, clExpSequenceObj );
   var currentValue2 = coordNum * 1000;
   var clExpSequenceObj = { Increment: increment2, CacheSize: cacheSize2, AcquireSize: acquireSize2, CurrentValue: currentValue2 };
   checkSequence( db, clSequenceName2, clExpSequenceObj );
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
         expR.push( { a: sortField, id1: currentValue1 + increment1 + Math.ceil( 100 / acquireSize1 ) * acquireSize1 * increment1 * k + increment1 * i, id2: currentValue2 + increment2 + Math.ceil( 100 / acquireSize2 ) * acquireSize2 * increment2 * k + increment2 * i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   commDropCL( db, COMMCSNAME, clName, true, true );
}
