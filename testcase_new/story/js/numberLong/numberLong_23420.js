/******************************************************************************
 * @Description   : seqDB-23420:NumberLong 验证
 * @Author        : Li Yuanyue
 * @CreateTime    : 2021.01.15
 * @LastEditTime  : 2021.01.15
 * @LastEditors   : Li Yuanyue
 *****************************************************************************/
testConf.clName = COMMCLNAME + "_23420";
testConf.skipStandAlone = true;
main( test );

function test ( args )
{
   var cl = args.testCL;
   var fileName = WORKDIR + "numberLong_23420.ini";

   var cmd = new Cmd();
   cmd.run( "rm -rf " + fileName + "*" );

   var file = new File( fileName );
   file.write( "SequoiaDB" );
   var lobOid = cl.putLob( fileName );

   // 1. truncateLob
   cl.truncateLob( lobOid, NumberLong( '27999990' ) );

   // 2. seek
   file.seek( NumberLong( "2" ) );

   // 3. truncate
   file.truncate( NumberLong( "3" ) );

   file.truncate();
   var ini = new IniFile( fileName );

   // 4. setValue
   ini.setValue( "auto", "?", NumberLong( 9999 ) );

   // 5. waitTasks
   db.waitTasks( NumberLong( '27999990' ) );

   cmd.run( "rm -rf " + fileName + "*" );
}