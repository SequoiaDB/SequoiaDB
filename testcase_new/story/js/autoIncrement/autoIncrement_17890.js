/***************************************************************************
@Description : seqDB-17890:Increment为正值，自增字段为嵌套字段，插入值在当前缓存范围内
@Modify list :
              2019-2-21  Zhao Xiaoni  Create
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
   var increment = 1;
   var currentValue = 1
   var cacheSize = 1000;
   var acquireSize = 11;
   var clName = COMMCLNAME + "_17890";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a.b.c", AcquireSize: acquireSize, CacheSize: cacheSize } } );
   commCreateIndex( dbcl, "index", { "a.b.c": 1 }, { Unique: true }, true );

   var expR = [];
   var cl = new Array();
   var coord = new Array();
   for( var k = 0; k < coordNum; k++ )
   {
      coord[k] = new Sdb( coordNodes[k] );
      cl[k] = coord[k].getCS( COMMCSNAME ).getCL( clName );
      //连接所有coord插入部分记录,coord缓存分别为[1,11],[12,22],[23,33]
      var doc = [];
      for( var i = 0; i < 3; i++ )
      {
         doc.push( { h: sortField } );
         expR.push( { h: sortField, a: { b: { c: currentValue + Math.ceil( 3 / acquireSize ) * acquireSize * increment * k + increment * i } } } );
         sortField++;
      }
      cl[k].insert( doc );
   }

   //coordB指定自增字段值插入记录，插入值落在当前coord缓存范围，大于nextValue
   var insertValue = 18;
   var record = { a: { b: { c: insertValue } }, h: sortField };
   cl[1].insert( record );
   expR.push( record );
   sortField++;

   //coordA再次插入，不指定自增字段
   cl[0].insert( { h: sortField } );
   expR.push( { h: sortField, a: { b: { c: 4 } } } );
   sortField++;

   //coordB再次插入，不指定自增字段
   cl[1].insert( { h: sortField } );
   expR.push( { h: sortField, a: { b: { c: insertValue + increment } } } );
   sortField++;

   //coordC再次插入，不指定自增字段
   cl[2].insert( { h: sortField } );
   expR.push( { h: sortField, a: { b: { c: 26 } } } );
   sortField++;

   //coordB指定自增字段值插入记录，插入值落在当前coord缓存范围，小于nextValue
   var insertValue = 16;
   var record = { a: { b: { c: insertValue } }, h: sortField };
   cl[1].insert( record );
   expR.push( record );
   sortField++;

   //coordA再次插入，不指定自增字段
   cl[0].insert( { h: sortField } );
   expR.push( { h: sortField, a: { b: { c: 5 } } } );
   sortField++;

   //coordB再次插入，不指定自增字段
   cl[1].insert( { h: sortField } );
   expR.push( { h: sortField, a: { b: { c: 20 } } } );
   sortField++;

   //coordC再次插入，不指定自增字段
   cl[2].insert( { h: sortField } );
   expR.push( { h: sortField, a: { b: { c: 27 } } } );
   sortField++;

   var actR = dbcl.find().sort( { h: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
