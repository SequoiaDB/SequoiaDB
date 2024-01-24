package com.sequoias3.dao.sequoiadb;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoias3.common.DBParamDefine;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.common.RegionParamDefine;
import com.sequoias3.config.SequoiadbConfig;
import com.sequoias3.core.Dir;
import com.sequoias3.core.ObjectMeta;
import com.sequoias3.core.Region;
import com.sequoias3.core.RegionSpace;
import com.sequoias3.dao.ConnectionDao;
import com.sequoias3.dao.DaoCollectionDefine;
import com.sequoias3.dao.RegionDao;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.utils.ShardingTypeUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

@Repository("RegionDao")
public class SequoiadbRegionDao implements RegionDao {
    private static final Logger logger = LoggerFactory.getLogger(SequoiadbRegionDao.class);

    @Autowired
    SdbDataSourceWrapper sdbDatasourceWrapper;

    @Autowired
    SequoiadbConfig config;

    @Autowired
    SdbBaseOperation sdbBaseOperation;

    @Autowired
    SequoiadbRegionSpaceDao sequoiadbRegionSpaceDao;

    @Override
    public void insertRegion(ConnectionDao connection, Region regionCon) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_LIST_COLLECTION);

            cl.insert(regionCon.toBson());
        }catch (BaseException e){
            if (e.getErrorType() == SDBError.SDB_IXM_DUP_KEY.name()) {
                logger.info("Duplicate key. regionName:{}",regionCon.getName());
                throw new S3ServerException(S3Error.DAO_DUPLICATE_KEY, "Duplicate key.");
            } else {
                throw e;
            }
        }catch (Exception e) {
            logger.error("insertRegion failed. errorMessage = " + e.getMessage());
            throw e;
        }
    }

    @Override
    public void updateRegion(ConnectionDao connection, Region regionCon) throws S3ServerException {
        try{
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(RegionSpace.REGION_SPACE_NAME, regionCon.getName());

            BSONObject updateData = new BasicBSONObject();
            if (regionCon.getDataCSShardingType() != null) {
                updateData.put(Region.DATA_CS_SHARDINGTYPE, regionCon.getDataCSShardingType());
            }
            if (regionCon.getDataCLShardingType() != null){
                updateData.put(Region.DATA_CL_SHARDINGTYPE, regionCon.getDataCLShardingType());
            }
            if (regionCon.getDataCSRange() != null){
                updateData.put(Region.DATA_CS_RANGE, regionCon.getDataCSRange());
            }
            BSONObject setUpdate = new BasicBSONObject();
            setUpdate.put(DBParamDefine.MODIFY_SET, updateData);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.update(matcher, setUpdate, hint);
        }catch (Exception e){
            logger.error("update region config failed. error:"+e.getMessage());
            throw e;
        }
    }

    @Override
    public Region queryRegion(String regionName) throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Region.REGION_NAME, regionName);

            BSONObject queryResult = cl.queryOne(matcher, null, null, null, 0);
            if (null == queryResult){
                return null;
            }
            return convertBsonToRegion(queryResult);
        }catch (Exception e) {
            logger.error("queryRegion failed. errorMessage = " + e.getMessage());
            throw e;
        }finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public List<String> queryRegionList() throws S3ServerException {
        Sequoiadb sdb = null;
        DBCursor cursor = null;
        try{
            sdb = sdbDatasourceWrapper.getSequoiadb();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_LIST_COLLECTION);

            BSONObject selector = new BasicBSONObject();
            selector.put(Region.REGION_NAME, 1);

            BSONObject orderBy = new BasicBSONObject();
            orderBy.put(Region.REGION_NAME, 1);

            cursor = cl.query(null, selector, orderBy, null);
            ArrayList<String> regionList = new ArrayList<>();
            while (cursor.hasNext()){
                BSONObject record = cursor.getNext();
                regionList.add(record.get(Region.REGION_NAME).toString());
            }
            return regionList;
        }catch (Exception e) {
            logger.error("queryRegionList failed. errorMessage = " + e.getMessage());
            throw e;
        }finally {
            sdbBaseOperation.releaseDBCursor(cursor);
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    @Override
    public void detectDomain(ConnectionDao connection, String domain) throws S3ServerException {
        try{
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();

            if (domain != null){
                if (!sdb.isDomainExist(domain)){
                    throw new S3ServerException(S3Error.REGION_INVALID_DOMAIN,
                            "Domain does not exist. domain:"+domain);
                }
            }
        }catch (BaseException e){
            throw e;
        }
    }

    @Override
    public void detectLocation(ConnectionDao connection, String CSName, String CLName, int locationType) throws S3ServerException{
        try{
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = null;
            if(!sdb.isCollectionSpaceExist(CSName)){
                throw new S3ServerException(S3Error.REGION_LOCATION_EXIST,
                        "DataLocation CollectionSpace does not exist. csName:"+CSName);
            }else {
                cs = sdb.getCollectionSpace(CSName);
                if(!cs.isCollectionExist(CLName)){
                    throw new S3ServerException(S3Error.REGION_LOCATION_EXIST,
                            "DataLocation Collection does not exist. csName:"+CSName+", clName:"+CLName);
                }
            }

            DBCollection cl = cs.getCollection(CLName);
            if (locationType == RegionParamDefine.LocationType.Meta){
                if ( !cl.isIndexExist(ObjectMeta.INDEX_CUR_KEY)){
                    BSONObject indexKey = new BasicBSONObject();
                    indexKey.put(ObjectMeta.META_BUCKET_ID, 1);
                    indexKey.put(ObjectMeta.META_KEY_NAME, 1);
                    sdbBaseOperation.createIndex(sdb, CSName, CLName,
                            ObjectMeta.INDEX_CUR_KEY, indexKey, true, true);
                }

                if (!cl.isIndexExist(ObjectMeta.INDEX_CUR_PARENTID1)){
                    BSONObject indexKeyParent1 = new BasicBSONObject();
                    indexKeyParent1.put(ObjectMeta.META_BUCKET_ID, 1);
                    indexKeyParent1.put(ObjectMeta.META_PARENTID1,1);
                    indexKeyParent1.put(ObjectMeta.META_KEY_NAME, 1);

                    sdbBaseOperation.createIndex(sdb, CSName, CLName,
                            ObjectMeta.INDEX_CUR_PARENTID1, indexKeyParent1, true, true);
                }

                if (!cl.isIndexExist(ObjectMeta.INDEX_CUR_PARENTID2)){
                    BSONObject indexKeyParent2 = new BasicBSONObject();
                    indexKeyParent2.put(ObjectMeta.META_BUCKET_ID, 1);
                    indexKeyParent2.put(ObjectMeta.META_PARENTID2,1);
                    indexKeyParent2.put(ObjectMeta.META_KEY_NAME, 1);

                    sdbBaseOperation.createIndex(sdb, CSName, CLName,
                            ObjectMeta.INDEX_CUR_PARENTID2, indexKeyParent2, true, true);
                }
            }

            if (locationType == RegionParamDefine.LocationType.MetaHis){
                if (!cl.isIndexExist(ObjectMeta.INDEX_HIS_KEY)){
                    BSONObject indexKey = new BasicBSONObject();
                    indexKey.put(ObjectMeta.META_BUCKET_ID, 1);
                    indexKey.put(ObjectMeta.META_KEY_NAME, 1);
                    indexKey.put(ObjectMeta.META_VERSION_ID, 1);

                    sdbBaseOperation.createIndex(sdb, CSName, CLName,
                            ObjectMeta.INDEX_HIS_KEY, indexKey, true, true);
                }
            }
        }catch (BaseException e){
            throw e;
        }
    }

    @Override
    public void deleteRegion(ConnectionDao connection, String regionName)
            throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao)connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Region.REGION_NAME, regionName);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            cl.delete(matcher, hint);
        }catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                return;
            } else {
                logger.error("deleteRegion failed. "+ ", regionName:" + regionName + ", error:"+e.getMessage());
                throw e;
            }
        }catch (Exception e){
            throw e;
        }
    }

    @Override
    public Region queryForUpdateRegion(ConnectionDao connection, String regionName)
            throws S3ServerException {
        try {
            Sequoiadb sdb = ((SdbConnectionDao) connection).getConnection();
            CollectionSpace cs = sdb.getCollectionSpace(config.getMetaCsName());
            DBCollection cl = cs.getCollection(DaoCollectionDefine.REGION_LIST_COLLECTION);

            BSONObject matcher = new BasicBSONObject();
            matcher.put(Region.REGION_NAME, regionName);

            BSONObject hint = new BasicBSONObject();
            hint.put("", "");

            BSONObject queryResult = cl.queryOne(matcher, null, null, hint, DBQuery.FLG_QUERY_FOR_UPDATE);
            if (null == queryResult){
                return null;
            }
            return convertBsonToRegion(queryResult);
        }catch (BaseException e) {
            if (e.getErrorCode() == SDBError.SDB_DMS_NOTEXIST.getErrorCode()) {
                //no cl ,return null
                return null;
            } else {
                logger.error("queryForUpdateRegion failed. "+ ", regionName:" + regionName + ", error:"+e.getMessage());
                throw e;
            }
        } catch (Exception e) {
            logger.error("queryForUpdateRegion failed. "+ ", regionName:" + regionName + ", error = " + e.getMessage());
            throw e;
        }
    }

    @Override
    public String getMetaCurCSName( Region region){
        StringBuilder csName = new StringBuilder();

        if (null != region){
            if (region.getMetaLocation() != null){
                csName.append(region.getMetaCSLocation());
            }else {
                csName.append(DBParamDefine.CS_S3);
                csName.append(region.getName());
                csName.append(DBParamDefine.CS_META);
            }
        }else {
            csName.append(config.getMetaCsName());
        }

        return csName.toString();
    }

    @Override
    public String getMetaCurCLName(Region region){
        if (region != null && region.getMetaLocation() != null){
            return region.getMetaCLLocation();
        }else {
            return DaoCollectionDefine.OBJECT_META_LIST;
        }
    }

    @Override
    public String getMetaHisCSName( Region region){
        StringBuilder csName = new StringBuilder();

        if (null != region){
            if (region.getMetaHisLocation() != null){
                csName.append(region.getMetaHisCSLocation());
            }else {
                csName.append(DBParamDefine.CS_S3);
                csName.append(region.getName());
                csName.append(DBParamDefine.CS_META);
            }
        }else {
            csName.append(config.getMetaCsName());
        }

        return csName.toString();
    }

    @Override
    public String getMetaHisCLName(Region region){
        if (region != null && region.getMetaHisLocation() != null){
            return region.getMetaHisCLLocation();
        }else {
            return DaoCollectionDefine.OBJECT_META_LIST_HISTORY;
        }
    }

    @Override
    public String getDataCSName(Region region, Date date){
        StringBuilder csName = new StringBuilder();

        if (null != region){
            if (region.getDataLocation() != null){
                csName.append(region.getDataCSLocation());
            }else {
                csName.append(DBParamDefine.CS_S3);
                csName.append(region.getName());
                csName.append(DBParamDefine.CS_DATA);
                DataShardingType type = DataShardingType.getShardingType(region.getDataCSShardingType());
                if (type != null){
                    csName.append("_");
                    csName.append(ShardingTypeUtils.getShardingTypeStr(type, date));
                }
                csName.append("_");
                csName.append((int)(Math.random() * region.getDataCSRange() + 1));
            }
        }else {
            csName.append(config.getDataCsName());
            csName.append("_");
            csName.append(ShardingTypeUtils.getShardingTypeStr(DataShardingType.YEAR, date));
            csName.append("_");
            csName.append((int)(Math.random() * config.getDataCSRange() + 1));
        }

        return csName.toString();
    }

    @Override
    public String getDataClName(Region region, Date date){
        StringBuilder clName = new StringBuilder();

        if (null != region){
            if (region.getDataLocation() != null){
                clName.append(region.getDataCLLocation());
            }else {
                clName.append(DaoCollectionDefine.OBJECT_DATA_LIST);
                DataShardingType type = DataShardingType.getShardingType(region.getDataCLShardingType());
                if (type != null){
                    clName.append("_");
                    clName.append(ShardingTypeUtils.getShardingTypeStr(type, date));
                }
            }
        }else {
            clName.append(DaoCollectionDefine.OBJECT_DATA_LIST);
            clName.append("_");
            clName.append(ShardingTypeUtils.getShardingTypeStr(DataShardingType.QUARTER, date));
        }
        return clName.toString();
    }

    @Override
    public void createMetaCSCL(Region region, String csMetaName,
                                String clMetaName, Boolean isHistory)
            throws S3ServerException{
        Sequoiadb sdb = null;
        try {
            if (region != null && region.getMetaLocation() != null) {
                throw new S3ServerException(S3Error.REGION_LOCATION_NOT_EXIST,
                        "location not exist. csName=" + csMetaName + ", clName=" + clMetaName);
            } else {
                sdb = sdbDatasourceWrapper.getSequoiadb();
                if (!sdb.isCollectionSpaceExist(csMetaName)) {
                    BSONObject option = null;
                    if (region != null && region.getMetaDomain() != null) {
                        option = new BasicBSONObject();
                        option.put("Domain", region.getMetaDomain());
                    }

                    if (DBParamDefine.CREATE_OK == sdbBaseOperation.createCS(sdb, csMetaName, option)) {
                        sequoiadbRegionSpaceDao.insertRegionCSList(csMetaName, region.getName());
                    }
                }

                BSONObject option = generateMetaCLOption();
                sdbBaseOperation.createCL(sdb, csMetaName, clMetaName, option);

                BSONObject indexKey = new BasicBSONObject();
                String indexName = ObjectMeta.INDEX_CUR_KEY;
                indexKey.put(ObjectMeta.META_BUCKET_ID, 1);
                indexKey.put(ObjectMeta.META_KEY_NAME, 1);
                if (isHistory) {
                    indexKey.put(ObjectMeta.META_VERSION_ID, 1);
                    indexName = ObjectMeta.INDEX_HIS_KEY;
                }

                sdbBaseOperation.createIndex(sdb, csMetaName, clMetaName,
                            indexName, indexKey, true, true);

                if (!isHistory){
                    BSONObject indexKeyParent1 = new BasicBSONObject();
                    indexKeyParent1.put(ObjectMeta.META_BUCKET_ID, 1);
                    indexKeyParent1.put(ObjectMeta.META_PARENTID1,1);
                    indexKeyParent1.put(ObjectMeta.META_KEY_NAME, 1);

                    sdbBaseOperation.createIndex(sdb, csMetaName, clMetaName,
                            ObjectMeta.INDEX_CUR_PARENTID1, indexKeyParent1, true, true);

                    BSONObject indexKeyParent2 = new BasicBSONObject();
                    indexKeyParent2.put(ObjectMeta.META_BUCKET_ID, 1);
                    indexKeyParent2.put(ObjectMeta.META_PARENTID2,1);
                    indexKeyParent2.put(ObjectMeta.META_KEY_NAME, 1);

                    sdbBaseOperation.createIndex(sdb, csMetaName, clMetaName,
                            ObjectMeta.INDEX_CUR_PARENTID2, indexKeyParent2, true, true);
                }
            }
        }finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    private BSONObject generateMetaCLOption(){
        BSONObject clOption = new BasicBSONObject();

        BSONObject shardingKey = new BasicBSONObject(ObjectMeta.META_KEY_NAME, 1);
        clOption.put("ShardingKey", shardingKey);
        clOption.put("ShardingType", "hash");
        clOption.put("ReplSize", -1);
        clOption.put("AutoSplit", true);

        return clOption;
    }

    @Override
    public void createDirCSCL(Region region, String metaCsName)
            throws S3ServerException {
        Sequoiadb sdb = null;
        try {
            sdb = sdbDatasourceWrapper.getSequoiadb();
            if (!sdb.isCollectionSpaceExist(metaCsName)) {
                if (region != null && region.getMetaLocation() != null) {
                    throw new S3ServerException(S3Error.REGION_LOCATION_NOT_EXIST,
                            "location not exist. csName=" + metaCsName);
                } else {
                    BSONObject option = null;
                    if (region != null && region.getMetaDomain() != null) {
                        option = new BasicBSONObject();
                        option.put("Domain", region.getMetaDomain());
                    }
                    if (DBParamDefine.CREATE_OK == sdbBaseOperation.createCS(sdb, metaCsName, option)) {
                        sequoiadbRegionSpaceDao.insertRegionCSList(metaCsName, region.getName());
                    }
                }
            }

            BSONObject clOption = new BasicBSONObject();
            BSONObject shardingKey = new BasicBSONObject(Dir.DIR_NAME, 1);
            clOption.put("ShardingKey", shardingKey);
            clOption.put("ShardingType", "hash");
            clOption.put("ReplSize", -1);
            clOption.put("AutoSplit", true);

            sdbBaseOperation.createCL(sdb, metaCsName, DaoCollectionDefine.OBJECT_DIR, null);

            BSONObject indexKey = new BasicBSONObject();
            String indexName = Dir.DIR_INDEX;
            indexKey.put(Dir.DIR_BUCKETID, 1);
            indexKey.put(Dir.DIR_DELIMITER, 1);
            indexKey.put(Dir.DIR_NAME, 1);
            sdbBaseOperation.createIndex(sdb, metaCsName, DaoCollectionDefine.OBJECT_DIR,
                    indexName, indexKey, true, true);

        }finally {
            sdbDatasourceWrapper.releaseSequoiadb(sdb);
        }
    }

    private Region convertBsonToRegion(BSONObject result){
        if (null == result){
            return null;
        }

        Region region = new Region();
        region.setName(result.get(Region.REGION_NAME).toString());
        region.setCreateTime((long)result.get(Region.REGION_CREATERTIME));
        if (result.get(Region.DATA_CS_SHARDINGTYPE) != null){
            region.setDataCSShardingType(result.get(Region.DATA_CS_SHARDINGTYPE).toString());
        }
        if (result.get(Region.DATA_CL_SHARDINGTYPE) != null){
            region.setDataCLShardingType(result.get(Region.DATA_CL_SHARDINGTYPE).toString());
        }
        if (result.get(Region.DATA_CS_RANGE) != null){
            region.setDataCSRange((int)result.get(Region.DATA_CS_RANGE));
        }
        if (result.get(Region.META_DOMAIN) != null){
            region.setMetaDomain(result.get(Region.META_DOMAIN).toString());
        }
        if (result.get(Region.DATA_DOMAIN) != null){
            region.setDataDomain(result.get(Region.DATA_DOMAIN).toString());
        }
        if (result.get(Region.DATA_LOBPAGESIZE) != null){
            region.setDataLobPageSize((Integer) result.get(Region.DATA_LOBPAGESIZE));
        }
        if (result.get(Region.DATA_REPLSIZE) != null){
            region.setDataReplSize((Integer) result.get(Region.DATA_REPLSIZE));
        }
        if (result.get(Region.DATA_LOCATION) != null){
            region.setDataLocation(result.get(Region.DATA_LOCATION).toString());
        }
        if (result.get(Region.META_LOCATION) != null){
            region.setMetaLocation(result.get(Region.META_LOCATION).toString());
        }
        if (result.get(Region.META_HIS_LOCATION) != null){
            region.setMetaHisLocation(result.get(Region.META_HIS_LOCATION).toString());
        }
        if (result.get(Region.DATA_CS_LOCATION) != null){
            region.setDataCSLocation(result.get(Region.DATA_CS_LOCATION).toString());
        }
        if (result.get(Region.DATA_CL_LOCATION) != null){
            region.setDataCLLocation(result.get(Region.DATA_CL_LOCATION).toString());
        }
        if (result.get(Region.META_CS_LOCATION) != null){
            region.setMetaCSLocation(result.get(Region.META_CS_LOCATION).toString());
        }
        if (result.get(Region.META_CL_LOCATION) != null){
            region.setMetaCLLocation(result.get(Region.META_CL_LOCATION).toString());
        }
        if (result.get(Region.META_HIS_CS_LOCATION) != null){
            region.setMetaHisCSLocation(result.get(Region.META_HIS_CS_LOCATION).toString());
        }
        if (result.get(Region.META_HIS_CL_LOCATION) != null){
            region.setMetaHisCLLocation(result.get(Region.META_HIS_CL_LOCATION).toString());
        }

        return region;
    }
}
