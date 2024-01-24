/******************************************************************************
@Description : jira1205 seqDB-9478 domain.alter不检查group上是否存在数据
@author :Shitong Wen
               
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var group = getGroup( db );
   if( group.length <= 1 )
   {
      return;
   }

   var newCS = CHANGEDPREFIX + '_newcs';
   var newCL = CHANGEDPREFIX + '_newcl';
   var domainName = CHANGEDPREFIX + '_domain';

   // Drop CS and domain in the beginning
   commDropDomain( db, domainName );

   //create  domain cs cl 
   commCreateDomain( db, domainName, [group[0], group[1]] );
   commCreateCS( db, newCS, false, "create CS specify domain",
      { "Domain": domainName } );
   db.getCS( newCS ).createCL( newCL, { Group: group[0] } );

   assert.tryThrow( SDB_DOMAIN_IS_OCCUPIED, function()
   {
      db.getDomain( domainName ).alter( { Groups: [group[1]] } );
   } );

   //split data
   for( var i = 0; i < 100; i++ ) { db.getCS( newCS ).getCL( newCL ).insert( { key: i } ) };
   db.getCS( newCS ).getCL( newCL ).alter( { ShardingKey: { key: 1 }, ShardingType: "hash" } );
   db.getCS( newCS ).getCL( newCL ).split( group[0], group[1], 50 );

   assert.tryThrow( SDB_DOMAIN_IS_OCCUPIED, function()
   {
      db.getDomain( domainName ).alter( { Groups: [group[0]] } );
   } );

   commDropDomain( db, domainName );
}

// Get group from Sdb
function getGroup ( db )
{
   var listGroups = db.listReplicaGroups();
   var groupArray = new Array();
   while( listGroups.next() )
   {
      if( listGroups.current().toObj()['GroupID'] >= DATA_GROUP_ID_BEGIN )
      {
         groupArray.push( listGroups.current().toObj()["GroupName"] );
      }
   }
   return groupArray;
}