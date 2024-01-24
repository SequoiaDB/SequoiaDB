/*******************************************************************
* @Description : test export with --select
*                seqDB-13579:--select取值非法
*                seqDB-13580:--select与--fields同时使用
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13579";

main( test );

function test ()
{
   var docs = [{ a: 1 }, { a: 2 }, { a: 3 }, { a: 4 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtSelect1();  // use --select '{ a: { \\$include: 1 }'
   testExprtSelect2();  // use --select '{ a: { \\$inclu: 1 } }'
   testExprtSelect3();  // use --select with --fields

   commDropCL( db, csname, clname );
}

function testExprtSelect1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13579.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --select '{ a: { \\$include: 1 }'" +
      " --type csv";
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtSelect2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13579.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --select '{ a: { \\$inclu: 1 } }'" +
      " --type csv";
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtSelect3 ()
{
   var csvfile = tmpFileDir + "sdbexprt13580.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --select '{ a: { \\$include: 1 } }'" +
      " --fields a" +
      " --type csv";
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}
