/******************************************************************************
 * @Description   : seqDB-23851:域上有回收站，域中增加新的数据组
 * @Author        : liuli
 * @CreateTime    : 2021.04.19
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23851";
   var clName = "cl_23851";
   var domainName = "domain_23851";
   var groupNames = commGetDataGroupNames( db );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropDomain( db, domainName );

   var domain = db.createDomain( domainName, [groupNames[0]], { AutoSplit: true } );
   var dbcs = db.createCS( csName, { Domain: domainName } );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true } );
   for( var i = 0; i < 1000; i++ ) 
   {
      dbcl.insert( { a: i } );
   }

   // CL执行truncate，domain新增group
   dbcl.truncate();
   domain.addGroups( { Groups: groupNames[1] } );
   var groups = [groupNames[0], groupNames[1]];
   checkDomain( domain, groups );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );
   checkRecycleItem( recycleName );

   // 恢复后CL没有数据在新增group中
   var data = db.getRG( groupNames[1] ).getMaster().connect();
   assert.tryThrow( [SDB_DMS_CS_NOTEXIST], function() 
   {
      data.getCS( csName ).getCL( clName ).find().toArray();
   } );
   data.close();

   commDropCS( db, csName );
   commDropDomain( db, domainName );
   cleanRecycleBin( db, csName );
}

function checkDomain ( domain, groups )
{
   var group = "";
   var listDom = domain.listGroups();
   while( listDom.next() )
   {
      domainGroups = listDom.current().toObj()["Groups"];
      for( var i = 0; i < domainGroups.length; i++ )
      {
         group = domainGroups[i]["GroupName"];
         if( groups.indexOf( group ) < 0 )
         {
            throw new Error( "Group error in domain" );
         }
      }
   }
}