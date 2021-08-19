package com.sequoias3.taskmanager;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DelimiterStatus;
import com.sequoias3.core.Bucket;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3ServerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import java.util.List;

@Component
@EnableScheduling
public class DelimiterScan {
    private static final Logger logger = LoggerFactory.getLogger(DelimiterScan.class);
    public final static long QUARTER_HOUR = 15 * 60 * 1000;
    public final static long HALF_HOUR    = 30 * 60 * 1000;

    @Autowired
    BucketDao bucketDao;

    @Autowired
    DelimiterQueue delimiterQueue;

    @Autowired
    TaskDao taskDao;

    @Autowired
    DaoMgr daoMgr;

    @Autowired
    Transaction transaction;

    @Scheduled(initialDelay = 1000 * 10,fixedDelay = QUARTER_HOUR)
    public void delimiterScanner(){
        //deleting status
        logger.debug("delimiter task scan.");
        try {
            List<Bucket> bucketList1 = bucketDao.getBucketListByDelimiterStatus(DelimiterStatus.DELETING, HALF_HOUR);
            if (bucketList1 != null){
                for (int i = 0; i < bucketList1.size(); i++){
                    delimiterQueue.addBucketName(bucketList1.get(i).getBucketName());
                }
            }
        }catch (Exception e){
            logger.error("scan deleting failed", e);
        }

        //tobedelete status
        try{
            List<Bucket> bucketList2 = bucketDao.getBucketListByDelimiterStatus(DelimiterStatus.TOBEDELETE, HALF_HOUR);
            if (bucketList2 != null){
                for (int i = 0; i < bucketList2.size(); i++){
                    //修改状态为deleting，修改时间
                    modifyToDeleting(bucketList2.get(i).getBucketName());
                    delimiterQueue.addBucketName(bucketList2.get(i).getBucketName());
                }
            }
        }catch (Exception e){
            logger.error("scan to be delete failed", e);
        }

        //creating status
        try{
            List<Bucket> bucketList3 = bucketDao.getBucketListByDelimiterStatus(DelimiterStatus.CREATING, HALF_HOUR);
            if (bucketList3 != null){
                for (int i = 0; i < bucketList3.size(); i++){
                    logger.info("find creating delimiter. bucketName:"+bucketList3.get(i).getBucketName());
                    checkCreatingDelimiter(bucketList3.get(i));
                }
            }
        }catch (Exception e){
            logger.error("scan creating failed", e);
        }
    }

    private void modifyToDeleting(String bucketName) throws S3ServerException {
        ConnectionDao connectionDao = daoMgr.getConnectionDao();
        transaction.begin(connectionDao);
        try{
            Bucket bucket = bucketDao.queryBucketForUpdate(connectionDao, bucketName);
            Bucket newBucket = null;

            //检查status是否还是tobedelete，如果已经不是，返回，如果还是，修改为deleting，并修改时间。
            if (bucket.getDelimiter1Status().equals(DelimiterStatus.TOBEDELETE.getName())){
                newBucket = new Bucket();
                newBucket.setDelimiter1Status(DelimiterStatus.DELETING.getName());
                newBucket.setDelimiter1ModTime(System.currentTimeMillis());
            }
            else if (bucket.getDelimiter2Status().equals(DelimiterStatus.TOBEDELETE.getName())){
                newBucket = new Bucket();
                newBucket.setDelimiter2Status(DelimiterStatus.DELETING.getName());
                newBucket.setDelimiter2ModTime(System.currentTimeMillis());
            }
            if (newBucket != null) {
                bucketDao.updateBucketDelimiter(connectionDao, bucketName, newBucket);
            }
            transaction.commit(connectionDao);
        }catch (Exception e){
            transaction.rollback(connectionDao);
            logger.error("modify tobedelete to deleting failed. bucketName:"+bucketName);
        }finally {
            daoMgr.releaseConnectionDao(connectionDao);
        }
    }

    private void checkCreatingDelimiter(Bucket bucket) throws S3ServerException{
        ConnectionDao connectionDao = daoMgr.getConnectionDao();
        transaction.begin(connectionDao);
        try{
            //检查taskId是否被锁定
            taskDao.queryTaskId(connectionDao, bucket.getTaskID());
            //如果未被锁定，则再次查询并锁定桶，恢复桶的禁用的分隔符
            Bucket curBucket = bucketDao.queryBucketForUpdate(connectionDao, bucket.getBucketName());
            //检查taskId是否还是原来的taskId
            if (bucket.getTaskID() == curBucket.getTaskID()) {
                //将未创建完的分隔符改为deleting
                Bucket newBucket = null;
                if (curBucket.getDelimiter1Status().equals(DelimiterStatus.CREATING.getName())) {
                    newBucket = new Bucket();
                    newBucket.setDelimiter(2);
                    newBucket.setDelimiter2Status(DelimiterStatus.NORMAL.getName());
                    newBucket.setDelimiter2ModTime(System.currentTimeMillis());
                    newBucket.setDelimiter1Status(DelimiterStatus.DELETING.getName());
                    newBucket.setDelimiter1ModTime(System.currentTimeMillis());
                }
                if (curBucket.getDelimiter2Status().equals(DelimiterStatus.CREATING.getName())) {
                    newBucket = new Bucket();
                    newBucket.setDelimiter(1);
                    newBucket.setDelimiter1Status(DelimiterStatus.NORMAL.getName());
                    newBucket.setDelimiter1ModTime(System.currentTimeMillis());
                    newBucket.setDelimiter2Status(DelimiterStatus.DELETING.getName());
                    newBucket.setDelimiter2ModTime(System.currentTimeMillis());
                }

                if (newBucket != null) {
                    bucketDao.updateBucketDelimiter(connectionDao, bucket.getBucketName(), newBucket);
                    delimiterQueue.addBucketName(bucket.getBucketName());
                }
            }

            transaction.commit(connectionDao);
        }catch (BaseException e){
            transaction.rollback(connectionDao);
            if (e.getErrorCode() == SDBError.SDB_TIMEOUT.getErrorCode()){
                logger.error("creating delimiter. bucketName:"+bucket.getBucketName());
                return;
            }else {
                logger.error("modify creating to deleting failed. bucketName:"+bucket.getBucketName());
            }
        }catch (Exception e){
            transaction.rollback(connectionDao);
            logger.error("modify creating to deleting failed. bucketName:"+bucket.getBucketName());
        }finally {
            daoMgr.releaseConnectionDao(connectionDao);
        }
    }
}
