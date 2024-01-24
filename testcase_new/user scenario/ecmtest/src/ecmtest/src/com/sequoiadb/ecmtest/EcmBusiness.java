/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:EcmBusiness.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2017-12-6下午3:12:59
 *  @version 1.00
 */
package com.sequoiadb.ecmtest;

import java.io.BufferedInputStream ;
import java.io.File ;
import java.io.FileInputStream ;
import java.io.FileNotFoundException ;
import java.io.IOException ;
import java.text.SimpleDateFormat ;
import java.util.Date ;

import com.sequoiadb.base.DBCursor ;
import com.sequoiadb.base.DBLob ;
import com.sequoiadb.base.DBQuery ;
import com.sequoiadb.base.SequoiadbDatasource;
import com.sequoiadb.base.Sequoiadb;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.exception.BaseException ;

import org.bson.BSONObject ;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;

public class EcmBusiness {
    public SequoiadbDatasource ds;
    private long startTime = 0;
    private long endTime = 0;
    
    public EcmBusiness(SequoiadbDatasource ds){
        this.ds = ds ;
    }
    
    private ObjectId writeLob(DBCollection cl, BufferedInputStream in){
        ObjectId oid = ObjectId.get();
        DBLob lob = null;
        try {
            byte[] buff = null ;
            buff = new byte[in.available()] ;
            in.read( buff );
            
            lob = cl.createLob( oid );
            lob.write( buff );
        } catch ( IOException e1 ) {
            // TODO Auto-generated catch block
            e1.printStackTrace();
            throw new BaseException(e1.getMessage());
        }finally{
            if ( lob != null ){
                lob.close();
            }
        }
        return oid;
    }
    
    private String writeBatch( DBCollection cl, Date date ){
        final int ECM_BATCH_STATUS = 1;
        final String SYS_CREATEUSERID = "20009002";
        final String SYS_LASTCHANGEDUSERID = "20009002";
        final int SYS_VERSION = 1;
        final boolean SYS_IS_LATEST_VERSION = true;
        
        String ECM_BATCH_ID = Util.getUUID();
        String ECM_BUSI_SERIAL_NO = ECM_BATCH_ID + "WZHXT";
        BasicBSONObject batchBson = new BasicBSONObject() ;
        //BSONObject ECM_BUSI_START_TIME = new BasicBSONObject();
        //ECM_BUSI_START_TIME.put("$date", DateUtils.getDate( date ));
        String ECM_UPLOAD_TIME = DateUtils.getTimestamp( date );
        //BSONObject SYS_CREATETS = new BasicBSONObject();
        //SYS_CREATETS.put("$date", DateUtils.getDate( date ));
        //BSONObject SYS_LASTCHANGEDTS = new BasicBSONObject();
        //SYS_LASTCHANGEDTS.put("$date", DateUtils.getDate( date ));
        String SYS_BATCH_ID = Util.getBatchID() ;
        batchBson.put(Const.ECM_BATCH_ID, ECM_BATCH_ID);
        batchBson.put(Const.ECM_BUSI_START_TIME, date);
        batchBson.put(Const.ECM_BUSI_SERIAL_NO, ECM_BUSI_SERIAL_NO);
        batchBson.put(Const.ECM_BATCH_STATUS, ECM_BATCH_STATUS);
        batchBson.put(Const.ECM_UPLOAD_TIME, ECM_UPLOAD_TIME);
        batchBson.put(Const.SYS_CREATETS, date);
        batchBson.put(Const.SYS_CREATEUSERID, SYS_CREATEUSERID);
        batchBson.put(Const.SYS_LASTCHANGEDTS, date);
        batchBson.put(Const.SYS_LASTCHANGEDUSERID, SYS_LASTCHANGEDUSERID);
        batchBson.put(Const.SYS_VERSION, SYS_VERSION);
        batchBson.put(Const.SYS_IS_LATEST_VERSION, SYS_IS_LATEST_VERSION);
        batchBson.put(Const.SYS_BATCH_ID, SYS_BATCH_ID);
        cl.insert(batchBson);
        return SYS_BATCH_ID ;
    }
    
    private void writePart(DBCollection cl, Date date, ObjectId oid, String batchId, String file){
        File f = new File(file);
        int SYS_RESOURCELENGTH = 0;
        SYS_RESOURCELENGTH = ( int ) f.length() ;
        
        int ECM_IS_INDEXTYPE = 0;
        String[] names = f.getName().split( "\\." );
        String ECM_FILE_FORMAT = names.length == 2 ? names[1]:"unknown" ;
        String SYS_CL_TIME = DateUtils.getYearMonth( date );
        String ECM_FILE_NO = Util.getUUID();
        int ECM_FILE_SIZE = SYS_RESOURCELENGTH;
        
        String ECM_UPLOAD_TIME = DateUtils.getTimestamp(date);
        String SYS_ORIFNAME = f.getAbsolutePath();
        
        //BSONObject SYS_CREATETS = new BasicBSONObject();
        //SYS_CREATETS.put("$date", DateUtils.getDate(date));
        //BSONObject SYS_LASTCHANGEDTS = new BasicBSONObject();
        //SYS_LASTCHANGEDTS.put("$date", DateUtils.getDate(date));
        
        final int SYS_PYH_VERSION = 1;
        final int SYS_MIN_VERSION = 1;
        final int SYS_MAX_VERSION = 2147483647;
        final boolean SYS_IS_LATEST_VERSION = true;
        final String SYS_CREATEUSERID = "20009002";
        String SYS_LASTCHANGEDUSERID = SYS_CREATEUSERID;
        String ECM_UPLOAD_USER = SYS_CREATEUSERID;

        BSONObject fileBson = new BasicBSONObject();
        fileBson.put(Const.SYS_RESOURCELENGTH, SYS_RESOURCELENGTH);
        fileBson.put(Const.ECM_IS_INDEXTYPE, ECM_IS_INDEXTYPE);
        fileBson.put(Const.ECM_FILE_FORMAT, ECM_FILE_FORMAT);
        fileBson.put(Const.SYS_CL_TIME, SYS_CL_TIME);
        fileBson.put(Const.ECM_FILE_NO, ECM_FILE_NO);
        fileBson.put(Const.ECM_FILE_SIZE, ECM_FILE_SIZE);
        fileBson.put(Const.ECM_UPLOAD_USER, ECM_UPLOAD_USER);
        fileBson.put(Const.ECM_UPLOAD_TIME, ECM_UPLOAD_TIME);
        fileBson.put(Const.SYS_ORIFNAME, SYS_ORIFNAME);
        fileBson.put(Const.SYS_CREATETS, date);
        fileBson.put(Const.SYS_CREATEUSERID, SYS_CREATEUSERID);
        fileBson.put(Const.SYS_LASTCHANGEDTS, date);
        fileBson.put(Const.SYS_LASTCHANGEDUSERID, SYS_LASTCHANGEDUSERID);
        fileBson.put(Const.FILE_OID, oid);
        fileBson.put(Const.SYS_PYH_VERSION, SYS_PYH_VERSION);
        fileBson.put(Const.SYS_MIN_VERSION, SYS_MIN_VERSION);
        fileBson.put(Const.SYS_MAX_VERSION, SYS_MAX_VERSION);
        fileBson.put(Const.SYS_MAX_VERSION, SYS_MAX_VERSION);
        fileBson.put(Const.SYS_IS_LATEST_VERSION, SYS_IS_LATEST_VERSION);
        fileBson.put(Const.SYS_BATCH_ID, batchId);
        cl.insert( fileBson );
    }
    
    private void removeLob(DBCollection cl, ObjectId oid){
        if ( oid != null ){
            cl.removeLob( oid ) ;
        }
    }
    
    public long getStartTime(){
        return startTime;
    }
    
    public long getEndTime(){
        return endTime ;
    }
    
    public void addBatch(String file){
        Sequoiadb db = null ;
        ObjectId oid  = null;
        DBCollection lobCL = null;
        BufferedInputStream in = null;
        boolean isTranFin = false;
        try {
            startTime = System.nanoTime() ;
            in = new BufferedInputStream(new FileInputStream(new File(file))) ;
            Date date = new Date() ;
            
            db = this.ds.getConnection();
            db.beginTransaction();
            CollectionSpace cs = db.getCollectionSpace( Const.ECM_CS_NAME );
            lobCL = cs.getCollection( Const.ECM_CL_LOB_PREFIX + DateUtils.getYearMonth( date ) );
            oid = writeLob(lobCL, in);
            DBCollection cl = cs.getCollection( Const.ECM_CL_DOC );
            String batchId = writeBatch(cl, date);
            
            cl = cs.getCollection( Const.ECM_CL_PART ) ;
            writePart( cl, date, oid, batchId, file ) ;
        } catch ( FileNotFoundException | BaseException | InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            removeLob(lobCL, oid);
            db.rollback();
            isTranFin = true;
        }finally{
            if (in != null){
                try {
                    in.close();
                } catch ( IOException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                } 
            }
            if (db != null){
                if (!isTranFin){
                    db.commit();
                }
                this.ds.releaseConnection( db );
            }
            endTime = System.nanoTime();
        }
    }
    
    private String queryDoc(DBCollection cl, String batchId){
        String sysBatchId = "";
        BSONObject cond = new BasicBSONObject();
        cond.put( Const.ECM_BATCH_ID, batchId );
        
        DBCursor cursor = cl.query( cond, null, null, null, 0, -1, DBQuery.FLG_QUERY_WITH_RETURNDATA );
        while( cursor.hasNext()) {
            sysBatchId = ( String ) cursor.getNext().get( Const.SYS_BATCH_ID ) ;
        }
        cursor.close();
        return sysBatchId ;
    }
    
    private ObjectId queryPart(DBCollection cl, String batchId){
        ObjectId oid = null ;
        if (batchId.isEmpty()){
            return oid;
        }
        
        BSONObject cond = new BasicBSONObject();
        cond.put( Const.SYS_BATCH_ID, batchId );
        
        DBCursor cursor = cl.query( cond, null, null, null, 0, -1, DBQuery.FLG_QUERY_WITH_RETURNDATA );
        while( cursor.hasNext()) {
            oid =  ( ObjectId ) cursor.getNext().get( Const.FILE_OID ) ;
        }
        cursor.close();
        
        return oid;
    }
    
    private void getLob(DBCollection cl, ObjectId oid){
        if (oid == null) return;
        
        DBLob lob = null;
        byte[] buff = new byte[4096];
        try{
            lob = cl.openLob( oid );
            long size = lob.getSize();
            
            while ( -1 != lob.read( buff ));
        }finally{
            if ( null != lob ){
                lob.close();
            }
        }
    }
    
    public void queryBatch() {
        Sequoiadb db = null ;
        try {
            startTime = System.nanoTime() ;
            db = this.ds.getConnection() ;
            Date date = new Date() ;
            CollectionSpace cs = db.getCollectionSpace( Const.ECM_CS_NAME );
            DBCollection cl = cs.getCollection( Const.ECM_CL_DOC );
            int year = DateUtils.getYear( date );
            int month = DateUtils.getMonth( date );
            
            String sdate = DateUtils.getDate( year, month, DateUtils.getRandomDay( year, month ) );
            String ecmBatchId = Util.getBatchIDByDate( sdate ) ; 
            String batchId = queryDoc(cl, ecmBatchId) ;
            cl = cs.getCollection( Const.ECM_CL_PART );
            ObjectId oid = queryPart(cl, batchId);
            
            cl = cs.getCollection( Const.ECM_CL_LOB_PREFIX + DateUtils.getYearMonth( date ));
            getLob(cl, oid);
        } catch ( BaseException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace() ;
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace() ;
        } finally {
            if (db != null){
                this.ds.releaseConnection( db );
            }
            endTime = System.nanoTime();
        }
    }
    
}
