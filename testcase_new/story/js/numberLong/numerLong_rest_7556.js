/*******************************************************************************
*@Description : seqDB-7556:rest_strict格式的参数校验
*@Modify List : 2016-3-28  Ting YU  Init
*******************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_7556";
   commDropCL( db, COMMCSNAME, clName );
   commCreateCL( db, COMMCSNAME, clName );

   var recs = [{ a: { $numberLong: "123a" } }];
   istErrFormat( COMMCSNAME, clName, recs );

   var recs = [{ a: { $numberLong: "1.1" } }];
   istErrFormat( COMMCSNAME, clName, recs );

   checkByQuery( COMMCSNAME, clName, [] );
   commDropCL( db, COMMCSNAME, clName );
}

function istErrFormat ( csName, clName, recs )
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
      'sort={a:1}'];
   var expErrno = 0;
   var curlInfo = runCurl( curlPara, expErrno );
   var actRecs = curlInfo.rtnJsn;

   //check count
   if( actRecs.length !== expRecs.length )
   {
      throw new Error( "actRecs.length: " + actRecs.length + "\nexpRecs.length: " + expRecs.length );
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
