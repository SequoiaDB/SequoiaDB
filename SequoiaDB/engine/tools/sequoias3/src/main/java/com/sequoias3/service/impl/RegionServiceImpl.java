package com.sequoias3.service.impl;

import com.sequoias3.common.DBParamDefine;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.common.RegionParamDefine;
import com.sequoias3.core.Bucket;
import com.sequoias3.core.Region;
import com.sequoias3.dao.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.ListRegionsResult;
import com.sequoias3.service.RegionService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class RegionServiceImpl implements RegionService {
    private static final Logger logger = LoggerFactory.getLogger(RegionServiceImpl.class);

    @Autowired
    RegionDao regionDao;

    @Autowired
    RegionSpaceDao regionSpaceDao;

    @Autowired
    BucketDao bucketDao;

    @Autowired
    DaoMgr daoMgr;

    @Autowired
    Transaction transaction;

    @Override
    public void putRegion(Region regionCon) throws S3ServerException {
        try {
            int tryTime = DBParamDefine.DB_DUPLICATE_MAX_TIME;
            //region check
            checkShardingType(regionCon);

            int configType = getConfigType(regionCon);

            while (tryTime > 0) {
                tryTime--;
                ConnectionDao connection = daoMgr.getConnectionDao();
                transaction.begin(connection);
                try {
                    Region oldRegion = regionDao.queryForUpdateRegion(connection, regionCon.getName());
                    if (oldRegion != null) {
                        logger.info("update region. regionName:" + regionCon.getName());
                        int oldConfigType = getConfigType(oldRegion);
                        if (configType != oldConfigType
                                && configType != RegionParamDefine.ConfigType.NoneType) {
                            throw new S3ServerException(S3Error.REGION_CONFLICT_TYPE,
                                    "config type must same as exist config.");
                        }

                        if (RegionParamDefine.ConfigType.DynamicType == configType) {
                            checkConflictDomain(regionCon, oldRegion);
                            regionDao.updateRegion(connection, regionCon);
                        } else if (RegionParamDefine.ConfigType.FixedType == configType) {
                            checkConflictLocation(regionCon, oldRegion);
                        }
                    } else {
                        logger.info("add region. regionName:" + regionCon.getName());
                        Region newRegion = new Region(regionCon);
                        if (RegionParamDefine.ConfigType.FixedType == configType) {
                            checkLocation(newRegion);
                            splitRegionLocation(newRegion);
                            regionDao.detectLocation(connection, newRegion.getDataCSLocation(), newRegion.getDataCLLocation(), RegionParamDefine.LocationType.Data);
                            regionDao.detectLocation(connection, newRegion.getMetaCSLocation(), newRegion.getMetaCLLocation(), RegionParamDefine.LocationType.Meta);
                            regionDao.detectLocation(connection, newRegion.getMetaHisCSLocation(), newRegion.getMetaHisCLLocation(), RegionParamDefine.LocationType.MetaHis);
                        } else {
                            checkPageSize(newRegion.getDataLobPageSize(), newRegion.getDataReplSize());
                            regionDao.detectDomain(connection, newRegion.getDataDomain());
                            regionDao.detectDomain(connection, newRegion.getMetaDomain());
                            if (null == newRegion.getDataCSShardingType()) {
                                newRegion.setDataCSShardingType(DataShardingType.YEAR.getName());
                            }
                            if (null == newRegion.getDataCLShardingType()) {
                                newRegion.setDataCLShardingType(DataShardingType.QUARTER.getName());
                            }
                            if (null == newRegion.getDataLobPageSize()) {
                                newRegion.setDataLobPageSize(262144);
                            }
                            if (null == newRegion.getDataReplSize()){
                                newRegion.setDataReplSize(-1);
                            }
                            if (null == newRegion.getDataCSRange()){
                                newRegion.setDataCSRange(1);
                            }
                        }

                        newRegion.setCreateTime(System.currentTimeMillis());
                        regionDao.insertRegion(connection, newRegion);
                        if (RegionParamDefine.ConfigType.FixedType != configType){
                            regionDao.createMetaCSCL(newRegion, regionDao.getMetaCurCSName(newRegion), regionDao.getMetaCurCLName(regionCon), false);
                            regionDao.createMetaCSCL(newRegion, regionDao.getMetaHisCSName(newRegion), regionDao.getMetaHisCLName(regionCon), true);
                        }
                        regionDao.createDirCSCL(newRegion, regionDao.getMetaCurCSName(newRegion));
                    }
                    transaction.commit(connection);
                    return;
                }catch (S3ServerException e) {
                    transaction.rollback(connection);
                    if (e.getError().getErrIndex() == S3Error.DAO_DUPLICATE_KEY.getErrIndex() && tryTime > 0) {
                        continue;
                    } else {
                        throw e;
                    }
                }catch (Exception e) {
                    transaction.rollback(connection);
                    throw e;
                } finally {
                    daoMgr.releaseConnectionDao(connection);
                }
            }
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.REGION_PUT_FAILED, "put failed. regionName:" + regionCon.getName(), e);
        }
    }

    @Override
    public GetRegionResult getRegion(String regionName) throws S3ServerException {
        try {
            Region region = regionDao.queryRegion(regionName);
            if (region == null) {
                throw new S3ServerException(S3Error.REGION_NO_SUCH_REGION,
                        "No such region. regionName:" + regionName);
            }

            List<Bucket> bucketList = bucketDao.getBucketListByRegion(null, regionName);

            GetRegionResult result = new GetRegionResult(region);
            result.setBuckets(bucketList);
            return result;
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.REGION_GET_FAILED, "get region failed. regionName;"+regionName, e);
        }
    }

    @Override
    public ListRegionsResult ListRegions() throws S3ServerException {
        //get region list
        ListRegionsResult result = new ListRegionsResult();
        try {
            List<String> regionList = regionDao.queryRegionList();
            if (regionList.size() > 0) {
                result.setRegions(regionList);
            }
            return result;
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.REGION_GET_LIST_FAILED, "list regions failed.", e);
        }
    }

    @Override
    public void deleteRegion(String regionName) throws S3ServerException {
        try {
            ConnectionDao connection = daoMgr.getConnectionDao();
            transaction.begin(connection);
            try {
                Region region = regionDao.queryForUpdateRegion(connection, regionName);
                if (null == region) {
                    throw new S3ServerException(S3Error.REGION_NO_SUCH_REGION,
                            "region is not found. regionName:" + regionName);
                }

                List<Bucket> bucketList = bucketDao.getBucketListByRegion(connection, regionName);
                if (bucketList.size() > 0) {
                    throw new S3ServerException(S3Error.REGION_NOT_EMPTY,
                            "region is not empty. regionName:" + regionName);
                }

                List<String> spaceList = regionSpaceDao.queryRegionCSList(connection, regionName);
                if (spaceList != null) {
                    for (int i = 0; i < spaceList.size(); i++) {
                        regionSpaceDao.dropRegionCollectionSpace(connection, spaceList.get(i));
                    }
                    regionSpaceDao.deleteRegionCSList(connection, regionName);
                }

                regionDao.deleteRegion(connection, regionName);
                transaction.commit(connection);
            } catch (Exception e) {
                transaction.rollback(connection);
                throw e;
            } finally {
                daoMgr.releaseConnectionDao(connection);
            }
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.REGION_DELETE_FAILED, "delete region failed. regionName:"+regionName, e);
        }
    }

    @Override
    public void headRegion(String regionName) throws S3ServerException {
        try {
            Region region = regionDao.queryRegion(regionName);
            if (region == null) {
                throw new S3ServerException(S3Error.REGION_NO_SUCH_REGION,
                        "No such region. regionName:" + regionName);
            }
        }catch (S3ServerException e){
            throw e;
        }catch (Exception e){
            throw new S3ServerException(S3Error.REGION_HEAD_FAILED, "head region failed. regionName:"+regionName, e);
        }
    }

    private void checkShardingType(Region config) throws S3ServerException{
        if (config.getDataCSShardingType() != null){
            if (null == DataShardingType.getShardingType(config.getDataCSShardingType())) {
                throw new S3ServerException(S3Error.REGION_INVALID_SHARDINGTYPE,
                        "ShardingType is invalid. shardingtype:"+config.getDataCSShardingType());
            }
        }

        if (config.getDataCLShardingType() != null){
            if (null == DataShardingType.getShardingType(config.getDataCLShardingType())) {
                throw new S3ServerException(S3Error.REGION_INVALID_SHARDINGTYPE,
                        "ShardingType is invalid. shardingtype:"+config.getDataCLShardingType());
            }
        }
    }

    private void checkConflictDomain(Region newRegion, Region oldRegion)
            throws S3ServerException{
        if (newRegion.getDataDomain() != null
                && !newRegion.getDataDomain().equals(oldRegion.getDataDomain())) {
            throw new S3ServerException(S3Error.REGION_CONFLICT_DOMAIN,
                    "can not modify DataDomain");
        }
        if (newRegion.getMetaDomain() != null
                && !newRegion.getMetaDomain().equals(oldRegion.getMetaDomain())) {
            throw new S3ServerException(S3Error.REGION_CONFLICT_DOMAIN,
                    "can not modify MetaDomain");
        }
        if (newRegion.getDataLobPageSize() != null
                && newRegion.getDataLobPageSize().intValue() != oldRegion.getDataLobPageSize().intValue()) {
            throw new S3ServerException(S3Error.REGION_CONFLICT_LOBPAGESIZE,
                    "can not modify lobpagesize，new lobpagesize:" + newRegion.getDataLobPageSize() + ", old lobpagesize:" + oldRegion.getDataLobPageSize());
        }
        if (newRegion.getDataReplSize() != null
                && newRegion.getDataReplSize().intValue() != oldRegion.getDataReplSize().intValue()) {
            throw new S3ServerException(S3Error.REGION_CONFLICT_REPLSIZE,
                    "can not modify replsize");
        }
    }

    private void checkConflictLocation(Region newRegion, Region oldRegion)
            throws S3ServerException{
        if (newRegion.getDataLocation() != null
                && !newRegion.getDataLocation().equals(oldRegion.getDataLocation())) {
            throw new S3ServerException(S3Error.REGION_CONFLICT_LOCATION,
                    "can not modify DataLocation");
        }
        if (newRegion.getMetaLocation() != null
                && !newRegion.getMetaLocation().equals(oldRegion.getMetaLocation())) {
            throw new S3ServerException(S3Error.REGION_CONFLICT_LOCATION,
                    "can not modify MetaLocation");
        }
        if (newRegion.getMetaHisLocation() != null
                && !newRegion.getMetaHisLocation().equals(oldRegion.getMetaHisLocation())) {
            throw new S3ServerException(S3Error.REGION_CONFLICT_LOCATION,
                    "can not modify MetaHisLocation");
        }
    }

    private void checkLocation(Region regionCon) throws S3ServerException{
        if (null == regionCon.getDataLocation()) {
            throw new S3ServerException(S3Error.REGION_LOCATION_NULL,
                    "DataLocation and MetaLocation and MetaHisLocation must specified at the same time.");
        }
        if (null == regionCon.getMetaLocation()) {
            throw new S3ServerException(S3Error.REGION_LOCATION_NULL,
                    "DataLocation and MetaLocation and MetaHisLocation must specified at the same time.");
        }
        if (null == regionCon.getMetaHisLocation()) {
            throw new S3ServerException(S3Error.REGION_LOCATION_NULL,
                    "DataLocation and MetaLocation and MetaHisLocation must specified at the same time.");
        }
        if (regionCon.getMetaLocation().equals(regionCon.getMetaHisLocation())){
            throw new S3ServerException(S3Error.REGION_LOCATION_SAME,
                    "MetaLocation must different with MetaHisLocation.");
        }
    }

    private void checkPageSize(Integer lobPageSize, Integer replSize) throws S3ServerException{
        if (lobPageSize != null){
            if (lobPageSize != 0
                    && lobPageSize != 4096
                    && lobPageSize != 8192
                    && lobPageSize != 16384
                    && lobPageSize != 32768
                    && lobPageSize != 65536
                    && lobPageSize != 131072
                    && lobPageSize != 262144
                    && lobPageSize != 524288){
                throw new S3ServerException(S3Error.REGION_INVALID_LOBPAGESIZE,
                        "lobpagesize must be one of 0，4096，8192，16384，32768，65536，131072，262144，524288.");
            }
        }

        if (replSize != null){
            if (replSize < -1
                    || replSize > 7){
                throw new S3ServerException(S3Error.REGION_INVALID_REPLSIZE,
                        "replsize must be one of -1,0,1-7.");
            }
        }
    }

    private int getConfigType(Region config) throws S3ServerException{
        Boolean isDynamicCon = false;
        Boolean isFixedCon   = false;

        if (config.getDataDomain() != null
                || config.getMetaDomain() != null
                || config.getDataCSShardingType() != null
                || config.getDataCLShardingType() != null
                || config.getDataLobPageSize() != null
                || config.getDataReplSize() != null){
            isDynamicCon = true;
        }

        if (config.getDataLocation() != null
                || config.getMetaLocation() != null
                || config.getMetaHisLocation() != null){
            isFixedCon = true;
        }

        if (isDynamicCon == true && isFixedCon == true){
            throw new S3ServerException(S3Error.REGION_CONFLICT_TYPE,
                    "cannot config dynamic and fixed at the same time");
        }else if (isDynamicCon == true){
            return RegionParamDefine.ConfigType.DynamicType;
        }else if (isFixedCon == true){
            return RegionParamDefine.ConfigType.FixedType;
        }else {
            return RegionParamDefine.ConfigType.NoneType;
        }
    }

    private void splitRegionLocation(Region regionCon) throws S3ServerException{
        if (regionCon.getDataLocation() != null){
            String location = regionCon.getDataLocation();
            String[] locationNames = location.split("\\.");
            if (locationNames.length != 2){
                throw new S3ServerException(S3Error.REGION_LOCATION_SPLIT,
                        "DataLocation is invalid. DataLocation:"+location);
            }

            regionCon.setDataCSLocation(locationNames[0]);
            regionCon.setDataCLLocation(locationNames[1]);
        }

        if (regionCon.getMetaLocation() != null){
            String location = regionCon.getMetaLocation();
            String[] locationNames = location.split("\\.");
            if (locationNames.length != 2){
                throw new S3ServerException(S3Error.REGION_LOCATION_SPLIT,
                        "MetaLocation is invalid. DataLocation:"+location);
            }

            regionCon.setMetaCSLocation(locationNames[0]);
            regionCon.setMetaCLLocation(locationNames[1]);
        }

        if (regionCon.getMetaHisLocation() != null){
            String location = regionCon.getMetaHisLocation();
            String[] locationNames = location.split("\\.");
            if (locationNames.length != 2){
                throw new S3ServerException(S3Error.REGION_LOCATION_SPLIT,
                        "MetaHisLocation is invalid. DataLocation:"+location);
            }

            regionCon.setMetaHisCSLocation(locationNames[0]);
            regionCon.setMetaHisCLLocation(locationNames[1]);
        }
    }
}
