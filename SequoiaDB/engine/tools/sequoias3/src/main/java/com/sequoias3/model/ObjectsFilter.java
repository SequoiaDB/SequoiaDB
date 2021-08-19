package com.sequoias3.model;

import com.sequoias3.core.Dir;
import com.sequoias3.core.FilterRecord;
import com.sequoias3.core.ObjectMeta;
import com.sequoias3.dao.QueryDbCursor;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;

public class ObjectsFilter {
    private static final Integer DIR  = 1;
    private static final Integer KEY = 2;

    private QueryDbCursor dirCursor;
    private QueryDbCursor keyCursor;
    private String prefix;
    private String delimiter;
    private String startAfter;
    private String encodingType;

    private int filterType;
    private Boolean hasGetFirst = false;
    private String lastKey;
    private String lastCommonPrefix;
    private int    prefixLength = 0;

    public ObjectsFilter(QueryDbCursor dirCursor, QueryDbCursor keyCursor, String prefix,
                         String delimiter, String startAfter, String encodingType, int filterType)
            throws S3ServerException {
        this.dirCursor    = dirCursor;
        this.keyCursor    = keyCursor;
        this.prefix       = prefix;
        this.delimiter    = delimiter;
        this.startAfter   = startAfter;
        this.encodingType = encodingType;
        if (this.dirCursor != null){
            if (this.dirCursor.hasNext()){
                this.dirCursor.getNext();
            }else {
                this.dirCursor = null;
            }
        }
        if (this.keyCursor != null){
            if (this.keyCursor.hasNext()){
                this.keyCursor.getNext();
            }else {
                this.keyCursor = null;
            }
        }
        this.filterType = filterType;
        if (startAfter == null){
            hasGetFirst = true;
        }
        if (prefix != null){
            prefixLength = prefix.length();
        }
    }

    //1.当使用目录法查询，目录表中的记录有可能有重复的commonprefix，
    // 但是curCursor中的数据都是不重复的，可以直接放在content中
    //2.当使用表扫描方式带分隔符的方式查询时，全部从curCursor中获取，
    // 这时获取的key有可能具有相同的commonprefix，需要逐条遍历确定，
    // 匹配到分隔符的归为commonprefix，匹配不到分隔符的记录在content中
    //3.当使用表扫描方式不带分隔符的方式查询时，全部从curCursor中获取，
    // 这时获取的每个key都可以直接放入content
    public FilterRecord getNextRecord()throws S3ServerException{
        if (filterType == FilterRecord.FILTER_DIR){
            return getRecordWithDir();
        }else if (filterType == FilterRecord.FILTER_DELIMITER){
            return getRecordWithDelimiter();
        }else if (filterType == FilterRecord.FILTER_NO_DELIMITER){
            return getRecordWithNoDelimiter();
        }else {
            return null;
        }
    }

    private FilterRecord getRecordWithDir() throws S3ServerException{
        while(true) {
            String key     = null;
            String dir     = null;
            Integer choice = null;
            if (dirCursor != null) {
                dir = dirCursor.getCurrent().get(Dir.DIR_NAME).toString();
            }
            if (keyCursor != null) {
                key = keyCursor.getCurrent().get(ObjectMeta.META_KEY_NAME).toString();
            }
            if (dir != null ) {
                choice = DIR;
            }
            if (key != null) {
                if (dir != null){
                    if (key.compareTo(dir) < 0){
                        choice = KEY;
                    }
                }else{
                    choice = KEY;
                }
            }
            if (choice == null){
                return null;
            }
            if (choice == DIR){
                lastKey = dir;
                BSONObject record = dirCursor.getCurrent();
                if (dirCursor.hasNext()) {
                    dirCursor.getNext();
                } else {
                    dirCursor = null;
                }
                String dirName = record.get(Dir.DIR_NAME).toString();
                FilterRecord result;
                if ((result = getNewCommonPrefix(dirName)) != null){
                    return result;
                }
            } else if (choice == KEY){
                lastKey = key;
                BSONObject record = this.keyCursor.getCurrent();
                Content content = new Content(record, encodingType);
                FilterRecord result = new FilterRecord();
                result.setContent(content);
                result.setRecordType(FilterRecord.CONTENT);
                if (keyCursor.hasNext()) {
                    keyCursor.getNext();
                } else {
                    keyCursor = null;
                }
                lastCommonPrefix = null;
                return result;
            }else {
                return null;
            }
        }
    }

    private FilterRecord getRecordWithDelimiter() throws S3ServerException{
        while (keyCursor != null) {
            BSONObject record = this.keyCursor.getCurrent();
            String key = record.get(ObjectMeta.META_KEY_NAME).toString();
            lastKey = key;
            if (keyCursor.hasNext()) {
                keyCursor.getNext();
            } else {
                keyCursor = null;
            }

            int delimiterIndex = key.indexOf(delimiter, prefixLength);
            if (-1 != delimiterIndex) {
                FilterRecord result;
                if ((result = getNewCommonPrefix(key)) != null){
                    return result;
                }
            } else {
                Content content = new Content(record, encodingType);
                FilterRecord result = new FilterRecord();
                result.setContent(content);
                result.setRecordType(FilterRecord.CONTENT);
                lastCommonPrefix = null;
                return result;
            }
        }
        return null;
    }

    private FilterRecord getNewCommonPrefix(String key) throws S3ServerException{
        if (lastCommonPrefix != null && key.startsWith(lastCommonPrefix)){
            return null;
        }
        int index = key.indexOf(delimiter, prefixLength);
        if (index == -1){
            return null;
        }
        String commonPrefix = key.substring(0, index + delimiter.length());
        lastCommonPrefix = commonPrefix;
        if (hasGetFirst ) {
            FilterRecord result = new FilterRecord();
            result.setCommonPrefix(new CommonPrefix(commonPrefix, encodingType));
            result.setRecordType(FilterRecord.COMMONPREFIX);
            return result;
        }else if (commonPrefix.compareTo(startAfter) > 0) {
            FilterRecord result = new FilterRecord();
            result.setCommonPrefix(new CommonPrefix(commonPrefix, encodingType));
            result.setRecordType(FilterRecord.COMMONPREFIX);
            hasGetFirst = true;
            return result;
        }
        return null;
    }

    private FilterRecord getRecordWithNoDelimiter() throws S3ServerException{
        if (keyCursor != null) {
            BSONObject record = this.keyCursor.getCurrent();
            if (keyCursor.hasNext()) {
                keyCursor.getNext();
            } else {
                keyCursor = null;
            }
            lastKey = record.get(ObjectMeta.META_KEY_NAME).toString();
            Content content = new Content(record, encodingType);
            FilterRecord result = new FilterRecord();
            result.setContent(content);
            result.setRecordType(FilterRecord.CONTENT);
            return result;
        }else {
            return null;
        }
    }

    public Boolean hasNext() throws S3ServerException{
        if (filterType == FilterRecord.FILTER_DIR){
            return hasNextWithDir();
        }else if (filterType == FilterRecord.FILTER_DELIMITER){
            return hasNextWithDelimiter();
        }else if (filterType == FilterRecord.FILTER_NO_DELIMITER){
            return hasNextWithNoDelimiter();
        }else {
            return null;
        }
    }

    private Boolean hasNextWithDir()throws S3ServerException{
        if (keyCursor != null){
            return true;
        }

        if (dirCursor != null){
            if (lastCommonPrefix == null){
                return true;
            }else {
                while (true) {
                    BSONObject record = dirCursor.getCurrent();
                    String dirName = record.get(Dir.DIR_NAME).toString();
                    if (!dirName.startsWith(lastCommonPrefix)) {
                        return true;
                    }
                    lastKey = dirName;
                    if (dirCursor.hasNext()) {
                        dirCursor.getNext();
                    } else {
                        return false;
                    }
                }
            }
        }

        return false;
    }

    private Boolean hasNextWithDelimiter() throws S3ServerException{
        if (keyCursor == null){
            return false;
        }

        if (lastCommonPrefix == null){
            return true;
        }

        while (keyCursor != null) {
            BSONObject record = this.keyCursor.getCurrent();
            if (keyCursor.hasNext()) {
                keyCursor.getNext();
            } else {
                keyCursor = null;
            }

            String key = record.get(ObjectMeta.META_KEY_NAME).toString();
            if (key.startsWith(lastCommonPrefix)){
                lastKey = key;
            }else {
                return true;
            }
        }
        return false;
    }

    private Boolean hasNextWithNoDelimiter() {
        if (keyCursor != null){
            return true;
        }
        return false;
    }

    public String getLastKey() {
        return lastKey;
    }

    public String getLastCommonPrefix() {
        return lastCommonPrefix;
    }

    public void setLastCommonPrefix(String lastCommonPrefix) {
        this.lastCommonPrefix = lastCommonPrefix;
    }
}
