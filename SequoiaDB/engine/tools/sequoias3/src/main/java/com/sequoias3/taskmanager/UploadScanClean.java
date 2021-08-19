package com.sequoias3.taskmanager;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.config.MultiPartUploadConfig;
import com.sequoias3.core.Part;
import com.sequoias3.core.TaskTable;
import com.sequoias3.core.UploadMeta;
import com.sequoias3.dao.*;
import org.bson.BSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

@Component
@EnableScheduling
public class UploadScanClean {
    private static final Logger logger = LoggerFactory.getLogger(UploadScanClean.class);
    public final static long MINUTE = 60 * 1000;

    @Autowired
    UploadDao uploadDao;

    @Autowired
    PartDao partDao;

    @Autowired
    MetaDao metaDao;

    @Autowired
    DataDao dataDao;

    @Autowired
    DaoMgr daoMgr;

    @Autowired
    Transaction transaction;

    @Autowired
    MultiPartUploadConfig config;

    @Autowired
    TaskDao uploadStatusDao;

    @Scheduled(initialDelay = 1000 * 60, fixedDelay = MINUTE)
    public void uploadScanClean(){
//        logger.debug("upload clean scan begin.");
        QueryDbCursor invalidUploads = null;
        try {
            invalidUploads = uploadDao.queryInvalidUploads();
            if (invalidUploads != null){
                long cleanCompleteTime = System.currentTimeMillis() -
                        config.getCompletereservetime() * 60 * 1000;
                while (invalidUploads.hasNext()){
                    BSONObject record = invalidUploads.getNext();
                    int uploadStatus   = (int) record.get(UploadMeta.META_STATUS);
                    long lastModified  = (long) record.get(UploadMeta.META_INIT_TIME);
                    if (uploadStatus == UploadMeta.UPLOAD_COMPLETE
                            && lastModified > cleanCompleteTime){
                        continue;
                    }
                    long uploadId     = (long) record.get(UploadMeta.META_UPLOAD_ID);
                    ConnectionDao connectionA = daoMgr.getConnectionDao();
                    transaction.begin(connectionA);
                    try{
                        UploadMeta uploadMeta = uploadDao.queryUploadByUploadId(connectionA,
                                null, null, uploadId, true);
                        if (uploadMeta != null && uploadMeta.getUploadStatus() != UploadMeta.UPLOAD_INIT){
                            QueryDbCursor partList = partDao.queryPartList(uploadId, false, null, null);
                            try {
                                if (uploadMeta.getUploadStatus() == UploadMeta.UPLOAD_COMPLETE) {
                                    while (partList.hasNext()) {
                                        Part part = new Part(partList.getNext());
                                        if (!uploadMeta.getCsName().equals(part.getCsName())
                                                || !uploadMeta.getClName().equals(part.getClName())
                                                || !uploadMeta.getLobId().equals(part.getLobId())) {
                                            if (part.getLobId() != null) {
                                                dataDao.deleteObjectDataByLobId(null,
                                                        part.getCsName(), part.getClName(), part.getLobId());
                                            }
                                        }
                                    }
                                }else {
                                    while (partList.hasNext()) {
                                        Part part = new Part(partList.getNext());
                                        if (part.getLobId() != null) {
                                            dataDao.deleteObjectDataByLobId(null,
                                                    part.getCsName(), part.getClName(), part.getLobId());
                                        }
                                    }
                                }
                            }finally {
                                metaDao.releaseQueryDbCursor(partList);
                            }
                            partDao.deletePart(null, uploadId, null);
                            uploadDao.deleteUploadByUploadId(connectionA, uploadMeta.getBucketId(),
                                    uploadMeta.getKey(), uploadId);
                            uploadStatusDao.deleteUploadId(uploadId);
                        }
                        transaction.commit(connectionA);
                    }catch (Exception e){
                        transaction.rollback(connectionA);
                        logger.info("clean upload failed. uploadId:" + uploadId, e);
                    }finally {
                        daoMgr.releaseConnectionDao(connectionA);
                    }
                }
            }
        } catch (Exception e){
            logger.error("scan complete uploads failed", e);
        } finally {
            metaDao.releaseQueryDbCursor(invalidUploads);
        }

        QueryDbCursor exceedUploads = null;
        try {
            long exceedTime = System.currentTimeMillis() -
                    config.getIncompletelifecycle() * 24 * 60 * 60 * 1000;
            exceedUploads = uploadDao.queryExceedUploads(exceedTime);
            if (exceedUploads != null){
                while (exceedUploads.hasNext()){
                    long uploadId = (long) (exceedUploads.getNext()).get(UploadMeta.META_UPLOAD_ID);
                    ConnectionDao connectionB = daoMgr.getConnectionDao();
                    transaction.begin(connectionB);
                    try {
                        UploadMeta uploadMeta = uploadDao.queryUploadByUploadId(connectionB,
                                null, null, uploadId, true);
                        if (uploadMeta != null && uploadMeta.getUploadStatus() == UploadMeta.UPLOAD_INIT){
                            uploadMeta.setUploadStatus(UploadMeta.UPLOAD_ABORT);
                            uploadDao.updateUploadMeta(connectionB, uploadMeta.getBucketId(),
                                    uploadMeta.getKey(), uploadId, uploadMeta);
                        }
                        transaction.commit(connectionB);
                    } catch (Exception e){
                        transaction.rollback(connectionB);
                        logger.info("clean upload failed. uploadId:" + uploadId, e);
                    } finally {
                        daoMgr.releaseConnectionDao(connectionB);
                    }
                }
            }
        } catch (Exception e){
            logger.error("scan complete uploads failed", e);
        } finally {
            metaDao.releaseQueryDbCursor(exceedUploads);
        }

        QueryDbCursor completeStatusCursor = null;
        try {
            completeStatusCursor = uploadStatusDao.queryUploadList();
            if (completeStatusCursor != null){
                while (completeStatusCursor.hasNext()){
                    long uploadId = (long) (completeStatusCursor.getNext()).get(TaskTable.TASK_ID);
                    ConnectionDao connectionC = daoMgr.getConnectionDao();
                    transaction.begin(connectionC);
                    try {
                        uploadDao.queryUploadByUploadId(connectionC, null, null, uploadId, true);
                        uploadStatusDao.deleteUploadId(uploadId);
                        transaction.commit(connectionC);
                    } catch (BaseException e){
                        transaction.rollback(connectionC);
                        if (e.getErrorCode() == SDBError.SDB_TIMEOUT.getErrorCode()){
                            logger.warn("upload is completing. uploadId:"+uploadId);
                            continue;
                        }else {
                            logger.error("clean invalid complete status failed. uploadId:"+uploadId, e);
                        }
                    }catch (Exception e){
                        transaction.rollback(connectionC);
                        logger.error("clean invalid complete status failed. uploadId:"+uploadId, e);
                    }finally {
                        daoMgr.releaseConnectionDao(connectionC);
                    }
                }
            }
        } catch (Exception e){
            logger.error("scan complete uploads failed", e);
        } finally {
            metaDao.releaseQueryDbCursor(completeStatusCursor);
        }
//        logger.debug("upload clean scan end.");
    }
}
