package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.RegionSpace;
import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.DaoCollectionDefine;
import com.sequoias3.dao.RegionSpaceDao;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import java.util.ArrayList;
import java.util.List;

@Repository("RegionSpaceDao")
public class SequoiadbRegionSpaceDao implements RegionSpaceDao {
    private static final Logger logger = LoggerFactory.getLogger(SequoiadbRegionSpaceDao.class);

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Autowired
    SequoiadbConfig config;

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    public void insertRegionCSList(String spaceName, String regionName) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_SPACE_LIST);

            BSONObject spaceData = new BasicBSONObject();
            spaceData.put(RegionSpace.REGION_SPACE_NAME, spaceName);
            spaceData.put(RegionSpace.REGION_SPACE_REGIONNAME, regionName);
            spaceData.put(RegionSpace.REGION_SPACE_CREATETIME, System.currentTimeMillis());

            cl.insert(spaceData);
        } catch (Exception e){
            logger.error("insert into RegionSpaceList failed. space name:"+spaceName+", regionName:"+regionName, e);
        }finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public List<String> queryRegionCSList(ConnectionDao connection, String regionName) throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor cursor = null;
        try {
            if (connection != null){
                sdb = ((SdbConnectionDao) connection).getConnection();
            }else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
            }
            ArrayList<String> regionSpaceList = new ArrayList<>();

            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_SPACE_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(RegionSpace.REGION_SPACE_REGIONNAME, regionName);

            BSONObject selector = new BasicBSONObject();
            selector.put(RegionSpace.REGION_SPACE_NAME, 1);

            BSONObject orderBy = new BasicBSONObject();
            orderBy.put(RegionSpace.REGION_SPACE_NAME, 1);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cursor = cl.query(matcher, selector, orderBy, hint);
            while (cursor.hasNext()){
                BSONObject record = cursor.getNext();
                regionSpaceList.add(record.get(RegionSpace.REGION_SPACE_NAME).toString());
            }

            return regionSpaceList;
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode() ||
                    e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cs or cl ,return null
                return null;
            } else {
                logger.error("query queryRegionCSList by region failed. error:",e.getErrorCode());
                throw e;
            }
        }
        catch (Exception e){
            logger.error("queryRegionCSList failed. error message:"+ e.getMessage());
            throw e;
        }finally {
            sdbBaseOperation.releaseDBCursor(cursor);
            if (null == connection) {
                sdbDatasourceWrapper.releaseSequoiadb(sdb);
            }
        }
    }

    @Override
    public void deleteRegionCSList(ConnectionDao connection, String regionName)
            throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao) connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_SPACE_LIST);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(RegionSpace.REGION_SPACE_REGIONNAME, regionName);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.delete(matcher, hint);
        }catch (Exception e){
            logger.error("deleteRegionCSList failed. error message:"+ e.getMessage());
            throw e;
        }
    }

    @Override
    public void dropRegionCollectionSpace(ConnectionDao connection, String CSName) throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao) connection).getConnection();
            sdbBaseOperation.dropCS(sdb, CSName);
        }catch (Exception e){
            logger.error("dropCS failed. error message:"+ e.getMessage());
            throw e;
        }
    }
}
