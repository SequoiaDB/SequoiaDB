package com.sequoias3.model;

import com.sequoias3.core.FilterRecord;
import com.sequoias3.core.UploadMeta;
import com.sequoias3.dao.QueryDbCursor;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;

public class UploadFilter {
    private QueryDbCursor uploadCursor;
    private String prefix;
    private String delimiter;
    private String keyMarker;
    private Long   uploadIdMarker;
    private String encodingType;
    private Owner  owner;

    private int filterType;
    private Boolean hasGetFirst = false;
    private String lastCommonPrefix;
    private int    prefixLength = 0;

    public UploadFilter(QueryDbCursor uploadCursor, String prefix, String keyMarker,
                        Long uploadIdMarker, String encodingType, Owner owner)
            throws S3ServerException {
        this.filterType     = FilterRecord.FILTER_NO_DELIMITER;
        this.uploadCursor   = uploadCursor;
        this.prefix         = prefix;
        this.keyMarker      = keyMarker;
        this.uploadIdMarker = uploadIdMarker;
        this.encodingType   = encodingType;
        this.owner          = owner;

        if (this.uploadCursor != null){
            if (this.uploadCursor.hasNext()){
                this.uploadCursor.getNext();
            }else {
                this.uploadCursor = null;
            }
        }
        if (keyMarker == null){
            hasGetFirst = true;
        }
        if (prefix != null){
            prefixLength = prefix.length();
        }
    }

    public UploadFilter(QueryDbCursor uploadCursor, String prefix, String keyMarker,
                        Long uploadIdMarker, String encodingType, Owner owner,
                        String delimiter) throws S3ServerException {
        this.filterType     = FilterRecord.FILTER_DELIMITER;
        this.uploadCursor   = uploadCursor;
        this.prefix         = prefix;
        this.delimiter      = delimiter;
        this.keyMarker      = keyMarker;
        this.uploadIdMarker = uploadIdMarker;
        this.encodingType   = encodingType;
        this.owner          = owner;

        if (this.uploadCursor != null){
            if (this.uploadCursor.hasNext()){
                this.uploadCursor.getNext();
            }else {
                this.uploadCursor = null;
            }
        }
        if (keyMarker == null){
            hasGetFirst = true;
        }
        if (prefix != null){
            prefixLength = prefix.length();
        }
    }

    public FilterRecord getNextRecord()throws S3ServerException{
        if (filterType == FilterRecord.FILTER_DELIMITER){
            return getUploadWithDelimiter();
        }else if (filterType == FilterRecord.FILTER_NO_DELIMITER){
            return getUploadWithNoDelimiter();
        }else {
            return null;
        }
    }

    private FilterRecord getUploadWithDelimiter() throws S3ServerException{
        while (uploadCursor != null) {
            BSONObject record =this.uploadCursor.getCurrent();
            String key = record.get(UploadMeta.META_KEY_NAME).toString();
            if (uploadCursor.hasNext()) {
                uploadCursor.getNext();
            } else {
                uploadCursor = null;
            }

            int delimiterIndex = key.indexOf(delimiter, prefixLength);
            if (-1 != delimiterIndex) {
                FilterRecord result;
                if ((result = getNewCommonPrefix(key.substring(0, delimiterIndex + delimiter.length()))) != null){
                    return result;
                }
            } else {
                lastCommonPrefix = null;
                if (!hasGetFirst) {
                    if (isFirstRecord(record.get(UploadMeta.META_KEY_NAME).toString(),
                            (long)record.get(UploadMeta.META_UPLOAD_ID))){
                        hasGetFirst = true;
                    }else {
                        continue;
                    }
                }
                Upload upload = new Upload(record, encodingType, owner);
                FilterRecord result = new FilterRecord();
                result.setUpload(upload);
                result.setRecordType(FilterRecord.UPLOAD);
                return result;
            }
        }
        return null;
    }

    private FilterRecord getUploadWithNoDelimiter() throws S3ServerException{
        while (uploadCursor != null) {
            BSONObject record =this.uploadCursor.getCurrent();
//            String key = record.get(UploadMeta.META_KEY_NAME).toString();
            if (uploadCursor.hasNext()) {
                uploadCursor.getNext();
            } else {
                uploadCursor = null;
            }

            if (!hasGetFirst) {
                if (isFirstRecord(record.get(UploadMeta.META_KEY_NAME).toString(),
                        (long)record.get(UploadMeta.META_UPLOAD_ID))){
                    hasGetFirst = true;
                }else {
                    continue;
                }
            }
            Upload upload = new Upload(record, encodingType, owner);
            FilterRecord result = new FilterRecord();
            result.setUpload(upload);
            result.setRecordType(FilterRecord.UPLOAD);
            return result;
        }
        return null;
    }

    private FilterRecord getNewCommonPrefix(String commonPrefix) throws S3ServerException{
        if (lastCommonPrefix != null && commonPrefix.startsWith(lastCommonPrefix)){
            return null;
        }
        lastCommonPrefix = commonPrefix;
        if (!hasGetFirst){
            if (commonPrefix.compareTo(keyMarker) > 0){
                hasGetFirst = true;
            }else {
                return null;
            }
        }

        FilterRecord result = new FilterRecord();
        result.setCommonPrefix(new CommonPrefix(commonPrefix, encodingType));
        result.setRecordType(FilterRecord.COMMONPREFIX);
        return result;
    }

    private Boolean isFirstRecord(String key, Long uploadId) {
        int com = key.compareTo(keyMarker);
        if (com > 0){
            return true;
        }else if (com == 0){
            if (uploadIdMarker != null) {
                if (uploadId > uploadIdMarker) {
                    return true;
                }
            }
        }
        return false;
    }

    public Boolean hasNext() throws S3ServerException{
        if(filterType == FilterRecord.FILTER_DELIMITER){
            return hasNextWithDelimiter();
        }else if (filterType == FilterRecord.FILTER_NO_DELIMITER){
            return hasNextWithNoDelimiter();
        }
        else {
            return false;
        }
    }

    private Boolean hasNextWithDelimiter() throws S3ServerException{
        if (lastCommonPrefix == null) {
            if (uploadCursor != null) {
                return true;
            }
        } else {
            while (uploadCursor != null){
                if (!uploadCursor.getCurrent().get(UploadMeta.META_KEY_NAME).toString().startsWith(lastCommonPrefix)) {
                    return true;
                }
                if (uploadCursor.hasNext()) {
                    uploadCursor.getNext();
                }else {
                    uploadCursor = null;
                }
            }
        }
        return false;
    }

    private Boolean hasNextWithNoDelimiter(){
        if (uploadCursor != null){
            return true;
        }
        return false;
    }
}
