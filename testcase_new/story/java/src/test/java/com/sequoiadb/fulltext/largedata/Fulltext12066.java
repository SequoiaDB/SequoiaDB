package com.sequoiadb.fulltext.largedata;

import java.util.List;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;

/**
 * @Description seqDB-12066: 集合上存在全文索引，删除集合
 * @author yinzhen
 * @date 2018/11/20
 */
public class Fulltext12066 extends FullTestBase {
    private CollectionSpace cs;
    private String clName = "cl12066";
    private String fullIndexName = "fullIndex12066";
    private String cappedName;
    private String esIndexName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        cs = sdb.getCollectionSpace( csName );
    }

    @Test
    public void test() throws Exception {
        // 在集合上创建1个全文索引，并插入大量包含索引字段的数据
        cl.createIndex( fullIndexName,
                "{\"a\":\"text\",\"b\":\"text\",\"c\":\"text\",\"d\":\"text\",\"e\":\"text\",\"f\":\"text\"}",
                false, false );
        FullTextDBUtils.insertData( cl, FullTextUtils.INSERT_NUMS );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIndexName,
                FullTextUtils.INSERT_NUMS ) );
        cappedName = FullTextDBUtils.getCappedName( cl, fullIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIndexName );

        // 直连主数据节点使用游标的方式获取固定集合中的一条记录
        List< DBCollection > cappedCLs = FullTextDBUtils.getCappedCLs( cl,
                fullIndexName );
        DBCollection cappedCL = cappedCLs.get( 0 );
        DBCursor cursor = cappedCL.query();
        cursor.getNext();

        // 删除集合
        cs.dropCollection( clName );
        Assert.assertFalse( cs.isCollectionExist( clName ) );
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }
}
