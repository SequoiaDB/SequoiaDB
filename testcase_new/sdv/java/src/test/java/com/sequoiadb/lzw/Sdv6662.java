package com.sequoiadb.lzw;

import java.util.Random;

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
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: Sdv6662 test content: query查询记录_st.compress.03.020 testlink case:
 * SeqDB-6662
 * 
 * @author zengxianquan
 * @date 2016年12月29日
 * @version 1.00
 */
public class Sdv6662 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl6662";
    private String dataGroupName = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        if ( LzwUtils3.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.disconnect();
        }
    }

    @Test
    public void test() {
        try {
            DBCollection cl = createCL();
            boolean isUnique = false;
            createIndex( isUnique );
            int dataCount = 1200;
            int strLength = 128 * 1024;
            String rec = insertData( cl, dataCount, strLength );
            LzwUtils3.checkCompressed( cl, dataGroupName );
            checkQuery( dataCount, rec );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    private DBCollection createCL() {
        DBCollection cl = null;
        BSONObject option = new BasicBSONObject();
        try {
            dataGroupName = LzwUtils3.getDataGroups( sdb ).get( 0 );
            option.put( "Group", dataGroupName );
            option.put( "Compressed", true );
            option.put( "CompressionType", "lzw" );
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    public void createIndex( boolean isUnique ) {
        DBCollection cl = null;
        BSONObject keyBson = new BasicBSONObject();
        keyBson.put( "value", 1 );
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cl = cs.getCollection( clName );
            cl.createIndex( "Idx", keyBson, isUnique, false );
        } catch ( BaseException e ) {
            Assert.fail( "failed to create index" );
        }
    }

    public String insertData( DBCollection cl, int dataCount, int strLength ) {
        String strRec = getRandomString( strLength );
        for ( int i = 0; i < dataCount / 2; i++ ) {
            cl.insert( "{_id:" + i + ",key:'" + strRec + i + "',value:" + i
                    + "}" );
        }

        LzwUtils3.waitCreateDict( cl, dataGroupName ); // 等待压缩字典的建立,最多等待60分钟

        for ( int i = dataCount / 2; i < dataCount; i++ ) {
            cl.insert( "{_id:" + i + ",key:'" + strRec + i + "',value:" + i
                    + "}" );
        }

        return strRec;
    }

    private String getRandomString( int length ) {
        String base = "abc";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length; i++ ) {
            int index = random.nextInt( base.length() );
            sb.append( base.charAt( index ) );
        }
        return sb.toString();
    }

    private void checkQuery( int dataCount, String strRec ) {
        DBCursor cursor1 = null;
        DBCursor cursor2 = null;
        try {
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            cursor1 = cl.query( null, null, "{_id:1}", "{'':'value'}", 10, 10 );
            cursor2 = cl.query( null, null, "{_id:1}", null, 10, 10 );
            int i = 10;
            while ( cursor1.hasNext() && cursor2.hasNext() ) {
                String actRec1 = cursor1.getNext().toString();
                String actRec2 = cursor2.getNext().toString();
                String exptRec = "{ \"_id\" : " + i + " , \"key\" : \"" + strRec
                        + i + "\" , \"value\" : " + i + " }";
                // System.out.println(actRec1);
                // System.out.println(actRec2);
                // System.out.println(exptRec);
                if ( !exptRec.equals( actRec1 )
                        || !exptRec.equals( actRec2 ) ) {
                    Assert.fail( "The data is error" );
                }
                i++;
            }
            Assert.assertEquals( i - 10, 10, "The data count is error" );

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( cursor1 != null ) {
                cursor1.close();
            }
            if ( cursor2 != null ) {
                cursor2.close();
            }
        }
    }
}
