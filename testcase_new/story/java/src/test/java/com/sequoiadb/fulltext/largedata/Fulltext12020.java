package com.sequoiadb.fulltext.largedata;

/**
 * @Description seqDB-12020:hash切分表中创建全文索引并切分后再插入记录
 * @author xiaoni Zhao
 * @date 2018/11/23
 */
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;

public class Fulltext12020 extends FullTestBase {
    private String clName = "ES_cl_12020";
    private String fullTextIndexName = "fullIndex12020";
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
                "{ShardingType:'hash', ShardingKey:{a:1}, Group:'" + srcGroup
                        + "'}" );
    }

    @Test
    public void test() throws Exception {
        cl.createIndex( fullTextIndexName,
                ( BSONObject ) JSON.parse(
                        "{a : 'text', b : 'text', c : 'text', d : 'text'}" ),
                false, false );
        cappedName = FullTextDBUtils.getCappedName( cl, fullTextIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, fullTextIndexName );
        cl.split( srcGroup, desGroup, 50 );
        FullTextDBUtils.insertData( cl, FullTextUtils.INSERT_NUMS );
        Assert.assertEquals( FullTextDBUtils.getCLGroups( cl ).size(), 2 );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullTextIndexName,
                FullTextUtils.INSERT_NUMS ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }
}
