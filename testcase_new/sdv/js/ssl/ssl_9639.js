/***************************************************************************
/***************************************************************************
@Description :seqDB-9639:SSL功能开启，使用SSL连接执行lob操作
@Modify list :
              2016-8-31  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_9639";
var cmd = new Cmd();

main( dbs );
function main ( dbs )
{
   try
   {
      // drop collection in the beginning
      commDropCL( dbs, csName, clName, true, true, "drop collection in the beginning" );

      // create cs /cl
      var dbCL = commCreateCL( dbs, csName, clName, {}, true, true );

      //put Lob 
      var lobfile = MakeLobfile();
      var lobOid = putLob( dbCL, lobfile );
      //get Lob
      var getLobfile = getLob( dbCL, lobOid );
      //check the lob 
      checkLobResult( lobfile, getLobfile );
      //delete the lob
      delteLob( dbCL, lobOid );

      //dropCS
      commDropCS( dbs, csName, false, "seqDB-9639: dropCS failed" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      dbs.close();
      cmd.run( "rm -rf " + lobfile );
      cmd.run( "rm -rf " + getLobfile );
   }
}

/******************************************************************************
*@Description : the function of make lobfile to be a lob  
                创建lobfile文件作为大对象
******************************************************************************/
function MakeLobfile ()
{
   var fileName = CHANGEDPREFIX + "_lobFile.txt";
   var lobfile = "/tmp/" + fileName;
   var file = new File( lobfile );
   var loopNum = 10;
   var content = null;
   for( var i = 0; i < loopNum; ++i )
   {
      content = content + i + "ABCDEFGHIJKLMNOPQRSTUVWXYZ123";
   }
   file.write( content );
   file.close();

   return lobfile;
}

/******************************************************************************
*@Description : the function of write lob to the collection 
                向集合cl中插入lobfile大对象
******************************************************************************/
function putLob ( cl, lobfile )     
{
   try
   {
      println( "---put lob---" );
      var OID = cl.putLob( lobfile );
      return OID;
   }
   catch( e )
   {
      println( "fail to put lobs, rc = " + e );
      throw e;
   }
}

/******************************************************************************
*@Description : the function of get lob to the collection                 
******************************************************************************/
function getLob ( dbcl, OID )     
{
   try
   {
      println( "---get lob---" );
      var getLobFile = CHANGEDPREFIX + "_getLob.txt";
      var lobfile = "/tmp/" + getLobFile;
      dbcl.getLob( OID, lobfile );
      return lobfile;
   }
   catch( e )
   {
      println( "fail to put lobs, rc = " + e );
      throw e;
   }
}

/******************************************************************************
*@Description : the function of delete lob to the collection                 
******************************************************************************/
function delteLob ( dbcl, OID )     
{
   try
   {
      println( "---delete lob---" );
      dbcl.deleteLob( OID );
      var count = dbcl.listLobs().current();
   }
   catch( e )
   {
      if( -29 !== e )
      {
         buildException( "deleteLob()", e, "delete lob", "-29", count );
         throw e;
      }
   }
}

