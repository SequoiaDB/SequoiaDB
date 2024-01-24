package com.sequoiadb.transaction.uniqueIndex;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-22089:插入2条记录，在事务中使用inc更新3次，创建唯一索引
 * @date 2020-4-16
 * @author zhaoyu
 *
 */
public class Transaction22089 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl22089";
    private String idxName = "idx22089";
    private DBCollection cl = null;
    private CollectionSpace cs = null;
    private ArrayList< BSONObject > expList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName );
        BSONObject record = ( BSONObject ) JSON.parse( "{a:1,b:1}" );
        expList.add( record );
        record = ( BSONObject ) JSON.parse( "{a:2,b:2}" );
        expList.add( record );
        cl.insert( expList );
    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
        sdb.close();
    }

    @Test
    public void test() throws Exception {
        try {
            TransUtils.beginTransaction( sdb );
            for ( int i = 0; i < 3; i++ ) {
                cl.update( "", "{$inc:{a:1}}", "" );
            }
            cl.createIndex( idxName, "{a:1}", true, true );
        } finally {
            sdb.rollback();

        }
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList );
        Assert.assertTrue( TransUtils.isCLConsistency( cl ) );
        sdb.setSessionAttr(
                ( BSONObject ) JSON.parse( "{PreferedInstance:'s'}" ) );

        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList );
    }
}
