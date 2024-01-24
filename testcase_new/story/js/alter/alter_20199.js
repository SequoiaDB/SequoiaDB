/************************************
*@Description: seqDB-20199 alterCS修改Domain
*@author:      luweikang
*@createDate:  2019.11.05
**************************************/

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   var groupNames = getGroupName( db );
   if( 1 === groupNames.length )
   {
      return;
   }

   var csName = "cs20199";
   var clName = "cl20199";
   var domainName = "domain20199";

   commDropCS( db, csName );
   commDropDomain( db, domainName );

   commCreateDomain( db, domainName, [groupNames[0], groupNames[1]] );
   var cs = commCreateCS( db, csName, false, "create CS" );
   commCreateCL( db, csName, clName, { Group: groupNames[0] } );

   cs.alter( { Domain: domainName } );
   checkAlterCSResult( csName, "Domain", domainName );

   commDropCS( db, csName );
   commDropDomain( db, domainName );
}