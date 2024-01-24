package com.sequoias3.dao;

import com.sequoias3.core.ObjectMeta;
import com.sequoias3.core.Region;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;

import java.util.List;

public interface MetaDao {
    void insertMeta(ConnectionDao connectionDao, String metaCsName, String metaClName,
                    ObjectMeta object, Boolean isHistory, Region region)
            throws S3ServerException;

    QueryDbCursor queryMetaByBucket(String metaCsName, String metaClName, long bucketId,
                                    String prefix, String startAfter, Long specifiedVId,
                                    Boolean isIncludeDM)
            throws S3ServerException;

    ObjectMeta queryMetaByObjectName(String metaCsName, String metaClName,
                                     long bucketId, String objectName, Long versionId,
                                     Boolean noVersionFlag)
            throws S3ServerException;

    ObjectMeta queryMetaByBucketId(ConnectionDao connection, String metaCsName,
                                   String metaClName, long bucketId)
            throws S3ServerException;

    QueryDbCursor queryMetaByBucketForUpdate(ConnectionDao connectionDao, String metaCsName,
                                             String metaClName, long bucketId, String prefix,
                                             String startAfter, int limitNum )
            throws S3ServerException;

    QueryDbCursor queryMetaByBucketInKeys(String metaCsName, String metaClName,
                                  long bucketId, List<String> keys)
            throws S3ServerException;

    QueryDbCursor queryMetaListByParentId(String metaCsName, String metaClName, long bucketId,
                                          String parentIdName, long parentId,  String prefix,
                                          String startAfter, Long versionIdMarker, Boolean isIncludeDeleteMarker)
            throws S3ServerException;

    Boolean queryOneOtherMetaByParentId(ConnectionDao connectionDao, String metaCsName, String metaClName, long bucketId,
                                        String objectName, String parentIdName, long parentId)
            throws S3ServerException;

    ObjectMeta queryForUpdate(ConnectionDao connectionDao, String metaCsName, String metaClName,
                              long bucketId, String objectName, Long versionId, Boolean noVersionFlag)
            throws S3ServerException;

    void updateMeta(ConnectionDao connectionDao, String metaCsName, String metaClName, long bucketId,
                    String objectName, Long versionId, ObjectMeta object)
            throws S3ServerException;

    void updateMetaParentId(ConnectionDao connectionDao, String metaCsName, String metaClName, long bucketId,
                            String objectName, String parentIdName, long parentId)
            throws S3ServerException;

    void removeMeta(ConnectionDao connectionDao, String metaCsName, String metaClName, long bucketId,
                    String objectName, Long versionId, Boolean noVersionFlag)
            throws S3ServerException;

    long getObjectNumber(String metaCsName, String metaClName, long bucketId)
            throws S3ServerException;



    void releaseQueryDbCursor(QueryDbCursor queryDbCursor);
}
