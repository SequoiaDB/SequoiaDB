package com.sequoiadb.fulltext.largedata;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;

/**
 * @Description seqDB-11989:range切分表中创建/删除全文索引
 * @author yinzhen
 * @date 2018/11/13
 */
public class Fulltext11989 extends FullTestBase {
    private String clName = "rangeTableIndex11989";
    private String fullIndexName = "fullIndex11989";
    private List< String > groupNames;
    private String cappedName;
    private String esIndexName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( IGNOREONEGROUP, "true" );

        // 创建 range 切分表并切分
        groupNames = CommLib.getDataGroupNames( sdb );
        caseProp.setProperty( CLNAME, clName );
        caseProp.setProperty( CLOPT,
                "{ShardingKey:{a:1}, ShardingType:'range', Group:'"
                        + groupNames.get( 0 ) + "'}" );
    }

    @Override
    protected void caseInit() throws Exception {
        cl.split( groupNames.get( 0 ), groupNames.get( 1 ),
                ( BSONObject ) JSON.parse( "{a:'a0'}" ),
                ( BSONObject ) JSON.parse( "{a:'a1000'}" ) );
    }

    @Test
    public void test() throws Exception {
        // 插入数据
        FullTextDBUtils.insertData( cl, FullTextUtils.INSERT_NUMS );

        // 创建全文索引，索引字段覆盖：非分区键、分区键
        cl.createIndex( fullIndexName,
                "{\"a\":\"text\",\"b\":\"text\",\"c\":\"text\",\"d\":\"text\",\"e\":\"text\"}",
                false, false );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIndexName,
                FullTextUtils.INSERT_NUMS ) );
        DBCollection cappedCL = FullTextDBUtils
                .getCappedCLs( cl, fullIndexName ).get( 0 );
        Assert.assertFalse( cappedCL.query().hasNext() );

        // 删除全文索引
        cappedName = FullTextDBUtils.getCappedName( cl, fullIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIndexName );
        FullTextDBUtils.dropFullTextIndex( cl, fullIndexName );
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
        Assert.assertTrue( FullTextUtils.isCLDataConsistency( cl ) );
    }

    @Override
    protected void caseFini() throws Exception {
        if ( !esIndexName.isEmpty() && !cappedName.isEmpty() ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedName ) );
        }
    }
}
