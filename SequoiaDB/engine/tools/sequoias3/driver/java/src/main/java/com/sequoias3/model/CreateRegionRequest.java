package com.sequoias3.model;

import com.sequoias3.common.DataShardingType;

public class CreateRegionRequest {

    private Region region;

    /**
     * Create a region request.
     *
     * @param name
     *   Region name.
     */
    public CreateRegionRequest(String name) {
        region = new Region();
        region.setName(name);
    }

    /**
     * Set the sharding type of Data CS.
     *
     * @param dataCSShardingType
     *   The sharding type of Data CS.
     *   In a new period of sharding type, a new data collection space will
     *   be created to store objects data.
     *   @see com.sequoias3.common.DataShardingType
     * @return
     *   CreateRegionRequest
     */
    public CreateRegionRequest withDataCSShardingType(DataShardingType dataCSShardingType) {
        region.setDataCSShardingType(dataCSShardingType);
        return this;
    }

    /**
     * Set the sharding type of Data CL.
     *
     * @param dataCLShardingType
     *   The sharding type of Data CL.
     *   In a new period of sharding type, a new collection will be create
     *   to store objects data.
     *   @see com.sequoias3.common.DataShardingType
     * @return
     *   CreateRegionRequest
     */
    public CreateRegionRequest withDataCLShardingType(DataShardingType dataCLShardingType) {
        region.setDataCLShardingType(dataCLShardingType);
        return this;
    }

    /**
     * Set the Data CS range.
     *
     * @param dataCSRange
     *   The Data CS range. Max number of data collection spaces in one period
     *   of dataCSShardingType.
     * @return
     *   CreateRegionRequest
     */
    public CreateRegionRequest withDataCSRange(Integer dataCSRange) {
        region.setDataCSRange(dataCSRange);
        return this;
    }

    /**
     * Set the domain of data collection space.
     *
     * @param dataDomain
     *   The domain of data collection space.
     * @return
     *   CreateRegionRequest
     */
    public CreateRegionRequest withDataDomain(String dataDomain) {
        region.setDataDomain(dataDomain);
        return this;
    }

    /**
     * Set LobPageSize of Data Collection Space.
     *
     * @param dataLobPageSize
     *   The LobPageSize of Data Collection Space. Valid values: 0，4096，8192，
     *   16384，32768，65536，131072，262144，524288. Default value: 262144.
     *   0 equals the default value 262144
     * @return
     *   CreateRegionRequest
     */
    public CreateRegionRequest withDataLobPageSize(Integer dataLobPageSize) {
        region.setDataLobPageSize(dataLobPageSize);
        return this;
    }

    /**
     * Set the ReplSize of Data Collection.
     *
     * @param dataReplSize
     *   The ReplSize of Data Collection. Valid values: -1, 0, 1 - 7 .
     *   Default value: -1.
     * @return
     *   CreateRegionRequest
     */
    public CreateRegionRequest withDataReplSize(Integer dataReplSize) {
        region.setDataReplSize(dataReplSize);
        return this;
    }

    /**
     * Set the domain of meta collection space.
     *
     * @param metaDomain
     *   The domain of meta collection space.
     * @return
     *   CreateRegionRequest
     */
    public CreateRegionRequest withMetaDomain(String metaDomain) {
        region.setMetaDomain(metaDomain);
        return this;
    }

    /**
     * Set the full collection name to store objects data.
     *
     * @param dataLocation
     *   The full collection name to store objects data, which contains
     *   collection space name and collection name, example: datacs.datacl
     * @return
     *   CreateRegionRequest
     */
    public CreateRegionRequest withDataLocation(String dataLocation) {
        region.setDataLocation(dataLocation);
        return this;
    }

    /**
     * The full collection name to store meta of objects.
     *
     * @param metaLocation
     *   The full collection name to store meta of objects, which contains
     *   collection space name and collection name, example: metacs.metacl
     * @return
     *   CreateRegionRequest
     */
    public CreateRegionRequest withMetaLocation(String metaLocation) {
        region.setMetaLocation(metaLocation);
        return this;
    }

    /**
     * Set the full collection name to store history meta of objects.
     *
     * @param metaHisLocation
     *   The full collection name to store history meta of objects, which contains
     *   collection space name and collection name, example: metacs.hismetacl
     * @return
     *   CreateRegionRequest
     */
    public CreateRegionRequest withMetaHisLocation(String metaHisLocation) {
        region.setMetaHisLocation(metaHisLocation);
        return this;
    }

    /**
     * Return the region configuration
     *
     * @return
     *   Region
     */
    public Region getRegion() {
        return region;
    }
}
