/*******************************************************************
* @Description : test export with --limit
*                seqDB-13592:--limit指定返回N条记录（1<=N<=总记录数），
*                            集合为普通表
*                seqDB-13593:--limit指定返回N条记录（1<=N<=总记录数），
*                            集合为分区表
*                seqDB-13594:--limit指定返回N条记录（1<=N<=总记录数），
*                            集合为主子表                
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;

main( test );

function test ()
{
   testExprtLimit1();    // test limit with normal cl

   if( commIsStandalone( db ) )
   {
      return;
   }
   testExprtLimit2();    // test limit with split cl
   testExprtLimit3();    // test limit with main cl
}

function testExprtLimit1 ()
{
   var docs = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i } );
   }
   var clname = COMMCLNAME + "_sdbexprt13592";
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13592.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --sort '{ a: 1 }'" +
      " --limit 5" +
      " --fields a" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n0\n1\n2\n3\n4\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
}

function testExprtLimit2 ()
{
   var docs = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i } );
   }
   var clname = COMMCLNAME + "_sdbexprt13593";
   var option = { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0 };
   var cl = commCreateCL( db, csname, clname, option );
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13593.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --sort '{ a: 1 }'" +
      " --limit 5" +
      " --fields a" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n0\n1\n2\n3\n4\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
}

function testExprtLimit3 ()
{
   var mainClName = COMMCLNAME + "_sdbexprtMain13593";
   var option = {
      ShardingType: "range", ShardingKey: { a: 1 },
      IsMainCL: true, ReplSize: 0
   };
   var cl = commCreateCL( db, csname, mainClName, option );
   var subClName1 = COMMCLNAME + "_sdbexprtSub13593_1";
   option = { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0 };
   commCreateCL( db, csname, subClName1, option );
   var subClName2 = COMMCLNAME + "_sdbexprtSub13593_2";
   commCreateCL( db, csname, subClName2, option );
   cl.attachCL( csname + "." + subClName1, { LowBound: { a: 0 }, UpBound: { a: 10 } } );
   cl.attachCL( csname + "." + subClName2, { LowBound: { a: 10 }, UpBound: { a: 20 } } );

   var docs = [{ a: 1 }, { a: 2 }, { a: 5 },
   { a: 11 }, { a: 12 }, { a: 15 }];

   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13592.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + mainClName +
      " --file " + csvfile +
      " --sort '{ a: 1 }'" +
      " --limit 5" +
      " --fields a" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n1\n2\n5\n11\n12\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, mainClName );
}