package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DBParamDefine;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.model.Owner;
import com.sequoias3.core.User;
import com.sequoias3.dao.DaoCollectionDefine;
import com.sequoias3.dao.UserDao;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

@Repository("UserDao")
public class SequoiadbUserDao implements UserDao {
    private static final Logger logger = LoggerFactory.getLogger(SequoiadbUserDao.class);
    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Autowired
    SequoiadbConfig config;

    @Override
    public void insertUser(User user) throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.USER_LIST_COLLECTION);

            BSONObject newUser = new BasicBSONObject();
            newUser.put(User.JSON_KEY_USERID, user.getUserId());
            newUser.put(User.JSON_KEY_USERNAME, user.getUserName());
            newUser.put(User.JSON_KEY_ROLE, user.getRole());
            newUser.put(User.JSON_KEY_ACCESS_KEY_ID, user.getAccessKeyID());
            newUser.put(User.JSON_KEY_SECRET_ACCESS_KEY, user.getSecretAccessKey());

            cl.insert(newUser);
        }catch (BaseException e) {
            if (e.getErrorType() == SDBError.SDB_IXM_DUP_KEY.name()) {
                throw new S3ServerException(S3Error.DAO_DUPLICATE_KEY, "Duplicate key.");
            }else{
                throw e;
            }
        } catch (Exception e) {
            logger.error("insertUser failed. errorMessage = " + e.getMessage(), e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public void deleteUser(String userName) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.USER_LIST_COLLECTION);

            BSONObject deleteUser = new BasicBSONObject();
            deleteUser.put(User.JSON_KEY_USERNAME, userName);

            cl.delete(deleteUser);
        } catch (Exception e) {
            logger.error("deleteUser failed. errorMessage = " + e.getMessage(), e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public void updateUserKeys(String userName, String accessKeyId, String secretAccessKey)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.USER_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(User.JSON_KEY_USERNAME, userName);
            BSONObject modifier = new BasicBSONObject();
            modifier.put(User.JSON_KEY_ACCESS_KEY_ID, accessKeyId);
            modifier.put(User.JSON_KEY_SECRET_ACCESS_KEY, secretAccessKey);
            BSONObject setModifier = new BasicBSONObject();
            setModifier.put(DBParamDefine.MODIFY_SET, modifier);

            cl.update(matcher, setModifier, null);
        } catch (Exception e) {
            logger.error("updateUserKeys failed. errorMessage = " + e.getMessage(), e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public User getUserByName(String userName) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.USER_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(User.JSON_KEY_USERNAME, userName);
            BSONObject queryResult = cl.queryOne(matcher, null, null, null, 0);
            if (null == queryResult) {
                return null;
            }

            return convertBsonToUser(queryResult);
        } catch (Exception e) {
            logger.error("getUserByName failed. errorMessage = " + e.getMessage(), e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public User getUserByAccessKeyID(String accessKeyID) throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.USER_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(User.JSON_KEY_ACCESS_KEY_ID, accessKeyID);
            BSONObject queryResult = cl.queryOne(matcher, null, null, null, 0);
            if (null == queryResult) {
                return null;
            }

            return convertBsonToUser(queryResult);
        } catch (BaseException e) {
            logger.error("getUserByAccessKeyID failed. errorMessage = " + e.getMessage(), e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public Owner getOwnerByUserID(long userId) throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.USER_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(User.JSON_KEY_USERID, userId);
            BSONObject queryResult = cl.queryOne(matcher, null, null, null, 0);
            if (null == queryResult) {
                return null;
            }

            return convertBsonToOwner(queryResult);
        } catch (BaseException e) {
            logger.error("getUserByAccessKeyID failed. errorMessage = " + e.getMessage(), e);
            throw e;
        } finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    private User convertBsonToUser(BSONObject bsonObject) {
        User user = new User();
        if (bsonObject.containsField(User.JSON_KEY_USERNAME)) {
            user.setUserName(bsonObject.get(User.JSON_KEY_USERNAME).toString());
        }
        if (bsonObject.containsField(User.JSON_KEY_USERID)) {
            user.setUserId((long) (bsonObject.get(User.JSON_KEY_USERID)));
        }
        if (bsonObject.containsField(User.JSON_KEY_ROLE)) {
            user.setRole(bsonObject.get(User.JSON_KEY_ROLE).toString());
        }
        if (bsonObject.containsField(User.JSON_KEY_ACCESS_KEY_ID)) {
            user.setAccessKeyID(bsonObject.get(User.JSON_KEY_ACCESS_KEY_ID).toString());
        }
        if (bsonObject.containsField(User.JSON_KEY_SECRET_ACCESS_KEY)) {
            user.setSecretAccessKey(bsonObject.get(User.JSON_KEY_SECRET_ACCESS_KEY).toString());
        }
        return user;
    }

    private Owner convertBsonToOwner(BSONObject bsonObject) {
        Owner owner = new Owner();
        if (bsonObject.containsField(User.JSON_KEY_USERNAME)) {
            owner.setUserName(bsonObject.get(User.JSON_KEY_USERNAME).toString());
        }
        if (bsonObject.containsField(User.JSON_KEY_USERID)) {
            owner.setUserId((long) (bsonObject.get(User.JSON_KEY_USERID)));
        }

        return owner;
    }
}
