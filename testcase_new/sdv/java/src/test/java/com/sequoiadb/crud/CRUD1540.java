package com.sequoiadb.crud;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Random;
import java.util.concurrent.LinkedBlockingQueue;

import com.sequoiadb.testcommon.CommLib;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-1540:并发记录和lob操作
 * @Author laojingtang
 * @Date 2018.01.04
 * @UpdataAuthor zhangyanan
 * @UpdateDate 2021.07.09
 * @Version 1.10
 */
public class CRUD1540 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "curdcl_1540";
    private DBCollection cl = null;
    private String index = "test_index";
    private List< String > coorURLs = new ArrayList<>();
    private Random random = new Random();
    private ArrayList< BSONObject > allRecords = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > insertRecords2 = new ArrayList< BSONObject >();
    private LinkedBlockingQueue< SaveOidAndMd5 > id2md5 = new LinkedBlockingQueue< SaveOidAndMd5 >();

    @BeforeClass
    public void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );

        // get all coord node url
        BSONObject obj = sdb.getReplicaGroup( "SYSCoord" ).getDetail();
        BasicBSONList group = ( BasicBSONList ) obj.get( "Group" );
        for ( Object o : group ) {
            BasicBSONObject node = ( BasicBSONObject ) o;
            String hostName = node.getString( "HostName" );
            String path = node.getString( "dbpath" );
            String[] s = path.split( "/" );
            String port = s[ s.length - 1 ];
            coorURLs.add( hostName + ":" + port );
        }
        int lobtimes = 20;
        writeLobAndGetMd5( cl, lobtimes );
        cl.createIndex( index, "{no:1}", false, false );
        int beginNo = 0;
        int recordNum = 40000;
        allRecords = CRUDUitls.insertData( cl, beginNo, recordNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        int insertBeginNo = 40000;
        int insertEndNo = 50000;
        threadInsert insert = new threadInsert( insertBeginNo, insertEndNo );
        es.addWorker( insert );
        int deleteBeginNo = 30000;
        int deleteEndNo = 40000;
        threadDelete delete = new threadDelete( deleteBeginNo, deleteEndNo );
        es.addWorker( delete );
        int updateBeginNo = 20000;
        int updateEndNo = 30000;
        threadUpdate update = new threadUpdate( updateBeginNo, updateEndNo );
        es.addWorker( update );
        int queryBeginNo = 0;
        int queryEndNo = 10000;
        threadQuery query = new threadQuery( queryBeginNo, queryEndNo );
        es.addWorker( query );

        ReadLob readLob = new ReadLob();
        WriteLob writeLob = new WriteLob();
        RemoveLob removeLob = new RemoveLob();
        es.addWorker( readLob );
        es.addWorker( writeLob );
        es.addWorker( removeLob );
        es.run();
        Assert.assertEquals( insert.getRetCode(), 0 );
        Assert.assertEquals( delete.getRetCode(), 0 );
        Assert.assertEquals( update.getRetCode(), 0 );
        Assert.assertEquals( query.getRetCode(), 0 );
        Assert.assertEquals( readLob.getRetCode(), 0 );
        Assert.assertEquals( writeLob.getRetCode(), 0 );
        Assert.assertEquals( removeLob.getRetCode(), 0 );

        checkLobData();
        presetData();
        // 指定索引查询校验修改线程数据
        List< BSONObject > sublist = allRecords.subList( updateBeginNo,
                updateEndNo );
        String matcher = "{$and:[{no:{$gte:" + updateBeginNo + "}},{no:{$lt:"
                + updateEndNo + "}}]}";
        DBCursor cursor = cl.query( matcher, "", "{'no':1}",
                "{'':'test_index'}" );
        CRUDUitls.checkRecords( sublist, cursor );
        // 校验所有数据
        CRUDUitls.checkRecords( cl, allRecords, "" );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public void presetData() {
        int beginNo1 = 20000;
        int endNo1 = 30000;
        BSONObject obj = new BasicBSONObject();
        for ( int i = beginNo1; i < endNo1; i++ ) {
            obj = allRecords.get( i );
            obj.put( "a", "updatetest" + beginNo1 );
        }
        allRecords.addAll( insertRecords2 );
        int beginNo2 = 30000;
        int endNo2 = 40000;
        List< BSONObject > sublist = allRecords.subList( beginNo2, endNo2 );
        allRecords.removeAll( sublist );
    }

    public byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    int getLobnum() {
        DBCursor cursor = cl.listLobs();
        int count = 0;
        while ( cursor.hasNext() ) {
            cursor.getNext();
            count++;
        }
        return count;
    }

    private class SaveOidAndMd5 {
        private ObjectId oid;
        private String md5;

        public SaveOidAndMd5( ObjectId oid, String md5 ) {
            this.oid = oid;
            this.md5 = md5;
        }

        public ObjectId getOid() {
            return oid;
        }

        public String getMd5() {
            return md5;
        }
    }

    private void writeLobAndGetMd5( DBCollection dbcl, int lobtimes ) {
        for ( int i = 0; i < lobtimes; i++ ) {
            int writeLobSize = random.nextInt( 1024 * 1024 );
            byte[] wlobBuff = getRandomBytes( writeLobSize );
            ObjectId oid = createAndWriteLob( dbcl, wlobBuff, i );

            // save oid and md5
            String prevMd5 = getMd5( wlobBuff );
            id2md5.offer( new SaveOidAndMd5( oid, prevMd5 ) );
        }
    }

    public static ObjectId createAndWriteLob( DBCollection dbcl, byte[] data,
            int i ) {
        Calendar cal = Calendar.getInstance();
        cal.set( Calendar.DATE, 1 );
        Date date = cal.getTime();
        SimpleDateFormat sf = new SimpleDateFormat( "yyyyMMdd" );
        int year = Integer.valueOf( sf.format( date ).substring( 0, 4 ) );
        int day = Integer.valueOf( sf.format( date ).substring( 6, 8 ) );
        cal = Calendar.getInstance();
        cal.set( Calendar.YEAR, year );
        cal.set( Calendar.MONTH, 0 );
        cal.set( Calendar.DATE, day + i );
        date = cal.getTime();

        ObjectId oid = dbcl.createLobID( date );
        DBLob lob = dbcl.createLob( oid );
        lob.write( data );
        lob.close();
        return oid;
    }

    private void checkLobData() {
        try ( DBCursor listLob = cl.listLobs()) {
            while ( listLob.hasNext() ) {
                BasicBSONObject obj = ( BasicBSONObject ) listLob.getNext();
                ObjectId existOid = obj.getObjectId( "Oid" );
                try ( DBLob rLob = cl.openLob( existOid )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    String curMd5 = getMd5( rbuff );
                    String prevMd5 = getLobMd5ByOid( existOid );
                    Assert.assertEquals( curMd5, prevMd5,
                            "the list oid:" + existOid.toString() );
                }
            }
        }
        // the list lobnums must be equal with the nums of the id2mda,if id2md5
        // is not empty
        Assert.assertEquals( id2md5.isEmpty(), true,
                "the remaining " + id2md5.size() + " oids were not found!" );
    }

    private String getLobMd5ByOid( ObjectId lobOid ) {
        Iterator< SaveOidAndMd5 > iterator = id2md5.iterator();
        boolean found = false;
        String findMd5 = "";
        while ( iterator.hasNext() ) {
            SaveOidAndMd5 current = iterator.next();
            ObjectId oid = current.getOid();
            if ( oid.equals( lobOid ) ) {
                findMd5 = current.getMd5();
                id2md5.remove( current );
                found = true;
                break;
            }
        }

        // if oid does not exist in the queue,than error
        if ( !found ) {
            throw new RuntimeException( "oid[" + lobOid + "] not found" );
        }
        return findMd5;
    }

    private String getCoordURLRandom() {
        return coorURLs.get( random.nextInt( coorURLs.size() ) );
    }

    public static String getRandomString( int length ) {
        String str = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^asssgggg!@#$";
        StringBuffer sbBuffer = new StringBuffer();
        // random generation 80-length string.
        Random random = new Random();
        StringBuffer subBuffer = new StringBuffer();
        int strLen = str.length();
        for ( int i = 0; i < strLen; i++ ) {
            int number = random.nextInt( strLen );
            subBuffer.append( str.charAt( number ) );
        }

        // generate a string at a specified length by subBuffer
        int times = length / str.length();
        for ( int i = 0; i < times; i++ ) {
            sbBuffer.append( subBuffer );
        }
        int subTimes = length % str.length();
        if ( subTimes != 0 ) {
            sbBuffer.append( str.substring( 0, subTimes ) );
        }
        return sbBuffer.toString();
    }

    public String getMd5( Object inbuff ) {
        MessageDigest md5 = null;
        String value = "";

        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
            } else if ( inbuff instanceof byte[] ) {
                md5.update( ( byte[] ) inbuff );
            } else {
                Assert.fail( "invalid parameter!" );
            }
            BigInteger bi = new BigInteger( 1, md5.digest() );
            value = bi.toString( 16 );
        } catch ( NoSuchAlgorithmException e ) {
            e.printStackTrace();
            Assert.fail( "fail to get md5!" + e.getMessage() );
        }
        return value;
    }

    private class threadInsert extends ResultStore {

        private int beginNo;
        private int endNo;

        private threadInsert( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @ExecuteOrder(step = 1)
        private void insert() {

            try ( Sequoiadb db = new Sequoiadb( getCoordURLRandom(), "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                insertRecords2 = CRUDUitls.insertData( cl, beginNo, endNo );
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}}]}";
                CRUDUitls.checkRecords( cl, insertRecords2, matcher );
            }
        }
    }

    private class threadDelete extends ResultStore {

        private int beginNo;
        private int endNo;

        private threadDelete( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;

        }

        @ExecuteOrder(step = 1)
        private void delete() {
            try ( Sequoiadb db = new Sequoiadb( getCoordURLRandom(), "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}}]}";
                cl.delete( matcher );

                ArrayList< BSONObject > deleteRecords = new ArrayList< BSONObject >();
                CRUDUitls.checkRecords( cl, deleteRecords, matcher );
            }
        }
    }

    private class threadUpdate extends ResultStore {
        private int beginNo;
        private int endNo;

        private threadUpdate( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @ExecuteOrder(step = 1)
        private void update() {
            try ( Sequoiadb db = new Sequoiadb( getCoordURLRandom(), "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}}]}";
                String modifier = "{$set:{a:'updatetest" + beginNo + "'}}";
                cl.update( matcher, modifier, "" );
            }
        }

    }

    private class threadQuery extends ResultStore {

        private int beginNo;
        private int endNo;

        private threadQuery( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @ExecuteOrder(step = 1)
        private void query() {
            try ( Sequoiadb db = new Sequoiadb( getCoordURLRandom(), "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}}]}";
                DBCursor cursor = cl.query( matcher, "", "{'no':1}", "" );
                List< BSONObject > sublist = allRecords.subList( beginNo,
                        endNo );
                CRUDUitls.checkRecords( sublist, cursor );
            }
        }
    }

    private class ReadLob extends ResultStore {
        @ExecuteOrder(step = 1)
        public void read() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( getCoordURLRandom(), "",
                    "" ) ;) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                SaveOidAndMd5 oidAndMd5 = id2md5.take();
                ObjectId oid = oidAndMd5.getOid();

                try ( DBLob rLob = cl.openLob( oid, DBLob.SDB_LOB_READ )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    String curMd5 = getMd5( rbuff );
                    String prevMd5 = oidAndMd5.getMd5();
                    Assert.assertEquals( curMd5, prevMd5 );
                }
                id2md5.offer( oidAndMd5 );
            }
        }
    }

    private class RemoveLob extends ResultStore {
        @ExecuteOrder(step = 1)
        public void read() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( getCoordURLRandom(), "",
                    "" ) ;) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                SaveOidAndMd5 oidAndMd5 = id2md5.take();
                cl.removeLob( oidAndMd5.getOid() );
            }
        }
    }

    private class WriteLob extends ResultStore {
        @ExecuteOrder(step = 1)
        public void write() {
            try ( Sequoiadb db = new Sequoiadb( getCoordURLRandom(), "",
                    "" ) ;) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int lobtimes = 2;
                writeLobAndGetMd5( cl, lobtimes );
            }
        }
    }
}
