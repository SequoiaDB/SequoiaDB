/***************************************************************************
@Description :seqDB-17722:increment为正值，插入值比序列的MaxValue稍小，再次插入自增字段未超MaxValue 
@Modify list :
              2019-1-25  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   if( commIsStandalone( db ) || coordNum !== 3 )
   {
      return;
   }
   var sortField = 0;
   var increment = 5;
   var currentValue = 1
   var cacheSize = 1000;
   var acquireSize = 11;
   var maxValue = 10001;
   var clName = COMMCLNAME + "_17722";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: acquireSize, CacheSize: cacheSize, Cycled: true, MaxValue: maxValue, Increment: increment } } );
   commCreateIndex( dbcl, "id", { id: 1 }, { Unique: true }, true );

   var expR = [];
   var cl = new Array();
   var coord = new Array();
   for( var k = 0; k < coordNum; k++ )
   {
      coord[k] = new Sdb( coordNodes[k] );
      cl[k] = coord[k].getCS( COMMCSNAME ).getCL( clName );
      //连接所有coord插入部分记录,coord缓存分别为[1,51],[56,106],[111,161]
      var doc = [];
      for( var i = 0; i < 3; i++ )
      {
         doc.push( { a: sortField } );
         expR.push( { a: sortField, id: currentValue + Math.ceil( 3 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl[k].insert( doc );
   }

   //coordB指定自增字段插入记录，插入值是序列的maxValue-1,coordB丢弃本coord的缓存，重新从catalog上获取新缓存,[1,51]
   cl[1].insert( { a: sortField, id: maxValue - 1 } );
   expR.push( { a: sortField, id: maxValue - 1 } );

   //coordA插入记录，消耗完本coord的缓存，[1,51]
   for( var i = 0; i < 8; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, id: 16 + i * increment } );
      sortField++;
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   //为了避免唯一键重复，删除集合中已有记录，方便校验后续生成的自增字段值
   dbcl.remove();
   expR = [];

   //coordA插入记录，插入成功，重新从catalog获取新的缓存,[46,106]
   for( var i = 0; i < 2; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, id: 56 + i * increment } );
      sortField++;
   }

   //coordB插入记录，插入成功，重新获取缓存，[1,51]
   for( var i = 0; i < 11; i++ )
   {
      cl[1].insert( { a: sortField } );
      expR.push( { a: sortField, id: 1 + i * increment } );
      sortField++;
   }

   //coordC插入记录，消耗完本coord的缓存，[111,161]
   for( var i = 0; i < 8; i++ )
   {
      cl[2].insert( { a: sortField } );
      expR.push( { a: sortField, id: 126 + i * increment } );
      sortField++;
   }

   //coordC插入记录，插入成功，重新从catalog获取新的缓存,[111,161]
   for( var i = 0; i < 2; i++ )
   {
      cl[2].insert( { a: sortField } );
      expR.push( { a: sortField, id: 111 + i * increment } );
      sortField++;
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
