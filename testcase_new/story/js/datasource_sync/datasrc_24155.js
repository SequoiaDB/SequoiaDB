/******************************************************************************
 * @Description   : seqDB-24155:源集群设置会话访问属性，设置preferedPeriod属性
 * @Author        : Wu Yan
 * @CreateTime    : 2021.05.06
 * @LastEditTime  : 2021.06.16
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var dataSrcName = "datasrc24155";
   var clName = CHANGEDPREFIX + "_datasource24155";
   var srcCSName = "datasrcCS_24155";
   var csName = "DS_24155";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   var groups = commGetGroups( datasrcDB )[0];;
   var groupName = groups[0].GroupName;
   var primaryPos = groups[0].PrimaryPos;
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ReplSize: -1, Group: groupName } );

   var cs = db.createCS( csName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];

   var expAccessNodeS = [];
   var expAccessNodeM = [];
   for( var i = 1; i < groups.length; i++ )
   {
      if( i !== primaryPos )
      {
         expAccessNodeS.push( groups[i]["HostName"] + ":" + groups[i]["svcname"] );
      }
      else
      {
         expAccessNodeM.push( groups[i]["HostName"] + ":" + groups[i]["svcname"] );
      }
   }

   //preferedPeriod为0     
   db.setSessionAttr( { PreferedInstance: "s", PreferedPeriod: 0 } );
   dbcl.insert( docs );
   // 数据源性能较差，直接访问可能未生效，sleep 5秒后再访问
   sleep( 5000 );
   findAndCheckAccessNodes( dbcl, expAccessNodeS );

   //preferedPeriod为5  
   db.setSessionAttr( { PreferedPeriod: 5 } );
   dbcl.insert( docs );
   findAndCheckAccessNodes( dbcl, expAccessNodeM );
   //5s后检查会话访问节点为备节点,时间是一个cpu的tick，不精准，改成10s后查看
   sleep( 10000 );
   findAndCheckAccessNodes( dbcl, expAccessNodeS );

   //preferedPeriod为-1
   db.setSessionAttr( { PreferedPeriod: -1 } )
   dbcl.insert( docs );
   findAndCheckAccessNodes( dbcl, expAccessNodeM );
   //5s后检查会话访问节点还是主节点
   sleep( 5000 );
   findAndCheckAccessNodes( dbcl, expAccessNodeM );

   //preferedPeriod为60
   db.setSessionAttr( { PreferedPeriod: 60 } )
   dbcl.insert( docs );
   findAndCheckAccessNodes( dbcl, expAccessNodeM );
   //60s后检查会话访问节点为备节点
   sleep( 65000 );
   findAndCheckAccessNodes( dbcl, expAccessNodeS );


   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   commDropCL( db, csName, clName, false, false );
   clearDataSource( csName, dataSrcName );
}

