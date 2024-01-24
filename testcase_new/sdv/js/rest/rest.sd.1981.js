/************************************************************************
*@Description:	用rest接口对单条记录进行创建cscl、增删改查等操作
               对应testlink用例为：seqDB-1981:不并发+单条记录_rest.sd.001
*@Author:  		TingYU  2015/10/29
************************************************************************/

//因问题单SEQUOIADBMAINSTREAM-8004缺陷，用例先跳过独立模式，待问题单修改后再进一步调整
main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME + "_restsdv1981";
   var clName = COMMCLNAME + "_restsdv1981";

   try
   {
      ready( csName );
      createCS( csName );
      createCL( csName, clName );

      var recs = [{ a: "20151023", b: 1, c: 2.3, d: { o: true } }];
      insertRecs( csName, clName, recs );
      updateRecs( csName, clName, recs );
      deleteRecs( csName, clName, recs );
      upsertRecs( csName, clName, recs );
      queryAndUpdateRecs( csName, clName, recs );
      queryAndRemoveRecs( csName, clName, recs );
      countCL( csName, clName, recs.length );

      dropCL( csName, clName );
      dropCS( csName );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
   }
}

function insertRecs ( csName, clName, recs )
{
   println( "\n---Begin to excute " + getFuncName() );

   for( var i in recs )
   {
      var recStr = JSON.stringify( recs[i] );

      var curlPara = ['cmd=insert',
         'name=' + csName + '.' + clName,
         'insertor=' + recStr];
      var expErrno = 0;
      var curlInfo = runCurl( curlPara, expErrno );
   }

   checkByQuery( csName, clName, recs );
}

function updateRecs ( csName, clName, recs )
{
   println( "\n---Begin to excute " + getFuncName() );

   var curlPara = ['cmd=update',
      'name=' + csName + '.' + clName,
      'updator={$set:{b:null}}',
      'filter={a:"20151023"}'];
   var expErrno = 0;
   var curlInfo = runCurl( curlPara, expErrno );

   recs[0]["b"] = null;
   checkByQuery( csName, clName, recs );
}

function deleteRecs ( csName, clName, recs )
{
   println( "\n---Begin to excute " + getFuncName() );

   var curlPara = ['cmd=delete',
      'name=' + csName + '.' + clName];
   var expErrno = 0;
   var curlInfo = runCurl( curlPara, expErrno );

   recs.splice( 0, recs.length );        //remove all elements
   checkByQuery( csName, clName, recs );
}

function upsertRecs ( csName, clName, recs )
{
   println( "\n---Begin to excute " + getFuncName() );

   var curlPara = ['cmd=upsert',
      'name=' + csName + '.' + clName,
      'updator={$set:{b:1}}',
      'filter={a:"20151023"}'];
   var expErrno = 0;
   var curlInfo = runCurl( curlPara, expErrno );

   recs.push( { a: "20151023", b: 1 } );
   checkByQuery( csName, clName, recs );
}

function queryAndUpdateRecs ( csName, clName, recs )
{
   println( "\n---Begin to excute " + getFuncName() );

   var curlPara = ['cmd=queryandupdate',
      'name=' + csName + '.' + clName,
      'updator={$inc:{b:9}}',
      'filter={a:"20151023"}'];
   var expErrno = 0;
   var curlInfo = runCurl( curlPara, expErrno );

   recs[0]["b"] += 9;
   checkByQuery( csName, clName, recs );
}

function queryAndRemoveRecs ( csName, clName, recs )
{
   println( "\n---Begin to excute " + getFuncName() );

   var curlPara = ['cmd=queryandremove',
      'name=' + csName + '.' + clName,
      'filter={a:"20151023"}'];
   var expErrno = 0;
   var curlInfo = runCurl( curlPara, expErrno );

   recs.splice( 0, recs.length );        //remove all elements
   checkByQuery( csName, clName, recs );
}

function dropCL ( csName, clName )
{
   println( "\n---Begin to excute " + getFuncName() );

   var curlPara = ["cmd=drop collection", "name=" + csName + '.' + clName];
   var expErrno = 0;
   var curlInfo = runCurl( curlPara, expErrno );

   var curlPara = ["cmd=drop collection", "name=" + csName + '.' + clName];
   var expErrno = -23;  //cl is not existed
   var curlInfo = runCurl( curlPara, expErrno );
}

function dropCS ( csName )
{
   println( "\n---Begin to excute " + getFuncName() );

   var curlPara = ["cmd=drop collectionspace", "name=" + csName]
   var expErrno = 0;
   var curlInfo = runCurl( curlPara, expErrno );

   var curlPara = ["cmd=drop collectionspace", "name=" + csName]
   var expErrno = -34;  //cs is not existed
   var curlInfo = runCurl( curlPara, expErrno );
}

function ready ( csName )
{
   println( "\n---Begin to excute " + getFuncName() );

   commDropCS( db, csName, true, "drop cs in begin" );
}