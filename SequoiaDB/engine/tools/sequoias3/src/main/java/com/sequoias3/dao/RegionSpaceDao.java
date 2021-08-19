package com.sequoias3.dao;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.exception.S3ServerException;

import java.util.List;

public interface RegionSpaceDao {
    List<String> queryRegionCSList(ConnectionDao connection, String regionName) throws S3ServerException;
    void deleteRegionCSList(ConnectionDao connection, String regionName) throws S3ServerException;
    void dropRegionCollectionSpace(ConnectionDao connection, String CSName) throws S3ServerException;

}
