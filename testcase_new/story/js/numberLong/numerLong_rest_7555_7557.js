/******************************************************************************
 * @Description   : seqDB-7555:rest_输入strict格式，查询显示
 *                  seqDB-7557:rest_strict格式的边界值校验
 * @Author        : Ting YU
 * @CreateTime    : 2021.11.25
 * @LastEditTime  : 2022.05.21
 * @LastEditors   : liuli
 ******************************************************************************/

// 开发修改导致预期结果出错，用例先跳过独立模式，待问题单修改后再做调整。SEQUOIADBMAINSTREAM-8004
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var clName = COMMCLNAME + "_7555";
   commDropCL( db, COMMCSNAME, clName );
   commCreateCL( db, COMMCSNAME, clName );

   var recsInBoundary = [];
   recsInBoundary.push( { _id: 2, a: { $numberLong: "-9223372036854775808" } } ); //long min
   recsInBoundary.push( { _id: 3, a: { $numberLong: "9223372036854775807" } } ); //long max
   insert( COMMCSNAME, clName, recsInBoundary );

   var recsOutBoundary = [];
   recsOutBoundary.push( { _id: 1, a: { $numberLong: "-9223372036854775809" } } ); //out of min value
   recsOutBoundary.push( { _id: 4, a: { $numberLong: "9223372036854775808" } } ); //out of max value
   insertErrorRec( COMMCSNAME, clName, recsOutBoundary );

   var expRecs = [];
   expRecs.push( "{ \"_id\": 2, \"a\": -9223372036854775808 }" );
   expRecs.push( "{ \"_id\": 3, \"a\": 9223372036854775807 }" );
   checkByQuery( COMMCSNAME, clName, expRecs );
   commDropCL( db, COMMCSNAME, clName );
}

function insert ( csName, clName, recs )
{
   for( var i in recs )
   {
      var recStr = JSON.stringify( recs[i] );

      var curlPara = ['cmd=insert',
         'name=' + csName + '.' + clName,
         'insertor=' + recStr];
      var expErrno = 0;
      var curlInfo = runCurl( curlPara, expErrno );
   }
}

function insertErrorRec ( csName, clName, recs )
{
   for( var i in recs )
   {
      var recStr = JSON.stringify( recs[i] );

      var curlPara = ['cmd=insert',
         'name=' + csName + '.' + clName,
         'insertor=' + recStr];
      var expErrno = -6;
      var curlInfo = runCurl( curlPara, expErrno );
   }
}

function checkByQuery ( csName, clName, expRecs )
{
   //run query command
   var curlPara = ['cmd=query',
      'name=' + csName + '.' + clName,
      'sort={_id:1}'];
   var expErrno = 0;
   var curlInfo = runCurl( curlPara, expErrno );
   var actRecs = curlInfo.rtnJsn;

   //check count
   if( actRecs.length !== expRecs.length )
   {
      throw new Error( "actRecs.length: " + actRecs.length + "expRecs.length: " + expRecs.length );
   }

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            throw new Error( "actRec: " + actRec + "\nexpRec: " + expRec );
         }
      }
   }
}
