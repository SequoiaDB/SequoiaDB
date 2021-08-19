package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.*;
import com.sequoias3.dao.DaoCollectionDefine;
import com.sequoias3.dao.IDGeneratorDao;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

@Component
public class SdbInitSystem {
    private static final Logger logger = LoggerFactory.getLogger(SdbInitSystem.class);

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Autowired
    SequoiadbConfig config;

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    @Autowired
    IDGeneratorDao idGeneratorDao;

    public void createSystemCS() throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            if(!sdb.isCollectionSpaceExist(config.getMetaCsName())){
                BSONObject option = null;
                if (config.getMetaDomain() != null) {
                    option = new BasicBSONObject();
                    option.put("Domain", config.getMetaDomain());
                }
                sdbBaseOperation.createCS(sdb, config.getMetaCsName(), option);
            }
        } catch (Exception e){
            logger.error("create system cs failed.", e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    public void createUserCL() throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            if(!cs.isCollectionExist(DaoCollectionDefine.USER_LIST_COLLECTION)) {
                sdbBaseOperation.createCL(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.USER_LIST_COLLECTION, null);
            }

            DBCollection cl = cs.getCollection(DaoCollectionDefine.USER_LIST_COLLECTION);

            String IDIndexName = "idIndex";
            if (!cl.isIndexExist(IDIndexName)) {
                BSONObject IDIndexKey = new BasicBSONObject();
                IDIndexKey.put(User.JSON_KEY_USERID, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.USER_LIST_COLLECTION,
                        IDIndexName, IDIndexKey, true, true);
            }

            String nameIndexName = "nameIndex";
            if (!cl.isIndexExist(nameIndexName)) {
                BSONObject nameIndexKey = new BasicBSONObject();
                nameIndexKey.put(User.JSON_KEY_USERNAME, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.USER_LIST_COLLECTION,
                        nameIndexName, nameIndexKey, true, true);
            }

            String accessIndexName = "accessIndex";
            if (!cl.isIndexExist(accessIndexName)) {
                BSONObject accessIndexKey = new BasicBSONObject();
                accessIndexKey.put(User.JSON_KEY_ACCESS_KEY_ID, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.USER_LIST_COLLECTION,
                        accessIndexName, accessIndexKey, true, true);
            }
        } catch (Exception e){
            logger.error("create user cl failed.", e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    public void createBucketCL() throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            if(!cs.isCollectionExist(DaoCollectionDefine.BUCKET_LIST_COLLECTION)) {
                sdbBaseOperation.createCL(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.BUCKET_LIST_COLLECTION, null);
            }
            DBCollection cl = cs.getCollection(DaoCollectionDefine.BUCKET_LIST_COLLECTION);

            if (!cl.isIndexExist(Bucket.ID_INDEX)) {
                BSONObject IDIndexKey = new BasicBSONObject();
                IDIndexKey.put(Bucket.BUCKET_ID, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.BUCKET_LIST_COLLECTION,
                        Bucket.ID_INDEX, IDIndexKey, true, true);
            }

            if (!cl.isIndexExist(Bucket.NAME_INDEX)) {
                BSONObject nameIndexKey = new BasicBSONObject();
                nameIndexKey.put(Bucket.BUCKET_NAME, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.BUCKET_LIST_COLLECTION,
                        Bucket.NAME_INDEX, nameIndexKey, true, true);
            }
        } catch (Exception e){
            logger.error("create bucket cl failed.", e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    public void createRegionCL() throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            if(!cs.isCollectionExist(DaoCollectionDefine.REGION_LIST_COLLECTION)) {
                sdbBaseOperation.createCL(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.REGION_LIST_COLLECTION, null);
            }

            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_LIST_COLLECTION);

            String indexName = "regionIndex";
            if (!cl.isIndexExist(indexName)) {
                BSONObject indexKey = new BasicBSONObject();
                indexKey.put(Region.REGION_NAME, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.REGION_LIST_COLLECTION,
                        indexName, indexKey, true, true);
            }
        } catch (Exception e){
            logger.error("create region cl failed.", e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    public void createRegionSpaceCL() throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            if (!cs.isCollectionExist(DaoCollectionDefine.REGION_SPACE_LIST)) {
                sdbBaseOperation.createCL(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.REGION_SPACE_LIST, null);
            }

            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_SPACE_LIST);

            String indexName = "regionSpaceIndex";
            if (!cl.isIndexExist(indexName)) {
                BSONObject indexKey = new BasicBSONObject();
                indexKey.put(RegionSpace.REGION_SPACE_REGIONNAME, 1);
                indexKey.put(RegionSpace.REGION_SPACE_NAME, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.REGION_SPACE_LIST,
                        indexName, indexKey, true, true);
            }
        } catch (Exception e) {
            logger.error("create region space cl failed.", e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    public void createIDGeneratorCL() throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            if (!cs.isCollectionExist(DaoCollectionDefine.ID_GENERATOR)){
                sdbBaseOperation.createCL(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.ID_GENERATOR, null);
            }

            DBCollection cl = cs.getCollection(DaoCollectionDefine.ID_GENERATOR);
            String indexName = "typeIndex";
            if (!cl.isIndexExist(indexName)){
                BSONObject indexKey = new BasicBSONObject();
                indexKey.put(IDGenerator.ID_TYPE, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.ID_GENERATOR, indexName,
                        indexKey, true, true);
            }

            idGeneratorDao.insertId(IDGenerator.TYPE_USER);
            idGeneratorDao.insertId(IDGenerator.TYPE_BUCKET);
            idGeneratorDao.insertId(IDGenerator.TYPE_PARENTID);
            idGeneratorDao.insertId(IDGenerator.TYPE_TASK);
            idGeneratorDao.insertId(IDGenerator.TYPE_UPLOAD);
            idGeneratorDao.insertId(IDGenerator.TYPE_ACLID);
        }catch (Exception e) {
            logger.error("create IDTable failed.", e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    public void createTaskCL() throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            if (!cs.isCollectionExist(DaoCollectionDefine.TASK_COLLECTION)){
                sdbBaseOperation.createCL(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.TASK_COLLECTION, null);
            }

            DBCollection cl = cs.getCollection(DaoCollectionDefine.TASK_COLLECTION);
            if ( !cl.isIndexExist(TaskTable.TASK_INDEX)){
                BSONObject indexKey = new BasicBSONObject();
                indexKey.put(TaskTable.TASK_TYPE, 1);
                indexKey.put(TaskTable.TASK_ID, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.TASK_COLLECTION, TaskTable.TASK_INDEX,
                        indexKey, true, true);
            }

        }catch (Exception e) {
            logger.error("create TaskTable failed.", e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    public void createUploadCL() throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            if (!cs.isCollectionExist(DaoCollectionDefine.UPLOAD_LIST)){
                sdbBaseOperation.createCL(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.UPLOAD_LIST, null);
            }

            DBCollection cl = cs.getCollection(DaoCollectionDefine.UPLOAD_LIST);
            if ( !cl.isIndexExist(UploadMeta.UPLOAD_INDEX)){
                BSONObject indexKey = new BasicBSONObject();
                indexKey.put(UploadMeta.META_BUCKET_ID, 1);
                indexKey.put(UploadMeta.META_KEY_NAME, 1);
                indexKey.put(UploadMeta.META_UPLOAD_ID, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.UPLOAD_LIST, UploadMeta.UPLOAD_INDEX,
                        indexKey, true, true);
            }

        }catch (Exception e) {
            logger.error("create upload failed.", e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    public void createPartCL() throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            if (!cs.isCollectionExist(DaoCollectionDefine.PART_LIST)){
                sdbBaseOperation.createCL(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.PART_LIST, null);
            }

            DBCollection cl = cs.getCollection(DaoCollectionDefine.PART_LIST);
            if ( !cl.isIndexExist(Part.PART_INDEX)){
                BSONObject indexKey = new BasicBSONObject();
                indexKey.put(Part.UPLOADID, 1);
                indexKey.put(Part.PARTNUMBER, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.PART_LIST, Part.PART_INDEX,
                        indexKey, true, true);
            }

        }catch (Exception e) {
            logger.error("create upload failed.", e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    public void createACLTable() throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            if (!cs.isCollectionExist(DaoCollectionDefine.ACL_LIST)){
                sdbBaseOperation.createCL(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.ACL_LIST, null);
            }

            DBCollection cl = cs.getCollection(DaoCollectionDefine.ACL_LIST);
            if ( !cl.isIndexExist(AclTable.ID_INDEX)){
                BSONObject indexKey = new BasicBSONObject();
                indexKey.put(AclTable.ACL_ID, 1);
                sdbBaseOperation.createIndex(sdb, config.getMetaCsName(),
                        DaoCollectionDefine.ACL_LIST, AclTable.ID_INDEX,
                        indexKey, false, false);
            }

        }catch (Exception e) {
            logger.error("create upload failed.", e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }
}
