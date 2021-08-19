package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.AclTable;
import com.sequoias3.dao.AclDao;
import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.DaoCollectionDefine;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.Grant;
import com.sequoias3.model.Grantee;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import java.util.ArrayList;
import java.util.List;

@Repository("AclDao")
public class SdbAclDao implements AclDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbAclDao.class);

    @Autowired
    SequoiadbConfig config;

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    @Autowired
    SdbDataSourceWrapper sdbDataSourceWrapper;

    @Override
    public void insertAcl(ConnectionDao connection, long aclId, Grant grant)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null){
                sdb = ((SdbConnectionDao)connection).getConnection();
            } else {
                sdb = sdbDataSourceWrapper.getSequoiadb();
            }

            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.ACL_LIST);

            BSONObject newGrant = new BasicBSONObject();
            newGrant.put(AclTable.ACL_ID, aclId);
            newGrant.put(AclTable.PERMISSION, grant.getPermission());
            newGrant.put(AclTable.GRANT_TYPE, grant.getGrantee().getType());
            newGrant.put(AclTable.GRANTEE_ID, grant.getGrantee().getId());
            newGrant.put(AclTable.GRANTEE_NAME, grant.getGrantee().getDisplayName());
            newGrant.put(AclTable.GRANTEE_URI, grant.getGrantee().getUri());
            newGrant.put(AclTable.EMAILADDRESS, grant.getGrantee().getEmailAddress());

            cl.insert(newGrant);
        } catch (Exception e) {
            logger.error("insert ACL failed. aclId=" + aclId + ", errorMessage = " + e.getMessage());
            throw e;
        } finally {
            if (connection == null) {
                sdbDataSourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public void deleteAcl(ConnectionDao connection, long aclId) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            if (connection != null){
                sdb = ((SdbConnectionDao)connection).getConnection();
            } else {
                sdb = sdbDataSourceWrapper.getSequoiadb();
            }
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.ACL_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(AclTable.ACL_ID, aclId);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.delete(matcher, hint);
        } catch (Exception e) {
            logger.error("delete ACL failed. aclId=" + aclId +", errorMessage = " + e.getMessage());
            throw e;
        } finally {
            if (connection == null) {
                sdbDataSourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public List<Grant> queryAcl(long aclId) throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor cursor = null;
        ArrayList<Grant> grantList = new ArrayList<>();
        try {
            sdb = sdbDataSourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.ACL_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(AclTable.ACL_ID, aclId);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cursor = cl.query(matcher, null, null, hint);

            while (cursor.hasNext()){
                grantList.add(converBsonToGrant(cursor.getNext()));
            }

            return grantList;
        } catch (Exception e) {
            logger.error("delete ACL failed. aclId=" + aclId +", errorMessage = " + e.getMessage());
            throw e;
        } finally {
            sdbBaseOperation.releaseDBCursor(cursor);
            sdbDataSourceWrapper.releaseSequoiadb(sdb);
        }
    }

    private Grant converBsonToGrant(BSONObject record){
        if (record == null){
            return null;
        }

        Grant grant = new Grant();
        grant.setPermission(record.get(AclTable.PERMISSION).toString());
        Grantee grantee = new Grantee();
        grantee.setXsiType(record.get(AclTable.GRANT_TYPE).toString());
        if (record.get(AclTable.GRANTEE_ID) != null){
            grantee.setId((long) record.get(AclTable.GRANTEE_ID));
        }
        if (record.get(AclTable.GRANTEE_NAME) != null){
            grantee.setDisplayName(record.get(AclTable.GRANTEE_NAME).toString());
        }
        if (record.get(AclTable.GRANTEE_URI) != null){
            grantee.setUri(record.get(AclTable.GRANTEE_URI).toString());
        }
        if (record.get(AclTable.EMAILADDRESS) != null){
            grantee.setEmailAddress(record.get(AclTable.EMAILADDRESS).toString());
        }
        grant.setGrantee(grantee);
        return grant;
    }
}
