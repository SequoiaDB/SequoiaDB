package com.sequoias3.service.impl;

import com.sequoias3.common.RestParamDefine;
import com.sequoias3.common.VersioningStatusType;
import com.sequoias3.core.*;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.*;
import com.sequoias3.service.ACLService;
import com.sequoias3.service.BucketService;
import com.sequoias3.service.ObjectService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.List;

@Service
public class ACLServiceImpl implements ACLService {
    private static final Logger logger = LoggerFactory.getLogger(ACLServiceImpl.class);

    @Autowired
    BucketDao bucketDao;

    @Autowired
    UserDao userDao;

    @Autowired
    BucketService bucketService;

    @Autowired
    ObjectService objectService;

    @Autowired
    RegionDao regionDao;

    @Autowired
    MetaDao metaDao;

    @Autowired
    DaoMgr daoMgr;

    @Autowired
    Transaction transaction;

    @Autowired
    IDGeneratorDao idGeneratorDao;

    @Autowired
    AclDao aclDao;

    @Override
    public void putBucketAcl(long ownerID, String bucketName,
                             AccessControlPolicy aclConfig)
            throws S3ServerException {
        Bucket bucketA = bucketService.getBucket(ownerID, bucketName);
        //检查请求中的ownerId和bucket的ownerId是否一致
        checkOwner(aclConfig, bucketA);

        //事务中
        ConnectionDao connection = daoMgr.getConnectionDao();
        transaction.begin(connection);
        try{
            Bucket bucket = bucketDao.queryBucketForUpdate(connection, bucketName);

            long aclId;
            //删除旧的acl，如果有的话
            if (bucket.getAclId() != null){
                aclId = bucket.getAclId();
                aclDao.deleteAcl(connection, aclId);
            }else {
                aclId = idGeneratorDao.getNewId(IDGenerator.TYPE_ACLID);
            }

            //插入新的acl, 并分析是否是private
            boolean isPrivate = insertGrants(connection, aclConfig, aclId);

            //is private
            if (bucket.getAclId() == null || isPrivate != bucket.isPrivate()){
                bucketDao.updateBucketAcl(connection, bucketName, aclId, isPrivate);
            }

            transaction.commit(connection);
        } catch (S3ServerException e){
            transaction.rollback(connection);
            throw e;
        } catch (Exception e){
            transaction.rollback(connection);
            throw new S3ServerException(S3Error.ACL_PUT_BUCKET_ACL_FAIL,
                    "put bucket acl failed.", e);
        } finally{
            daoMgr.releaseConnectionDao(connection);
        }
    }

    @Override
    public AccessControlPolicy getBucketAcl(long ownerId, String bucketName) throws S3ServerException {
        Bucket bucket = bucketService.getBucket(ownerId, bucketName);
        AccessControlPolicy result = new AccessControlPolicy();

        Owner owner = userDao.getOwnerByUserID(bucket.getOwnerId());
        result.setOwner(owner);

        List<Grant> grantList;
        if (bucket.getAclId() != null) {
            grantList = aclDao.queryAcl(bucket.getAclId());
        }else {
            Grant ownerGrant = new Grant();
            ownerGrant.setPermission(RestParamDefine.Acl.ACL_FULLCONTROL);
            ownerGrant.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_USER, owner.getUserId(), owner.getUserName(), null, null));
            grantList = new ArrayList<>();
            grantList.add(ownerGrant);
        }

        result.setGrants(grantList);
        return result;
    }

    @Override
    public String putObjectAcl(long ownerID, ObjectUri objectUri,
                             AccessControlPolicy aclConfig)
            throws S3ServerException {
        Bucket bucket = bucketService.getBucket(ownerID, objectUri.getBucketName());

        Region region = null;
        if (bucket.getRegion() != null) {
            region = regionDao.queryRegion(bucket.getRegion());
        }

        String metaCsName    = regionDao.getMetaCurCSName(region);
        String metaClName    = regionDao.getMetaCurCLName(region);
        String metaHisCsName = regionDao.getMetaHisCSName(region);
        String metaHisClName = regionDao.getMetaHisCLName(region);

        //事务中
        ConnectionDao connection = daoMgr.getConnectionDao();
        transaction.begin(connection);
        try {
            checkOwner(aclConfig, bucket);

            String versionCsName = null;
            String versionClName = null;
            ObjectMeta versionMeta = null;

            ObjectMeta metaResult = metaDao.queryForUpdate(connection, metaCsName, metaClName,
                    bucket.getBucketId(), objectUri.getObjectName(), null, null);
            if (null == metaResult) {
                if (!objectUri.isWithVersionId()) {
                    throw new S3ServerException(S3Error.OBJECT_NO_SUCH_KEY,
                            "no such key:" + objectUri.getObjectName());
                }else {
                    throw new S3ServerException(S3Error.OBJECT_NO_SUCH_VERSION,
                            "no such version:" + objectUri.getObjectName());
                }
            }

            versionMeta = metaResult;
            versionCsName = metaCsName;
            versionClName = metaClName;
            if (objectUri.isWithVersionId()){
                if (!isVersionMeta(metaResult, objectUri)){
                    ObjectMeta objectMetaHis = findHisVersionIdMeta(connection,
                            bucket, objectUri, metaHisCsName, metaHisClName);
                    if (objectMetaHis == null) {
                        throw new S3ServerException(S3Error.OBJECT_NO_SUCH_VERSION,
                                "no such version. object:" + objectUri.getObjectName()
                                        + ",version:" + objectUri.getVersionId()
                                        + ", noVersionFlag:" + objectUri.getNullVersionFlag());
                    }else {
                        versionMeta = objectMetaHis;
                        versionCsName = metaHisCsName;
                        versionClName = metaHisClName;
                    }
                }
            }

            if (versionMeta.getDeleteMarker()){
                throw new S3ServerException(S3Error.METHOD_NOT_ALLOWED,
                        "resource : DeleteMarker");
            }

            long aclId;
            if (versionMeta.getAclId() != null){
                aclId = versionMeta.getAclId();
                aclDao.deleteAcl(connection, aclId);
            }else {
                aclId = idGeneratorDao.getNewId(IDGenerator.TYPE_ACLID);
            }

            boolean isPrivate = insertGrants(connection, aclConfig, aclId);

            if (versionMeta.getAclId() == null){
                versionMeta.setAclId(aclId);
                metaDao.updateMeta(connection, versionCsName, versionClName,
                        bucket.getBucketId(), objectUri.getObjectName(),
                        versionMeta.getVersionId(), versionMeta);
            }

            if(!isPrivate && bucket.getAclId() == null){
                bucketDao.updateBucketAcl(connection, objectUri.getBucketName(), null, false);
            }

            transaction.commit(connection);

            VersioningStatusType versioningStatusType = VersioningStatusType.getVersioningStatus(bucket.getVersioningStatus());
            String reVersionId = null;
            if (versioningStatusType != VersioningStatusType.NONE){
                if (versionMeta.getNoVersionFlag()){
                    reVersionId = "null";
                }else {
                    reVersionId = String.valueOf(versionMeta.getVersionId());
                }
            }
            return reVersionId;
        } catch (S3ServerException e){
            transaction.rollback(connection);
            throw e;
        } catch (Exception e){
            transaction.rollback(connection);
            throw new S3ServerException(S3Error.ACL_PUT_OBJECT_ACL_FAIL,
                    "put object acl failed.", e);
        } finally{
            daoMgr.releaseConnectionDao(connection);
        }
    }

    @Override
    public AccessControlPolicy getObjectAcl(long ownerId, ObjectUri objectUri)
            throws S3ServerException {
        Bucket bucket = bucketService.getBucket(ownerId, objectUri.getBucketName());
        AccessControlPolicy result = new AccessControlPolicy();

        Owner owner = userDao.getOwnerByUserID(bucket.getOwnerId());
        result.setOwner(owner);

        ObjectMeta objectMeta = objectService.getSourceObjectMeta(ownerId, null, objectUri);
        if (objectMeta.getDeleteMarker()) {
            if (objectUri.isWithVersionId()) {
                throw new S3ServerException(S3Error.METHOD_NOT_ALLOWED, "object with versionId is a deleteMarker");
            } else {
                throw new S3ServerException(S3Error.OBJECT_NO_SUCH_KEY, "object is a deleteMarker");
            }
        }

        List<Grant> grantList;
        if (objectMeta.getAclId() != null) {
            grantList = aclDao.queryAcl(objectMeta.getAclId());
        }else {
            Grant ownerGrant = new Grant();
            ownerGrant.setPermission(RestParamDefine.Acl.ACL_FULLCONTROL);
            ownerGrant.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_USER, owner.getUserId(), owner.getUserName(), null, null));
            grantList = new ArrayList<>();
            grantList.add(ownerGrant);
        }

        result.setGrants(grantList);
        return result;
    }

    private boolean insertGrants(ConnectionDao connection, AccessControlPolicy aclConfig,
                              long aclId) throws S3ServerException{
        if (aclConfig == null || aclConfig.getGrants() == null){
            return true;
        }
        List<Grant> grantList = aclConfig.getGrants();
        boolean isPrivate = true;
        for (int i=0; i < grantList.size(); i++){
            Grant grant = grantList.get(i);
            if (grant.getGrantee().getType().equals(RestParamDefine.Acl.TYPE_USER)){
                long granteeId = grantList.get(i).getGrantee().getId();
                Owner grantee = userDao.getOwnerByUserID(granteeId);
                if (grantee == null){
                    throw new S3ServerException(S3Error.ACL_INVALID_ID, "invalid grantee id. id:"+granteeId);
                }
                grant.getGrantee().setDisplayName(grantee.getUserName());
                if (granteeId != aclConfig.getOwner().getUserId()){
                    isPrivate = false;
                }
            }else {
                isPrivate = false;
            }
            aclDao.insertAcl(connection, aclId, grant);
        }
        return isPrivate;
    }

    private void checkOwner(AccessControlPolicy aclConfig, Bucket bucket) throws S3ServerException{
        if (aclConfig.getOwner() != null) {
            if (aclConfig.getOwner().getUserId() != bucket.getOwnerId()) {
                throw new S3ServerException(S3Error.ACL_INVALID_ID,
                        "owner is not match");
            }
        }else {
            Owner owner = userDao.getOwnerByUserID(bucket.getOwnerId());
            aclConfig.setOwner(owner);
        }
    }

    private boolean isVersionMeta(ObjectMeta metaResult, ObjectUri objectUri) throws S3ServerException{
        if (objectUri.getVersionId() != null) {
            if (objectUri.getVersionId().equals(metaResult.getVersionId())) {
                if (metaResult.getNoVersionFlag()) {
                    throw new S3ServerException(S3Error.OBJECT_NO_SUCH_VERSION,
                            "no such key:" + objectUri.getObjectName()
                                    + ", version:" + objectUri.getVersionId());
                }
                return true;
            }
        } else if (objectUri.getNullVersionFlag() != null && objectUri.getNullVersionFlag()) {
            if (metaResult.getNoVersionFlag()){
                return true;
            }
        }
        return false;
    }

    private ObjectMeta findHisVersionIdMeta(ConnectionDao connection, Bucket bucket,
                                            ObjectUri objectUri, String metaHisCsName,
                                            String metaHisClName)
            throws S3ServerException{
        if (objectUri.getVersionId() != null) {
            return metaDao.queryForUpdate(connection, metaHisCsName,
                    metaHisClName, bucket.getBucketId(), objectUri.getObjectName(), objectUri.getVersionId(), false);
        } else if (objectUri.getNullVersionFlag() != null && objectUri.getNullVersionFlag()){
            return metaDao.queryForUpdate(connection, metaHisCsName,
                    metaHisClName, bucket.getBucketId(), objectUri.getObjectName(),
                    null, true);
        }
        return null;
    }
}
