/****************************************************
@description:   update, abnormal, invalid value
                testlink cases: seqDB-12652
@modify list:
                2017-11-29 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_12652";
var varCL;

// 目前版本不支持分区键字段修改，因此屏蔽
//main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   commDropCL( db, csName, clName, true, true, "drop cl in begin" );

   var opt = { ShardingKey: { a: 1 } };
   varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );

   varCL.insert( { a: 1, b: 1 } );
   updateAndCheck();

   commDropCL( db, csName, clName, false, false, "drop cl in clean" );
}

function updateAndCheck ()
{
   var word = "update";

   /*****valid hexadecimal 0x00008000*****/
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$inc:{"a":1,"b":1}}', 'flag=0x00008000'], [-178], "updateAndCheck: fail to run rest command: " + word );

   // check	
   /*
   try
   {
      var size = varCL.count({a:2, b:2});
      if (1 != size)
      {
         throw new Error("size: " + size);
      }
   }	
   catch(e)
   {
      throw e;
   }*/

   /*****valid decimal system 32768*****/
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$inc:{"a":1,"b":1}}', 'flag=32768'], [-178], "updateAndCheck: fail to run rest command: " + word );

   // check
   /*	
   try
   {
      var size = varCL.count({a:3, b:3});
      if (1 != size)
      {
         throw new Error("size: " + size);
      }
   }	
   catch(e)
   {
      throw e;
   }*/

   /*****valid octal number system 100000*****/
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$inc:{"a":1,"b":1}}', 'flag=100000'], [-178], "updateAndCheck: fail to run rest command: " + word );

   // check	
   /*
   try
   {
      var size = varCL.count({a:4, b:4});
      if (1 != size)
      {
         throw new Error("size: " + size);
      }
   }	
   catch(e)
   {
      throw e;
   }*/

   /*****valid 123*****/
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$inc:{"a":1,"b":1}}', 'flag=123'], [0], "updateAndCheck: fail to run rest command: " + word );

   // check	
   var size = varCL.count( { a: 5, b: 5 } );
   if( 0 != size )
   {
      throw new Error( "size: " + size );
   }

   /****invalid****/
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$inc:{"a":1,"b":1}}', 'flag=test'], [-6], "updateAndCheck: fail to run rest command: " + word );

   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$inc:{"a":1,"b":1}}', 'flag=true'], [-6], "updateAndCheck: fail to run rest command: " + word );

   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$inc:{"a":1,"b":1}}', 'flag=#?%'], [-6], "updateAndCheck: fail to run rest command: " + word );

   // check	
   var size = varCL.count( { a: 2, b: 2 } );
   if( 0 != size )
   {
      throw new Error( "size: " + size );
   }
}
