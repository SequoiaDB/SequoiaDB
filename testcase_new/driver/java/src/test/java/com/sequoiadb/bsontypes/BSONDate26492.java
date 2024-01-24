package com.sequoiadb.bsontypes;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDate;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import static org.testng.Assert.assertEquals;

/**
 * @description seqDB-26492:java驱动使用 LocalDateTime 方式读写日期数据
 * @author gongmeiyan
 * @date 2022/05/13
 * @version 1.00
 */
public class BSONDate26492 extends SdbTestBase {
    private String clName = "cl_26492";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
            sdb.createCollectionSpace( SdbTestBase.csName );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
    }

    @DataProvider(name = "generateDataProvider")
    public Object[][] generateDataProvider() {
        return new Object[][] {
                // Date with time zone problem
                { 1, "0001-01-01 00:03:01", "yyyy-MM-dd HH:mm:ss" },
                { 2, "1581-12-31 12:59:59", "yyyy-MM-dd HH:mm:ss" },
                { 3, "1582-01-31 01:01:01", "yyyy-MM-dd HH:mm:ss" },
                { 4, "1582-01-01 00:00:01.666666",
                        "yyyy-MM-dd HH:mm:ss.SSSSSS" },
                { 5, "1899-12-31 13:01:01", "yyyy-MM-dd HH:mm:ss" },
                // Date without zone problem
                { 6, "1900-01-01 00:00:01", "yyyy-MM-dd HH:mm:ss" },
                { 7, "1900-01-02 01:01:01", "yyyy-MM-dd HH:mm:ss" },
                { 8, "1970-01-01 00:00:01", "yyyy-MM-dd HH:mm:ss" },
                { 9, "2021-04-01 12:34:59", "yyyy-MM-dd HH:mm:ss" },
                { 10, "2022-04-01 23:59:59", "yyyy-MM-dd HH:mm:ss" },
                { 11, "1986-01-01 11:00:01.111111",
                        "yyyy-MM-dd HH:mm:ss.SSSSSS" },
                { 12, "9999-12-31 00:00:00", "yyyy-MM-dd HH:mm:ss" } };
    }

    @Test(dataProvider = "generateDataProvider")
    public void testDate( int id, String date, String format ) {
        // write data
        BSONObject record = new BasicBSONObject();
        DateTimeFormatter dateTimeFormatter = DateTimeFormatter
                .ofPattern( format );
        LocalDateTime localDateTime = LocalDateTime.parse( date,
                dateTimeFormatter );
        BSONDate expectDate = BSONDate.valueOf( localDateTime );
        record.put( "id", id );
        record.put( "date", expectDate );
        cl.insertRecord( record );

        // read and check data
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "id", id );
        try ( DBCursor cursor = cl.query( matcher, null, null, null ) ;) {
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                BSONDate actualDate = ( BSONDate ) obj.get( "date" );

                assertEquals( actualDate.toLocalDateTime(),
                        expectDate.toLocalDateTime(),
                        "check date are unequal\n" + "actualDate: "
                                + actualDate.toLocalDateTime() + "\n"
                                + "expectDate: "
                                + expectDate.toLocalDateTime() );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }
}
