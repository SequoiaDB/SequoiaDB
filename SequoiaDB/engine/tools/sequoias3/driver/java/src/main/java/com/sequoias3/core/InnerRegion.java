package com.sequoias3.core;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.model.Region;

@JacksonXmlRootElement(localName = "RegionConfiguration")
public class InnerRegion extends Object{
    public static final String REGION_NAME          = "Name";
    public static final String DATA_CS_SHARDINGTYPE = "DataCSShardingType";
    public static final String DATA_CL_SHARDINGTYPE = "DataCLShardingType";
    public static final String DATA_CS_RANGE        = "DataCSRange";
    public static final String DATA_DOMAIN          = "DataDomain";
    public static final String DATA_LOBPAGESIZE     = "DataLobPageSize";
    public static final String DATA_REPLSIZE        = "DataReplSize";
    public static final String META_DOMAIN          = "MetaDomain";
    public static final String DATA_LOCATION        = "DataLocation";
    public static final String META_LOCATION        = "MetaLocation";
    public static final String META_HIS_LOCATION    = "MetaHisLocation";

    @JsonProperty(REGION_NAME)
    private String name;

    @JsonProperty(DATA_CS_SHARDINGTYPE)
    private String dataCSShardingType;

    @JsonProperty(DATA_CL_SHARDINGTYPE)
    private String dataCLShardingType;

    @JsonProperty(DATA_CS_RANGE)
    private Integer dataCSRange;

    @JsonProperty(DATA_DOMAIN)
    private String dataDomain;

    @JsonProperty(DATA_LOBPAGESIZE)
    private Integer dataLobPageSize;

    @JsonProperty(DATA_REPLSIZE)
    private Integer dataReplSize;

    @JsonProperty(META_DOMAIN)
    private String metaDomain;

    @JsonProperty(DATA_LOCATION)
    private String dataLocation;

    @JsonProperty(META_LOCATION)
    private String metaLocation;

    @JsonProperty(META_HIS_LOCATION)
    private String metaHisLocation;

    public InnerRegion() {
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public void setDataCSShardingType(String dataCSShardingType) {
        this.dataCSShardingType = dataCSShardingType;
    }

    public String getDataCSShardingType() {
        return dataCSShardingType;
    }

    public void setDataCLShardingType(String dataCLShardingType) {
        this.dataCLShardingType = dataCLShardingType;
    }

    public String getDataCLShardingType() {
        return dataCLShardingType;
    }

    public void setDataCSRange(Integer dataCSRange) {
        this.dataCSRange = dataCSRange;
    }

    public Integer getDataCSRange() {
        return dataCSRange;
    }

    public void setDataDomain(String dataDomain) {
        this.dataDomain = dataDomain;
    }

    public String getDataDomain() {
        return dataDomain;
    }

    public void setDataLobPageSize(Integer dataLobPageSize) {
        this.dataLobPageSize = dataLobPageSize;
    }

    public Integer getDataLobPageSize() {
        return dataLobPageSize;
    }

    public void setDataReplSize(Integer dataReplSize) {
        this.dataReplSize = dataReplSize;
    }

    public Integer getDataReplSize() {
        return dataReplSize;
    }

    public void setMetaDomain(String metaDomain) {
        this.metaDomain = metaDomain;
    }

    public String getMetaDomain() {
        return metaDomain;
    }

    public void setDataLocation(String dataLocation) {
        this.dataLocation = dataLocation;
    }

    public String getDataLocation() {
        return dataLocation;
    }

    public void setMetaLocation(String metaLocation) {
        this.metaLocation = metaLocation;
    }

    public String getMetaLocation() {
        return metaLocation;
    }

    public void setMetaHisLocation(String metaHisLocation) {
        this.metaHisLocation = metaHisLocation;
    }

    public String getMetaHisLocation() {
        return metaHisLocation;
    }

    public InnerRegion(Region request) {
        this.name = request.getName();
        if (request.getDataCSShardingType() != null) {
            this.dataCSShardingType = request.getDataCSShardingType().getDesc();
        }
        if(request.getDataCLShardingType() != null) {
            this.dataCLShardingType = request.getDataCLShardingType().getDesc();
        }
        this.dataCSRange        = request.getDataCSRange();
        this.dataDomain         = request.getDataDomain();
        this.dataLobPageSize    = request.getDataLobPageSize();
        this.dataReplSize       = request.getDataReplSize();
        this.metaDomain         = request.getMetaDomain();
        this.dataLocation       = request.getDataLocation();
        this.metaLocation       = request.getMetaLocation();
        this.metaHisLocation    = request.getMetaHisLocation();
    }

}
