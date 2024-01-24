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
 * FileName: Sdv6661 test content: query查询记录_st.compress.03.020 testlink case:
 * SeqDB-6661
 * 
 * @author zengxianquan
 * @date 2016年12月29日
 * @version 1.00
 */
public class Sdv6661 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl6661";
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

    public String insertData( DBCollection cl, int dataCount, int strLength ) {
        String strRec = getRandomString( strLength );
        for ( int i = 0; i < dataCount / 2; i++ ) {
            cl.insert( "{_id:" + i + ",key:'" + strRec + i + "'}" );
        }

        LzwUtils3.waitCreateDict( cl, dataGroupName ); // 等待压缩字典的建立,最多等待60分钟

        for ( int i = dataCount / 2; i < dataCount; i++ ) {
            cl.insert( "{_id:" + i + ",key:'" + strRec + i + "'}" );
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
        DBCursor cursor = null;
        try {
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            cursor = cl.query( null, null, "{_id:1}", null, 10, 10 );
            int i = 10;
            while ( cursor.hasNext() ) {
                String actRec = cursor.getNext().toString();
                String exptRec = "{ \"_id\" : " + i + " , \"key\" : \"" + strRec
                        + i + "\" }";
                if ( !exptRec.equals( actRec ) ) {
                    Assert.fail( "The data is error" );
                }
                i++;
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
    }
}
