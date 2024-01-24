/************************************
*@Description: Domain存在cs执行addGroups/setGroups/removeGroups修改组
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15062，seqDB-15063，seqDB-15064
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
   var csName = CHANGEDPREFIX + "_cs_15062";
   var clName = CHANGEDPREFIX + "_cl_15062";
   var domainName = CHANGEDPREFIX + "_domain_15062";
   var group1 = allGroupName[0];
   var group2 = allGroupName[1];
   var group3 = allGroupName[2];

   commDropDomain( db, domainName );
   var domain = commCreateDomain( db, domainName, [group1, group2], { AutoSplit: true } );
   db.createCS( csName, { Domain: domainName } )
   var clOption = { ShardingKey: { a: 1 }, ShardingType: 'hash' };
   commCreateCL( db, csName, clName, clOption, true, true );

   domain.addGroups( { Groups: [group3] } );
   checkDomain( db, domainName, [group1, group2, group3], true, undefined );

   //delete domainGroup where cl all group
   domainRemoveGroups( domain, { Groups: [group1, group2] } );
   //delete domainGroup where cl soma group
   domainRemoveGroups( domain, { Groups: [group1, group3] } );
   checkDomain( db, domainName, [group1, group2, group3], true, undefined );
   //delete domainGroup where cl not in group
   domain.removeGroups( { Groups: [group3] } );
   checkDomain( db, domainName, [group1, group2], true, undefined );

   //alter domain to all group
   domain.setAttributes( { Groups: [group1, group2, group3] } );
   checkDomain( db, domainName, [group1, group2, group3], true, undefined );
   //alter domain where cl not in group
   domainSetAtt( domain, { Groups: [group3] } );
   //alter domain where cl not all in group
   domainSetAtt( domain, { Groups: [group1, group3] } )
   checkDomain( db, domainName, [group1, group2, group3], true, undefined );

   db.dropCS( csName );
   commDropDomain( db, domainName );
}

function domainSetAtt ( domain, alterOption )
{
   assert.tryThrow( SDB_DOMAIN_IS_OCCUPIED, function()
   {
      domain.setAttributes( alterOption );
   } );
}

function domainRemoveGroups ( domain, groups )
{
   assert.tryThrow( SDB_DOMAIN_IS_OCCUPIED, function()
   {
      domain.removeGroups( groups );
   } );
}