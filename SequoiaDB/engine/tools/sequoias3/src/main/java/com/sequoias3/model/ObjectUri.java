package com.sequoias3.model;

import com.sequoias3.common.RestParamDefine;
import com.sequoias3.core.ObjectMeta;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

public class ObjectUri {
    private String  bucketName;
    private String  objectName;
    private boolean withVersionId   = false;
    private Long    versionId       = null;
    private Boolean nullVersionFlag = null;

    public ObjectUri(String uri) throws S3ServerException{
        String decodeUri;
        try {
            decodeUri = URLDecoder.decode(uri, "UTF-8");
        }catch (UnsupportedEncodingException e){
            throw new S3ServerException(S3Error.OBJECT_COPY_INVALID_SOURCE, "Invalid source. source url = " + uri);
        }
        int beginBucket;
        if (decodeUri.startsWith(RestParamDefine.REST_DELIMITER)){
            beginBucket = 1;
        } else {
            beginBucket = 0;
        }
        int beginObject = decodeUri.indexOf(RestParamDefine.REST_DELIMITER, beginBucket);
        if (beginObject == -1) {
            throw new S3ServerException(S3Error.OBJECT_COPY_INVALID_SOURCE, "Invalid source. source url = " + uri);
        }

        this.bucketName = decodeUri.substring(beginBucket, beginObject);
        if (this.bucketName.length() == 0){
            throw new S3ServerException(S3Error.OBJECT_COPY_INVALID_SOURCE, "Invalid source. source url = " + uri);
        }

        int beginVersionId = decodeUri.indexOf(RestParamDefine.REST_SOURCE_VERSIONID, beginObject);
        if (beginVersionId == -1){
            this.objectName = decodeUri.substring(beginObject+1);
        }else {
            this.objectName = decodeUri.substring(beginObject+1, beginVersionId);
            this.withVersionId = true;
            String version = decodeUri.substring(beginVersionId + RestParamDefine.REST_SOURCE_VERSIONID.length());
            convertVersionId(version);
        }

        if (this.objectName.length() == 0){
            throw new S3ServerException(S3Error.OBJECT_COPY_INVALID_SOURCE, "Invalid source. source url = " + uri);
        }
    }

    public ObjectUri(String bucketName, String objectName, String versionId) throws S3ServerException{
        this.bucketName = bucketName;
        this.objectName = objectName;

        if (versionId != null){
            this.withVersionId = true;
            convertVersionId(versionId);
        }
    }

    private void convertVersionId(String versionId)
            throws S3ServerException{
        try {
            if (versionId.equals(ObjectMeta.NULL_VERSION_ID)) {
                this.nullVersionFlag = true;
            } else {
                this.versionId =  Long.parseLong(versionId);
            }
        }catch (NumberFormatException e) {
            throw new S3ServerException(S3Error.OBJECT_INVALID_VERSION,
                    "version id is invalid. version id=" + versionId);
        }catch (Exception e){
            throw new S3ServerException(S3Error.OBJECT_INVALID_VERSION,
                    "versionId is invalid. versionId="+versionId+",e:"+e.getMessage());
        }
    }

    public String getBucketName(){
        return bucketName;
    }

    public String getObjectName(){
        return objectName;
    }

    public boolean isWithVersionId(){
        return withVersionId;
    }

    public Long getVersionId(){
        return versionId;
    }

    public Boolean getNullVersionFlag(){
        return nullVersionFlag;
    }
}
