/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:ECMTest.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2017-12-7上午9:52:40
 *  @version 1.00
 */
package com.sequoiadb.ecmtest ;

import java.io.File ;
import java.io.FileInputStream ;
import java.io.FileNotFoundException ;
import java.io.IOException ;
import java.util.ArrayList ;
import java.util.Collection ;
import java.util.Date ;
import java.util.LinkedList ;
import java.util.List ;
import java.util.Properties ;
import java.util.Queue ;
import java.util.concurrent.ExecutorService ;
import java.util.concurrent.Executors ;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.bson.BSONObject ;
import org.bson.BasicBSONObject ;
import org.bson.types.BasicBSONList ;
import org.bson.util.JSON ;

import com.sequoiadb.base.CollectionSpace ;
import com.sequoiadb.base.DBCollection ;
import com.sequoiadb.base.DBCursor ;
import com.sequoiadb.base.SequoiadbDatasource ;

import com.sequoiadb.base.Sequoiadb ;
import com.sequoiadb.datasource.DatasourceOptions ;
import com.sequoiadb.exception.BaseException ;
import com.sequoiadb.exception.SDBError ;
import com.sequoiadb.net.ConfigOptions ;

public class ECMTest {
    SequoiadbDatasource ds ;
    Properties prop ;
    private static boolean isExit = false ;

    public ECMTest( Properties prop ) {
        this.prop = prop ;

        String coordUrls = prop.getProperty( "coordUrl" ) ;
        String[] tmp = coordUrls.split( "," ) ;
        List< String > urls = new ArrayList< String >() ;
        for ( int i = 0; i < tmp.length; ++i ) {
            urls.add( tmp[i] ) ;
        }
        String usr = prop.getProperty( "usr" ) ;
        String pwd = prop.getProperty( "pwd" ) ;

        ConfigOptions copt = new ConfigOptions() ;
        if (prop.containsKey( "connectionTimeOut" )){
        copt.setConnectTimeout( Integer.parseInt( prop
                .getProperty( "connectionTimeOut" ).trim() ) ) ;
        }
        if (prop.containsKey( "maxAutoConnectRetryTime" )){
        copt.setMaxAutoConnectRetryTime( Integer.parseInt( prop
                .getProperty( "maxAutoConnectRetryTime" ).trim() ) ) ;
        }
        DatasourceOptions opt = new DatasourceOptions() ;
        if (prop.containsKey( "maxCount" )){
        opt.setMaxCount( Integer.parseInt( prop.getProperty( "maxCount" ).trim() ) ) ;
        }
        if (prop.containsKey( "deltaIncCount" )){
        opt.setDeltaIncCount( Integer.parseInt( prop
                .getProperty( "deltaIncCount" ).trim() ) ) ;
        }
        if (prop.containsKey( "maxIdleCount" )){
        opt.setMaxIdleCount( Integer.parseInt( prop
                .getProperty( "maxIdleCount" ).trim() ) ) ;
        }
        if (prop.containsKey( "keepAliveTimeout" )){
        opt.setKeepAliveTimeout( Integer.parseInt( prop
                .getProperty( "keepAliveTimeout" ).trim() ) ) ;
        }
        if (prop.containsKey( "validateConnection" )){
        opt.setValidateConnection( Boolean.parseBoolean( prop
                .getProperty( "validateConnection" ).trim() ) ) ;
        }
        if (prop.containsKey( "syncCoordInterval" )){
        opt.setSyncCoordInterval( Integer.parseInt( prop
                .getProperty( "syncCoordInterval" ).trim() ) ) ;
        }
        if (prop.containsKey( "checkInterval" )){
        opt.setCheckInterval( Integer.parseInt( prop
                .getProperty( "checkInterval" ).trim() ) ) ;
        }

        this.ds = new SequoiadbDatasource( urls, usr, pwd, copt, opt ) ;

    }

    private void createTestDomain( Sequoiadb db ) {
        BasicBSONList groupNames = new BasicBSONList() ;
        DBCursor cursor = db.listReplicaGroups() ;
        while ( cursor.hasNext() ) {
            String groupName = ( String ) cursor.getNext().get( "GroupName" ) ;
            if ( groupName.equals( "SYSCoord" )
                    || groupName.equals( "SYSSpare" )
                    || groupName.equals( "SYSCatalogGroup" ) ) {
                continue ;
            }
            groupNames.add( groupName ) ;
        }
        try {
            BasicBSONObject opt = new BasicBSONObject() ;
            opt.put( "Groups", groupNames ) ;
            opt.put( "AutoSplit", true ) ;
            db.createDomain( Const.ECM_DOMAIN_NAEM, opt ) ;
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -215 ) {
                throw e ;
            }

        }
    }

    private void createTestCS( Sequoiadb db ) {
        try {
            BasicBSONObject opt = new BasicBSONObject() ;
            opt.put( "Domain", Const.ECM_DOMAIN_NAEM ) ;
            opt.put( "LobPageSize", Const.lobPageSize ) ;
            db.createCollectionSpace( Const.ECM_CS_NAME, opt ) ;
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -33 ) {
                throw e ;
            }
        }
    }

    private DBCollection createCL( Sequoiadb db, String name, BSONObject opt ) {
        CollectionSpace cs = null ;
        DBCollection cl = null ;
        try {
            cs = db.getCollectionSpace( Const.ECM_CS_NAME ) ;
            cl = cs.createCollection( name, opt ) ;

        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -22 ) {
                throw e ;
            }
            if ( cs != null ) {
                cl = cs.getCollection( name ) ;
            }
        }
        return cl ;
    }

    private void attachCL( DBCollection mainCL, DBCollection subCL,
            BSONObject bound ) {
        try{
            mainCL.attachCollection( subCL.getFullName(), bound ) ;
        }catch(BaseException e){
            if (e.getErrorCode() != -235){
                throw e;
            }
        }
    }

    private void createIndex( DBCollection cl, String name, BSONObject opt ) {
        cl.createIndex( name, opt, false, false ) ;
    }

    private List< BSONObject > buildAttachOpts() {
        List< BSONObject > opts = new ArrayList< BSONObject >() ;

        BasicBSONObject opt = new BasicBSONObject() ;
        BasicBSONObject low = new BasicBSONObject() ;
        BasicBSONObject up = new BasicBSONObject() ;
        
        
        Date date = new Date(2017 - 1900, 0, 1);
        low.put( Const.SYS_CREATETS,
                JSON.parse( String.format( "{\"%s\":%d}", "$minKey", 1 ) ) ) ;
        up.put( Const.SYS_CREATETS, date ) ;

        opt.put( "LowBound", low ) ;
        opt.put( "UpBound", up ) ;
        opts.add( opt ) ;

        opt = new BasicBSONObject() ;
        low = new BasicBSONObject() ;
        up = new BasicBSONObject() ;

        low.put( Const.SYS_CREATETS, date ) ;
        date = new Date(2018 - 1900,0,1);
        up.put( Const.SYS_CREATETS, date ) ;
        opt.put( "LowBound", low ) ;
        opt.put( "UpBound", up ) ;
        opts.add( opt ) ;

        opt = new BasicBSONObject() ;
        low = new BasicBSONObject() ;
        up = new BasicBSONObject() ;

        low.put( Const.SYS_CREATETS, date) ;
        date = new Date(2019 - 1900,0,1) ;
        up.put( Const.SYS_CREATETS, date) ;
        opt.put( "LowBound", low ) ;
        opt.put( "UpBound", up ) ;
        opts.add( opt ) ;

        opt = new BasicBSONObject() ;
        low = new BasicBSONObject() ;
        up = new BasicBSONObject() ;
       

        low.put( Const.SYS_CREATETS, date ) ;
        date = new Date(2020 - 1900,0,1) ;
        up.put( Const.SYS_CREATETS, date) ;
        opt.put( "LowBound", low ) ;
        opt.put( "UpBound", up ) ;
        opts.add( opt ) ;

        opt = new BasicBSONObject() ;
        low = new BasicBSONObject() ;
        up = new BasicBSONObject() ;

        low.put( Const.SYS_CREATETS, date) ;
        up.put( Const.SYS_CREATETS,
                JSON.parse( String.format( "{\"%s\":%d}", "$maxKey", 1 ) ) ) ;
        opt.put( "LowBound", low ) ;
        opt.put( "UpBound", up ) ;
        opts.add( opt ) ;

        return opts ;
    }

    private void createDoc( Sequoiadb db ) {
        BasicBSONObject opt = new BasicBSONObject() ;
        BasicBSONObject subOpt = new BasicBSONObject() ;
        subOpt.put( Const.SYS_CREATETS, 1 ) ;
        opt.put( "ShardingKey", subOpt ) ;
        opt.put( "ShardingType", "range" ) ;
        opt.put( "ReplSize", -1 ) ;
        opt.put( "Compressed", true ) ;
        opt.put( "CompressionType", "lzw" ) ;
        opt.put( "IsMainCL", true ) ;

        DBCollection mainCL = createCL( db, Const.ECM_CL_DOC, opt ) ;

        opt = new BasicBSONObject() ;
        subOpt = new BasicBSONObject() ;
        subOpt.put( "_id", 1 ) ;
        opt.put( "ShardingKey", subOpt ) ;
        opt.put( "ShardingType", "hash" ) ;
        opt.put( "ReplSize", -1 ) ;
        opt.put( "Compressed", true ) ;
        opt.put( "CompressionType", "lzw" ) ;
        opt.put( "EnsureShardingIndex", false ) ;
        String[] subCLNames = { Const.ECM_CL_DOC + "Min",
                Const.ECM_CL_DOC + "2017", Const.ECM_CL_DOC + "2018",
                Const.ECM_CL_DOC + "2019", Const.ECM_CL_DOC + "Max" } ;
        List< BSONObject > opts = buildAttachOpts() ;

        for ( int index = 0; index < subCLNames.length; ++index ) {
            DBCollection subCL = createCL( db, subCLNames[index], opt ) ;
            attachCL( mainCL, subCL, opts.get( index ) ) ;
        }

        opt = new BasicBSONObject() ;
        opt.put( Const.ECM_BATCH_ID, 1 ) ;

        createIndex( mainCL, "INDEX_ECM_BATCH_ID", opt ) ;
        opt = new BasicBSONObject() ;
        opt.put( Const.ECM_BUSI_SERIAL_NO, 1 ) ;
        createIndex( mainCL, "INDEX_ECM_SERIAL_NO", opt ) ;
    }

    private void createPart( Sequoiadb db ) {
        BasicBSONObject opt = new BasicBSONObject() ;
        BasicBSONObject subOpt = new BasicBSONObject() ;
        subOpt.put( Const.SYS_CREATETS, 1 ) ;
        opt.put( "ShardingKey", subOpt ) ;
        opt.put( "ShardingType", "range" ) ;
        opt.put( "ReplSize", -1 ) ;
        opt.put( "Compressed", true ) ;
        opt.put( "CompressionType", "lzw" ) ;
        opt.put( "IsMainCL", true ) ;

        DBCollection mainCL = createCL( db, Const.ECM_CL_PART, opt ) ;

        opt = new BasicBSONObject() ;
        subOpt = new BasicBSONObject() ;
        subOpt.put( "_id", 1 ) ;
        opt.put( "ShardingKey", subOpt ) ;
        opt.put( "ShardingType", "hash" ) ;
        opt.put( "ReplSize", -1 ) ;
        opt.put( "Compressed", true ) ;
        opt.put( "CompressionType", "lzw" ) ;
        opt.put( "EnsureShardingIndex", false ) ;
        String[] subCLNames = { Const.ECM_CL_PART + "Min",
                Const.ECM_CL_PART + "2017", Const.ECM_CL_PART + "2018",
                Const.ECM_CL_PART + "2019", Const.ECM_CL_PART + "Max" } ;
        List< BSONObject > opts = buildAttachOpts() ;

        for ( int index = 0; index < subCLNames.length; ++index ) {
            DBCollection subCL = createCL( db, subCLNames[index], opt ) ;
            attachCL( mainCL, subCL, opts.get( index ) ) ;
        }

        opt = new BasicBSONObject() ;
        opt.put( Const.ECM_FILE_NO, 1 ) ;
        createIndex( mainCL, "INDEX_ECM_FILE_NO", opt ) ;

        opt = new BasicBSONObject() ;
        opt.put( Const.SYS_BATCH_ID, 1 ) ;
        createIndex( mainCL, "INDEX_SYS_BATCH_ID", opt ) ;
    }

    private void createLob( Sequoiadb db ) {
        BasicBSONObject opt = new BasicBSONObject() ;
        BasicBSONObject subOpt = new BasicBSONObject() ;
        subOpt.put( "_id", 1 ) ;
        opt.put( "ShardingKey", subOpt ) ;
        opt.put( "ShardingType", "hash" ) ;
        opt.put( "ReplSize", -1 ) ;
        for ( int year = 2017; year <= 2020; ++year ) {
            for ( int month = 1; month <= 12; ++month ) {
                String suffix = String.format( "%04d%02d", year, month ) ;
                createCL( db, Const.ECM_CL_LOB_PREFIX + suffix, opt ) ;
            }
        }
    }

    public boolean init() {
        Sequoiadb sdb = null ;
        try {
            sdb = this.ds.getConnection() ;
            createTestDomain( sdb ) ;
            createTestCS( sdb ) ;
            createDoc( sdb ) ;
            createPart( sdb ) ;
            createLob( sdb ) ;

        } catch ( BaseException | InterruptedException e ) {
            e.printStackTrace() ;
            return false ;
        } finally {
            if ( sdb != null ) {
                this.ds.releaseConnection( sdb ) ;
            }
        }
        return true ;
    }

    public void Do() {
        int thrdNum = Integer.parseInt( prop.getProperty( "threadCount" ).trim() ) ;
        String dataDir = prop.getProperty( "testDataDir" ) ;
        
        if (thrdNum == 0){
            System.err.println("threadCount is zero");
            System.exit( 1 );
        }
        ExecutorService fixedThreadPool = Executors.newFixedThreadPool( thrdNum ) ;
        int writeTranCountPreMin = Integer.parseInt( prop
                .getProperty( "writeTranCountPerMins" ).trim() ) ;
        int readTranCountPerMin = Integer.parseInt( prop
                .getProperty( "readTranCountPerMins" ).trim() ) ;
        int maxWriteTranCountPerMin = Integer.parseInt( prop
                .getProperty( "maxWriteTranCountPerMins" ).trim() ) ;
        
        long start = System.currentTimeMillis() ;
        long prevWCnt = 0 ;
        long prevRCnt = 0 ;
        Queue< ECMWriteTask > wBusyQueue = new LinkedList< ECMWriteTask >() ;
        Queue< ECMWriteTask > wFreeQueue = new LinkedList< ECMWriteTask >() ;
        Queue< ECMReadTask > rBusyQueue = new LinkedList< ECMReadTask >() ;
        Queue< ECMReadTask > rFreeQueue = new LinkedList< ECMReadTask >() ;
        long prevUpdate = 0 ;
        long prevStat = 0 ;
        long prevMins = 0 ;
        boolean stopWrite = false ;
        boolean stopRead = false ;

        int timedTaskCount = Integer.parseInt( prop
                .getProperty( "timedTaskCount" ).trim() ) ;
        int timedTaskPeriod = Integer.parseInt( prop
                .getProperty( "timedTaskPeriod" ).trim() ) ;
        int timedTaskDelay = Integer.parseInt( prop
                .getProperty( "timedTaskDelay" ).trim() ) ;
        int timedTaskBatchCount = Integer.parseInt( prop
                .getProperty( "timedTaskBatchCount" ).trim() ) ;
        
        ScheduledExecutorService scheduledThreadPool = 
                Executors.newScheduledThreadPool( timedTaskCount );
        Queue< ECMTimedTask > tTaskQueue = new LinkedList< ECMTimedTask >() ;
        for (int i = 1; i <= timedTaskCount; ++i) {
            ECMTimedTask tTask = new ECMTimedTask( dataDir, ds, timedTaskBatchCount );
            scheduledThreadPool.scheduleAtFixedRate(tTask, timedTaskDelay * i,
                    timedTaskPeriod, TimeUnit.MILLISECONDS);
            tTaskQueue.offer(tTask);
        }

        while ( !isExit ) {
            if ( !stopWrite || wBusyQueue.isEmpty()) {
                for ( int pos = 0; pos < 3; ++pos ) {
                    ECMWriteTask wTask = wFreeQueue.poll() ;
                    if ( wTask == null ) {
                        wTask =  new ECMWriteTask( dataDir, ds ) ;
                    }
                    fixedThreadPool.execute( wTask ) ;
                    wBusyQueue.add( wTask ) ;
                }
            }

            if ( !stopRead || rBusyQueue.isEmpty()) {
                for ( int pos = 0; pos < 2; ++pos ) {
                    ECMReadTask rTask = rFreeQueue.poll() ;
                    if ( rTask == null ) {
                        rTask = new ECMReadTask( ds ) ;
                    }
                    fixedThreadPool.execute( rTask ) ;
                    rBusyQueue.add( rTask ) ;
                }
            }

            long current = System.currentTimeMillis() ;
            int curWCnt = ECMTask.getWCount() ;
            int curRCnt = ECMTask.getRCount() ;
            Date day = new Date() ;
            int hours = day.getHours() ;
            if ( hours >= 10 && hours <= 12 || hours >= 14 && hours <= 16 ) {
                if ( curWCnt - prevWCnt > maxWriteTranCountPerMin ) {
                    prevWCnt = curWCnt;
                    stopWrite = true ;
                }
            } else {
                if ( curWCnt - prevWCnt > writeTranCountPreMin ) {
                    prevWCnt = curWCnt;
                    stopWrite = true ;
                }
            }

            if ( curRCnt - prevRCnt > readTranCountPerMin ) {
                prevRCnt = curRCnt ;
                stopRead = true ;
            }

            if ( current - start >= 1000
                    && ( current - start ) / 1000 != prevUpdate ) {
                prevUpdate = ( current - start ) / 1000 ;
                int index = 0;
                while (  index++ < wBusyQueue.size() ) {
                    ECMWriteTask wTask = wBusyQueue.poll() ;
                    if ( wTask.isFini() ) {
                        wFreeQueue.offer( wTask ) ;
                    } else {
                        wBusyQueue.offer( wTask ) ;
                    }
                }
                
                index = 0;
                while ( index++ < rBusyQueue.size() ) {
                    ECMReadTask rTask = rBusyQueue.poll() ;
                    if ( rTask.isFini() ) {
                        rFreeQueue.offer( rTask ) ;
                    } else {
                        rBusyQueue.offer( rTask ) ;
                    }
                }
            }

            if ( current - start >= 10000
                    && ( current - start ) / 10000 != prevStat ) {
                prevStat = ( current - start ) / 10000 ;
                for ( ECMWriteTask wTask : wBusyQueue ) {
                    wTask.outputStat() ;
                    wTask.outputWarn() ;
                }
                
                for ( ECMWriteTask wTask : wFreeQueue ) {
                    wTask.outputStat() ;
                }

                for ( ECMReadTask rTask : rBusyQueue ) {
                    rTask.outputStat() ;
                    rTask.outputWarn() ;
                }
                
                for ( ECMReadTask rTask : rFreeQueue ) {
                    rTask.outputStat() ;
                }
                
                for ( ECMTimedTask tTask : tTaskQueue) {
                    tTask.outputStat();
                }
            }

            if ( current - start >= 60000
                    && prevMins != ( current - start ) / 60000 ) {
                prevMins = ( current - start ) / 60000 ;
                stopWrite = false ;
                stopRead = false ;
            }

            if ( stopWrite && stopRead || (wBusyQueue.size() + rBusyQueue.size()> 2 * thrdNum)) {
                try {
                    Thread.sleep( 100 ) ;
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace() ;
                }
            }
        }

        scheduledThreadPool.shutdown();
        fixedThreadPool.shutdown() ;
        this.ds.close() ;
    }

    public static void main( String[] args ) {
        if ( args.length != 1 ) {
            System.out.println( "<parameter's file>" ) ;
            return ;
        }

        Properties prop = new Properties() ;
        try {
            prop.load( new FileInputStream( new File( args[0] ) ) ) ;
        } catch ( FileNotFoundException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace() ;
            return ;
        } catch ( IOException e ) {
            e.printStackTrace() ;
            return ;
        }

        ECMTest test = new ECMTest( prop ) ;
        if ( !test.init() ) {
            return ;
        }

        test.Do() ;
    }
}
