package com.sequoias3.model;

import com.sequoias3.common.DataShardingType;
import com.sequoias3.core.InnerRegion;

public class Region {
    private static final String REGION_NAME          = "Name";
    private static final String DATA_CS_SHARDINGTYPE = "DataCSShardingType";
    private static final String DATA_CL_SHARDINGTYPE = "DataCLShardingType";
    private static final String DATA_CS_RANGE        = "DataCSRange";
    private static final String DATA_DOMAIN          = "DataDomain";
    private static final String DATA_LOBPAGESIZE     = "DataLobPageSize";
    private static final String DATA_REPLSIZE        = "DataReplSize";
    private static final String META_DOMAIN          = "MetaDomain";
    private static final String DATA_LOCATION        = "DataLocation";
    private static final String META_LOCATION        = "MetaLocation";
    private static final String META_HIS_LOCATION    = "MetaHisLocation";

    private String name;
    private DataShardingType dataCSShardingType;
    private DataShardingType dataCLShardingType;
    private Integer dataCSRange;
    private String dataDomain;
    private Integer dataLobPageSize;
    private Integer dataReplSize;
    private String metaDomain;
    private String dataLocation;
    private String metaLocation;
    private String metaHisLocation;

    /**
     * Create a new Region
     */
    public Region() {
    }

    /**
     * Set region name.
     * @param name region name
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * Get region name
     * @return region name
     */
    public String getName() {
        return name;
    }

    /**
     * Set sharding type of data collection space.
     * @param dataCSShardingType sharding type
     */
    public void setDataCSShardingType(DataShardingType dataCSShardingType) {
        this.dataCSShardingType = dataCSShardingType;
    }

    /**
     * Get sharding type of data collection space.
     * @return sharding type
     */
    public DataShardingType getDataCSShardingType() {
        return dataCSShardingType;
    }

    /**
     * Set sharding type of data collection.
     * @param dataCLShardingType sharding type
     */
    public void setDataCLShardingType(DataShardingType dataCLShardingType) {
        this.dataCLShardingType = dataCLShardingType;
    }

    /**
     * Get sharding type of data collection.
     * @return sharding type
     */
    public DataShardingType getDataCLShardingType() {
        return dataCLShardingType;
    }

    /**
     * Set dataCSRange.
     * @param dataCSRange Max number of data collection spaces in one period.
     */
    public void setDataCSRange(Integer dataCSRange) {
        this.dataCSRange = dataCSRange;
    }

    /**
     * Get dataCSRange.
     * @return Max number of data collection spaces in one period.
     */
    public Integer getDataCSRange() {
        return dataCSRange;
    }

    /**
     * Set the domain of dataCS.
     * @param dataDomain domain name
     */
    public void setDataDomain(String dataDomain) {
        this.dataDomain = dataDomain;
    }

    /**
     * Get the domain of dataCS
     * @return domain name
     */
    public String getDataDomain() {
        return dataDomain;
    }

    /**
     * Set LobPageSize of Data Collection Space.
     * @param dataLobPageSize lobPageSize
     */
    public void setDataLobPageSize(Integer dataLobPageSize) {
        this.dataLobPageSize = dataLobPageSize;
    }

    /**
     * Get LobPageSize of Data Collection Space.
     * @return lobPageSize
     */
    public Integer getDataLobPageSize() {
        return dataLobPageSize;
    }

    /**
     * Set the ReplSize of Data Collection.
     * @param dataReplSize
     *   The ReplSize of Data Collection. Valid values: -1, 0, 1 - 7 .
     */
    public void setDataReplSize(Integer dataReplSize) {
        this.dataReplSize = dataReplSize;
    }

    /**
     * Get the ReplSize of Data Collection.
     * @return ReplSize of Data Collection.
     */
    public Integer getDataReplSize() {
        return dataReplSize;
    }

    /**
     * Set the domain of metaCS
     * @param metaDomain domain name
     */
    public void setMetaDomain(String metaDomain) {
        this.metaDomain = metaDomain;
    }

    /**
     * Get the domain of metaCS。
     * @return domain name
     */
    public String getMetaDomain() {
        return metaDomain;
    }

    /**
     * Set the full collection name to store objects data.
     * @param dataLocation
     *   The full collection name to store objects data, which contains
     *   collection space name and collection name, example: datacs.datacl
     */
    public void setDataLocation(String dataLocation) {
        this.dataLocation = dataLocation;
    }

    /**
     * Get the full collection name to store objects data.
     * @return  full collection name of data.
     */
    public String getDataLocation() {
        return dataLocation;
    }

    /**
     * Set the full collection name to store objects meta.
     * @param metaLocation
     *   The full collection name to store objects meta, which contains
     *   collection space name and collection name, example: datacs.datacl
     */
    public void setMetaLocation(String metaLocation) {
        this.metaLocation = metaLocation;
    }

    /**
     * Get the full collection name to store objects meta.
     * @return full collection name of meta
     */
    public String getMetaLocation() {
        return metaLocation;
    }

    /**
     * Set the full collection name to store objects history meta.
     * @param metaHisLocation
     *   The full collection name to store history meta of objects, which contains
     *   collection space name and collection name, example: metacs.hismetacl
     */
    public void setMetaHisLocation(String metaHisLocation) {
        this.metaHisLocation = metaHisLocation;
    }

    /**
     * Get the full collection name to store objects history meta.
     * @return full collection name of history meta.
     */
    public String getMetaHisLocation() {
        return metaHisLocation;
    }

    /**
     *
     * @return region configuration
     */
    public String toString() {
        StringBuffer buffer = new StringBuffer();
        buffer.append("[" + Region.REGION_NAME + "=" + this.name);
        buffer.append("," + Region.DATA_CS_SHARDINGTYPE + "=" + this.dataCSShardingType);
        buffer.append("," + Region.DATA_CL_SHARDINGTYPE + "=" + this.dataCLShardingType);
        buffer.append("," + Region.DATA_CS_RANGE + "=" + this.dataCSRange);
        buffer.append("," + Region.DATA_DOMAIN + "=" + this.dataDomain);
        buffer.append("," + Region.DATA_LOBPAGESIZE + "=" + this.dataLobPageSize);
        buffer.append("," + Region.DATA_REPLSIZE + "=" + this.dataReplSize);
        buffer.append("," + Region.META_DOMAIN + "=" + this.metaDomain);
        buffer.append("," + Region.DATA_LOCATION + "=" + this.dataLocation);
        buffer.append("," + Region.META_LOCATION + "=" + this.metaLocation);
        buffer.append("," + Region.META_HIS_LOCATION + "=" + this.metaHisLocation + "]");
        return buffer.toString();
    }

}
