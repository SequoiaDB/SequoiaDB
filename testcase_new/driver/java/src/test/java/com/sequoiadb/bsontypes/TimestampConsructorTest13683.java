package com.sequoiadb.bsontypes;

import org.bson.types.BSONTimestamp;
import org.testng.Assert;
import org.testng.annotations.Test;

import java.sql.Timestamp;
import java.util.Date;

/**
 * Created by laojingtang on 17-12-7.
 */
public class TimestampConsructorTest13683 {
    @Test
    public void testDate() {
        Date date = new Date();
        BSONTimestamp bsonTimestamp = new BSONTimestamp( date );
        long t1 = date.getTime();
        long t2 = bsonTimestamp.getDate().getTime()
                + bsonTimestamp.getInc() / 1000;
        Assert.assertEquals( t2, t1 );
        assertBSONTImestampCanUse( bsonTimestamp );
    }

    @Test
    public void testTimestamp() {
        Timestamp sqlTimestamp = new Timestamp( System.currentTimeMillis() );
        BSONTimestamp bsonTimestamp = new BSONTimestamp( sqlTimestamp );

        long t1 = sqlTimestamp.getTime() / 1000;
        long t2 = bsonTimestamp.getTime();
        Assert.assertEquals( t2, t1 );
        Assert.assertEquals( bsonTimestamp.getInc(),
                sqlTimestamp.getNanos() / 1000 );
        assertBSONTImestampCanUse( bsonTimestamp );
    }

    @Test
    public void testNoPara() {
        BSONTimestamp bsonTimestamp = new BSONTimestamp();
        assertBSONTImestampCanUse( bsonTimestamp );
    }

    @Test
    public void testTimeAndInc() {
        BSONTimestamp bsonTimestamp = new BSONTimestamp( 1024, 1 );
        Assert.assertEquals( bsonTimestamp.getTime(), 1024 );
        Assert.assertEquals( bsonTimestamp.getInc(), 1 );
        assertBSONTImestampCanUse( bsonTimestamp );
    }

    private void assertBSONTImestampCanUse( BSONTimestamp b ) {
        b.getDate();
        b.getTime();
        b.getInc();
        b.toTimestamp();
        b.toString();
        b.toDate();
    }

}
