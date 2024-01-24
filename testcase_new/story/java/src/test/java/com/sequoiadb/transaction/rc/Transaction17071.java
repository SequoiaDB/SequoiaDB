package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17071:同一事务下，增删改操作后读记录
 * @date 2019-1-17
 * @author yinzhen
 *
 */
@Test(groups = { "ru", "rc", "rcwaitlock" })
public class Transaction17071 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl17071";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'textIndex17071'}";

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "textIndex17071", "{a:1}", false, false );
    }

    @AfterClass
    public void tearDown() {
        sdb.commit();
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启事务插入记录R1
        TransUtils.beginTransaction( sdb );
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        cl.insert( record );
        expList.add( record );

        // 事务中查询记录
        TransUtils.queryAndCheck( cl, hintTbScan, expList );
        TransUtils.queryAndCheck( cl, hintIxScan, expList );

        // 更新记录
        cl.update( "", "{$inc:{a:1}}", "{'':'textIndex17071'}" );

        // 事务中查询记录
        record = ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" );
        expList.clear();
        expList.add( record );
        TransUtils.queryAndCheck( cl, hintTbScan, expList );
        TransUtils.queryAndCheck( cl, hintIxScan, expList );

        // 删除记录
        cl.delete( "", "{'':'a'}" );

        // 事务中查询记录
        expList.clear();
        TransUtils.queryAndCheck( cl, hintTbScan, expList );
        TransUtils.queryAndCheck( cl, hintIxScan, expList );

        // 事务提交
        sdb.commit();

        // 读记录走表扫描
        TransUtils.queryAndCheck( cl, hintTbScan, expList );

        // 读记录走索引扫描
        TransUtils.queryAndCheck( cl, "{a:{$exists:1}}", null, null, hintIxScan,
                expList );
    }
}
