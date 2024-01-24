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
import java.util.Date;
import static org.testng.Assert.assertEquals;

/**
 * @description seqDB-26587:java驱动使用LocalDateTime方式写入日期数据,使用util.date/sql.date方式读取数据
 * @author gongmeiyan
 * @date 2022/05/15
 * @version 1.00
 */
public class BSONDate26587 extends SdbTestBase {
    private String clName = "cl_26587";
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
                { 1, "0001-01-01 00:00:00", "0001-01-02",
                        "yyyy-MM-dd HH:mm:ss" },
                { 2, "1581-12-31 00:00:59", "1581-12-20",
                        "yyyy-MM-dd HH:mm:ss" },
                { 3, "1582-01-01 00:00:01", "1581-12-21",
                        "yyyy-MM-dd HH:mm:ss" },
                { 4, "1899-12-31 10:29:01", "1899-12-31",
                        "yyyy-MM-dd HH:mm:ss" },
                // Date without zone problem
                { 5, "1900-01-01 00:00:00", "1899-12-31",
                        "yyyy-MM-dd HH:mm:ss" },
                { 6, "1900-01-02 01:01:00", "1900-01-02",
                        "yyyy-MM-dd HH:mm:ss" },
                { 7, "1970-01-01 00:00:59", "1970-01-01",
                        "yyyy-MM-dd HH:mm:ss" },
                { 8, "2022-04-01 01:14:23", "2022-04-01",
                        "yyyy-MM-dd HH:mm:ss" },
                { 9, "9999-12-31 00:00:00", "9999-12-31",
                        "yyyy-MM-dd HH:mm:ss" } };
    }

    @Test(dataProvider = "generateDataProvider")
    public void testDate( int id, String date, String expectSqlDate,
                          String format ) {
        // write data
        BSONObject record = new BasicBSONObject();
        DateTimeFormatter formatter = DateTimeFormatter.ofPattern( format );
        LocalDateTime localDateTime = LocalDateTime.parse( date, formatter );
        BSONDate expectBsonDate = BSONDate.valueOf( localDateTime );
        record.put( "date", expectBsonDate );
        record.put( "id", id );
        cl.insertRecord( record );

        // read and check data
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "id", id );
        try ( DBCursor cursor = cl.query( matcher, null, null, null ) ;) {
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                Date utilDate = ( Date ) obj.get( "date" );
                String actualUtilDate = utilDate.toString();

                java.sql.Date sqlDate = new java.sql.Date( utilDate.getTime() );
                String actualSqlDate = sqlDate.toString();

                assertEquals( actualUtilDate, expectBsonDate.toString(),
                        "check date are unequal\n" + "actualUtilDate: "
                                + actualUtilDate + "\n" + "expectBsonDate: "
                                + expectBsonDate.toString() );

                assertEquals( actualSqlDate, expectSqlDate,
                        "check date are unequal\n" + "actualSqlDate: "
                                + actualSqlDate + "\n" + "expectSqlDate: "
                                + expectSqlDate );
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