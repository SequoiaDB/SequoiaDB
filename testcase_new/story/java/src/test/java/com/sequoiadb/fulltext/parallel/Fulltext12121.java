package com.sequoiadb.fulltext.parallel;

import java.util.List;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-12121:并发删除同一条记录
 * @Author yinzhen
 * @Date 2019-4-28
 */
public class Fulltext12121 extends FullTestBase {
    private String clName = "cl12121";
    private String fullIdxName = "idx12121";
    private String esIndexName;
    private String cappedCLName;
    private int insertNum = 20000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        // 创建全文索引
        cl.createIndex( fullIdxName,
                "{'a':'text','b':'text','c':'text', 'd':'text', 'e':'text', 'f':'text'}",
                false, false );
        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIdxName );
        cappedCLName = FullTextDBUtils.getCappedName( cl, fullIdxName );
        FullTextDBUtils.insertData( cl, insertNum );
        cl.insert( "{a:'idx12121', b:'b12121'}" );
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, fullIdxName, 20001 ) );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        for ( int i = 0; i < 10; i++ ) {
            thExecutor.addWorker( new DeleteRecord() );
        }
        thExecutor.run();

        // 固定集合中新增一条操作类型为删除，值正确的记录
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, fullIdxName, insertNum ) );
        DBCollection cappedCL = FullTextDBUtils.getCappedCLs( cl, fullIdxName )
                .get( 0 );
        List< BSONObject > records = FullTextDBUtils
                .getReadList( cappedCL.query() );
        Assert.assertEquals( records.get( 0 ).get( "Type" ), 2 );

        // 全文检索校验
        FullTextUtils.isRecordEqualsByMulQueryMode( cl );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
    }

    private class DeleteRecord {
        @ExecuteOrder(step = 1, desc = "多线程并发删除同一条包含全文索引字段的记录")
        private void deleteRecord() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.delete( "{a:'idx12121', b:'b12121'}",
                        "{'':'" + fullIdxName + "'}" );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }
}