/************************************
*@Description: 空Domain执行addGroups/setGroups/removeGroups修改组
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15059，seqDB-15060，seqDB-15061
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 2 >= allGroupName.length )
   {
      return;
   }
   var domainName = CHANGEDPREFIX + "_domain_15059";
   var group1 = allGroupName[0];
   var group2 = allGroupName[1];
   var group3 = allGroupName[2];

   commDropDomain( db, domainName );
   var domain = commCreateDomain( db, domainName, [group1] );

   domain.addGroups( { Groups: [group2, group3] } );
   checkDomain( db, domainName, [group1, group2, group3], undefined, undefined );

   domain.removeGroups( { Groups: [group1] } );
   checkDomain( db, domainName, [group2, group3], undefined, undefined );

   domain.setGroups( { Groups: [group1] } );
   checkDomain( db, domainName, [group1], undefined, undefined );

   domain.setGroups( { Groups: [group2] } );
   checkDomain( db, domainName, [group2], undefined, undefined );

   commDropDomain( db, domainName );
}