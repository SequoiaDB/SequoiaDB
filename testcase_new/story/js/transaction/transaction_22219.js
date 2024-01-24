/* *****************************************************************************
@discretion: seqDB-22219:事务中修改集合的分区属性，触发切分操作
*@author:      wuyan
*@createdate:  2020.5.26
***************************************************************************** */
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      return;
   }

   var csName = CHANGEDPREFIX + "_cs_22219";
   var clName1 = CHANGEDPREFIX + "_altercl_22219a";
   var clName2 = CHANGEDPREFIX + "_altercl_22219b";
   var clName3 = CHANGEDPREFIX + "_altercl_22219c";
   var domainName = "domain22219";
   commDropCL( db, COMMCSNAME, clName1, true, true, "drop CL in the beginning" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "drop CL in the beginning" );
   commDropCS( db, csName, true, "drop CS in the beginning." );
   commDropDomain( db, domainName );

   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2, { ShardingKey: { a: 1 } } );
   var groupName1 = groups[0][0].GroupName;
   var groupName2 = groups[1][0].GroupName;
   commCreateDomain( db, domainName, [groupName1, groupName2], { AutoSplit: true } );
   commCreateCS( db, csName, true, "create CS", { Domain: domainName } );
   var dbcl3 = commCreateCL( db, csName, clName3 );
   var dataNums = 10;
   alterSharding( dbcl1, dataNums );
   alterAutoSpilt( dbcl2, dataNums );
   enableSharding( dbcl3, dataNums );

   commDropCS( db, csName, true, "drop CS in the ending." );
   commDropDomain( db, domainName );
   commDropCL( db, COMMCSNAME, clName1, true, true, "drop CL in the ending" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "drop CL in the ending" );
}

function alterSharding ( dbcl, dataNums )
{
   db.transBegin();
   insertData( dbcl, dataNums );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.enableSharding( { ShardingKey: { a: 1 }, AutoSplit: true } );
   } );

   db.transCommit();
   checkCount( dbcl, dataNums );
}

function alterAutoSpilt ( dbcl, dataNums )
{
   db.transBegin();
   insertData( dbcl, dataNums );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.alter( { AutoSplit: true } );
   } );

   db.transCommit();
   checkCount( dbcl, dataNums );
}

function enableSharding ( dbcl, dataNums )
{
   db.transBegin();
   insertData( dbcl, dataNums );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.enableSharding( { ShardingKey: { a: 1 } } );
   } );

   db.transCommit();
   checkCount( dbcl, dataNums );
}