/*******************************************************************
* @Description : test export with equal delrecord delfield delchar
*                                       -r  -a  -e
*                seqDB-8199:导出时指定字符串分隔符、字段分隔符、
*                           记录分隔符相同                 
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt8199";
var docs = [{ a: "a1", b: "b1", c: "c1" },
{ a: "a2", b: "b2", c: "c2" }];

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );

   cl.insert( docs );

   testExprt1();  // test export with equal delrecord delfield
   testExprt2();  // test export with equal delrecord delchar
   testExprt3();  // test export with equal delfield delchar

   commDropCL( db, csname, clname );
}

function testExprt1 ()
{
   var csvfile = tmpFileDir + "sdbexprt8199.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --fields a,b" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " -r '\n' -a '\n'" +
      " --file " + csvfile;
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}

function testExprt2 ()
{
   var csvfile = tmpFileDir + "sdbexprt8199.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --fields a,b" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " -r '\n' -e '\n'" +
      " --file " + csvfile;
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}

function testExprt3 ()
{
   var csvfile = tmpFileDir + "sdbexprt8199.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --fields a,b" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " -a '\"' -e '\"'" +
      " --file " + csvfile;
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}
