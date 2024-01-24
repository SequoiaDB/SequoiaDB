package com.sequoias3.service.impl;

import com.sequoias3.common.DBParamDefine;
import com.sequoias3.common.DelimiterStatus;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.common.VersioningStatusType;
import com.sequoias3.config.MultiPartUploadConfig;
import com.sequoias3.context.Context;
import com.sequoias3.context.ContextManager;
import com.sequoias3.core.*;
import com.sequoias3.model.*;
import com.sequoias3.model.Content;
import com.sequoias3.model.ListObjectsResult;
import com.sequoias3.model.Owner;
import com.sequoias3.model.PutDeleteResult;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.service.BucketService;
import com.sequoias3.service.ObjectService;
import com.sequoias3.taskmanager.OutStreamFlushQueue;
import com.sequoias3.utils.DataFormatUtils;
import com.sequoias3.utils.DirUtils;
import com.sequoias3.utils.MD5Utils;
import com.sequoias3.utils.RestUtils;
import org.apache.commons.codec.binary.Hex;
import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import sun.misc.BASE64Decoder;

import javax.servlet.ServletOutputStream;
import java.io.InputStream;
import java.security.MessageDigest;
import java.util.*;

import static com.sequoias3.utils.DataFormatUtils.parseDate;

@Service
public class ObjectServiceImpl implements ObjectService {
    private static final Logger logger = LoggerFactory.getLogger(ObjectServiceImpl.class);

    @Autowired
    BucketDao bucketDao;

    @Autowired
    BucketService bucketService;

    @Autowired
    UserDao userDao;

    @Autowired
    DataDao dataDao;

    @Autowired
    MetaDao metaDao;

    @Autowired
    ContextManager contextManager;

    @Autowired
    OutStreamFlushQueue outStreamFlushQueue;

    @Autowired
    DaoMgr daoMgr;

    @Autowired
    Transaction transaction;

    @Autowired
    RegionDao regionDao;

    @Autowired
    DirDao dirDao;

    @Autowired
    IDGeneratorDao idGenerator;

    @Autowired
    UploadDao uploadDao;

    @Autowired
    PartDao partDao;

    @Autowired
    MultiPartUploadConfig multiPartUploadConfig;

    @Autowired
    TaskDao uploadStatusDao;

    @Autowired
    RestUtils restUtils;

    @Autowired
    AclDao aclDao;

    @Override
    public PutDeleteResult putObject(Bucket bucket, String objectName,
                                     String contentMD5, Map<String, String> headers,
                                     Map<String, String> xMeta, InputStream inputStream,
                                     Long contentLength)
            throws S3ServerException {
        Region region = null;
        if (bucket.getRegion() != null) {
            region = regionDao.queryRegion(bucket.getRegion());
        }

        //get cs and cl
        Date createDate      = new Date();
        String dataCsName    = regionDao.getDataCSName(region, createDate);
        String dataClName    = regionDao.getDataClName(region, createDate);

        DataAttr insertResult;
        try {//insert lob
            insertResult = dataDao.insertObjectData(dataCsName, dataClName, inputStream, region);
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.OBJECT_PUT_fAILED, "put object failed.", e);
        }

        try {
            //check md5
            if (null != contentMD5) {
                if (!MD5Utils.isMd5EqualWithETag(contentMD5, insertResult.geteTag())) {
                    throw new S3ServerException(S3Error.OBJECT_BAD_DIGEST,
                            "The Content-MD5 you specified does not match what we received.");
                }
            }

            if (contentLength != null && insertResult.getSize() != contentLength){
                throw new S3ServerException(S3Error.OBJECT_INCOMPLETE_BODY,
                        "content length is " + contentLength +
                                " and receive " + insertResult.getSize() + " bytes");
            }

            VersioningStatusType versioningStatusType = VersioningStatusType.getVersioningStatus(bucket.getVersioningStatus());

            //build meta
            ObjectMeta objectMeta = buildObjectMeta(objectName, bucket.getBucketId(),
                    headers, xMeta, dataCsName, dataClName, false,
                    generateNoVersionFlag(versioningStatusType));
            objectMeta.seteTag(insertResult.geteTag());
            objectMeta.setSize(insertResult.getSize());
            objectMeta.setLobId(insertResult.getLobId());

            ObjectMeta deleteObject = writeObjectMeta(objectMeta, objectName, bucket.getBucketId(),
                    versioningStatusType, region);
            deleteObjectLobAndAcl(deleteObject);

            //build response
            PutDeleteResult response = new PutDeleteResult();
            response.seteTag(insertResult.geteTag());
            if (VersioningStatusType.ENABLED == versioningStatusType){
                response.setVersionId(String.valueOf(objectMeta.getVersionId()));
            }else if(VersioningStatusType.SUSPENDED == versioningStatusType){
                response.setVersionId(ObjectMeta.NULL_VERSION_ID);
            }
            return response;
        }catch (S3ServerException e){
            if (e.getError() != S3Error.DAO_TRANSACTION_COMMIT_FAILED) {
                cleanRedundencyLob(dataCsName, dataClName, insertResult.getLobId());
            }
            if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex()) {
                throw new S3ServerException(S3Error.OBJECT_PUT_fAILED,
                        "bucket+key duplicate too times. bucket:"+bucket.getBucketName() +" key:"+objectName, e);
            } else {
                throw e;
            }
        }catch (Exception e){
            cleanRedundencyLob(dataCsName, dataClName, insertResult.getLobId());
            throw new S3ServerException(S3Error.OBJECT_PUT_fAILED, "put object failed.", e);
        }
    }

    @Override
    public GetResult getObject(long ownerID, String bucketName, String objectName,
                               Long versionId, Boolean isNoVersion, Map headers,
                               Range range)
            throws S3ServerException {
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);

            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }

            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);
            String metaHisCsName = regionDao.getMetaHisCSName(region);
            String metaHisClName = regionDao.getMetaHisCLName(region);

            int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;
            while (tryTime > 0) {
                tryTime--;
                try {
                    ObjectMeta versionIdMeta = getObjectMeta(metaCsName, metaClName,
                            metaHisCsName, metaHisClName, bucket.getBucketId(),
                            objectName, versionId, isNoVersion);

                    DataLob dataLob = null;
                    if (!versionIdMeta.getDeleteMarker()){
                        checkMatchModify(headers, versionIdMeta);
                        dataLob = dataDao.getDataLobForRead(versionIdMeta.getCsName(), versionIdMeta.getClName(),
                                versionIdMeta.getLobId());
                        try {
                            analyseRangeWithLob(range, dataLob);
                        }catch (Exception e){
                            dataDao.releaseDataLob(dataLob);
                            throw e;
                        }
                    }
                    return new GetResult(versionIdMeta, dataLob);
                }catch (S3ServerException e){
                    if (e.getError() == S3Error.DAO_LOB_FNE){
                        continue;
                    }else{
                        throw e;
                    }
                } catch (Exception e){
                    throw e;
                }
            }
            throw new S3ServerException(S3Error.OBJECT_NO_SUCH_KEY,
                    "Lob is not find");
        }catch (S3ServerException e) {
            throw e;
        } catch (Exception e) {
            throw new S3ServerException(S3Error.OBJECT_GET_FAILED,
                    "get object failed. bucket:" + bucketName + ", object=" + objectName, e);
        }
    }

    @Override
    public CopyObjectResult copyObject(long ownerID, String destBucket, String destObject,
                           Map<String, String> requestHeaders,
                           Map<String, String> xMeta, ObjectUri sourceUri,
                           boolean directiveCopy, ServletOutputStream outputStream)
            throws S3ServerException {
        try {
            Bucket bucket = bucketService.getBucket(ownerID, destBucket);
            CopyObjectResult copyObjectResult = new CopyObjectResult();

            //get source object meta
            ObjectMeta sourceMeta = getSourceObjectMeta(ownerID, requestHeaders, sourceUri);
            if (sourceMeta.getDeleteMarker()) {
                if (sourceUri.isWithVersionId()) {
                    throw new S3ServerException(S3Error.OBJECT_COPY_DELETE_MARKER, "source object with versionId is a deleteMarker");
                } else {
                    throw new S3ServerException(S3Error.OBJECT_NO_SUCH_KEY, "source object is a deleteMarker");
                }
            }
            //check if itself change, 参照s3顺序，先判断对象合法存在
            if (destBucket.equals(sourceUri.getBucketName())
                    && destObject.equals(sourceUri.getObjectName())
                    && !sourceUri.isWithVersionId()) {
                if (directiveCopy) {
                    throw new S3ServerException(S3Error.OBJECT_COPY_WITHOUT_CHANGE, "copy an object to itself without changing the object's metadata.");
                }
            }

            if (!directiveCopy){
                if (restUtils.getXMetaLength(xMeta) > RestParamDefine.X_AMZ_META_LENGTH) {
                    throw new S3ServerException(S3Error.OBJECT_METADATA_TOO_LARGE,
                            "metadata headers exceed the maximum. xMeta:" + xMeta.toString());
                }
            }

            //create a new lob
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            Date createDate = new Date();
            String dataCsName = regionDao.getDataCSName(region, createDate);
            String dataClName = regionDao.getDataClName(region, createDate);

            copyObjectResult.seteTag(sourceMeta.geteTag());

            VersioningStatusType versioningStatusType = VersioningStatusType.getVersioningStatus(bucket.getVersioningStatus());
            ObjectMeta deleteObject = null;
            Long flushIndex = null;
            DataAttr newLobData = dataDao.createNewData(dataCsName, dataClName, region);
            try {
                flushIndex = outStreamFlushQueue.add(outputStream);
                if (sourceMeta.getSize() > 0) {
                    dataDao.copyObjectData(dataCsName, dataClName, newLobData.getLobId(),
                            0, sourceMeta.getCsName(), sourceMeta.getClName(),
                            sourceMeta.getLobId(), 0, sourceMeta.getSize());
                }
                ObjectMeta objectMeta;
                if (directiveCopy) {
                    objectMeta = sourceMeta;
                    objectMeta.setLastModified(System.currentTimeMillis());
                    objectMeta.setBucketId(bucket.getBucketId());
                    objectMeta.setKey(destObject);
                    objectMeta.setCsName(dataCsName);
                    objectMeta.setClName(dataClName);
                    objectMeta.setLobId(newLobData.getLobId());
                    objectMeta.setNoVersionFlag(generateNoVersionFlag(versioningStatusType));
                    objectMeta.setAclId(null);
                } else {
                    objectMeta = buildObjectMeta(destObject, bucket.getBucketId(),
                            requestHeaders, xMeta, dataCsName, dataClName, false,
                            generateNoVersionFlag(versioningStatusType));
                    objectMeta.seteTag(sourceMeta.geteTag());
                    objectMeta.setSize(sourceMeta.getSize());
                    objectMeta.setLobId(newLobData.getLobId());
                }

                deleteObject = writeObjectMeta(objectMeta, destObject,
                        bucket.getBucketId(), versioningStatusType, region);

                deleteObjectLobAndAcl(deleteObject);
                copyObjectResult.setLastModified(DataFormatUtils.formatDate(objectMeta.getLastModified()));
                if (!sourceMeta.getNoVersionFlag()){
                    copyObjectResult.setSourceVersionId(sourceMeta.getVersionId());
                }
                if (!objectMeta.getNoVersionFlag()){
                    copyObjectResult.setVersionId(objectMeta.getVersionId());
                }
            } catch (S3ServerException e) {
                if (e.getError() != S3Error.DAO_TRANSACTION_COMMIT_FAILED) {
                    cleanRedundencyLob(dataCsName, dataClName, newLobData.getLobId());
                }
                throw e;
            } catch (Exception e) {
                cleanRedundencyLob(dataCsName, dataClName, newLobData.getLobId());
                throw new S3ServerException(S3Error.OBJECT_COPY_FAILED,
                        "copy object failed. bucket:" + destBucket + ", object=" + destObject, e);
            } finally {
                outStreamFlushQueue.remove(flushIndex, outputStream);
            }
            return copyObjectResult;
        } catch (S3ServerException e) {
            throw e;
        } catch (Exception e) {
            throw new S3ServerException(S3Error.OBJECT_COPY_FAILED,
                    "copy object failed. bucket:" + destBucket + ", object=" + destObject, e);
        }
    }

    public ObjectMeta getSourceObjectMeta(long ownerID, Map<String, String> requestHeaders,
                                          ObjectUri sourceUri)
            throws S3ServerException{
        Bucket bucket = bucketService.getBucket(ownerID, sourceUri.getBucketName());

        Region region = null;
        if (bucket.getRegion() != null) {
            region = regionDao.queryRegion(bucket.getRegion());
        }

        String metaCsName    = regionDao.getMetaCurCSName(region);
        String metaClName    = regionDao.getMetaCurCLName(region);
        String metaHisCsName = regionDao.getMetaHisCSName(region);
        String metaHisClName = regionDao.getMetaHisCLName(region);

        ObjectMeta sourceMeta = getObjectMeta(metaCsName, metaClName, metaHisCsName, metaHisClName,
                bucket.getBucketId(), sourceUri.getObjectName(), sourceUri.getVersionId(), sourceUri.getNullVersionFlag());

        if (!sourceMeta.getDeleteMarker() && requestHeaders != null) {
            Map<String, String> matchHeaders = new HashMap<>();
            matchHeaders.put(RestParamDefine.GetObjectReqHeader.REQ_IF_MATCH, requestHeaders.get(RestParamDefine.CopyObjectHeader.IF_MATCH));
            matchHeaders.put(RestParamDefine.GetObjectReqHeader.REQ_IF_NONE_MATCH, requestHeaders.get(RestParamDefine.CopyObjectHeader.IF_NONE_MATCH));
            matchHeaders.put(RestParamDefine.GetObjectReqHeader.REQ_IF_MODIFIED_SINCE, requestHeaders.get(RestParamDefine.CopyObjectHeader.IF_MODIFIED_SINCE));
            matchHeaders.put(RestParamDefine.GetObjectReqHeader.REQ_IF_UNMODIFIED_SINCE, requestHeaders.get(RestParamDefine.CopyObjectHeader.IF_UNMODIFIED_SINCE));
            checkMatchModify(matchHeaders, sourceMeta);
        }

        return sourceMeta;
    }

    private ObjectMeta getObjectMeta(String metaCsName, String metaClName,
                                     String metaHisCsName, String metaHisClName,
                                     long bucketId, String objectName,
                                     Long versionId, Boolean isNoVersion)
            throws  S3ServerException{
        ObjectMeta versionIdMeta;
        ObjectMeta objectMeta = metaDao.queryMetaByObjectName(metaCsName, metaClName,
                bucketId, objectName, null, null);
        if (null == objectMeta) {
            if (versionId != null || isNoVersion != null){
                throw new S3ServerException(S3Error.OBJECT_NO_SUCH_VERSION,
                        "no such version. objectName:" + objectName + ",version:" + versionId);
            }else {
                throw new S3ServerException(S3Error.OBJECT_NO_SUCH_KEY, "no such key. objectName:" + objectName);
            }
        }

        if (versionId != null) {
            if (versionId == objectMeta.getVersionId() && !objectMeta.getNoVersionFlag()){
                versionIdMeta = objectMeta;
            }else {
                ObjectMeta objectMetaHis = metaDao.queryMetaByObjectName(metaHisCsName,
                        metaHisClName, bucketId, objectName, versionId, false);
                if (null == objectMetaHis) {
                    throw new S3ServerException(S3Error.OBJECT_NO_SUCH_VERSION,
                            "no such version. object:" + objectName + ",version:" + versionId);
                }
                versionIdMeta = objectMetaHis;
            }
        }else if(isNoVersion != null) {
            if (objectMeta.getNoVersionFlag()) {
                versionIdMeta = objectMeta;
            } else {
                ObjectMeta objectMetaHis = metaDao.queryMetaByObjectName(metaHisCsName,
                        metaHisClName, bucketId, objectName, null, true);
                if (null == objectMetaHis) {
                    throw new S3ServerException(S3Error.OBJECT_NO_SUCH_VERSION,
                            "no such version. object:" + objectName + ",version:" + versionId);
                }
                versionIdMeta = objectMetaHis;
            }
        }else{
            versionIdMeta = objectMeta;
        }
        return versionIdMeta;
    }

    @Override
    public void readObjectData(DataLob data, ServletOutputStream outputStream, Range range)
            throws S3ServerException{
        if (data == null || outputStream == null){
            throw new S3ServerException(S3Error.OBJECT_GET_FAILED,
                    "get object data failed. ");
        }
        try{
            data.read(outputStream, range);
        }catch (S3ServerException e){
            throw e;
        } catch (Exception e) {
            throw new S3ServerException(S3Error.OBJECT_GET_FAILED,
                    "get object data failed. ", e);
        }
    }

    @Override
    public void releaseGetResult(GetResult result){
        dataDao.releaseDataLob(result.getData());
    }

    @Override
    public PutDeleteResult deleteObject(long ownerID, String bucketName, String objectName)
            throws S3ServerException {
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);

            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }

            VersioningStatusType versioningStatusType = VersioningStatusType.getVersioningStatus(bucket.getVersioningStatus());
            Boolean noVersionFlag = generateNoVersionFlag(versioningStatusType);

            PutDeleteResult response = null;
            switch (versioningStatusType) {
                case NONE:
                    String metaCsName    = regionDao.getMetaCurCSName(region);
                    String metaClName    = regionDao.getMetaCurCLName(region);
                    ObjectMeta objectMeta = removeMetaForNoneVersioning(metaCsName, metaClName, bucket, objectName);
                    deleteObjectLobAndAcl(objectMeta);
                    return null;
                case SUSPENDED:
                case ENABLED:
                    ObjectMeta deleteMarker = buildObjectMeta(objectName,
                            bucket.getBucketId(), null, null,
                            null, null, true,
                            noVersionFlag);
                    ObjectMeta oldObject = writeObjectMeta(deleteMarker, objectName,
                            bucket.getBucketId(), versioningStatusType, region);
                    deleteObjectLobAndAcl(oldObject);

                    response = new PutDeleteResult();
                    if (deleteMarker.getNoVersionFlag()){
                        response.setVersionId(ObjectMeta.NULL_VERSION_ID);
                    }else {
                        response.setVersionId(String.valueOf(deleteMarker.getVersionId()));
                    }
                    response.setDeleteMarker(true);
                    break;
                default:
                    break;
            }
            return response;
        }catch (S3ServerException e) {
            if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex()) {
                throw new S3ServerException(S3Error.OBJECT_DELETE_FAILED,
                        "bucket+key duplicate too times. bucket:"+bucketName+" key:"+objectName);
            } else {
                throw e;
            }
        } catch (Exception e) {
            throw new S3ServerException(S3Error.OBJECT_DELETE_FAILED,
                    "delete object failed. bucket:" + bucketName + ", object=" + objectName, e);
        }
    }

    @Override
    public PutDeleteResult deleteObject(long ownerID, String bucketName, String objectName,
                                        Long versionId)
            throws S3ServerException {
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);

            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }

            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);
            String metaHisCsName = regionDao.getMetaHisCSName(region);
            String metaHisClName = regionDao.getMetaHisCLName(region);

            VersioningStatusType versioningStatusType = VersioningStatusType.getVersioningStatus(bucket.getVersioningStatus());
            ObjectMeta deleteObject = null;
            switch (versioningStatusType){
                case NONE:
                    if (versionId == null) {
                        ObjectMeta objectMeta = removeMetaForNoneVersioning(metaCsName, metaClName, bucket, objectName);
                        deleteObject = objectMeta;
                    }
                    break;
                case SUSPENDED:
                case ENABLED:
                    /* Try one more time if specified version does not found.
                     * Maybe the specified version is moving from
                     * history meta cl to meta cl*/
                    int tryTime = 2;
                    while(tryTime > 0){
                        tryTime--;
                        ConnectionDao connection = daoMgr.getConnectionDao();
                        transaction.begin(connection);
                        try{
                            ObjectMeta objectMeta = null;
                            if (versionId == null){
                                objectMeta = metaDao.queryForUpdate(connection, metaCsName,
                                        metaClName, bucket.getBucketId(), objectName, null, true);
                            }else{
                                objectMeta = metaDao.queryForUpdate(connection, metaCsName,
                                        metaClName, bucket.getBucketId(), objectName, versionId, false);
                            }

                            if (objectMeta != null){
                                deleteObject = objectMeta;
                                ObjectMeta objectMeta1 = metaDao.queryForUpdate(connection, metaHisCsName,
                                        metaHisClName, bucket.getBucketId(), objectName, null, null);
                                if (objectMeta1 != null){
                                    objectMeta1.setParentId1(objectMeta.getParentId1());
                                    objectMeta1.setParentId2(objectMeta.getParentId2());
                                    metaDao.updateMeta(connection, metaCsName, metaClName, bucket.getBucketId(),
                                            objectName, null, objectMeta1);
                                    metaDao.removeMeta(connection, metaHisCsName, metaHisClName, bucket.getBucketId(),
                                            objectName, objectMeta1.getVersionId(), null);
                                }else {
                                    deleteDirForObject(connection, metaCsName, metaClName, bucket, objectName);
                                    metaDao.removeMeta(connection, metaCsName, metaClName, bucket.getBucketId(),
                                            objectName, objectMeta.getVersionId(), null);
                                    Bucket newBucket = bucketDao.getBucketById(null, bucket.getBucketId());
                                    if (newBucket != null && newBucket.getDelimiter() != bucket.getDelimiter()){
                                        deleteDirForObject(connection, metaCsName, metaClName, newBucket, objectName);
                                    }
                                }
                            }else{
                                ObjectMeta objectMeta2 = null;
                                if (versionId == null){
                                    objectMeta2 = metaDao.queryForUpdate(connection, metaHisCsName,
                                            metaHisClName, bucket.getBucketId(), objectName, null, true);
                                }else {
                                    objectMeta2 = metaDao.queryForUpdate(connection, metaHisCsName,
                                            metaHisClName, bucket.getBucketId(), objectName, versionId, false);
                                }

                                if (objectMeta2 != null){
                                    deleteObject = objectMeta2;
                                    metaDao.removeMeta(connection, metaHisCsName, metaHisClName, bucket.getBucketId(),
                                            objectName, objectMeta2.getVersionId(), null);
                                }else{
                                    logger.debug("can not find the key:{} with versionId:{} in history", objectName, versionId);
                                    throw new S3ServerException(S3Error.OBJECT_NO_SUCH_VERSION, "");
                                }
                            }
                            transaction.commit(connection);
                            break;
                        } catch (S3ServerException e){
                            transaction.rollback(connection);
                            if (e.getError() == S3Error.OBJECT_NO_SUCH_VERSION){
                                continue;
                            }else {
                                throw e;
                            }
                        } catch(Exception e){
                            transaction.rollback(connection);
                            throw e;
                        } finally {
                            daoMgr.releaseConnectionDao(connection);
                        }
                    }
                    break;
                default:
                    break;
            }

            deleteObjectLobAndAcl(deleteObject);
            PutDeleteResult response = null;
            if (deleteObject != null && deleteObject.getDeleteMarker()){
                response = new PutDeleteResult();
                response.setDeleteMarker(true);
                response.setVersionId(String.valueOf(versionId));
            }
            return response;
        }catch (S3ServerException e) {
            throw e;
        } catch (Exception e) {
            throw new S3ServerException(S3Error.OBJECT_DELETE_FAILED,
                    "delete object failed. bucket:" + bucketName + ", object=" + objectName, e);
        }
    }

    @Override
    public ListObjectsResult listObjects(long ownerID, String bucketName, String prefix,
                                         String delimiter, String startAfter, Integer maxKeys,
                                         String continueToken, String encodingType, Boolean fetchOwner)
            throws S3ServerException {
        Context queryContext = null;
        QueryDbCursor dbCursorKeys = null;
        QueryDbCursor dbCursorDir = null;
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            ListObjectsResult listObjectsResult = new ListObjectsResult(bucketName, maxKeys,
                    encodingType, prefix, startAfter, delimiter, continueToken);

            if (maxKeys == 0) {
                return listObjectsResult;
            }

            if (delimiter != null && delimiter.length() == 0) {
                delimiter = null;
            }

            if (prefix != null && prefix.length() == 0) {
                prefix = null;
            }

            if (startAfter != null && startAfter.length() == 0){
                startAfter = null;
            }

            String metaCsName = regionDao.getMetaCurCSName(region);
            String metaClName = regionDao.getMetaCurCLName(region);

            String newStartAfter = startAfter;
            if (null != continueToken) {
                queryContext = contextManager.get(continueToken);
                if (null == queryContext) {
                    throw new S3ServerException(S3Error.OBJECT_INVALID_TOKEN,
                            "The continuation token provided is incorrect.token:"+continueToken);
                }
                if (!IsContextMatch(queryContext, prefix, startAfter, delimiter)){
                    queryContext = null;
                }else{
                    newStartAfter = queryContext.getLastKey();
                }
            }

            Owner owner = null;
            if (fetchOwner){
                owner = userDao.getOwnerByUserID(ownerID);
            }
            int count = 0;
            int maxNumber = Math.min(maxKeys, RestParamDefine.MAX_KEYS_DEFAULT);

            String parentIdName = getParentName(delimiter, bucket);
            ObjectsFilter filter;
            if (parentIdName != null){
                Long parentId = getParentId(metaCsName, bucket.getBucketId(), prefix, delimiter);
                dbCursorDir = dirDao.queryDirList(metaCsName, bucket.getBucketId(),
                        delimiter, prefix, newStartAfter);
                if (parentId != null){
                    dbCursorKeys = metaDao.queryMetaListByParentId(metaCsName,
                            metaClName, bucket.getBucketId(), parentIdName, parentId,
                            prefix, newStartAfter, null, false);
                }

                filter = new ObjectsFilter(dbCursorDir, dbCursorKeys,
                        prefix, delimiter, newStartAfter, encodingType, FilterRecord.FILTER_DIR);
            }else {
                dbCursorKeys = metaDao.queryMetaByBucket(metaCsName, metaClName,
                        bucket.getBucketId(), prefix, newStartAfter,
                        null, false);

                if (null == delimiter){
                    filter = new ObjectsFilter(null, dbCursorKeys,
                            prefix, delimiter, newStartAfter, encodingType, FilterRecord.FILTER_NO_DELIMITER);
                }else {
                    filter = new ObjectsFilter(null, dbCursorKeys,
                            prefix, delimiter, newStartAfter, encodingType, FilterRecord.FILTER_DELIMITER);
                }
            }

            if (queryContext != null){
                filter.setLastCommonPrefix(queryContext.getLastCommonPrefix());
            }

            LinkedHashSet<Content> contentList = listObjectsResult.getContentList();
            LinkedHashSet<CommonPrefix> commonPrefixesList = listObjectsResult.getCommonPrefixList();
            FilterRecord matcher;
            while ((matcher =  filter.getNextRecord()) != null){
                if (matcher.getRecordType() == FilterRecord.COMMONPREFIX){
                    if (!commonPrefixesList.contains(matcher.getCommonPrefix())) {
                        commonPrefixesList.add(matcher.getCommonPrefix());
                        count++;
                    }
                }else if (matcher.getRecordType() == FilterRecord.CONTENT){
                    matcher.getContent().setOwner(owner);
                    contentList.add(matcher.getContent());
                    count++;
                }

                if (count >= maxNumber){
                    break;
                }
            }

            listObjectsResult.setKeyCount(count);
            if (filter.hasNext()){
                if (null == queryContext) {
                    queryContext = contextManager.create(bucket.getBucketId());
                    queryContext.setPrefix(prefix);
                    queryContext.setStartAfter(startAfter);
                    queryContext.setDelimiter(delimiter);
                }

                queryContext.setLastKey(filter.getLastKey());
                queryContext.setLastCommonPrefix(filter.getLastCommonPrefix());
                listObjectsResult.setIsTruncated(true);
                listObjectsResult.setNextContinueToken(queryContext.getToken());
            }else {
                contextManager.release(queryContext);
            }

            return listObjectsResult;
        } catch (S3ServerException e){
            contextManager.release(queryContext);
            throw e;
        } catch (Exception e){
            contextManager.release(queryContext);
            throw new S3ServerException(S3Error.OBJECT_LIST_FAILED, "error message:"+e.getMessage(), e);
        }finally {
            metaDao.releaseQueryDbCursor(dbCursorDir);
            metaDao.releaseQueryDbCursor(dbCursorKeys);
        }
    }

    @Override
    public ListObjectsResultV1 listObjectsV1(long ownerID, String bucketName, String prefix, String delimiter, String startAfter, Integer maxKeys, String encodingType)
            throws S3ServerException {
        QueryDbCursor dbCursorKeys = null;
        QueryDbCursor dbCursorDir = null;
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            ListObjectsResultV1 listObjectsResult = new ListObjectsResultV1(bucketName, maxKeys,
                    encodingType, prefix, startAfter, delimiter);

            if (maxKeys == 0) {
                return listObjectsResult;
            }

            if (delimiter != null && delimiter.length() == 0) {
                delimiter = null;
            }

            if (prefix != null && prefix.length() == 0) {
                prefix = null;
            }

            String metaCsName = regionDao.getMetaCurCSName(region);
            String metaClName = regionDao.getMetaCurCLName(region);

            Owner owner = userDao.getOwnerByUserID(ownerID);
            int count = 0;
            int maxNumber = Math.min(maxKeys, RestParamDefine.MAX_KEYS_DEFAULT);

            String parentIdName = getParentName(delimiter, bucket);
            ObjectsFilter filter;
            if (parentIdName != null){
                Long parentId = getParentId(metaCsName, bucket.getBucketId(), prefix, delimiter);
                dbCursorDir = dirDao.queryDirList(metaCsName, bucket.getBucketId(),
                        delimiter, prefix, startAfter);
                if (parentId != null){
                    dbCursorKeys = metaDao.queryMetaListByParentId(metaCsName,
                            metaClName, bucket.getBucketId(), parentIdName, parentId,
                            prefix, startAfter, null, false);
                }

                filter = new ObjectsFilter(dbCursorDir, dbCursorKeys,
                        prefix, delimiter, startAfter, encodingType, FilterRecord.FILTER_DIR);
            }else {
                dbCursorKeys = metaDao.queryMetaByBucket(metaCsName, metaClName,
                        bucket.getBucketId(), prefix, startAfter,
                        null, false);

                if (null == delimiter){
                    filter = new ObjectsFilter(null, dbCursorKeys,
                            prefix, delimiter, startAfter, encodingType, FilterRecord.FILTER_NO_DELIMITER);
                }else {
                    filter = new ObjectsFilter(null, dbCursorKeys,
                            prefix, delimiter, startAfter, encodingType, FilterRecord.FILTER_DELIMITER);
                }
            }

            LinkedHashSet<Content> contentList = listObjectsResult.getContentList();
            LinkedHashSet<CommonPrefix> commonPrefixesList = listObjectsResult.getCommonPrefixList();
            FilterRecord matcher;
            while ((matcher =  filter.getNextRecord()) != null){
                if (matcher.getRecordType() == FilterRecord.COMMONPREFIX){
                    if (!commonPrefixesList.contains(matcher.getCommonPrefix())) {
                        commonPrefixesList.add(matcher.getCommonPrefix());
                        count++;
                    }
                }else if (matcher.getRecordType() == FilterRecord.CONTENT){
                    matcher.getContent().setOwner(owner);
                    contentList.add(matcher.getContent());
                    count++;
                }

                if (count >= maxNumber){
                    break;
                }
            }

            if (filter.hasNext()){
                listObjectsResult.setIsTruncated(true);
                if (matcher.getRecordType() == FilterRecord.COMMONPREFIX){
                    listObjectsResult.setNextMarker(matcher.getCommonPrefix().getPrefix());
                }else if (matcher.getRecordType() == FilterRecord.CONTENT){
                    listObjectsResult.setNextMarker(matcher.getContent().getKey());
                }
            }

            return listObjectsResult;
        } catch (S3ServerException e){
            throw e;
        } catch (Exception e){
            throw new S3ServerException(S3Error.OBJECT_LIST_V1_FAILED, "error message:"+e.getMessage(), e);
        }finally {
            metaDao.releaseQueryDbCursor(dbCursorDir);
            metaDao.releaseQueryDbCursor(dbCursorKeys);
        }
    }

    @Override
    public ListVersionsResult listVersions(long ownerID, String bucketName, String prefix,
                                           String delimiter, String keyMarker, String versionIdMarker,
                                           Integer maxKeys, String encodingType)
            throws S3ServerException {
        QueryDbCursor queryDbCursorCur = null;
        QueryDbCursor queryDbCursorHis = null;
        QueryDbCursor queryDbCursorDir = null;
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            ListVersionsResult listVersionsResult = new ListVersionsResult(bucketName, maxKeys,
                    encodingType, prefix, delimiter, keyMarker, versionIdMarker);

            if (maxKeys == 0) {
                return listVersionsResult;
            }
            
            if (delimiter != null && delimiter.length() == 0) {
                delimiter = null;
            }

            if (prefix != null && prefix.length() == 0) {
                prefix = null;
            }

            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);
            String metaHisCsName = regionDao.getMetaHisCSName(region);
            String metaHisClName = regionDao.getMetaHisCLName(region);
            //get sdb and cursor
            Long specifiedVersionId = null;
            if (versionIdMarker != null && versionIdMarker.length() > 0){
                specifiedVersionId = getInnerVersionId(metaCsName, metaClName, metaHisCsName, metaHisClName,
                        keyMarker, versionIdMarker,bucket);
            }

            int count = 0;
            int maxNumber = Math.min(maxKeys, RestParamDefine.MAX_KEYS_DEFAULT);
            Owner owner = userDao.getOwnerByUserID(ownerID);

            VersionsFilter filter;
            String parentIdName = getParentName(delimiter, bucket);
            if (parentIdName == null) {
                queryDbCursorCur = metaDao.queryMetaByBucket(metaCsName, metaClName,
                        bucket.getBucketId(), prefix, keyMarker, specifiedVersionId, true);
                if (queryDbCursorCur == null) {
                    return listVersionsResult;
                }

                queryDbCursorHis = metaDao.queryMetaByBucket(metaHisCsName, metaHisClName,
                        bucket.getBucketId(), prefix, keyMarker, specifiedVersionId, true);

                if (delimiter != null) {
                    filter = new VersionsFilter(queryDbCursorCur,
                            queryDbCursorDir, queryDbCursorHis, prefix, delimiter,
                            keyMarker, specifiedVersionId, encodingType, owner,
                            FilterRecord.FILTER_DELIMITER);
                }else {
                    filter = new VersionsFilter(queryDbCursorCur,
                            queryDbCursorDir, queryDbCursorHis, prefix, delimiter,
                            keyMarker, specifiedVersionId, encodingType, owner,
                            FilterRecord.FILTER_NO_DELIMITER);
                }
            }else {
                Long parentId = getParentId(metaCsName, bucket.getBucketId(), prefix, delimiter);
                queryDbCursorDir = dirDao.queryDirList(metaCsName, bucket.getBucketId(),
                        delimiter, prefix, keyMarker);
                if (parentId != null){
                    queryDbCursorCur = metaDao.queryMetaListByParentId(metaCsName,
                            metaClName, bucket.getBucketId(), parentIdName, parentId,
                            prefix, keyMarker, specifiedVersionId, true);
                }

                filter = new VersionsFilter(queryDbCursorCur, queryDbCursorDir,
                        null, prefix, delimiter, keyMarker, specifiedVersionId,
                        encodingType, owner, FilterRecord.FILTER_DIR);
            }

            FilterRecord matcher = null;
            LinkedHashSet<CommonPrefix> commonPrefixesList = listVersionsResult.getCommonPrefixList();
            while (count < maxNumber){
                if (!filter.isFilterReady()){
                    metaDao.releaseQueryDbCursor(queryDbCursorHis);
                    List<String> keys = filter.getCurKeys();
                    if (keys.size() > 0){
                        queryDbCursorHis = metaDao.queryMetaByBucketInKeys(metaHisCsName, metaHisClName, bucket.getBucketId(), keys);
                        filter.setHisCursor(queryDbCursorHis);
                    }
                }

                if ((matcher = filter.getNextRecord()) != null){
                    if (matcher.getRecordType() == FilterRecord.COMMONPREFIX){
                        if (!commonPrefixesList.contains(matcher.getCommonPrefix())){
                            commonPrefixesList.add(matcher.getCommonPrefix());
                            count++;
                        }
                    }else if (matcher.getRecordType() == FilterRecord.DELETEMARKER){
                        listVersionsResult.getDeleteMarkerList().add(matcher.getDeleteMarker());
                        count++;
                    }else if (matcher.getRecordType() == FilterRecord.VERSION){
                        listVersionsResult.getVersionList().add(matcher.getVersion());
                        count++;
                    }
                }else {
                    break;
                }
            }

            if (filter.hasNext()){
                listVersionsResult.setIsTruncated(true);
                if (matcher.getRecordType() == FilterRecord.COMMONPREFIX){
                    listVersionsResult.setNextKeyMarker(matcher.getCommonPrefix().getPrefix());
                }else if (matcher.getRecordType() == FilterRecord.DELETEMARKER){
                    listVersionsResult.setNextKeyMarker(matcher.getDeleteMarker().getKey());
                    listVersionsResult.setNextVersionIdMarker(matcher.getDeleteMarker().getVersionId());
                }else if (matcher.getRecordType() == FilterRecord.VERSION){
                    listVersionsResult.setNextKeyMarker(matcher.getVersion().getKey());
                    listVersionsResult.setNextVersionIdMarker(matcher.getVersion().getVersionId());
                }
            }

            return listVersionsResult;
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.OBJECT_LIST_VERSIONS_FAILED,
                    "List versions failed. bucket:"+bucketName, e);
        }finally {
            metaDao.releaseQueryDbCursor(queryDbCursorCur);
            metaDao.releaseQueryDbCursor(queryDbCursorHis);
            metaDao.releaseQueryDbCursor(queryDbCursorDir);
        }
    }

    @Override
    public Boolean isEmptyBucket(ConnectionDao connection, Bucket bucket, Region region) throws S3ServerException{
        try {
            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);
            String metaHisCsName = regionDao.getMetaHisCSName(region);
            String metaHisClName = regionDao.getMetaHisCLName(region);

            if (null != metaDao.queryMetaByBucketId(connection, metaCsName, metaClName, bucket.getBucketId())) {
                return false;
            }
            if (null != metaDao.queryMetaByBucketId(connection, metaHisCsName, metaHisClName, bucket.getBucketId())){
                return false;
            }

            return true;
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw e;
        }
    }

    @Override
    public void deleteObjectByBucket(Bucket bucket) throws S3ServerException {
        try {
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            String metaCsName    = regionDao.getMetaCurCSName(region);
            String metaClName    = regionDao.getMetaCurCLName(region);
            String metaHisCsName = regionDao.getMetaHisCSName(region);
            String metaHisClName = regionDao.getMetaHisCLName(region);
            long bucketId = bucket.getBucketId();

            deleteObjectByClBucket(metaHisCsName, metaHisClName, bucketId);
            deleteObjectByClBucket(metaCsName, metaClName, bucketId);
            dirDao.delete(null, metaCsName, bucketId, null, null);
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "unknown error", e);
        }
    }

    @Override
    public InitiateMultipartUploadResult initMultipartUpload(long ownerID, String bucketName,
                                                             String objectName,
                                                             Map<String, String> requestHeaders,
                                                             Map<String, String> xMeta)
            throws S3ServerException {
        //check key length
        if (objectName.length() > RestParamDefine.KEY_LENGTH){
            throw new S3ServerException(S3Error.OBJECT_KEY_TOO_LONG,
                    "ObjectName is too long. objectName:"+objectName);
        }

        //check meta length
        if (restUtils.getXMetaLength(xMeta) > RestParamDefine.X_AMZ_META_LENGTH){
            throw new S3ServerException(S3Error.OBJECT_METADATA_TOO_LARGE,
                    "metadata headers exceed the maximum. xMeta:"+xMeta.toString());
        }

        //get and check bucket
        Bucket bucket = bucketService.getBucket(ownerID, bucketName);

        Region region = null;
        if (bucket.getRegion() != null) {
            region = regionDao.queryRegion(bucket.getRegion());
        }
        Date createDate      = new Date();
        String dataCsName    = regionDao.getDataCSName(region, createDate);
        String dataClName    = regionDao.getDataClName(region, createDate);

        UploadMeta uploadMeta = buildUploadMeta(bucket.getBucketId(), objectName,
                requestHeaders, xMeta);

        DataAttr dataAttr = dataDao.createNewData(dataCsName, dataClName, region);

        long uploadId = 0L;
        try {
            uploadId = idGenerator.getNewId(IDGenerator.TYPE_UPLOAD);
            uploadMeta.setUploadId(uploadId);
            ConnectionDao connection = daoMgr.getConnectionDao();
            transaction.begin(connection);
            try {
                uploadDao.insertUploadMeta(connection, bucket.getBucketId(), objectName, uploadId, uploadMeta);

                Part nPart = new Part(uploadId, 0, dataCsName, dataClName, dataAttr.getLobId(), 0, null);
                partDao.insertPart(connection, uploadId,0, nPart);

                transaction.commit(connection);
            } catch (Exception e){
                transaction.rollback(connection);
                throw e;
            } finally {
                daoMgr.releaseConnectionDao(connection);
            }

            InitiateMultipartUploadResult result =
                    new InitiateMultipartUploadResult(bucketName, objectName, uploadId);
            return result;
        } catch (S3ServerException e){
            if (e.getError() != S3Error.DAO_TRANSACTION_COMMIT_FAILED) {
                cleanRedundencyLob(dataCsName, dataClName, dataAttr.getLobId());
            }else {
                try {
                    if (null == uploadDao.queryUploadByUploadId(null, bucket.getBucketId(), objectName, uploadId, false)) {
                        cleanRedundencyLob(dataCsName, dataClName, dataAttr.getLobId());
                    }
                } catch (Exception e2) {
                    logger.error("check uploadId failed. uploadId:" + uploadId + ", error:" + e2.getMessage());
                }
            }
            throw e;
        } catch (Exception e){
            cleanRedundencyLob(dataCsName, dataClName, dataAttr.getLobId());
            throw new S3ServerException(S3Error.PART_INIT_MULTIPART_UPLOAD_FAILED, "init upload failed", e);
        }
    }

    @Override
    public String uploadPart(Bucket bucket, String objectName,
                             long uploadId, int partNumber, String contentMD5,
                             InputStream inputStream, long contentLength)
            throws S3ServerException {
        try {
            Region region = null;
            if (bucket.getRegion() != null) {
                region = regionDao.queryRegion(bucket.getRegion());
            }
            Date createDate = new Date();
            String dataCsName = regionDao.getDataCSName(region, createDate);
            String dataClName = regionDao.getDataClName(region, createDate);

            initZeroPartSize(bucket, uploadId, objectName, contentLength, dataCsName, dataClName, region);

            insertNewPartIfNull(uploadId, partNumber, dataCsName, dataClName);

            DataAttr newDataLob = null;
            ConnectionDao connectionB = daoMgr.getConnectionDao();
            transaction.begin(connectionB);
            try {
                Part nPart = new Part(uploadId, partNumber, dataCsName, dataClName, null, 0, null);
                Part oldPart = partDao.queryPartByPartnumber(connectionB, uploadId, partNumber);
                if (oldPart == null){
                    partDao.insertPart(connectionB, uploadId, partNumber, nPart);
                }

                UploadMeta upload = uploadDao.queryUploadByUploadId(connectionB, bucket.getBucketId(),
                        objectName, uploadId, false);
                if (upload == null || upload.getUploadStatus() != UploadMeta.UPLOAD_INIT) {
                    throw new S3ServerException(S3Error.PART_NO_SUCH_UPLOAD, "no such upload. uploadId:" + uploadId);
                }

                if (uploadStatusDao.queryUploadId(uploadId)) {
                    throw new S3ServerException(S3Error.PART_COMPLETING_CONFLICT, "The uploadId is completing");
                }

                if (oldPart != null && contentLength == oldPart.getSize() && contentLength != 0) {
                    newDataLob = dataDao.createNewData(dataCsName, dataClName, region);
                    nPart.setLobId(newDataLob.getLobId());
                } else {
                    Part sameSizePart = partDao.queryPartBySize(null, uploadId, contentLength);
                    if (sameSizePart != null) {
                        nPart.setCsName(sameSizePart.getCsName());
                        nPart.setClName(sameSizePart.getClName());
                        nPart.setLobId(sameSizePart.getLobId());
                    } else {
                        newDataLob = dataDao.createNewData(dataCsName, dataClName, region);
                        nPart.setLobId(newDataLob.getLobId());
                    }
                }

                String eTag = null;
                if (contentLength > 0) {
                    DataAttr dataAttr;
                    try {
                        dataAttr = dataDao.insertObjectData(nPart.getCsName(), nPart.getClName(),
                                inputStream, region, nPart.getLobId(),
                                (partNumber - 1) * contentLength, contentLength);
                    } catch (S3ServerException e) {
                        if (e.getError().getErrIndex() == S3Error.DAO_LOB_PIECES_INFO_OVERFLOW.getErrIndex()) {
                            createNewMinusPart(uploadId, dataCsName, dataClName, region, contentLength);
                        }
                        throw e;
                    } catch (Exception e) {
                        throw e;
                    }
                    if (contentLength != dataAttr.getSize()) {
                        throw new S3ServerException(S3Error.OBJECT_INCOMPLETE_BODY,
                                "content length is " + contentLength +
                                        " and receive " + dataAttr.getSize() + " bytes");
                    }
                    if (null != contentMD5) {
                        if (!MD5Utils.isMd5EqualWithETag(contentMD5, dataAttr.geteTag())) {
                            throw new S3ServerException(S3Error.OBJECT_BAD_DIGEST,
                                    "The Content-MD5 you specified does not match what we received." +
                                            " contentMD5:" + contentMD5
                                            + ", eTag:" + dataAttr.geteTag()
                                            + ", contentLength:" + contentLength
                                            + ", receiveSize:" + dataAttr.getSize());
                        }
                    }

                    eTag = dataAttr.geteTag();
                } else {
                    MessageDigest MD5 = MessageDigest.getInstance("MD5");
                    eTag = new String(Hex.encodeHex(MD5.digest()));
                }
                nPart.setEtag(eTag);
                nPart.setSize(contentLength);
                partDao.updatePart(connectionB, uploadId, partNumber, nPart);

                if (oldPart != null && oldPart.getLobId() != null) {
                    int tryTimeB = DBParamDefine.DB_DUPLICATE_MAX_TIME;
                    oldPart.setPartNumber(oldPart.getPartNumber() - RestParamDefine.PART_NUMBER_MAX);
                    while (tryTimeB > 0) {
                        tryTimeB--;
                        ConnectionDao connectionC = daoMgr.getConnectionDao();
                        transaction.begin(connectionC);
                        try {
                            oldPart.setPartNumber(oldPart.getPartNumber() - 10000);
                            partDao.insertPart(connectionC, uploadId, oldPart.getPartNumber(), oldPart);
                            transaction.commit(connectionC);
                            break;
                        } catch (S3ServerException e) {
                            transaction.rollback(connectionC);
                            if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex()) {
                                continue;
                            } else {
                                throw e;
                            }
                        } catch (Exception e) {
                            transaction.rollback(connectionC);
                            throw e;
                        } finally {
                            daoMgr.releaseConnectionDao(connectionC);
                        }
                    }
                }

                transaction.commit(connectionB);
                return eTag;
            }catch (S3ServerException e){
                transaction.rollback(connectionB);
                if (newDataLob != null && e.getError() != S3Error.DAO_TRANSACTION_COMMIT_FAILED) {
                    cleanRedundencyLob(dataCsName, dataClName, newDataLob.getLobId());
                }
                throw e;
            } catch (Exception e) {
                transaction.rollback(connectionB);
                if (newDataLob != null) {
                    cleanRedundencyLob(dataCsName, dataClName, newDataLob.getLobId());
                }
                throw e;
            } finally {
                daoMgr.releaseConnectionDao(connectionB);
            }

        } catch (S3ServerException e) {
            throw e;
        } catch (Exception e) {
            throw new S3ServerException(S3Error.PART_UPLOAD_PART_FAILED,
                    "upload part failed. objectName=" + objectName +
                            ", uploadId=" + uploadId +
                            ", partNumber=" + partNumber, e);
        }
    }

    @Override
    public CompleteMultipartUploadResult completeUpload(long ownerID, String bucketName,
                                                        String objectName, String uploadIdStr,
                                                        List<Part> reqPartList,
                                                        ServletOutputStream outputStream)
            throws S3ServerException {
        Bucket bucket = bucketService.getBucket(ownerID, bucketName);
        long uploadId = restUtils.convertUploadId(uploadIdStr);

        Region region = null;
        if (bucket.getRegion() != null) {
            region = regionDao.queryRegion(bucket.getRegion());
        }
        Date createDate      = new Date();
        String dataCsName    = regionDao.getDataCSName(region, createDate);
        String dataClName    = regionDao.getDataClName(region, createDate);

        Long flushIndex = null;
        int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;
        try {
            while (tryTime > 0) {
                tryTime--;
                ObjectMeta oldObjectMeta = null;
                DataAttr newDataLob = null;
                boolean mutexLock = false;  //uploadId的互斥锁
                ConnectionDao connection = daoMgr.getConnectionDao();
                transaction.begin(connection);
                try {
                    UploadMeta upload = uploadDao.queryUploadByUploadId(connection, bucket.getBucketId(),
                            objectName, uploadId, true);
                    if (upload == null || upload.getUploadStatus() != UploadMeta.UPLOAD_INIT) {
                        throw new S3ServerException(S3Error.PART_NO_SUCH_UPLOAD,
                                "no such upload. uploadId:" + uploadId);
                    }
                    // 只upload表中的记录加U锁，可能会被其他并发任务读取upload写新的part(Uploadpart)，
                    // 如果该part查找到size一致的part并写入对应lob，如果该lob就是最终合并的lob，就会把已经合并完成的lob内容破坏
                    // 如果修改为complete状态，会导致同步日志过大
                    //增加uploadId在task表中，相当于是个互斥锁，防止其他upload part再向该uploadId中增加内容
                    uploadStatusDao.insertUploadId(uploadId);
                    mutexLock = true;
                    //准备
                    List<Part> partArray = new ArrayList<>();
                    List<Part> baseArray = new ArrayList<>();
                    getLocalPartList(connection, uploadId, partArray, baseArray);

                    //检查
                    List<Part> completeList = partArray;
                    if (multiPartUploadConfig.isPartlistinuse() && reqPartList != null) {
                        checkRequsetPartlist(reqPartList, partArray);
                        completeList = reqPartList;
                    }

                    //合并策略选择
                    Boolean reBuildLob = IsNeedRebuild(completeList, partArray, baseArray);
                    String destCSName = null;
                    String destCLName = null;
                    ObjectId destLobId = null;
                    if (reBuildLob) {
                        //create a new lob
                        newDataLob = dataDao.createNewData(dataCsName, dataClName, region);
                        destCSName = dataCsName;
                        destCLName = dataClName;
                        destLobId = newDataLob.getLobId();
                    } else {
                        destCSName = baseArray.get(0).getCsName();
                        destCLName = baseArray.get(0).getClName();
                        destLobId = baseArray.get(0).getLobId();
                    }

                    logger.debug("complete begin");
                    //合并
                    //TODO:在刷空白字符前应判断是否需要返回versionId，否则后面刷过之后再写header，客户端也接收不到了
                    if(null == flushIndex) {
                        flushIndex = outStreamFlushQueue.add(outputStream);
                    }
                    String eTag = null;
                    long writeOffset = 0;
                    for (int i = 0; i < completeList.size(); i++) {
                        if (completeList.get(i) == null) {
                            continue;
                        }

                        int partNumber = completeList.get(i).getPartNumber();
                        Part part = partArray.get(partNumber - 1);
                        eTag = completeList.get(i).getEtag();
                        if (part.getSize() > 0
                            && (!part.getCsName().equals(destCSName)
                                || !part.getClName().equals(destCLName)
                                || !part.getLobId().equals(destLobId))) {
                            dataDao.copyObjectData(destCSName, destCLName, destLobId, writeOffset,
                                    part.getCsName(), part.getClName(), part.getLobId(),
                                    part.getSize() * (part.getPartNumber() - 1),
                                    part.getSize());
                        }

                        writeOffset += part.getSize();
                    }
                    dataDao.completeDataLobWithOffset(destCSName, destCLName, destLobId, writeOffset);
                    upload.setCsName(destCSName);
                    upload.setClName(destCLName);
                    upload.setLobId(destLobId);
                    upload.setUploadStatus(UploadMeta.UPLOAD_COMPLETE);
                    upload.setLastModified(System.currentTimeMillis());
                    uploadDao.updateUploadMeta(connection, bucket.getBucketId(), objectName, uploadId, upload);
                    logger.debug("complete end");

                    //写元数据
                    String completeEtag;
                    if (completeList.size() == 1) {
                        completeEtag = trimQuotes(eTag);
                    } else {
                        completeEtag = trimQuotes(eTag) + "-f";
                    }
                    VersioningStatusType versioningStatusType = VersioningStatusType.getVersioningStatus(bucket.getVersioningStatus());
                    ObjectMeta objectMeta = buildObjectMetaFromUpload(upload, false,
                            generateNoVersionFlag(versioningStatusType));
                    objectMeta.seteTag(completeEtag);
                    objectMeta.setSize(writeOffset);

                    oldObjectMeta = writeObjectMetaForMultiUpload(connection, objectMeta, objectName,
                                    bucket.getBucketId(), versioningStatusType, region);
                    logger.debug("write cur meta end");

                    transaction.commit(connection);
                    deleteObjectLobAndAcl(oldObjectMeta);
                    cleanUploadStatus(uploadId);

                    //build response
                    CompleteMultipartUploadResult response = new CompleteMultipartUploadResult();
                    response.seteTag(completeEtag);
                    response.setBucket(bucketName);
                    response.setKey(objectName);
                    response.setVersionId(objectMeta.getVersionId());
                    return response;
                } catch (S3ServerException e) {
                    transaction.rollback(connection);
                    if (newDataLob != null && e.getError() != S3Error.DAO_TRANSACTION_COMMIT_FAILED) {
                        cleanRedundencyLob(dataCsName, dataClName, newDataLob.getLobId());
                    }
                    if (mutexLock) {
                        cleanUploadStatus(uploadId);
                    }
                    if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex()) {
                        if(tryTime > 0) {
                            continue;
                        }else{
                            throw new S3ServerException(S3Error.OPERATION_CONFLICT,
                                    "complete upload failed. bucket:" + bucketName + ", uploadId:" + uploadId, e);
                        }
                    } else {
                        throw e;
                    }
                } catch (Exception e) {
                    transaction.rollback(connection);
                    if (newDataLob != null) {
                        cleanRedundencyLob(dataCsName, dataClName, newDataLob.getLobId());
                    }
                    if (mutexLock) {
                        cleanUploadStatus(uploadId);
                    }
                    throw new S3ServerException(S3Error.PART_COMPLETE_MULTIPART_UPLOAD_FAILED,
                            "complete upload failed. bucket:" + bucketName + ", uploadId:" + uploadId, e);
                } finally {
                    daoMgr.releaseConnectionDao(connection);
                }
            }
        }finally {
            outStreamFlushQueue.remove(flushIndex, outputStream);
        }
        throw new S3ServerException(S3Error.PART_COMPLETE_MULTIPART_UPLOAD_FAILED,
                "complete upload failed. bucket:" + bucketName + ", uploadId:" + uploadId);
    }

    @Override
    public void abortUpload(long ownerID, String bucketName, String objectName,
                            String uploadIdStr)
            throws S3ServerException {
        Bucket bucket = bucketService.getBucket(ownerID, bucketName);
        long uploadId = restUtils.convertUploadId(uploadIdStr);

        ConnectionDao connection = daoMgr.getConnectionDao();
        transaction.begin(connection);
        try {
            UploadMeta upload = uploadDao.queryUploadByUploadId(connection, bucket.getBucketId(),
                    objectName, uploadId, true);
            if (upload == null || upload.getUploadStatus() != UploadMeta.UPLOAD_INIT) {
                throw new S3ServerException(S3Error.PART_NO_SUCH_UPLOAD,
                        "no such upload. uploadId:" + uploadId);
            }

            upload.setUploadStatus(UploadMeta.UPLOAD_ABORT);
            uploadDao.updateUploadMeta(connection, bucket.getBucketId(), objectName, uploadId, upload);
            transaction.commit(connection);
        } catch (S3ServerException e) {
            transaction.rollback(connection);
            throw e;
        } catch (Exception e) {
            transaction.rollback(connection);
            throw new S3ServerException(S3Error.PART_ABORT_MULTIPART_UPLOAD_FAILED,
                    "abort upload failed. bucket:"+bucketName+", uploadId:"+uploadId, e);
        } finally {
            daoMgr.releaseConnectionDao(connection);
        }
    }

    @Override
    public ListPartsResult listParts(long ownerID, String bucketName, String objectName,
                                     String uploadIdStr, Integer partNumberMarker,
                                     Integer maxParts, String encodingType)
            throws S3ServerException {
        QueryDbCursor partsCursor = null;
        try {
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);
            long uploadId = restUtils.convertUploadId(uploadIdStr);
            UploadMeta upload = uploadDao.queryUploadByUploadId(null, bucket.getBucketId(),
                    objectName, uploadId, false);
            if (upload == null || upload.getUploadStatus() != UploadMeta.UPLOAD_INIT) {
                throw new S3ServerException(S3Error.PART_NO_SUCH_UPLOAD,
                        "no such upload. uploadId:" + uploadId);
            }

            int maxNumber = Math.min(maxParts, RestParamDefine.MAX_KEYS_DEFAULT);
            ListPartsResult result = new ListPartsResult(bucketName, objectName, uploadId,
                    maxNumber, partNumberMarker, userDao.getOwnerByUserID(ownerID), encodingType);

            partsCursor = partDao.queryPartList(uploadId, true,
                    partNumberMarker, maxNumber + 1);
            if (partsCursor != null) {
                LinkedHashSet<Part> partList = result.getPartList();
                int count = 0;
                while (partsCursor.hasNext() && count < maxNumber) {
                    Part part = new Part(partsCursor.getNext(), encodingType);
                    partList.add(part);
                    count++;
                }

                if (partsCursor.hasNext()) {
                    result.setIsTruncated(true);
                    result.setNextPartNumberMarker((int) (partsCursor.getCurrent().get(Part.PARTNUMBER)));
                }
            }
            return result;
        } catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.PART_LIST_PARTS_FAILED,
                    "List Parts failed. bucket:"+bucketName+", uploadId:"+uploadIdStr, e);
        }finally {
            metaDao.releaseQueryDbCursor(partsCursor);
        }
    }

    @Override
    public ListMultipartUploadsResult listUploadLists(long ownerID, String bucketName,
                                                      String prefix, String delimiter,
                                                      String keyMarker, Long uploadIdMarker,
                                                      Integer maxKeys, String encodingType)
            throws S3ServerException{
        QueryDbCursor dbCursorUploads = null;
        try{
            Bucket bucket = bucketService.getBucket(ownerID, bucketName);

            int count = 0;
            int maxNumber = Math.min(maxKeys, RestParamDefine.MAX_KEYS_DEFAULT);
            Owner owner = userDao.getOwnerByUserID(ownerID);

            ListMultipartUploadsResult result = new ListMultipartUploadsResult(bucketName,
                    maxNumber, encodingType, prefix, delimiter, keyMarker, uploadIdMarker);

            dbCursorUploads = uploadDao.queryUploadsByBucket(bucket.getBucketId(),
                    prefix, keyMarker, uploadIdMarker, UploadMeta.UPLOAD_INIT);
            if (dbCursorUploads == null){
                return result;
            }

            UploadFilter filter = null;
            if (delimiter != null){
                filter = new UploadFilter(dbCursorUploads, prefix, keyMarker,
                        uploadIdMarker, encodingType, owner, delimiter);
            }else {
                filter = new UploadFilter(dbCursorUploads, prefix, keyMarker,
                        uploadIdMarker, encodingType, owner);
            }

            LinkedHashSet<Upload> uploadList = result.getUploadList();
            LinkedHashSet<CommonPrefix> commonPrefixList = result.getCommonPrefixList();
            FilterRecord matcher;
            while ((matcher =  filter.getNextRecord()) != null){
                if (matcher.getRecordType() == FilterRecord.COMMONPREFIX){
                    if (!commonPrefixList.contains(matcher.getCommonPrefix())) {
                        commonPrefixList.add(matcher.getCommonPrefix());
                        count++;
                    }
                }else if (matcher.getRecordType() == FilterRecord.UPLOAD){
                    uploadList.add(matcher.getUpload());
                    count++;
                }

                if (count >= maxNumber){
                    break;
                }
            }

            if (filter.hasNext()){
                result.setIsTruncated(true);
                if (matcher.getRecordType() == FilterRecord.COMMONPREFIX){
                    result.setNextKeyMarker(matcher.getCommonPrefix().getPrefix());
                }else {
                    result.setNextKeyMarker(matcher.getUpload().getKey());
                    result.setNextUploadIdMarker(matcher.getUpload().getUploadId());
                }
            }
            return result;
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.PART_LIST_MULTIPART_UPLOADS_FAILED,
                    "List Uploads failed. bucket:"+bucketName, e);
        }finally {
            metaDao.releaseQueryDbCursor(dbCursorUploads);
        }
    }

    private void initZeroPartSize(Bucket bucket, long uploadId, String objectName,
                              long contentLength, String dataCsName,
                              String dataClName, Region region)
            throws S3ServerException{
        DataAttr dataMinus = null;
        ConnectionDao connection = daoMgr.getConnectionDao();
        transaction.begin(connection);
        try {
            //query uploadId
            UploadMeta upload = uploadDao.queryUploadByUploadId(connection, bucket.getBucketId(),
                    objectName, uploadId, false);
            if (upload == null || upload.getUploadStatus() != UploadMeta.UPLOAD_INIT) {
                throw new S3ServerException(S3Error.PART_NO_SUCH_UPLOAD, "no such upload. uploadId:" + uploadId);
            }
            Part partZero = partDao.queryPartByPartnumber(connection, uploadId, 0);
            if (partZero != null && partZero.getSize() == 0){
                partZero.setSize(contentLength);
                partDao.updatePart(connection, uploadId, 0, partZero);
            }
            if (contentLength != partZero.getSize()){
                Part partMinus = partDao.queryPartByPartnumber(connection, uploadId, -1);
                if (partMinus == null){
                    dataMinus = dataDao.createNewData(dataCsName, dataClName, region);
                    Part nPart = new Part(uploadId, -1, dataCsName, dataClName, dataMinus.getLobId(), contentLength, null);
                    partDao.insertPart(connection, uploadId,-1, nPart);
                }
            }
            transaction.commit(connection);
        } catch (S3ServerException e) {
            transaction.rollback(connection);
            if (dataMinus != null){
                if (e.getError() != S3Error.DAO_TRANSACTION_COMMIT_FAILED) {
                    cleanRedundencyLob(dataCsName, dataClName, dataMinus.getLobId());
                }else {
                    try{
                        if(null == partDao.queryPartByPartnumber(null, uploadId, -1)){
                            cleanRedundencyLob(dataCsName, dataClName, dataMinus.getLobId());
                        }
                    }catch (Exception e2){
                        logger.error("check part -1 failed. uploadId:" + uploadId + ", error:" + e2.getMessage());
                    }
                }
            }
            if (e.getError().getErrIndex() != S3Error.DAO_DUPLICATE_KEY.getErrIndex()) {
                throw e;
            }
        } catch (Exception e) {
            transaction.rollback(connection);
            if (dataMinus != null){
                cleanRedundencyLob(dataCsName, dataClName, dataMinus.getLobId());
            }
            throw e;
        } finally {
            daoMgr.releaseConnectionDao(connection);
        }
    }

    private void insertNewPartIfNull(long uploadId, int partNumber,
                                     String dataCsName, String dataClName)
            throws S3ServerException{
        int tryTimeA = DBParamDefine.DB_DUPLICATE_MAX_TIME;
        while (tryTimeA > 0){
            tryTimeA--;
            ConnectionDao connectionA = daoMgr.getConnectionDao();
            transaction.begin(connectionA);
            try {
                Part nPart = new Part(uploadId, partNumber, dataCsName, dataClName,
                        null, 0, null);
                Part oldPart = partDao.queryPartByPartnumber(connectionA, uploadId, partNumber);
                if (oldPart == null) {
                    partDao.insertPart(connectionA, uploadId, partNumber, nPart);
                }
                transaction.commit(connectionA);
                break;
            }  catch (S3ServerException e) {
                transaction.rollback(connectionA);
                if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex() && tryTimeA > 0) {
                    continue;
                }else {
                    throw e;
                }
            } catch (Exception e) {
                transaction.rollback(connectionA);
                throw e;
            } finally {
                daoMgr.releaseConnectionDao(connectionA);
            }
        }
    }

    private void checkRequsetPartlist(List<Part> reqPartList, List<Part> locPartArray)
            throws S3ServerException{
        int lastPartNumber = 0;
        for (int i = 0; i < reqPartList.size(); i++){
            Part part = reqPartList.get(i);
            int partNumber = part.getPartNumber();
            if (partNumber < RestParamDefine.PART_NUMBER_MIN
                    || partNumber > RestParamDefine.PART_NUMBER_MAX){
                throw new S3ServerException(S3Error.PART_INVALID_PART,
                        "partNumber " + partNumber + " is not exist.");
            }

            if (partNumber <= lastPartNumber){
                throw new S3ServerException(S3Error.PART_INVALID_PARTORDER,
                        "The partNumber is " + partNumber +
                                ", lastPartNumber is " + lastPartNumber);
            }

            if (partNumber > locPartArray.size()){
                throw new S3ServerException(S3Error.PART_INVALID_PART,
                        "partNumber " + partNumber + " is not exist.");
            }

            if (locPartArray.get(partNumber-1) == null){
                throw new S3ServerException(S3Error.PART_INVALID_PART,
                        "partNumber " + partNumber + " is not exist.");
            }

            if (!trimQuotes(part.getEtag()).equals(locPartArray.get(partNumber-1).getEtag())){
                throw new S3ServerException(S3Error.PART_INVALID_PART,
                        "the tag not matched. partNumber:" + partNumber +
                                " . reqEtag:" + part.getEtag() +
                                ", locEtag:" + locPartArray.get(partNumber-1).getEtag());
            }

            if (multiPartUploadConfig.isPartSizeLimit()) {
                if (i != reqPartList.size() - 1){
                    if (locPartArray.get(partNumber - 1).getSize() < 5 * 1024 * 1024L) {
                        throw new S3ServerException(S3Error.PART_ENTITY_TOO_SMALL,
                                "part size is invalid. size:" + locPartArray.get(partNumber - 1).getSize());
                    }

                    if (locPartArray.get(partNumber - 1).getSize() > 5 * 1024 * 1024 * 1024L){
                        throw new S3ServerException(S3Error.PART_ENTITY_TOO_LARGE,
                                "part size is invalid. size:" + locPartArray.get(partNumber - 1).getSize());
                    }
                }
            }

            lastPartNumber = partNumber;
        }
    }

    private String trimQuotes(String str){
        if (str == null){
            return str;
        }

        str = str.trim();
        if (str.startsWith("\"")){
            str = str.substring(1);
        }

        if (str.endsWith("\"")){
            str = str.substring(0, str.length() - 1);
        }

        return str;
    }

    private void getLocalPartList(ConnectionDao connection, long uploadId, List<Part> partArray, List<Part> baseArray)
            throws S3ServerException{
        QueryDbCursor partCursor = partDao.queryPartListForUpdate(connection, uploadId);
        if (partCursor == null || !partCursor.hasNext()){
            throw new S3ServerException(S3Error.PART_INVALID_PART, "not found any uploaded part");
        }
        int index = 0;
        partCursor.getNext();
        while (true){
            BSONObject record = partCursor.getCurrent();
            int partNumber = (int) record.get(Part.PARTNUMBER);
            if (partNumber-1 == index){
                Part part = new Part(record);
                partArray.add(part);
                if (index == 0){
                    baseArray.add(part);
                }else{
                    if (baseArray.get(0) != null && baseArray.get(0).getLobId().equals(part.getLobId())){
                        baseArray.add(part);
                    }else {
                        baseArray.add(null);
                    }
                }
                if (partCursor.hasNext()){
                    partCursor.getNext();
                }else {
                    break;
                }
            }else{
                partArray.add(null);
                baseArray.add(null);
            }
            index++;
        }
        return;
    }

    private Boolean IsNeedRebuild(List<Part> completeList, List<Part> locPartArray, List<Part> baseArray){
        Boolean reBuildLob = false;
        if (baseArray.get(0) == null || baseArray.get(0).getSize() == 0){
            reBuildLob = true;
        }else {
            long newOffset  = 0;
            long baseSize   = baseArray.get(0).getSize();
            for (int i = 0; i < completeList.size(); i++){
                if (completeList.get(i) == null){
                    continue;
                }

                int partnumber = completeList.get(i).getPartNumber();
                if (baseArray.get(0).getLobId().equals(locPartArray.get(partnumber-1).getLobId())){
                    if (newOffset != baseSize * (partnumber - 1)){
                        reBuildLob = true;
                        break;
                    }else {
                        newOffset += baseSize;
                    }
                }else {
                    int begin = (int)(newOffset/baseSize);
                    int end   = (int)((newOffset + locPartArray.get(partnumber-1).getSize() - 1)/baseSize);
                    for (int j = begin; j <= end && j < baseArray.size(); j++){
                        if (baseArray.get(j) != null){
                            reBuildLob = true;
                            break;
                        }
                    }
                    if (reBuildLob){
                        break;
                    }else {
                        newOffset += locPartArray.get(partnumber-1).getSize();
                    }
                }
            }

            if (!reBuildLob){
                for (int j = (int) (newOffset / baseSize); j < baseArray.size() && j >= 1; j++) {
                    if (baseArray.get(j) != null){
                        reBuildLob = true;
                        break;
                    }
                }
            }
        }
        return reBuildLob;
    }

    private void createNewMinusPart(long uploadId, String dataCsName, String dataClName,
                            Region region, long size) throws S3ServerException{
        DataAttr dataMinus = dataDao.createNewData(dataCsName, dataClName, region);
        try {
            Part partMinus = partDao.queryPartBySize(null, uploadId, null);
            int partNumber = -2;
            if (partMinus.getPartNumber() < -1){
                partNumber = partMinus.getPartNumber() - 1;
            }
            Part nPart = new Part(uploadId, partNumber, dataCsName, dataClName, dataMinus.getLobId(), size, null);
            partDao.insertPart(null, uploadId, partNumber, nPart);
        }catch (Exception e){
            logger.warn("create new minus part fail. uploadId:" + uploadId, e);
            cleanRedundencyLob(dataCsName, dataClName, dataMinus.getLobId());
        }
    }

    private String getParentName(String delimiter, Bucket bucket){
        if (delimiter != null) {
            if (bucket.getDelimiter() == 1) {
                if (delimiter.equals(bucket.getDelimiter1())
                        && DelimiterStatus.getDelimiterStatus(bucket.getDelimiter1Status()) == DelimiterStatus.NORMAL) {
                    return ObjectMeta.META_PARENTID1;
                }
            } else {
                if (delimiter.equals(bucket.getDelimiter2())
                        && DelimiterStatus.getDelimiterStatus(bucket.getDelimiter2Status()) == DelimiterStatus.NORMAL) {
                    return ObjectMeta.META_PARENTID2;
                }
            }
        }
        return null;
    }

    private Long getParentId(String metaCsName, long bucketId, String prefix, String delimiter)
            throws S3ServerException{
        Long parentId = null;
        if (prefix != null){
            String dirName = DirUtils.getDir(prefix, delimiter);
            if (dirName != null){
                Dir dir = dirDao.queryDir(null, metaCsName, bucketId,
                        delimiter, dirName, false);
                if (dir != null){
                    parentId = dir.getID();
                }
            }else {
                parentId = 0L;
            }
        }else {
            parentId = 0L;
        }
        return parentId;
    }

    private ObjectMeta removeMetaForNoneVersioning(String metaCsName, String metaClName, Bucket bucket, String objectName)
            throws S3ServerException{
        ConnectionDao connection = daoMgr.getConnectionDao();
        transaction.begin(connection);
        try {
            ObjectMeta objectMeta = metaDao.queryForUpdate(connection, metaCsName,
                    metaClName, bucket.getBucketId(), objectName, null, null);
            if (objectMeta != null){
                deleteDirForObject(connection, metaCsName, metaClName, bucket, objectName);
                metaDao.removeMeta(connection, metaCsName, metaClName, bucket.getBucketId(),
                        objectName, null, null);
                Bucket newBucket = bucketDao.getBucketById(null, bucket.getBucketId());
                if (newBucket != null && newBucket.getDelimiter() != bucket.getDelimiter()){
                    deleteDirForObject(connection, metaCsName, metaClName, newBucket, objectName);
                }
            }
            transaction.commit(connection);
            return objectMeta;
        } catch (S3ServerException e){
            transaction.rollback(connection);
            throw e;
        } catch(Exception e){
            transaction.rollback(connection);
            throw e;
        } finally {
            daoMgr.releaseConnectionDao(connection);
        }
    }

    private void deleteObjectByClBucket(String metaCsName, String metaClName, long bucketId)
            throws S3ServerException{
        QueryDbCursor queryDbCursor = metaDao.queryMetaByBucket(metaCsName, metaClName,
                bucketId, null, null, null, true);
        if (queryDbCursor == null){
            return;
        }
        ConnectionDao connection = daoMgr.getConnectionDao();
        try {
            while (queryDbCursor.hasNext()) {
                BSONObject record = queryDbCursor.getNext();
                String key = record.get(ObjectMeta.META_KEY_NAME).toString();
                Long versionId = (Long)record.get(ObjectMeta.META_VERSION_ID);
                metaDao.removeMeta(connection, metaCsName, metaClName, bucketId,
                        key, versionId, null);
                if (record.get(ObjectMeta.META_CS_NAME) != null
                        && record.get(ObjectMeta.META_CL_NAME) != null
                        && record.get(ObjectMeta.META_LOB_ID) != null) {
                    String dataCsName = record.get(ObjectMeta.META_CS_NAME).toString();
                    String dataClName = record.get(ObjectMeta.META_CL_NAME).toString();
                    ObjectId lobId = (ObjectId)record.get(ObjectMeta.META_LOB_ID);
                    dataDao.deleteObjectDataByLobId(connection, dataCsName, dataClName, lobId);
                }
                if (record.get(ObjectMeta.META_ACLID) != null) {
                    aclDao.deleteAcl(connection, (long) record.get(ObjectMeta.META_ACLID));
                }
            }
        } catch (S3ServerException e) {
            throw e;
        }catch (Exception e) {
            throw e;
        }finally {
            daoMgr.releaseConnectionDao(connection);
            metaDao.releaseQueryDbCursor(queryDbCursor);
        }
    }

    private ObjectMeta buildObjectMeta(String objectName, long bucketId, Map headers,
                                       Map xMeta, String dataCsName, String dataClName,
                                       Boolean isDeleteMarker, Boolean noVersionFlag) {
        ObjectMeta objectMeta = new ObjectMeta();
        objectMeta.setKey(objectName);
        objectMeta.setBucketId(bucketId);
        objectMeta.setCsName(dataCsName);
        objectMeta.setClName(dataClName);
        objectMeta.setLastModified(System.currentTimeMillis());
        objectMeta.setMetaList(xMeta);
        objectMeta.setDeleteMarker(isDeleteMarker);
        objectMeta.setNoVersionFlag(noVersionFlag);

        if (headers != null) {
            if (headers.containsKey(RestParamDefine.PutObjectHeader.CACHE_CONTROL)) {
                objectMeta.setCacheControl(headers.get(RestParamDefine.PutObjectHeader.CACHE_CONTROL).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_DISPOSITION)) {
                objectMeta.setContentDisposition(headers.get(RestParamDefine.PutObjectHeader.CONTENT_DISPOSITION).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_ENCODING)) {
                objectMeta.setContentEncoding(headers.get(RestParamDefine.PutObjectHeader.CONTENT_ENCODING).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_TYPE)) {
                objectMeta.setContentType(headers.get(RestParamDefine.PutObjectHeader.CONTENT_TYPE).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.EXPIRES)) {
                objectMeta.setExpires(headers.get(RestParamDefine.PutObjectHeader.EXPIRES).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_LANGUAGE)) {
                objectMeta.setContentLanguage(headers.get(RestParamDefine.PutObjectHeader.CONTENT_LANGUAGE).toString());
            }
        }

        return objectMeta;
    }

    private ObjectMeta buildObjectMetaFromUpload(UploadMeta upload, Boolean isDeleteMarker,
                                                 Boolean noVersionFlag) {
        ObjectMeta objectMeta = new ObjectMeta();
        objectMeta.setKey(upload.getKey());
        objectMeta.setBucketId(upload.getBucketId());
        objectMeta.setCsName(upload.getCsName());
        objectMeta.setClName(upload.getClName());
        objectMeta.setLobId(upload.getLobId());
        objectMeta.setLastModified(System.currentTimeMillis());
        objectMeta.setMetaList(upload.getMetaList());
        objectMeta.setDeleteMarker(isDeleteMarker);
        objectMeta.setNoVersionFlag(noVersionFlag);

        objectMeta.setCacheControl(upload.getCacheControl());
        objectMeta.setContentDisposition(upload.getContentDisposition());
        objectMeta.setContentEncoding(upload.getContentEncoding());
        objectMeta.setContentType(upload.getContentType());
        objectMeta.setExpires(upload.getExpires());
        objectMeta.setContentLanguage(upload.getContentLanguage());
        return objectMeta;
    }

    private UploadMeta buildUploadMeta(long bucketId, String objectName, Map headers, Map xMeta) {
        UploadMeta uploadMeta = new UploadMeta();
        uploadMeta.setKey(objectName);
        uploadMeta.setBucketId(bucketId);
        uploadMeta.setLastModified(System.currentTimeMillis());
        uploadMeta.setMetaList(xMeta);
        uploadMeta.setUploadStatus(UploadMeta.UPLOAD_INIT);

        if (headers != null) {
            if (headers.containsKey(RestParamDefine.PutObjectHeader.CACHE_CONTROL)) {
                uploadMeta.setCacheControl(headers.get(RestParamDefine.PutObjectHeader.CACHE_CONTROL).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_DISPOSITION)) {
                uploadMeta.setContentDisposition(headers.get(RestParamDefine.PutObjectHeader.CONTENT_DISPOSITION).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_ENCODING)) {
                uploadMeta.setContentEncoding(headers.get(RestParamDefine.PutObjectHeader.CONTENT_ENCODING).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_TYPE)) {
                uploadMeta.setContentType(headers.get(RestParamDefine.PutObjectHeader.CONTENT_TYPE).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.EXPIRES)) {
                uploadMeta.setExpires(headers.get(RestParamDefine.PutObjectHeader.EXPIRES).toString());
            }

            if (headers.containsKey(RestParamDefine.PutObjectHeader.CONTENT_LANGUAGE)) {
                uploadMeta.setContentLanguage(headers.get(RestParamDefine.PutObjectHeader.CONTENT_LANGUAGE).toString());
            }
        }

        return uploadMeta;
    }

    private Boolean checkMatchModify(Map headers, ObjectMeta objectMeta) throws S3ServerException{
        String eTag           = objectMeta.geteTag();
        long lastModifiedTime = objectMeta.getLastModified();
        boolean isMatch     = false;
        boolean isNoneMatch = false;

        Object matchEtag = headers.get(RestParamDefine.GetObjectReqHeader.REQ_IF_MATCH);
        if (null != matchEtag){
            String matchEtagString = trimQuotes(matchEtag.toString());
            if (!matchEtagString.equals(eTag)){
                throw new S3ServerException(S3Error.OBJECT_IF_MATCH_FAILED,
                        "if-match failed: if-match value:" + matchEtag.toString() +
                                ", current object eTag:" + eTag);
            }else{
                isMatch = true;
            }
        }

        Object noneMatchEtag = headers.get(RestParamDefine.GetObjectReqHeader.REQ_IF_NONE_MATCH);
        if (null != noneMatchEtag){
            String noneMatchEtagString = trimQuotes(noneMatchEtag.toString());
            if (noneMatchEtagString.equals(eTag)){
                throw new S3ServerException(S3Error.OBJECT_IF_NONE_MATCH_FAILED,
                        "if-none-match failed: if-none-match value:" + noneMatchEtag.toString() +
                                ", current object eTag:" + eTag);
            }else{
                isNoneMatch = true;
            }
        }

        Object unModifiedSince = headers.get(RestParamDefine.GetObjectReqHeader.REQ_IF_UNMODIFIED_SINCE);
        if (null != unModifiedSince){
            Date date = parseDate(unModifiedSince.toString());
            if (getSecondTime(date.getTime()) < getSecondTime(lastModifiedTime)) {
                if (!isMatch) {
                    throw new S3ServerException(S3Error.OBJECT_IF_UNMODIFIED_SINCE_FAILED,
                            "if-unmodified-since failed: if-unmodified-since value:" + unModifiedSince.toString() +
                                    ", current object lastModifiedTime:" + new Date(lastModifiedTime));
                }
            }
        }

        Object modifiedSince = headers.get(RestParamDefine.GetObjectReqHeader.REQ_IF_MODIFIED_SINCE);
        if (null != modifiedSince){
            Date date = parseDate(modifiedSince.toString());
            if (getSecondTime(date.getTime()) >= getSecondTime(lastModifiedTime)) {
                if (!isNoneMatch) {
                    throw new S3ServerException(S3Error.OBJECT_IF_MODIFIED_SINCE_FAILED,
                            "if-modified-since failed: if-modified-since value:" + modifiedSince.toString() +
                                    ", current object lastModifiedTime:" + new Date(lastModifiedTime));
                }
            }
        }

        return true;
    }

    private Boolean IsContextMatch(Context queryContext, String prefix, String startAfter,
                                 String delimiter){
        if(queryContext.getDelimiter() != null){
            if (!(queryContext.getDelimiter().equals(delimiter))) {
                return false;
            }
        }else if (delimiter != null){
            return false;
        }

        if (queryContext.getPrefix() != null) {
            if (!(queryContext.getPrefix().equals(prefix))) {
                return false;
            }
        } else if (prefix != null){
            return false;
        }

        if (queryContext.getStartAfter() != null) {
            if (!(queryContext.getStartAfter().equals(startAfter))) {
                return false;
            }
        }else if (startAfter != null){
            return false;
        }

        return true;
    }

    private Boolean generateNoVersionFlag(VersioningStatusType status){
        if (null == status){
            return false;
        }
        switch(status){
            case ENABLED:
                return false;
            default:
                return true;
        }
    }

    private void deleteObjectLobAndAcl(ObjectMeta deleteObject){
        if (deleteObject != null && !deleteObject.getDeleteMarker()) {
            if (deleteObject.getAclId() != null) {
                try {
                    aclDao.deleteAcl(null, deleteObject.getAclId());
                } catch (Exception e) {
                    logger.error("delete acl failed. aclId:{}", deleteObject.getAclId());
                }
            }

            if(deleteObject.getLobId() != null) {
                int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;
                while (tryTime > 0) {
                    tryTime--;
                    try {
                        dataDao.deleteObjectDataByLobId(null, deleteObject.getCsName(),
                                deleteObject.getClName(), deleteObject.getLobId());
                        break;
                    } catch (S3ServerException e) {
                        if (e.getError() == S3Error.OBJECT_IS_IN_USE && tryTime > 0) {
                            if (tryTime == DBParamDefine.DB_DUPLICATE_MAX_TIME - 1) {
                                logger.error("lob is in use.csName:{}, clName:{}, lobId:{}",
                                        deleteObject.getCsName(), deleteObject.getClName(),
                                        deleteObject.getLobId());
                            }
                            try {
                                Thread.sleep(100L);
                            } catch (Exception e2) {
                                logger.error("thread sleep fail.", e2);
                            }
                            continue;
                        } else {
                            logger.error("delete lob fail. csName:{}, clName:{}, lobId:{}",
                                    deleteObject.getCsName(), deleteObject.getClName(),
                                    deleteObject.getLobId());
                            break;
                        }
                    }
                }
            }
        }
    }

    private void cleanRedundencyLob(String csName, String clName, ObjectId lobId){
        try {
            dataDao.deleteObjectDataByLobId(null, csName,
                    clName, lobId);
        }catch (Exception e){
            logger.error("cleanRedundencyLob failed. csName:" + csName +
                    ", clName:" + clName +
                    ", lobId:" + lobId.toString());
        }
    }

    private void cleanUploadStatus(long uploadId){
        try {
            uploadStatusDao.deleteUploadId(uploadId);
        }catch (Exception e1){
            logger.error("deleteUploadId failed.", e1);
        }
    }

    private void buildDirForObject(ConnectionDao connection, String metaCsName, Bucket bucket,
                                   String objectName, ObjectMeta objectMeta, Region region)
            throws S3ServerException{
        long bucketId = bucket.getBucketId();
        if (bucket.getDelimiter1() != null) {
            DelimiterStatus status = DelimiterStatus.getDelimiterStatus(bucket.getDelimiter1Status());
            if (status == DelimiterStatus.NORMAL
                    || status == DelimiterStatus.CREATING
                    || status == DelimiterStatus.SUSPENDED) {
                String dirName = DirUtils.getDir(objectName, bucket.getDelimiter1());
                if (dirName != null) {
                    Dir dir = dirDao.queryDir(connection, metaCsName, bucketId, bucket.getDelimiter1(), dirName, true);
                    if (dir != null) {
                        objectMeta.setParentId1(dir.getID());
                    } else {
                        long newId = idGenerator.getNewId(IDGenerator.TYPE_PARENTID);
                        Dir newDir = new Dir(bucketId, bucket.getDelimiter1(), dirName, newId);
                        dirDao.insertDir(connection, metaCsName, newDir, region);
                        objectMeta.setParentId1(newId);
                    }
                }else {
                    objectMeta.setParentId1(0L);
                }
            }
        }

        if (bucket.getDelimiter2() != null) {
            DelimiterStatus status = DelimiterStatus.getDelimiterStatus(bucket.getDelimiter2Status());
            if (status == DelimiterStatus.NORMAL
                    || status == DelimiterStatus.CREATING
                    || status == DelimiterStatus.SUSPENDED) {
                String dirName = DirUtils.getDir(objectName, bucket.getDelimiter2());
                if (dirName != null) {
                    Dir dir = dirDao.queryDir(connection, metaCsName, bucketId, bucket.getDelimiter2(), dirName, true);
                    if (dir != null) {
                        objectMeta.setParentId2(dir.getID());
                    } else {
                        long newId = idGenerator.getNewId(IDGenerator.TYPE_PARENTID);
                        Dir newDir = new Dir(bucketId, bucket.getDelimiter2(), dirName, newId);
                        dirDao.insertDir(connection, metaCsName, newDir, region);
                        objectMeta.setParentId2(newId);
                    }
                }else {
                    objectMeta.setParentId2(0L);
                }
            }
        }
    }

    private void deleteDirForObject(ConnectionDao connection, String metaCsName, String metaClName,
                                    Bucket bucket, String objectName)
            throws S3ServerException{
        if (bucket.getDelimiter1() != null) {
            DelimiterStatus status = DelimiterStatus.getDelimiterStatus(bucket.getDelimiter1Status());
            if (status == DelimiterStatus.NORMAL
                    || status == DelimiterStatus.CREATING
                    || status == DelimiterStatus.SUSPENDED) {
                String dirName = DirUtils.getDir(objectName, bucket.getDelimiter1());
                if (dirName != null) {
                    Dir dir = dirDao.queryDir(connection, metaCsName, bucket.getBucketId(),
                            bucket.getDelimiter1(), dirName, true);
                    if (dir != null) {
                        String parentIdName = ObjectMeta.META_PARENTID1;
                        if(!metaDao.queryOneOtherMetaByParentId(null, metaCsName, metaClName,
                                bucket.getBucketId(), objectName, parentIdName, dir.getID())){
                            dirDao.delete(connection, metaCsName, dir.getBucketId(),
                                    bucket.getDelimiter1(), dirName);
                        }
                    }
                }
            }
        }

        if (bucket.getDelimiter2() != null) {
            DelimiterStatus status = DelimiterStatus.getDelimiterStatus(bucket.getDelimiter2Status());
            if (status == DelimiterStatus.NORMAL
                    || status == DelimiterStatus.CREATING
                    || status == DelimiterStatus.SUSPENDED) {
                String dirName = DirUtils.getDir(objectName, bucket.getDelimiter2());
                if (dirName != null) {
                    Dir dir = dirDao.queryDir(connection, metaCsName, bucket.getBucketId(),
                            bucket.getDelimiter2(), dirName, true);
                    if (dir != null) {
                        String parentIdName = ObjectMeta.META_PARENTID2;
                        if(!metaDao.queryOneOtherMetaByParentId(null, metaCsName, metaClName,
                                bucket.getBucketId(), objectName, parentIdName, dir.getID())){
                            dirDao.delete(connection, metaCsName, bucket.getBucketId(),
                                    bucket.getDelimiter2(), dirName);
                        }
                    }
                }
            }
        }
    }

    //writeObjectMeta retrun the oldObject meta to delete old lob
    private ObjectMeta writeObjectMeta(ObjectMeta objectMeta, String objectName, long bucketId,
                                 VersioningStatusType versioningStatusType, Region region)
            throws S3ServerException{
        String metaCsName    = regionDao.getMetaCurCSName(region);
        String metaClName    = regionDao.getMetaCurCLName(region);
        String metaHisCSName = regionDao.getMetaHisCSName(region);
        String metaHisClName = regionDao.getMetaHisCLName(region);

        int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;
        while (tryTime > 0) {
            tryTime--;
            ObjectMeta deleteObject = null;
            ConnectionDao connection = daoMgr.getConnectionDao();
            transaction.begin(connection);
            try {
                ObjectMeta metaResult = metaDao.queryForUpdate(connection, metaCsName, metaClName,
                        bucketId, objectName, null, null);
                if (null == metaResult) {
                    objectMeta.setVersionId(0);
                    Bucket bucket = bucketDao.getBucketById(connection, bucketId);
                    if (bucket == null){
                        throw new S3ServerException(S3Error.BUCKET_NOT_EXIST, "bucket is deleting before insert meta.");
                    }
                    buildDirForObject(connection, metaCsName, bucket, objectName, objectMeta, region);
                    metaDao.insertMeta(connection, metaCsName, metaClName, objectMeta,
                            false, region);
                    Bucket newBucket = bucketDao.getBucketById(connection, bucketId);
                    if (newBucket == null){
                        throw new S3ServerException(S3Error.BUCKET_NOT_EXIST, "bucket is deleting after insert meta.");
                    }
                    if (bucket.getDelimiter() != newBucket.getDelimiter()) {
                        transaction.rollback(connection);
                        continue;
                    }
                    transaction.commit(connection);
                } else {
                    objectMeta.setVersionId(metaResult.getVersionId() + 1);
                    objectMeta.setParentId1(metaResult.getParentId1());
                    objectMeta.setParentId2(metaResult.getParentId2());
                    if (VersioningStatusType.NONE == versioningStatusType
                            || (VersioningStatusType.SUSPENDED == versioningStatusType
                            && metaResult.getNoVersionFlag())) {
                        metaDao.updateMeta(connection, metaCsName, metaClName, bucketId,
                                objectName, null, objectMeta);
                        transaction.commit(connection);
                        deleteObject = metaResult;
                    } else {
                        metaDao.insertMeta(connection, metaHisCSName, metaHisClName,
                                metaResult, true, region);
                        metaDao.updateMeta(connection, metaCsName, metaClName, bucketId,
                                objectName, null, objectMeta);
                        ObjectMeta nullMeta = null;
                        if (VersioningStatusType.SUSPENDED == versioningStatusType) {
                            nullMeta = metaDao.queryForUpdate(connection, metaHisCSName, metaHisClName,
                                    bucketId, objectName, null, true);
                            if (null != nullMeta) {
                                metaDao.removeMeta(connection, metaHisCSName, metaHisClName, bucketId,
                                        objectName, nullMeta.getVersionId(), null);
                            }
                            deleteObject = nullMeta;
                        }
                        transaction.commit(connection);
                    }
                }
            } catch (S3ServerException e) {
                transaction.rollback(connection);
                if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex() && tryTime > 0) {
                    continue;
                } else {
                    throw e;
                }
            } catch (Exception e) {
                transaction.rollback(connection);
                throw e;
            } finally {
                daoMgr.releaseConnectionDao(connection);
            }

            return deleteObject;
        }
        return null;
    }

    //writeObjectMeta retrun the oldObject meta to delete old lob
    private ObjectMeta writeObjectMetaForMultiUpload(ConnectionDao connection, ObjectMeta objectMeta, String objectName, long bucketId,
                                       VersioningStatusType versioningStatusType, Region region)
            throws S3ServerException{
        String metaCsName    = regionDao.getMetaCurCSName(region);
        String metaClName    = regionDao.getMetaCurCLName(region);
        String metaHisCSName = regionDao.getMetaHisCSName(region);
        String metaHisClName = regionDao.getMetaHisCLName(region);

        ObjectMeta deleteObject = null;

        ObjectMeta metaResult = metaDao.queryForUpdate(connection, metaCsName, metaClName,
                bucketId, objectName, null, null);
        if (null == metaResult) {
            objectMeta.setVersionId(0);
            Bucket bucket = bucketDao.getBucketById(connection, bucketId);
            if (bucket == null){
                throw new S3ServerException(S3Error.BUCKET_NOT_EXIST, "bucket is deleting before insert meta.");
            }
            buildDirForObject(connection, metaCsName, bucket, objectName, objectMeta, region);
            metaDao.insertMeta(connection, metaCsName, metaClName, objectMeta,
                    false, region);
            Bucket newBucket = bucketDao.getBucketById(connection, bucketId);
            if (newBucket == null){
                throw new S3ServerException(S3Error.BUCKET_NOT_EXIST, "bucket is deleting after insert meta.");
            }
            if (bucket.getDelimiter() != newBucket.getDelimiter()) {
                throw new S3ServerException(S3Error.DAO_DUPLICATE_KEY, "bucket.getDelimiter() != newBucket.getDelimiter()");
            }
        } else {
            objectMeta.setVersionId(metaResult.getVersionId() + 1);
            objectMeta.setParentId1(metaResult.getParentId1());
            objectMeta.setParentId2(metaResult.getParentId2());
            if (VersioningStatusType.NONE == versioningStatusType
                    || (VersioningStatusType.SUSPENDED == versioningStatusType
                    && metaResult.getNoVersionFlag())) {
                metaDao.updateMeta(connection, metaCsName, metaClName, bucketId,
                        objectName, null, objectMeta);
                transaction.commit(connection);
                deleteObject = metaResult;
            } else {
                metaDao.insertMeta(connection, metaHisCSName, metaHisClName,
                        metaResult, true, region);
                metaDao.updateMeta(connection, metaCsName, metaClName, bucketId,
                        objectName, null, objectMeta);
                ObjectMeta nullMeta = null;
                if (VersioningStatusType.SUSPENDED == versioningStatusType) {
                    nullMeta = metaDao.queryForUpdate(connection, metaHisCSName, metaHisClName,
                            bucketId, objectName, null, true);
                    if (null != nullMeta) {
                        metaDao.removeMeta(connection, metaHisCSName, metaHisClName, bucketId,
                                objectName, nullMeta.getVersionId(), null);
                    }
                    deleteObject = nullMeta;
                }
            }
        }

        return deleteObject;
    }

    private Long getInnerVersionId(String metaCsName, String metaClName,
                                   String metaHisCsName, String metaHisClName, String keyMarker, String versionIdMarker, Bucket bucket)
            throws S3ServerException{
        try {
            Long versionId = null;
            if (versionIdMarker.equals(ObjectMeta.NULL_VERSION_ID)) {
                ObjectMeta metaCur = metaDao.queryMetaByObjectName(metaCsName, metaClName, bucket.getBucketId(), keyMarker, null, true);
                if (metaCur != null){
                    versionId = metaCur.getVersionId();
                }else {
                    ObjectMeta metaHis = metaDao.queryMetaByObjectName(metaHisCsName, metaHisClName, bucket.getBucketId(), keyMarker, null, true);
                    if (metaHis != null) {
                        versionId = metaHis.getVersionId();
                    }
                }
            } else {
                versionId = Long.parseLong(versionIdMarker);
            }
            return versionId;
        }catch (NumberFormatException e){
            logger.error("Parse versionIdMarker failed, versionIdMarker:{}",
                    versionIdMarker);
            throw new S3ServerException(S3Error.OBJECT_INVALID_VERSION,
                    "Parse versionIdMarker failed, versionIdMarker:"+ versionIdMarker, e);
        }catch (Exception e){
            logger.error("isExistKeyVersion failed, versionIdMarker:{}",
                    versionIdMarker);
            throw new S3ServerException(S3Error.OBJECT_INVALID_VERSION,
                    "isExistKeyVersion failed, versionIdMarker:"+ versionIdMarker, e);
        }
    }

    private void analyseRangeWithLob(Range range, DataLob dataLob) throws S3ServerException{
        if (null == range){
            return;
        }

        long contentLength = dataLob.getSize();
        if (range.getStart() >= contentLength){
            throw new S3ServerException(S3Error.OBJECT_RANGE_NOT_SATISFIABLE,
                    "start > contentlength. start:" + range.getStart() +
                            ", contentlength:" + contentLength);
        }

        //final bytes
        if (range.getStart() == -1){
            if (range.getEnd() == 0){
                throw new S3ServerException(S3Error.OBJECT_RANGE_NOT_SATISFIABLE,
                        " range is invalid,range=-0 ");
            }
            if(range.getEnd() < contentLength) {
                range.setStart(contentLength - range.getEnd());
                range.setEnd(contentLength-1);
            }else {
                range.setStart(0);
                range.setEnd(contentLength-1);
            }
        }

        //from start to the final of Lob
        if (range.getEnd() == -1 || range.getEnd() >= contentLength){
            range.setEnd(contentLength - 1);
        }

        //from 0 - final of Lob
        if (range.getStart() == 0 && range.getEnd() == contentLength - 1){
            range.setContentLength(contentLength);
            return;
        }

        long readLength  = range.getEnd() - range.getStart() + 1;
        range.setContentLength(readLength);
    }

    private long getSecondTime(long millionSecond){
        return millionSecond/1000;
    }
}
