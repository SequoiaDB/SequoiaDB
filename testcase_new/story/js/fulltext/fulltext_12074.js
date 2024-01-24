/***************************************************************************
@Description :seqDB-12074 :切分表中存在全文索引，删除集合 
@Modify list :
              2018-11-02  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12074";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建切分集合，并创建普通索引及全文索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "hash", ShardingKey: { a: 1 } } );
   commCreateIndex( dbcl, "fullIndex_12074", { a: "text" } );
   commCreateIndex( dbcl, "commIndex_12074", { b: 1 } );

   //插入lob、记录(包含索引及全文索引的记录) 
   var records = new Array();
   for( var i = 0; i < 100; i++ )
   {
      var record = { a: "a" + i, b: "b" + i };
      records.push( record );
   }
   dbcl.insert( records );
   var testFile = CHANGEDPREFIX + "_lobTest.file";
   lobGenerateFile( testFile );
   dbcl.putLob( testFile );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12074", 100 );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, "fullIndex_12074" );
   dropCL( db, COMMCSNAME, clName, false, false );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function lobGenerateFile ( fileName, fileLine )
{
   if( undefined == fileLine )
   {
      fileLine = 1000;
   }

   var cnt = 0;
   while( true == lobFileIsExist( fileName ) )
   {
      File.remove( fileName );
      if( cnt > 10 ) break;
      cnt++;
      sleep( 10 );
   }

   if( 10 <= cnt )
      throw new Error( "failed to remove file: " + fileName );
   var file = new File( fileName );
   for( var i = 0; i < fileLine; ++i )
   {
      var record = '{ no:' + i + ', score:' + i + ', interest:["movie", "photo"],' +
         '  major:"计算机软件与理论", dep:"计算机学院",' +
         '  info:{name:"Holiday", age:22, sex:"男"} }';
      file.write( record );
   }
   if( false == lobFileIsExist( fileName ) )
   {
      throw new Error( "NoFile: " + fileName );
   }
}

function lobFileIsExist ( fileName )
{
   var isExist = false;
   try
   {
      var cmd = new Cmd();
      cmd.run( "ls " + fileName );
      isExist = true;
   }
   catch( e )
   {
      if( 2 == e.message ) { isExist = false; }
   }
   return isExist;
}
