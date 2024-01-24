package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.TaskTable;
import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.DaoCollectionDefine;
import com.sequoias3.dao.QueryDbCursor;
import com.sequoias3.dao.TaskDao;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

@Repository("TaskDao")
public class SdbTask implements TaskDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbTask.class);

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    @Autowired
    SequoiadbConfig config;

    @Override
    public void insertTaskId(ConnectionDao connection, Long taskId)
            throws S3ServerException {
        try {
            Sequoiadb sdb      = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl    =  cs.getCollection(DaoCollectionDefine.TASK_COLLECTION);

            BSONObject insert = new BasicBSONObject();
            insert.put(TaskTable.TASK_ID, taskId);
            insert.put(TaskTable.TASK_TYPE, TaskTable.TASK_TYPE_DELIMITER);

            cl.insert(insert);
        }catch (Exception e){
            throw e;
        }
    }

    @Override
    public Long queryTaskId(ConnectionDao connection, Long taskId) throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao) connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.TASK_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(TaskTable.TASK_TYPE, TaskTable.TASK_TYPE_DELIMITER);
            matcher.put(TaskTable.TASK_ID, taskId);

            BSONObject hint = new BasicBSONObject();
            hint.put("", TaskTable.TASK_INDEX);

            BSONObject result = cl.queryOne(matcher, null, null, hint, DBQuery.FLG_QUERY_FOR_UPDATE);
            Long id = null;
            if (result != null) {
                id = (long) result.get(TaskTable.TASK_ID);
            }
            return id;
        }catch (Exception e){
            throw e;
        }
    }

    @Override
    public void deleteTaskId(ConnectionDao connection, Long taskId) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null) {
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.TASK_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(TaskTable.TASK_TYPE, TaskTable.TASK_TYPE_DELIMITER);
            matcher.put(TaskTable.TASK_ID, taskId);

            BSONObject hint = new BasicBSONObject();
            hint.put("", TaskTable.TASK_INDEX);

            cl.delete(matcher, hint);
        }catch (Exception e){
            throw e;
        }finally {
            if (connection == null){
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public void insertUploadId(long uploadId) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb      = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl    =  cs.getCollection(DaoCollectionDefine.TASK_COLLECTION);

            BSONObject insert = new BasicBSONObject();
            insert.put(TaskTable.TASK_ID, uploadId);
            insert.put(TaskTable.TASK_TYPE, TaskTable.TASK_COMPLETE_UPLOAD);

            cl.insert(insert);
        }catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_IXM_DUP_KEY.getErrorCode()) {
                throw e;
            }
        }catch (Exception e){
            throw e;
        }finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public void deleteUploadId(long uploadId) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.TASK_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(TaskTable.TASK_TYPE, TaskTable.TASK_COMPLETE_UPLOAD);
            matcher.put(TaskTable.TASK_ID, uploadId);

            BSONObject hint = new BasicBSONObject();
            hint.put("", TaskTable.TASK_INDEX);

            cl.delete(matcher, hint);
        } catch (Exception e){
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public boolean queryUploadId(long uploadId) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.TASK_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(TaskTable.TASK_TYPE, TaskTable.TASK_COMPLETE_UPLOAD);
            matcher.put(TaskTable.TASK_ID, uploadId);

            BSONObject hint = new BasicBSONObject();
            hint.put("", TaskTable.TASK_INDEX);

            BSONObject result = cl.queryOne(matcher, null, null, hint, DBQuery.FLG_QUERY_FOR_UPDATE);
            if (result != null) {
                return true;
            }else {
                return false;
            }
        } catch (Exception e){
            logger.error("queryUploadId failed. uploadId:"+uploadId, e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    public QueryDbCursor queryUploadList() throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor dbCursor = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.TASK_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(TaskTable.TASK_TYPE, TaskTable.TASK_COMPLETE_UPLOAD);

            BSONObject hint = new BasicBSONObject();
            hint.put("", TaskTable.TASK_INDEX);

            dbCursor = cl.query(matcher, null, null, hint, 0);
            return new SdbQueryDbCursor(sdb, dbCursor);
        } catch (Exception e){
            sdbBaseOperation.releaseDBCursor(dbCursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
            logger.error("query upload list failed.", e);
            throw e;
        }
    }
}
