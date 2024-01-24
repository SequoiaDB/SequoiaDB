package com.sequoiadb.snapshot;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import org.testng.Assert;
import org.testng.annotations.Test;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.exception.BaseException;
/**
 * @Description: seqDB-22931: Java Driver realize new snapshot interface: SDB_SNAP_INDEXSTATS
 * @Author Zixian Yan
 * @Date 2020.10.20
 */
public class Snapshot22931 extends SdbTestBase {
   private Sequoiadb sdb;
   private DBCollection cl;
   private CollectionSpace cs;
   private List< String > groupNameList;
   private String groupName;
   private String clName = "cl_22931";
   private String indexName = "idx_22931";

   @BeforeClass
   public void setup(){
       sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
       cs  = sdb.getCollectionSpace(SdbTestBase.csName);

       groupNameList = CommLib.getDataGroupNames( sdb );
       groupName = groupNameList.get( 0 );

       cl  = cs.createCollection( clName, new BasicBSONObject( "Group", groupName ) );
       cl.insert( new BasicBSONObject( "a", 1) );
       cl.createIndex( indexName, "{a: 1}", true, false );
   }

   @Test
   public void test() throws Exception{
       DBCursor   cursor = null;
       BSONObject result = new BasicBSONObject();
       String cs_clName = csName + "." + clName;

       sdb.analyze( new BasicBSONObject( "Collection", cs_clName ) );

       Node masterNode = sdb.getReplicaGroup( groupName ).getMaster();
       String nodeName = masterNode.getNodeName();
       cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_INDEXSTATS,"{'Index': '"+ indexName
                               + "', 'NodeName':'" + nodeName + "'}", null, null );
       result = cursor.getCurrent();

       String result_collection = result.get( "Collection" ).toString();
       String result_indexName  = result.get( "Index" ).toString();
       String result_statInfo   = result.get( "StatInfo" ).toString();

       Assert.assertEquals( result_collection, cs_clName );
       Assert.assertEquals( result_indexName, indexName );

       if( result_statInfo.isEmpty() ){
          throw new Exception( "Failed!!!!, StatInfo is is Empty." );
       }
   }

   @AfterClass
   public void tearDown(){
      try{
         sdb.getCollectionSpace( csName ).dropCollection( clName );
      } finally{
         sdb.close();
      }
   }
}
