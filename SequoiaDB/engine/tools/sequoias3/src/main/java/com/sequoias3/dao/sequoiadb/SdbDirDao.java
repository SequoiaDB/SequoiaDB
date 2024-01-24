package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DBParamDefine;
import com.sequoias3.core.Dir;
import com.sequoias3.core.Region;
import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.DaoCollectionDefine;
import com.sequoias3.dao.DirDao;
import com.sequoias3.dao.QueryDbCursor;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

@Repository("DirDao")
public class SdbDirDao implements DirDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbDirDao.class);

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    @Autowired
    SequoiadbRegionSpaceDao sequoiadbRegionSpaceDao;

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Override
    public void insertDir(ConnectionDao connection, String metaCsName, Dir dir, Region region)
            throws S3ServerException {
        try{
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            insert(sdb, metaCsName, dir);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()
                    || e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                logger.info("no dir collection. cl = {}.{}", metaCsName, DaoCollectionDefine.OBJECT_DIR);
                throw new S3ServerException(S3Error.REGION_COLLECTION_NOT_EXIST, "no dir collection");
            } else {
                logger.error("insert dir failed. error:"+e);
                throw e;
            }
        }catch (Exception e){
            logger.error("insert dir failed. error:"+e);
            throw e;
        }
    }

    private void insert(Sequoiadb sdb, String metaCsName, Dir dir)
            throws S3ServerException{
        try {
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection cl = cs.getCollection(DaoCollectionDefine.OBJECT_DIR);

            BSONObject insertData = new BasicBSONObject();
            insertData.put(Dir.DIR_BUCKETID, dir.getBucketId());
            insertData.put(Dir.DIR_DELIMITER, dir.getDelimiter());
            insertData.put(Dir.DIR_NAME, dir.getName());
            insertData.put(Dir.DIR_ID, dir.getID());
            cl.insert(insertData);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_IXM_DUP_KEY.getErrorCode()) {
                logger.error("duplicate dir. csname:{}, dirName:{}", metaCsName, dir.getName());
                throw new S3ServerException(S3Error.DAO_DUPLICATE_KEY,
                        "Duplicate key. csname:" + metaCsName +
                                ", dirName:" + dir.getName());
            }else{
                throw e;
            }
        }catch (Exception e){
            logger.error("Insert dir:{} failed", dir.getName());
            throw e;
        }
    }

    @Override
    public Dir queryDir(ConnectionDao connection, String metaCsName, Long bucketId,
                                 String delimiter, String dirName, Boolean forUpdate)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else{
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection    cl = cs.getCollection(DaoCollectionDefine.OBJECT_DIR);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Dir.DIR_BUCKETID, bucketId);
            matcher.put(Dir.DIR_DELIMITER, delimiter);
            matcher.put(Dir.DIR_NAME, dirName);
            Integer flag = 0;
            if (forUpdate){
                flag = DBQuery.FLG_QUERY_FOR_UPDATE;
            }

            BSONObject hint = new BasicBSONObject();
            hint.put("", Dir.DIR_INDEX);

            BSONObject record = cl.queryOne(matcher, null, null, hint, flag);
            if (record != null){
                Dir dir = new Dir(bucketId, delimiter, dirName, (long)record.get(Dir.DIR_ID));
                return dir;
            }else{
                return null;
            }
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                return null;
            } else {
                logger.error("query dir failed. dirName:{}, error:{}", dirName, e.getMessage());
                throw e;
            }
        } catch (Exception e){
            logger.error("query dir failed.");
            throw e;
        }finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public QueryDbCursor queryDirList(String metaCsName, Long bucketId,
                                      String delimiter, String dirPrefix, String startAfter)
            throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor  cursor = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection    cl = cs.getCollection(DaoCollectionDefine.OBJECT_DIR);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Dir.DIR_BUCKETID, bucketId);
            matcher.put(Dir.DIR_DELIMITER, delimiter);
            BSONObject nameMatcher = new BasicBSONObject();
            if (dirPrefix != null && dirPrefix.length() > 0){
                nameMatcher.put(DBParamDefine.GREATER, dirPrefix);
                String prefixEnd = dirPrefix.substring(0,dirPrefix.length()-1) + (char)(dirPrefix.charAt(dirPrefix.length()-1)+1);
                nameMatcher.put(DBParamDefine.LESS_THAN, prefixEnd);
            }
            if (startAfter != null){
                if (dirPrefix == null || startAfter.compareTo(dirPrefix) > 0) {
                    nameMatcher.put(DBParamDefine.GREATER, startAfter);
                }
            }
            if (!nameMatcher.isEmpty()) {
                matcher.put(Dir.DIR_NAME, nameMatcher);
            }

            BSONObject hint = new BasicBSONObject();
            hint.put("", Dir.DIR_INDEX);

            cursor = cl.query(matcher, null, null, hint);
            return new SdbQueryDbCursor(sdb, cursor);
        }catch (BaseException e){
            sdbBaseOperation.releaseDBCursor(cursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                return null;
            } else {
                logger.error("query dir list failed. error:"+e);
                throw e;
            }
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(cursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            logger.error("query dir list failed.");
            throw e;
        }
    }

    @Override
    public void delete(ConnectionDao connection, String metaCsName, Long bucketId, String delimiter, String dirName)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try{
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(metaCsName);
            DBCollection    cl = cs.getCollection(DaoCollectionDefine.OBJECT_DIR);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Dir.DIR_BUCKETID, bucketId);
            if (dirName != null) {
                matcher.put(Dir.DIR_NAME, dirName);
            }
            if (delimiter != null) {
                matcher.put(Dir.DIR_DELIMITER, delimiter);
            }

            BSONObject hint = new BasicBSONObject();
            hint.put("", Dir.DIR_INDEX);

            cl.delete(matcher, hint);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return;
            }else {
                throw new S3ServerException(S3Error.DAO_DB_ERROR, "db error.", e);
            }
        }catch (Exception e){
            logger.error("remove dir failed. error:"+e.getMessage());
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "db error.", e);
        }finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }



}
