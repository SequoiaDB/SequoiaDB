/*******************************************************************
* @Description : test export with --skip
*                seqDB-13590:--skip取值非法
*                seqDB-11101:skip/limit参数校验
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13590";

main( test );

function test ()
{
   var docs = [{ a: 1 }, { a: 3 }, { a: 2 }, { a: 4 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtSkip();    // test skip "abc"

   commDropCL( db, csname, clname );
}

function testExprtSkip ()
{
   var csvfile = tmpFileDir + "sdbexprt13588.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --skip \"abc\"" +
      " --fields a" +
      " --type csv";
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}