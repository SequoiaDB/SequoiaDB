package com.sequoiadb.fulltext.largedata;

/**
 * @Description seqDB-12021: range切分表中创建全文索引并切分后再插入记录
 * @author xiaoni Zhao
 * @date 2018/11/23
 */
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;

public class Fulltext12021 extends FullTestBase {
    private String clName = "ES_cl_12021";
    private String fullTextIndexName = "fullIndex12021";
    private String srcGroup = null;
    private String desGroup = null;
    private String cappedName = null;
    private String esIndexName = null;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( IGNOREONEGROUP, "true" );

        List< String > groupsName = CommLib.getDataGroupNames( sdb );
        srcGroup = groupsName.get( 0 );
        desGroup = groupsName.get( 1 );
        caseProp.setProperty( CLNAME, clName );
        caseProp.setProperty( CLOPT,
                "{ShardingType:'range', ShardingKey:{recordId:1, a:1}, Group:'"
                        + srcGroup + "'}" );
    }

    @Test
    public void test() throws Exception {
        cl.createIndex( fullTextIndexName,
                ( BSONObject ) JSON.parse(
                        "{a : 'text', b : 'text', c : 'text', d : 'text'}" ),
                false, false );
        cappedName = FullTextDBUtils.getCappedName( cl, fullTextIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, fullTextIndexName );
        int desRange = 100000;
        cl.split( srcGroup, desGroup,
                ( BSONObject ) JSON.parse( "{recordId:0}" ),
                ( BSONObject ) JSON.parse( "{recordId:" + desRange + "}" ) );
        FullTextDBUtils.insertData( cl, FullTextUtils.INSERT_NUMS );
        checkData( desGroup, "{recordId:{$gte:0,$lt:" + desRange + "}}",
                desRange );
        checkData( srcGroup,
                "{recordId:{$gte:" + desRange + ",$lt:"
                        + FullTextUtils.INSERT_NUMS + "}}",
                ( FullTextUtils.INSERT_NUMS - desRange ) );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullTextIndexName,
                FullTextUtils.INSERT_NUMS ) );
    }

    public void checkData( String group, String matcher, int expectedCount ) {
        Sequoiadb dataDb = null;
        DBCollection cl = null;
        long count;
        try {
            dataDb = sdb.getReplicaGroup( group ).getMaster().connect();
            cl = dataDb.getCollectionSpace( csName ).getCollection( clName );
            count = cl.getCount( matcher );
            Assert.assertEquals( count, expectedCount );
        } finally {
            dataDb.close();
        }
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }
}
