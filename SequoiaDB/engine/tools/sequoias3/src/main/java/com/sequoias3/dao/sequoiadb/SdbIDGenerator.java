package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.common.DBParamDefine;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.IDGenerator;
import com.sequoias3.dao.DaoCollectionDefine;
import com.sequoias3.dao.IDGeneratorDao;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import java.util.ArrayList;
import java.util.List;

@Repository("IDGeneratorDao")
public class SdbIDGenerator implements IDGeneratorDao {

    @Autowired
    SdbDataSourceWrapper sdbDataSourceWrapper;

    @Autowired
    SequoiadbConfig config;

    @Override
    public Long getNewId(int type) throws S3ServerException {
        Sequoiadb sdb = null;
        try{
            sdb = sdbDataSourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection    cl = cs.getCollection(DaoCollectionDefine.ID_GENERATOR);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(IDGenerator.ID_TYPE, type);

            BSONObject update = new BasicBSONObject();
            update.put(IDGenerator.ID_ID, 1);
            BSONObject updateId = new BasicBSONObject();
            updateId.put(DBParamDefine.INCREASE, update);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");
            DBCursor cursor = cl.queryAndUpdate(matcher, null, null, hint,
                    updateId, 0, 1, 0, true);
            if (cursor.hasNext()){
                BSONObject record = cursor.getNext();
                return (long)record.get(IDGenerator.ID_ID);
            }else {
                throw new S3ServerException(S3Error.DAO_DB_ERROR, "get id failed.");
            }
        }catch (Exception e) {
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "get id failed.", e);
        }finally {
            sdbDataSourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public void insertId(int type) throws S3ServerException {
        Sequoiadb sdb = null;
        try{
            sdb = sdbDataSourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection    cl = cs.getCollection(DaoCollectionDefine.ID_GENERATOR);

            BSONObject insert = new BasicBSONObject();
            insert.put(IDGenerator.ID_TYPE, type);
            insert.put(IDGenerator.ID_ID, 0L);
            List<BSONObject> insertList = new ArrayList<>();
            insertList.add(insert);
            cl.insert(insertList, DBCollection.FLG_INSERT_CONTONDUP);
        }catch (Exception e) {
            throw new S3ServerException(S3Error.DAO_DB_ERROR, "init id failed.", e);
        }finally {
            sdbDataSourceWrapper.releaseSequoiadb(sdb);
        }
    }
}
