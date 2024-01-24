/***************************************************************************
@Description :seqDB-12037 :在全文索引字段上执行普通查询 
@Modify list :
              2018-10-08  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   };

   var clName = COMMCLNAME + "_ES_12073";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建集合，并创建普通索引及全文索引 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_12073";
   commCreateIndex( dbcl, textIndexName, { content: "text" } );
   commCreateIndex( dbcl, "commIndex_12073", { content: 1 } );

   //插入lob、记录(包含索引及全文索引的记录) 
   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( 10, "string", ["content"] );
   dbcl.insert( records );
   var testFile = CHANGEDPREFIX + "_lobTest.file";
   lobGenerateFile( testFile );
   dbcl.putLob( testFile );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var cappedCLs = dbOperator.getCappedCLs( COMMCSNAME, clName, textIndexName );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10 );

   //删除集合，检查结果
   var commCS = db.getCS( COMMCSNAME );
   commCS.dropCL( clName );

   //集合删除成功，lob文件、索引文件、固定集合文件均被删除，主备节点一致，es中最终无该集合中的记录
   var esOperator = new ESOperator();
   checkAllResult( dbcl, esOperator, cappedCLs[0], esIndexNames[0] );

   dropCL( db, COMMCSNAME, clName, true, true );

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

function checkAllResult ( dbcl, esOperator, cappedCL )
{
   try
   {
      dbcl.find();
      dbcl.listIndexes();
      dbcl.listLobs();
   }
   catch( e )
   {
      if( e.message != SDB_DMS_NOTEXIST )
      {
         throw e;
      }
   }
   try
   {
      cappedCL.find();
   }
   catch( e )
   {
      if( e.message != SDB_DMS_NOTEXIST )
      {
         throw e;
      }
   }
   sleep( 1000 );
   try
   {
      var queryCond = '{"query" : {"match_all" : {}}}';
      esOperator.findFromES( esIndexNames[0], queryCond );
   }
   catch( e )
   {
      if( e != "esIndexNames is not defined" )
      {
         throw new Error( e );
      }
   }

}
