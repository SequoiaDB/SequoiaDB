package com.sequoias3.dao;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.core.Region;
import com.sequoias3.exception.S3ServerException;

import java.util.Date;
import java.util.List;

public interface RegionDao {
    void insertRegion(ConnectionDao connection, Region regionCon) throws S3ServerException;
    void updateRegion(ConnectionDao connection, Region regionCon) throws S3ServerException;
    void deleteRegion(ConnectionDao connection, String regionName) throws S3ServerException;
    Region queryForUpdateRegion(ConnectionDao connection, String regionName) throws S3ServerException;

    Region queryRegion(String regionName) throws S3ServerException;
    List<String> queryRegionList() throws S3ServerException;

    void detectDomain(ConnectionDao connection, String domain) throws S3ServerException;
    void detectLocation(ConnectionDao connection, String CSName, String CLName, int locationType)
            throws S3ServerException;

    String getMetaCurCSName(Region region);

    String getMetaCurCLName(Region region);

    String getMetaHisCSName(Region region);

    String getMetaHisCLName(Region region);

    String getDataCSName(Region region, Date date);

    String getDataClName(Region region, Date date);

    void createMetaCSCL(Region region, String csMetaName,
                        String clMetaName, Boolean isHistory)
            throws S3ServerException;

    void createDirCSCL(Region region, String metaCsName) throws S3ServerException;
}
