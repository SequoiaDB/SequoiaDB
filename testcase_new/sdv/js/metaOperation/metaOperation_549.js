/************************************
*@Description：seqDB-549:创建CL且指定组不在所属域内
*@Author：     Zhao Xiaoni 2020-5-11
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );

function test()
{
   var csName = "cs_549";
   var clName = "cl_549";
   var domainName = "domain_549";
   commDropCS( db, csName );
   commDropDomain( db, domainName );
   var groups = commGetGroups( db );
   commCreateDomain( db, domainName, [ groups[0][0].GroupName ] );
   var cs = commCreateCS( db, csName, undefined, undefined, { Domain: domainName } );
   
   try 
   {
      cs.createCL( clName, { Group: groups[1][0].GroupName } );
      throw "create cl should be failed!";
   }
   catch( e )
   {
      if( e !== -216 )
      {
         throw new Error( e );
      }
   }

   commDropCS( db, csName, false );
   commDropDomain( db, domainName, false );
}
