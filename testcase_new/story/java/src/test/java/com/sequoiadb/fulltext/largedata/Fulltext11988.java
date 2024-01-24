package com.sequoiadb.fulltext.largedata;

import java.util.List;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;

/**
 * @Description seqDB-11988:hash切分表加入域使用自动切分，创建/删除全文索引
 * @author yinzhen
 * @date 2018/11/13
 */
public class Fulltext11988 extends FullTestBase {
    private String clName = "hashTableIndex11988";
    private String fullIndexName = "fullIndex11988";
    private List< String > groupNames;
    private String csName = "cs11988";
    private String doMainName = "doMain11988";
    private String esIndexName;
    private String cappedCLName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( IGNOREONEGROUP, "true" );

        groupNames = CommLib.getDataGroupNames( sdb );
        caseProp.setProperty( DOMAIN, doMainName );
        caseProp.setProperty( DOMAINOPT, "{Groups:['" + groupNames.get( 0 )
                + "', '" + groupNames.get( 1 ) + "']}" );

        // hash 切分表加入域使用自动切分
        caseProp.setProperty( CSNAME, csName );
        caseProp.setProperty( CSOPT, "{Domain:'" + doMainName + "'}" );

        caseProp.setProperty( CLNAME, clName );
        caseProp.setProperty( CLOPT,
                "{ShardingKey:{a:1}, ShardingType:'hash', AutoSplit:true}" );
    }

    @Test
    public void test() throws Exception {
        FullTextDBUtils.insertData( cl, FullTextUtils.INSERT_NUMS );

        // 创建全文索引，索引字段覆盖：分区键、非分区键
        String indexKey = "{\"a\":\"text\",\"b\":\"text\",\"c\":\"text\",\"d\":\"text\",\"e\":\"text\",\"g\":\"text\"}";
        cl.createIndex( fullIndexName, indexKey, false, false );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIndexName,
                FullTextUtils.INSERT_NUMS ) );
        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIndexName );
        cappedCLName = FullTextDBUtils.getCappedName( cl, fullIndexName );

        // 删除索引
        FullTextDBUtils.dropFullTextIndex( cl, fullIndexName );
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
    }
}
