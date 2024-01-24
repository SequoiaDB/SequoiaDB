/******************************************************************************
 * @Description   : seqDB-12246:unset存在的字段
 *                  seqDB-12252:addtoset往指定数组字段中增加一个值
 * @Author        : Wang Wenjing
 * @CreateTime    : 2017.07.25
 * @LastEditTime  : 2021.06.28
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );

   varCL.insert( { a: 2, b: { name: "YoYo", age: 23, phone: [12, 56, "reqnf"] }, c: "jkdi" } );

   varCL.update( { $unset: { c: "jkdi" } } );

   var rc = varCL.find( { c: "jkdi" } );

   var size = 0;
   while( true )
   {
      var i = rc.next();
      if( !i )
      {
         break;
      }
      else
      {
         size++;
      }
   }
   assert.equal( size, 0 );

   varCL.update( { $inc: { salary: 100 } } );

   var rc1 = varCL.find( { salary: 100 } );


   var size1 = 0;
   while( true )
   {
      var i = rc1.next();
      if( !i )
      {
         break;
      }
      else
      {
         size1++;
      }
   }
   assert.equal( size1, 1 );

   varCL.update( { $push: { "b.phone": 3 } } );
   var rc2 = varCL.find( { "b.phone.3": 3 } );

   var size2 = 0;
   while( true )
   {
      var i = rc2.next();
      if( !i )
      {
         break;
      }
      else
      {
         size2++;
      }
   }
   assert.equal( size2, 1 );
   varCL.update( { $pull: { "b.phone": 3 } } );
   var rc3 = varCL.find( { "b.phone.3": 3 } );

   var size3 = 0;
   while( true )
   {
      var i = rc3.next();
      if( !i )
      {
         break;
      }
      else
      {
         size3++;
      }
   }
   assert.equal( size3, 0 );

   varCL.update( { $push_all: { array: [3, 4] } } );

   var rc4 = varCL.find( { array: [3, 4] } );

   var size4 = 0;
   while( true )
   {
      var i = rc4.next();
      if( !i )
      {
         break;
      }
      else
      {
         size4++;
      }
   }
   assert.equal( size4, 1 );


   varCL.update( { $pull_all: { array: [3, 4] } } );

   var rc5 = varCL.find( { array: [] } );

   var size5 = 0;
   while( true )
   {
      var i = rc5.next();
      if( !i )
      {
         break;
      }
      else
      {
         size5++;
      }
   }
   assert.equal( size5, 1 );

   varCL.update( { $pop: { "b.phone": 2 } } );
   var rc6 = varCL.find( { "b.phone": [12] } );

   var size6 = 0;
   while( true )
   {
      var i = rc6.next();
      if( !i )
      {
         break;
      }
      else
      {
         size6++;
      }
   }
   assert.equal( size6, 1 );

   varCL.update( { $addtoset: { "b.phone": [12] } } );

   var rc7 = varCL.find( { "b.phone": [12] } );

   var size7 = 0;
   while( true )
   {
      var i = rc7.next();
      if( !i )
      {
         break;
      }
      else
      {
         size7++;
      }
   }

   assert.equal( size7, 1 );
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );
}