package com.sequoiadb.sdb;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-20053:interrupt()接口测试
 * @Author XiaoNi Huang
 * @Date 2019.10.22
 */

public class Interrupt20053 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl20053";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standalone." );
        }
        cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }

        cl = cs.createCollection( clName );
        List< BSONObject > insertor = new ArrayList<>();
        for ( int i = 0; i < 2000; i++ ) {
            insertor.add( new BasicBSONObject( "a", i ) );
        }
        cl.insert( insertor );
    }

    @Test
    public void test() {
        DBCursor cursor = cl.query();

        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection newCL = db.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor curs = newCL.query();
            db.interrupt();
            try {
                while( curs.hasNext() ) {
                    curs.getNext();
                }
                Assert.fail( "expect fail but success." );
            } catch ( BaseException e ) {
                Assert.assertFalse( curs.hasNext() );
                if ( e.getErrorCode() != -36 )
                    throw e;
            }

            Assert.assertTrue( cursor.hasNext() );
        } finally {
            db.close();
            cursor.close();
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            sdb.close();
        }
    }
}