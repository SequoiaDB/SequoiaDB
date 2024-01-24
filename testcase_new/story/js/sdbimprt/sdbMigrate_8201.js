/*******************************************************************************
*@Description : import record that the size like : [(32M-2048B)+(>2048B)].
*                                                    ---------   ------
*                                                   can import   cannot import
*               [SEQUOIADBMAINSTREAM-489]
*@Modify list :
*               2014-11-10  xiaojun Hu  Change
*               2019-12-02  Siqin Chen  Change
*******************************************************************************/

main( test );

function test ()
{

   var clName = COMMCLNAME + "_import8201";
   var imprtFile = tmpFileDir + "8201.json";

   commDropCL( db, COMMCSNAME, clName, true, true );
   cmd.run( "rm -rf " + imprtFile );

   var cl = commCreateCL( db, COMMCSNAME, clName );
   readyData( imprtFile );   // 5461 + 1 line
   testImportData8201( COMMCSNAME, clName, imprtFile, cl );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( "rm -rf " + imprtFile );
}

function readyData ( imprtFile )
{
   // 5461 * 6144 = 33552384[32M-2048]
   var file = fileInit( imprtFile );
   var lineNum = 5461;
   var recordSize = 6144;   // size more than 2048
   var fixChar = "_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890#_";
   var fixRecord = '{"fieldLarge":""}';
   var rcrdValLen = recordSize - fixRecord.length - 1; // record value length
   var recordValue = "";
   for( var i = 0; i < parseInt( rcrdValLen / fixChar.length ); ++i )
   {
      recordValue += fixChar;
   }
   for( var i = 0; i < parseInt( rcrdValLen % fixChar.length ); ++i )
   {
      recordValue += fixChar.charAt( i );
   }
   var conRecord = '{"fieldLarge":"' + recordValue + '"}';  // complete record
   for( var i = 0; i < lineNum; ++i )
   {
      file.write( conRecord + "\n" );
   }
   // add the last record
   recordValue = "";
   conRecord = "";
   // when the size of last record equal 4047[Import]
   for( var i = 0; i < parseInt( 4046 / fixChar.length ); ++i )
   {
      recordValue += fixChar;
   }
   for( var i = 0; i < parseInt( 4046 % fixChar.length ); ++i )
   {
      recordValue += fixChar.charAt( i );
   }
   conRecord = '{"fieldLarge":"' + recordValue + '"}';  // complete record
   file.write( conRecord );
}

//当读取一条记录,该记录刚好在一个block的起始,并且剩下的block不足以存下记录时,导致blockID没有正确计算,导致数据覆盖
function testImportData8201 ( csName, clName, imprtFile, cl )
{
   var imprtOption = installDir + "bin/sdbimprt " + "--hostname " + COORDHOSTNAME + " --svcname " + COORDSVCNAME +
      " -c " + csName + " -l " + clName + " --file " + imprtFile +
      " --type json ";
   cmd.run( imprtOption );
   var cnt = 0;
   while( 5462 != cl.count() && 1000 > cnt )
   {
      cnt++;
      sleep( 3 );
   }
   if( 5462 != cl.count() )
   {
      throw new Error( "expect count: 5462, actual count: " + cl.count() );
   }
}
