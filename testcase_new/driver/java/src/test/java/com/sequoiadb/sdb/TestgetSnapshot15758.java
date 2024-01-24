package com.sequoiadb.sdb;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestgetSnapshot15758 seqDB-15758 test interface: getSnapshot(int
 * snapType, BSONObject matcher, BSONObject selector, BSONObject orderBy,
 * BSONObject hint, long skipRows, long returnRows)
 * 
 * @author chensiqin
 * @Date 2018.9.7
 * @version 1.00
 */
public class TestgetSnapshot15758 extends SdbTestBase {
    private Sequoiadb sdb;

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestgetSnapshot15758 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    @Test
    public void test() {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "IsPrimary", true );
        matcher.put( "Status", "Normal" );
        BSONObject selector = new BasicBSONObject();
        selector.put( "ServiceStatus", 1 );
        selector.put( "IsPrimary", 1 );
        BSONObject orderBy = new BasicBSONObject();
        orderBy.put( "NodeName", 1 );
        BSONObject hint = new BasicBSONObject();
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_HEALTH,
                "{ \"IsPrimary\" : true , \"ServiceStatus\" : true }", "", "" );
        int length = 0;
        while ( cursor.hasNext() ) {
            cursor.getNext();
            length++;
        }
        cursor.close();

        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_HEALTH, matcher, selector,
                orderBy, hint, length - 1, 1 );
        int num = 0;
        while ( cursor.hasNext() ) {
            num++;
            Assert.assertEquals( cursor.getNext().toString(),
                    "{ \"IsPrimary\" : true , \"ServiceStatus\" : true }" );
        }
        Assert.assertEquals( num, 1 );
        cursor.close();
    }

    @AfterClass
    public void tearDown() {
        try {
            this.sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}
