/************************************************************************
*@Description: 用Rest查询访问计划缓存快照
               对应testlink用例：eqDB-14512:获取访问计划快照
*@Author:      FanYu  2018/04/03
************************************************************************/



main( test );

function test ()
{
   var csName = COMMCSNAME + "_14512";
   var clName = COMMCLNAME + "_14512";

   commDropCS( db, csName, true, "drop cs in begin" );

   var curlPara = ['cmd=create collectionspace', 'name=' + csName];
   runCurl( curlPara );
   curlPara = ['cmd=create collection&name=' + csName + '.' + clName, 'options={ReplSize:-1}'];
   runCurl( curlPara );
   //insert 
   var recs = [];
   var totalCnt = 1000;
   for( var i = 0; i < totalCnt; i++ ) 
   {
      var rec = { NO: i, Flag: true, Index: "str_" + i.toString() };
      recs.push( rec );
   }
   var cl = db.getCS( csName ).getCL( clName );
   cl.insert( recs );

   var cond1 = "{NO:{$lt:100}}";
   var cond2 = "{NO:{$gte:500}}";
   var cond3 = "{$and:[{NO:{$lt:500}},{NO:{$gte:200}}]}"
   queryRecs( csName, clName, recs, cond1 );
   queryRecs( csName, clName, recs, cond2 );
   queryRecs( csName, clName, recs, cond3 );

   var actInfo = snapshot( csName, clName, "accessplans" );
   var expInfo = ["{\"MinTimeSpentQuery\":{\"ReturnNum\":100}}", "{\"MinTimeSpentQuery\":{\"ReturnNum\":300}}", "{\"MinTimeSpentQuery\":{\"ReturnNum\":500}}"];
   check( actInfo, expInfo );
   curlPara = ["cmd=drop collection&name=" + csName + '.' + clName];
   runCurl( curlPara );
   curlPara = ["cmd=drop collectionspace&name=" + csName]
   runCurl( curlPara );

   commDropCS( db, csName, true, "drop cs in begin" );
}

function queryRecs ( csName, clName, recs, cond )
{
   var curlPara = ['cmd=query&name=' + csName + '.' + clName + '&filter=' + cond];
   runCurl( curlPara );
}

function snapshot ( csName, clName, type )
{
   var curlPara = ['cmd=snapshot ' + type + '&filter={Collection:"' + csName + '.' + clName + '"}',
      'selector={MinTimeSpentQuery.ReturnNum:{$include:1}}&sort={MinTimeSpentQuery.ReturnNum:1}'];
   runCurl( curlPara );
   var resp = infoSplit;
   resp.shift();
   return resp;
}

function check ( actInfo, expInfo )
{
   if( actInfo.length != expInfo.length )
   {
      throw new Error( "actInfo.length: " + actInfo.length + "\nexpInfo.length: " + expInfo.length );
   }
   for( var i = 0; i < actInfo.length; i++ )
   {
      if( actInfo[i].replace( /\//g, "" ).replace( /\s+/g, "" ) != expInfo[i] )
      {
         throw new Error( "actInfo:" + JSON.stringify( actInfo[i] ) + "\nexpInfo:" + expInfo[i] );
      }
   }
}
