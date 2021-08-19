package com.sequoias3.taskmanager;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DelimiterStatus;
import com.sequoias3.core.Bucket;
import com.sequoias3.core.Region;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3ServerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

@Component
@EnableScheduling
public class DelimiterClean {
    private static final Logger logger = LoggerFactory.getLogger(DelimiterClean.class);
    public final static long ONE_MINUTE = 60 * 1000;

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

    @Autowired
    DirDao dirDao;

    @Autowired
    RegionDao regionDao;

    @Scheduled(fixedDelay = ONE_MINUTE)
    public void delimiterClean(){
        String bucketName = delimiterQueue.getBucketName();
        if (bucketName == null){
            return;
        }

        try {
            Bucket bucket = bucketDao.getBucketByName(bucketName);
            if (bucket == null){
                return;
            }
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            String metaCsName = regionDao.getMetaCurCSName(region);
            if ((bucket.getDelimiter1Status() != null && bucket.getDelimiter1Status().equals(DelimiterStatus.DELETING.getName()))
                    || (bucket.getDelimiter2Status() != null && bucket.getDelimiter2Status().equals(DelimiterStatus.DELETING.getName()))){
                ConnectionDao connectionDao = daoMgr.getConnectionDao();
                transaction.begin(connectionDao);
                try {
                    if (bucket.getTaskID() != null) {
                        Long taskId = taskDao.queryTaskId(connectionDao, bucket.getTaskID());
                        if (taskId == null) {
                            Bucket curBucket = bucketDao.queryBucketForUpdate(connectionDao, bucketName);
                            if (bucket.getTaskID() == curBucket.getTaskID()){
                                //异常，taskId不存在，bucket中却还存在taskId
                                logger.error("bucket taskId exist, taskId in task table is null");
                                cleanDelimiter(connectionDao, metaCsName, curBucket);
                            }
                        }else {
                            //清理分隔符
                            Bucket curBucket = bucketDao.getBucketByName(bucketName);
                            cleanDelimiter(connectionDao, metaCsName, curBucket);
                            //删除任务表中的taskId
                            taskDao.deleteTaskId(connectionDao, taskId);
                        }
                    }else {
                        Bucket newBucket = bucketDao.queryBucketForUpdate(connectionDao, bucketName);
                        if (bucket.getTaskID() == null){
                            //异常，bucket没有taskId，分隔符状态却为deleting状态
                            logger.error("bucket taskId exist, taskId in task table is null");
                            cleanDelimiter(connectionDao, metaCsName, newBucket);
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
        }catch (Exception e){
            logger.error("clean deleting bucket failed. bucketName:"+bucketName, e);
        }
    }

    private void cleanDelimiter(ConnectionDao connectionDao, String metaCsName, Bucket bucket)
            throws S3ServerException {
        Integer delimiterFlag = null;
        if (bucket.getDelimiter1Status() != null && bucket.getDelimiter1Status().equals(DelimiterStatus.DELETING.getName())){
            dirDao.delete(connectionDao, metaCsName, bucket.getBucketId(), bucket.getDelimiter1(), null);
            delimiterFlag = 1;
        }
        if (bucket.getDelimiter2Status() != null && bucket.getDelimiter2Status().equals(DelimiterStatus.DELETING.getName())){
            dirDao.delete(connectionDao, metaCsName, bucket.getBucketId(), bucket.getDelimiter2(), null);
            delimiterFlag = 2;
        }

        if (delimiterFlag != null){
            bucketDao.cleanBucketDelimiter(connectionDao, bucket.getBucketName(), delimiterFlag);
        }
    }
}
