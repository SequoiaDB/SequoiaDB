/*******************************************************************************
@Description : SSL common functions
@Modify list :
               2016-8-31  wuyan  Init
*******************************************************************************/
var csName = CHANGEDPREFIX + "_cs";;
var cmd = new Cmd();
var installDir = initPath();
db.updateConf({ usessl: true},{role:"coord"});
var dbs = new SecureSdb( COORDHOSTNAME, COORDSVCNAME );

//inspect the index is created success or not.
function inspecIndex ( cl, indexName, indexKey, keyValue, idxUnique, idxEnforced )
{
   if( undefined == idxUnique ) { idxUnique = false; }
   if( undefined == idxEnforced ) { idxEnforced = false; }
   try
   {
      if( undefined == cl || undefined == indexName || undefined == indexKey || undefined == keyValue )
      {
         println( " wrong argument when inspect index " );
         throw "ErrArg";
      }
      var getIndex = new Boolean( true );
      try
      {
         getIndex = cl.getIndex( indexName );
      }
      catch( e )
      {
         getIndex = undefined;
      }
      if( undefined == getIndex )
      {
         println( "Don't have the index, name = " + indexName );
         throw "ErrIdxName";
      }
      //println(cl.getIndex( indexName )) ;
      var indexDef = getIndex.toString();
      indexDef = eval( '(' + indexDef + ')' );
      var index = indexDef["IndexDef"];

      if( keyValue != index["key"][indexKey] )
      {
         println( "Wrong index name or key value : " + index["key"][indexKey] );
         throw "ErrIdxValue";
      }
      if( idxUnique != index["unique"] )
      {
         println( "Wrong index unique : " + index["unique"] );
         throw "ErrIdxUnique";
      }
      if( idxEnforced != index["enforced"] )
      {
         println( "Wrong index enforced : " + index["enforced"] );
         throw "ErrIdxEnforced";
      }
      println( "Success to inspect index : " + indexName );
   }
   catch( e )
   {
      println( "argument value:'" + indexName + "','" + indexKey + "','" + keyValue + "','" + idxUnique + "','" + idxEnforced );
      println( "Failed to inspect index : " + indexName + " rc=: " + e );
      throw e;
   }
}

/****************************************************
@description: check the result of query data
@modify list:
              2016-8-31 yan WU init
****************************************************/
function checkCLData ( dbcl, expRecs )
{
   println( "\n---Begin to check cl data." );

   var rc = dbcl.find();
   var recsArray = [];
   while( rc.next() )
   {
      recsArray.push( rc.current().toObj() );
   }
   var actRecs = JSON.stringify( recsArray );
   //var expRec = JSON.stringify( expRecs )
   if( actRecs !== expRecs )
   {
      throw buildException( "checkCLdata", null, "[find]",
         "[recs:" + JSON.stringify( expRecs ) + "]",
         "[recs:" + actRecs + "]" );
   }
   println( "cl records: " + actRecs );
}

/******************************************************************************
*@Description : initalize the global variable in the begninning.
                ��ʼ��ȫ�ֱ���LocalPath��installPath,��ȡsdbimprt����Ŀ¼
******************************************************************************/
function initPath ()
{
   try
   {
      var local = cmd.run( "pwd" ).split( "\n" );   //��õ�ǰĿ¼,cmd.run()�������ؽ�����ں������һ����
      LocalPath = local[0];
      println( "LocalPath=" + LocalPath );
      try
      {
         // ����ؽ��Ϊ INSTALL_DIR=/opt/sequoiadb(Ĭ�ϰ�װĿ¼)        
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb' );
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR"' );
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR" |cut -d "=" -f 2' );
         var installPath = tmpDir.split( "\n" )[0] + "/";
      }
      catch( e )
      {
         //�Ҳ�����װĿ¼���ص�ǰĿ¼ 
         installPath = LocalPath + "/";
         println( "catch erro in try" );
      }

      println( "instatllpath=" + installPath );
   }
   catch( e )
   {
      println( "failed to get global variable : cmd/LocalPath/installPath" + e );
      throw e;
   }
   return installPath;
}


function checkLobResult ( lobfile, getLobfile )
{
   // ��ȡputLob�ļ���getLob�ļ���Md5ֵ
   try
   {
      println( "---check the lob---" );
      var putMd5 = cmd.run( "md5sum " + lobfile ).split( " " );
      var getMd5 = cmd.run( "md5sum " + getLobfile ).split( " " );
      if( putMd5 != getMd5 )
      {
         buildException( "checkLobResult()", "the md5 is not equal", "check md5", "md5 is equal", "putMd5: " + putMd5 + ", getMd5: " + getMd5 );
      }
   }
   catch( e )
   {
      throw e;
   }


}
