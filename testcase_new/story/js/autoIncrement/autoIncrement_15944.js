/******************************************************************************
 * @Description   : seqDB-15944:集合中存在多自增字段，插入记录
 * @Author        : zhaoyu
 * @CreateTime    : 2018.10.16
 * @LastEditTime  : 2021.02.03
 * @LastEditors   : Lai Xuan
 ******************************************************************************/
main( test );
function test ()
{
   var sortField = 0;
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15944";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      AutoIncrement: [{ Field: "id1", AcquireSize: 10 },
      { Field: "id2", AcquireSize: 1, Increment: -1, CacheSize: 10 }]
   } );
   var expR = [];
   for( var j = 0; j < 100; j++ )
   {
      var doc = [];
      for( var i = 1; i < 6; i++ )
      {
         doc.push( { a: sortField } );
         expR.push( { a: sortField, id1: j * 5 + i, id2: -( j * 5 + i ) } );
         sortField++;
      }
      dbcl.insert( doc );
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   for( var j = 0; j < 100; j++ )
   {
      var doc = [];
      for( var i = 0; i < 5; i++ )
      {
         doc.push( { a: sortField, id1: i, id2: i } );
         expR.push( { a: sortField, id1: i, id2: i } );
         sortField++;
      }
      dbcl.insert( doc );
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
