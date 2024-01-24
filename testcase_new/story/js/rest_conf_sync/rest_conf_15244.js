/******************************************************************************
 * @Description : test update and delete run config 
 *                seqDB-15244:更新删除节点配置
 * @author      : Fan YU
 * @date        ：2018.10.17
 ******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   //get a group in cluster
   var groupName = commGetGroups( db )[0][0].GroupName;
   //get a node of the group in cluster
   var nodePort = commGetGroupNodes( db, groupName )[0].svcname;
   //configure need to update and delete
   var config = "{auditnum:25}";
   //the options of snapshot and configure
   var options = "{GroupName:\"" + groupName + "\",SvcName:\"" + nodePort + "\"}";
   var selector = "{auditnum:" + null + "}";
   //delete configure
   deleteConf( config, options );
   //get the default value of auditnum
   var defval = getCurrentConfBySnapShot( options, selector );

   //update configure
   updateConf( config, options );
   //get current configure value after update configure
   var actval = getCurrentConfBySnapShot( options, selector );
   //check result
   checkResult( config, actval );

   //delete configure
   deleteConf( config, options );
   //get current configure value after delete configure
   var actval1 = getCurrentConfBySnapShot( options, selector );
   //check result
   checkResult( defval, actval1 );
}

function updateConf ( configs, options )
{
   var curlPara = ['cmd=update config', 'configs=' + configs, 'options=' + options];
   var expErrno = 0;
   runCurl( curlPara );
   var resp = infoSplit;
   var actErrno = JSON.parse( resp[0] ).errno;
   if( expErrno != actErrno )
   {
      throw new Error( "update Configure failed,info = " + infoSplit.toString() );
   }
}

function deleteConf ( configs, options )
{
   var curlPara = ['cmd=delete config', 'configs=' + configs, 'options=' + options];
   var expErrno = 0;
   runCurl( curlPara );
   var resp = infoSplit;
   var actErrno = JSON.parse( resp[0] ).errno;
   if( expErrno != actErrno )
   {
      throw new Error( "delete Configure failed,info = " + infoSplit.toString() );
   }
}

function getCurrentConfBySnapShot ( options, selector )
{
   var curlPara = ['cmd=snapshot configs', "filter=" + options, "selector=" + selector];
   var expErrno = 0;
   runCurl( curlPara );
   var resp = infoSplit;
   var actErrno = JSON.parse( resp[0] ).errno;
   if( expErrno != actErrno )
   {
      throw new Error( "get configures by snapshot failed,info = " + infoSplit.toString() );
   }
   resp.shift();
   return resp;
}

function checkResult ( expInfo, actInfo )
{
   var expVal = JSON.parse( expInfo ).auditnum;
   var actVal = JSON.parse( actInfo ).auditnum;
   if( expVal != actVal )
   {
      throw new Error( "expVal is not equal to actVal,expInfo = " + expInfo + ",actInfo = " + actInfo );
   }
}

