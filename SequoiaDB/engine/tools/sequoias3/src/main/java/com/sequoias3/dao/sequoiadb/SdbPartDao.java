package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DBParamDefine;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.Part;
import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.DaoCollectionDefine;
import com.sequoias3.dao.PartDao;
import com.sequoias3.dao.QueryDbCursor;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

@Repository("PartDao")
public class SdbPartDao implements PartDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbPartDao.class);

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Autowired
    SequoiadbConfig config;

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    @Override
    public void insertPart(ConnectionDao connection, long uploadId, long partNumber, Part part)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.PART_LIST);

            BSONObject partMeta = convertMetaToBson(part);

            cl.insert(partMeta);
        }catch (BaseException e){
            if (e.getErrorType() == SDBError.SDB_IXM_DUP_KEY.name()) {
                throw new S3ServerException(S3Error.DAO_DUPLICATE_KEY, "Duplicate key.");
            } else {
                throw e;
            }
        }
        catch (Exception e) {
            logger.error("insert part failed. uploadId:{}, partnumber:{}", uploadId, partNumber);
            throw e;
        }finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public void updatePart(ConnectionDao connection, long uploadId, long partNumber, Part part)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.PART_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Part.UPLOADID, uploadId);
            matcher.put(Part.PARTNUMBER, partNumber);

            BSONObject updatePart = convertMetaToBson(part);
            BSONObject setUpdate = new BasicBSONObject();
            setUpdate.put(DBParamDefine.MODIFY_SET, updatePart);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.update(matcher, setUpdate, hint);
        } catch (Exception e) {
            logger.error("update part failed. uploadId:{}, partnumber:{}", uploadId, partNumber);
            throw e;
        }finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public Part queryPartByPartnumber(ConnectionDao connection, long uploadId, long partNumber) throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao) connection).getConnection();

            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.PART_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Part.UPLOADID, uploadId);
            matcher.put(Part.PARTNUMBER, partNumber);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            BSONObject result = cl.queryOne(matcher, null, null, hint, DBQuery.FLG_QUERY_FOR_UPDATE);
            if (result == null){
                return null;
            }else {
                return new Part(result);
            }
        }catch (Exception e){
            logger.error("query part by partnumber failed. uploadId:{}, partnumber:{}", uploadId, partNumber);
            throw e;
        }
    }

    @Override
    public Part queryPartBySize(ConnectionDao connection, long uploadId, Long size)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            } else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }

            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.PART_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Part.UPLOADID, uploadId);
            if (size != null) {
                matcher.put(Part.SIZE, size);
            }

            //0,-1两个分块都是预先准备的lob，size相同的话就可以使用
            //其他负数的partnumber也是
            BSONObject partnumberMatcher = new BasicBSONObject();
            partnumberMatcher.put(DBParamDefine.NOT_SMALL, -1000);
            matcher.put(Part.PARTNUMBER, partnumberMatcher);

            BSONObject order = new BasicBSONObject();
            order.put(Part.PARTNUMBER, 1);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            BSONObject result = cl.queryOne(matcher, null, order, hint, 0);
            if (result == null){
                return null;
            }else {
                return new Part(result);
            }
        }catch (Exception e){
            logger.error("query part size failed. uploadId:{}, size:{}", uploadId, size);
            throw e;
        }finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public void deletePart(ConnectionDao connection, long uploadId, Long partNumber)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            } else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }

            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.PART_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Part.UPLOADID, uploadId);
            if (partNumber != null) {
                matcher.put(Part.PARTNUMBER, partNumber);
            }

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.delete(matcher, hint);
        }catch (Exception e){
            logger.error("delete part failed. uploadId:{}, partnumber:{}", uploadId, partNumber);
            throw e;
        }finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public QueryDbCursor queryPartList(long uploadId, Boolean onlyPositiveNo,
                                       Integer marker, Integer maxSize)
            throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor dbCursor = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.PART_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Part.UPLOADID, uploadId);
            BSONObject partNumberMatcher = new BasicBSONObject();
            if (onlyPositiveNo) {
                partNumberMatcher.put(DBParamDefine.GREATER, 0);
            }
            if (marker != null){
                partNumberMatcher.put(DBParamDefine.GREATER, marker);
            }
            if (!partNumberMatcher.isEmpty()) {
                matcher.put(Part.PARTNUMBER, partNumberMatcher);
            }
            BSONObject isNull = new BasicBSONObject();
            isNull.put(DBParamDefine.IS_NULL, 0);
            matcher.put(Part.LOBID, isNull);

            BSONObject order = new BasicBSONObject();
            order.put(Part.PARTNUMBER, 1);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            long returnRow = -1L;
            if (maxSize != null){
                returnRow = maxSize;
            }

            dbCursor = cl.query(matcher, null, order, hint, 0, returnRow);
            return new SdbQueryDbCursor(sdb, dbCursor);
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            logger.error("query part list failed. uploadId:{}, marker:{}", uploadId, marker);
            throw e;
        }
    }

    @Override
    public QueryDbCursor queryPartListForUpdate(ConnectionDao connection, long uploadId) throws S3ServerException {
        DBCursor dbCursor = null;
        try {
            Sequoiadb sdb = ((SdbConnectionDao) connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.PART_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Part.UPLOADID, uploadId);
            BSONObject partnumberMatcher = new BasicBSONObject();
            partnumberMatcher.put(DBParamDefine.GREATER, 0);
            matcher.put(Part.PARTNUMBER, partnumberMatcher);

            BSONObject order = new BasicBSONObject();
            order.put(Part.PARTNUMBER, 1);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            dbCursor = cl.query(matcher, null, order, hint, DBQuery.FLG_QUERY_FOR_UPDATE);
            return new SdbQueryDbCursor(sdb, dbCursor);
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            logger.error("query parts failed. uploadId:{}", uploadId);
            throw e;
        }
    }

    private BSONObject convertMetaToBson(Part part){
        BSONObject partMeta = new BasicBSONObject();
        partMeta.put(Part.UPLOADID, part.getUploadId());
        partMeta.put(Part.PARTNUMBER, part.getPartNumber());
        partMeta.put(Part.CSNAME, part.getCsName());
        partMeta.put(Part.CLNAME, part.getClName());
        partMeta.put(Part.LOBID, part.getLobId());
        partMeta.put(Part.SIZE, part.getSize());
        partMeta.put(Part.ETAG, part.getEtag());
        partMeta.put(Part.LASTMODIFIED, part.getLastModified());

        return partMeta;
    }
}
