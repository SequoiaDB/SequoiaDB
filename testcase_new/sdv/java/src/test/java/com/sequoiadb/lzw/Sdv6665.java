package com.sequoiadb.lzw;

import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: Sdv6665 test content: hash范围切分，分区键为单个字段_st.compress.03.024 testlink
 * case: SeqDB-6665
 * 
 * @author zengxianquan
 * @date 2016年12月30日
 * @version 1.00
 */
public class Sdv6665 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl6665";
    private List< String > dataGroupNames = null;
    private DBCollection cl = null;
    private String sourceGroupName;
    private String destGroupName;
    private String strRec;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        if ( LzwUtils3.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        dataGroupNames = LzwUtils3.getDataGroups( sdb );
        createCL();
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

        sourceGroupName = dataGroupNames.get( 0 );
        destGroupName = dataGroupNames.get( 1 );
        try {
            int dataCount = 1200;
            int strLength = 128 * 1024;
            insertData( cl, dataCount, strLength );
            LzwUtils3.checkCompressed( cl, sourceGroupName );
            split( sourceGroupName, destGroupName );
            LzwUtils3.waitCreateDict( cl, destGroupName ); // 等待压缩字典的建立,最多等待60分钟
            insertDataAgain( cl, 1000, strLength );
            LzwUtils3.checkCompressed( cl, destGroupName );
            checkSplit( sdb, sourceGroupName, 1100 );
            checkSplit( sdb, destGroupName, 1100 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    private void createCL() {
        BSONObject option = new BasicBSONObject();
        BSONObject shardingKey = new BasicBSONObject();
        try {

            option.put( "Group", dataGroupNames.get( 0 ) );
            option.put( "Compressed", true );
            option.put( "CompressionType", "lzw" );
            option.put( "ShardingType", "hash" );
            shardingKey.put( "_id", 1 );
            option.put( "ShardingKey", shardingKey );

            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public String insertData( DBCollection cl, int dataCount, int strLength ) {
        strRec = getRandomString( strLength );
        for ( int i = 0; i < dataCount / 2; i++ ) {
            cl.insert( "{_id:" + i + ",key:'" + strRec + "'}" );
        }

        LzwUtils3.waitCreateDict( cl, sourceGroupName ); // 等待压缩字典的建立,最多等待60分钟

        for ( int i = dataCount / 2; i < dataCount; i++ ) {
            cl.insert( "{_id:" + i + ",key:'" + strRec + "'}" );
        }
        return strRec;
    }

    public String insertDataAgain( DBCollection cl, int dataCount,
            int strLength ) {
        for ( int i = 1200; i < 1200 + dataCount; i++ ) {
            cl.insert( "{_id:" + i + ",key:'" + strRec + "'}" );
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

    private void split( String sourceGroupName, String destGroupName ) {
        try {
            double percent = 50;
            cl.split( sourceGroupName, destGroupName, percent );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    private void checkSplit( Sequoiadb db, String dataGroupName,
            long expectDataCount ) {
        DBCollection splitCL = null;
        Sequoiadb dataDB = db.getReplicaGroup( dataGroupName ).getMaster()
                .connect();
        splitCL = dataDB.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        int count = ( int ) splitCL.getCount();
        int offSet = ( int ) ( 0.3 * expectDataCount );
        if ( Math.abs( count - expectDataCount ) > offSet ) {
            Assert.fail( "the split result is wrong:" + count );
        }
    }
}
