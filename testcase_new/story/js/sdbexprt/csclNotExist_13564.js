/*******************************************************************
* @Description : test export with --cscl multi cs
*                seqDB-13564:--cscl指定的集合空间/集合不存在
*                seqDB-8202:--clname指定集合不存在
*                seqDB-8203:--csname指定集合空间不存在            
* @author      : Liang XueWang 
*
*******************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_13564";
   commCreateCL( db, COMMCSNAME, clName, {}, true, true );

   testExprtNotExistCs( clName );
   testExprtNotExistCl( clName );

   commDropCL( db, COMMCSNAME, clName );
}

function testExprtNotExistCs ( clName )
{
   var csvfile = tmpFileDir + "sdbexprt13564.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --file " + csvfile +
      " --type csv" +
      " --cscl notExistCs " +
      " --force true";
   testRunCommand( command, 8 );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c notExistCs " +
      " -l " + clName +
      " --file " + csvfile +
      " --type csv" +
      " --force true";
   testRunCommand( command, 8 );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtNotExistCl ()
{
   var jsonfile = tmpFileDir + "sdbexprt13564.json";
   cmd.run( "rm -rf " + jsonfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --file " + jsonfile +
      " --type json" +
      " --cscl " + COMMCSNAME + ".notExistCl";
   testRunCommand( command, 8 );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l notExistCl" +
      " --file " + jsonfile +
      " --type json";
   testRunCommand( command, 8 );

   cmd.run( "rm -rf " + jsonfile );
}