/******************************************************************************
 * @Description   : seqDB-26321:getDomainName接口验证
 * @Author        : ZhangYanan
 * @CreateTime    : 2022.04.02
 * @LastEditTime  : 2022.04.02
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var domainName = "domain_26321";
   var csName1 = "cs_26321_1";
   var csName2 = "cs_26321_2";
   var options = { "Domain": domainName };

   //清理集合、创建domain
   commDropCS( db, csName1, true, "clear cs in the beginning." );
   commDropCS( db, csName2, true, "clear cs in the beginning." );
   createDomain( domainName );

   db.createCS( csName1, options );
   var cs2 = db.createCS( csName2 );

   tryCatch( ["cmd=get domain name", "name=" + csName1], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).Domain, domainName, infoSplit );

   tryCatch( ["cmd=get domain name", "name=" + csName2], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).Domain, null, infoSplit );

   cs2.setDomain( options );
   tryCatch( ["cmd=get domain name", "name=" + csName2], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).Domain, domainName, infoSplit );

   commDropCS( db, csName1, false, "clear cs in the beginning." );
   commDropCS( db, csName2, false, "clear cs in the beginning." );
   db.dropDomain( domainName );
}

function createDomain ( domainName )
{
   try
   {
      db.dropDomain( domainName );
   } catch( e )
   {
      if( e != SDB_CAT_DOMAIN_NOT_EXIST )
      {
         throw new Error( e );
      }
   }
   var groupName = testPara.groups[0][0].GroupName;

   db.createDomain( domainName, [groupName] );
}