package com.sequoias3.model;

import com.sequoias3.core.Dir;
import com.sequoias3.core.FilterRecord;
import com.sequoias3.core.ObjectMeta;
import com.sequoias3.dao.QueryDbCursor;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;

import java.util.ArrayList;
import java.util.List;

public class VersionsFilter {
    private static final int MAX_NUMBER = 100;

    private static final Integer FIRST  = 1;
    private static final Integer SECOND = 2;
    private static final Integer THIRD  = 3;

    private QueryDbCursor dirCursor;
    private QueryDbCursor curCursor;
    private QueryDbCursor hisCursor;
    private Owner         owner;

    private String prefix;
    private String delimiter;
    private String keyMarker;
    private Long   versionIdMarker;
    private String encodingType;

    private int filterType;
    private Boolean hasGetFirst = false;
    private String lastCommonPrefix;
    private int    prefixLength = 0;

    private List<FilterRecord> curList;
    private List<String>       curKeys;
    private List<Long>         curVersionIds;
    private int            count;
    private int            index;

    public VersionsFilter(QueryDbCursor curCursor, QueryDbCursor dirCursor,
                          QueryDbCursor hisCursor, String prefix, String delimiter,
                          String keyMarker, Long versionIdMarker,
                          String encodingType, Owner owner, int filterType)
            throws S3ServerException {
        this.curCursor       = curCursor;
        this.dirCursor       = dirCursor;
        this.hisCursor       = hisCursor;
        this.prefix          = prefix;
        this.delimiter       = delimiter;
        this.keyMarker       = keyMarker;
        this.versionIdMarker = versionIdMarker;
        this.encodingType    = encodingType;
        this.owner           = owner;
        this.filterType      = filterType;
        this.curList         = new ArrayList<>();
        this.curKeys         = new ArrayList<>();
        this.curVersionIds   = new ArrayList<>();
        if (prefix != null){
            prefixLength = prefix.length();
        }
        if (this.dirCursor != null){
            if (this.dirCursor.hasNext()){
                this.dirCursor.getNext();
            }else {
                this.dirCursor = null;
            }
        }
        if (this.curCursor != null){
            if (this.curCursor.hasNext()){
                this.curCursor.getNext();
            }else {
                this.curCursor = null;
            }
        }
        if (this.hisCursor != null){
            if (this.hisCursor.hasNext()){
                this.hisCursor.getNext();
            }else {
                this.hisCursor = null;
            }
        }
        if (keyMarker == null){
            hasGetFirst = true;
        }
    }

    // 当使用目录法查询时，初始，历史Cursor中还没有查到数据，
    // 这时过滤器还没有准备好，需要从CurCursor中读取100条数据，
    // 并拿这100条数据到历史表中查询记录，将查询的历史记录放进过滤器，过滤器开始使用
    // 当使用目录法查询时，已经读完历史Cursor中本次查询的一批数据，
    // 但是CurCursor中还有数据的时候，过滤器要暂停，再从CurCursor
    // 中读取100条数据，并拿这100条数据到历史表中查询记录放进过滤器，过滤器继续使用
    public Boolean isFilterReady(){
        if (filterType == FilterRecord.FILTER_DIR){
            return isFilterReadyWithDir();
        }else {
            return true;
        }
    }

    private Boolean isFilterReadyWithDir(){
        if (curCursor == null){
            return true;
        }else if (hisCursor != null){
            return true;
        }else if (index < count){
            return true;
        }else {
            return false;
        }
    }

    public FilterRecord getNextRecord() throws S3ServerException{
        if (filterType == FilterRecord.FILTER_DIR){
            return getVersionWithDir();
        }else if (filterType == FilterRecord.FILTER_DELIMITER){
            return getVersionWithDelimiter();
        } else if(filterType == FilterRecord.FILTER_NO_DELIMITER){
            return getVersionWithNoDelimiter();
        }else {
            return null;
        }
    }

    private FilterRecord getVersionWithDir() throws S3ServerException{
        while (true){
            String key       = null;
            String hisKey    = null;
            String strDir    = null;

            if (dirCursor != null){
                strDir = dirCursor.getCurrent().get(Dir.DIR_NAME).toString();
            }
            if (index < count){
                key = curKeys.get(index);
            }
            if (hisCursor != null){
                hisKey = hisCursor.getCurrent().get(ObjectMeta.META_KEY_NAME).toString();
            }
            Integer choice = getChoice(strDir, key, hisKey);
            if (choice == null){
                return null;
            }

            if (choice == FIRST){
                if (dirCursor.hasNext()){
                    dirCursor.getNext();
                }else {
                    dirCursor = null;
                }
                FilterRecord result;
                if((result = getCommonPrefix(strDir)) != null){
                    return result;
                }
            }else if (choice == SECOND){
                index++;
                lastCommonPrefix = null;
                if (!hasGetFirst){
                    if (isFirstRecord(key, curVersionIds.get(index-1))){
                        hasGetFirst = true;
                    }else {
                        continue;
                    }
                }
                return curList.get(index-1);
            }else if (choice == THIRD){
                lastCommonPrefix = null;
                BSONObject record = hisCursor.getCurrent();
                if (hisCursor.hasNext()){
                    hisCursor.getNext();
                }else {
                    hisCursor = null;
                }
                if (!hasGetFirst){
                    if (isFirstRecord(hisKey, (long)record.get(ObjectMeta.META_VERSION_ID))){
                        hasGetFirst = true;
                    }else {
                        continue;
                    }
                }

                return getVersionDeleteMarker(record, false, owner);
            }else {
                return null;
            }
        }
    }

    private FilterRecord getVersionWithDelimiter() throws S3ServerException{
        while (true){
            String  key      = null;
            String  hisKey   = null;
            Boolean isLatest = true;
            Integer choice;

            if (curCursor != null){
                key = curCursor.getCurrent().get(ObjectMeta.META_KEY_NAME).toString();
            }
            if (hisCursor != null){
                hisKey = hisCursor.getCurrent().get(ObjectMeta.META_KEY_NAME).toString();
            }

            choice = getChoice(key, hisKey, null);
            if (choice == null){
                return null;
            }

            BSONObject record = null;
            String commonKey = null;
            if (choice == FIRST){
                isLatest = true;
                int delimiterIndex = key.indexOf(delimiter, prefixLength);
                if (-1 != delimiterIndex) {
                    commonKey = key;
                }else {
                    record = curCursor.getCurrent();
                }
                if (curCursor.hasNext()){
                    curCursor.getNext();
                }else {
                    curCursor = null;
                }
            }else if (choice == SECOND){
                isLatest = false;
                int delimiterIndex = hisKey.indexOf(delimiter, prefixLength);
                if (-1 != delimiterIndex) {
                    commonKey = hisKey;
                }else {
                    record = hisCursor.getCurrent();
                }
                if (hisCursor.hasNext()){
                    hisCursor.getNext();
                }else {
                    hisCursor = null;
                }
            }else {
                return null;
            }

            if (commonKey != null){
                FilterRecord result;
                if ((result = getCommonPrefix(commonKey)) != null){
                    return result;
                }
            }
            if (record != null){
                lastCommonPrefix = null;
                if (!hasGetFirst){
                    if (isFirstRecord(record.get(ObjectMeta.META_KEY_NAME).toString(),
                            (long)record.get(ObjectMeta.META_VERSION_ID))){
                        hasGetFirst = true;
                    }else {
                        continue;
                    }
                }

                return getVersionDeleteMarker(record, isLatest, owner);
            }
        }
    }

    private FilterRecord getVersionWithNoDelimiter() throws S3ServerException{
        while (true){
            String  key      = null;
            String  hisKey   = null;
            Boolean isLatest;
            Integer choice;

            if (curCursor != null){
                key = curCursor.getCurrent().get(ObjectMeta.META_KEY_NAME).toString();
            }
            if (hisCursor != null){
                hisKey = hisCursor.getCurrent().get(ObjectMeta.META_KEY_NAME).toString();
            }

            choice = getChoice(key, hisKey, null);
            if (choice == null){
                return null;
            }

            BSONObject record;
            if (choice == FIRST){
                record = curCursor.getCurrent();
                if (curCursor.hasNext()){
                    curCursor.getNext();
                }else {
                    curCursor = null;
                }
                isLatest = true;
            }else if (choice == SECOND){
                record = hisCursor.getCurrent();
                if (hisCursor.hasNext()){
                    hisCursor.getNext();
                }else {
                    hisCursor = null;
                }
                isLatest = false;
            }else {
                return null;
            }

            if (!hasGetFirst){
                if (isFirstRecord(record.get(ObjectMeta.META_KEY_NAME).toString(),
                        (long)record.get(ObjectMeta.META_VERSION_ID))){
                    hasGetFirst = true;
                }else {
                    continue;
                }
            }

            return getVersionDeleteMarker(record, isLatest, owner);
        }
    }

    private Boolean isFirstRecord(String key, long versionId) {
        int com = key.compareTo(keyMarker);
        if (com > 0){
            return true;
        }else if (com == 0){
            if (versionIdMarker != null) {
                if (versionId < versionIdMarker) {
                    return true;
                }
            }
        }
        return false;
    }

    private Integer getChoice(String first, String second, String third){
        String lastStr = null;
        Integer choice = null;
        if (first != null){
            choice  = FIRST;
            lastStr = first;
        }
        if (second != null){
            if (lastStr != null) {
                if (second.compareTo(lastStr) < 0) {
                    choice  = SECOND;
                    lastStr = second;
                }
            }else {
                choice  = SECOND;
                lastStr = second;
            }
        }
        if (third != null){
            if (lastStr != null){
                if (third.compareTo(lastStr) < 0){
                    choice  = THIRD;
                    lastStr = third;
                }
            }else {
                choice  = THIRD;
                lastStr = third;
            }
        }

        return choice;
    }

    private FilterRecord getCommonPrefix(String key) throws S3ServerException{
        if (lastCommonPrefix != null && key.startsWith(lastCommonPrefix)){
            return null;
        }
        int index = key.indexOf(delimiter, prefixLength);
        if (index == -1){
            return null;
        }
        String commonPrefix = key.substring(0, index + delimiter.length());
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

    private FilterRecord getVersionDeleteMarker(BSONObject record, Boolean isLatest,
                                                Owner owner)throws S3ServerException{
        FilterRecord result = new FilterRecord();
        if ((Boolean)record.get(ObjectMeta.META_DELETE_MARKER)) {
            RawVersion deleteMarker = new RawVersion(record, encodingType, isLatest, owner);
            result.setRecordType(FilterRecord.DELETEMARKER);
            result.setDeleteMarker(deleteMarker);
        }else {
            Version version = new Version(record, encodingType, isLatest, owner);
            result.setRecordType(FilterRecord.VERSION);
            result.setVersion(version);
        }
        return result;
    }

    public Boolean hasNext() throws S3ServerException{
        if (filterType == FilterRecord.FILTER_DIR){
            return hasNextWithDir();
        }else if(filterType == FilterRecord.FILTER_DELIMITER){
            return hasNextWithDelimiter();
        }else if (filterType == FilterRecord.FILTER_NO_DELIMITER){
            return hasNextWithNoDelimiter();
        }
        else {
            return false;
        }
    }

    private Boolean hasNextWithDir() throws S3ServerException{
        if (index < count
                || hisCursor != null
                || curCursor != null){
            return true;
        }else if (dirCursor != null){
            if (lastCommonPrefix != null){
                while (true) {
                    BSONObject record = dirCursor.getCurrent();
                    String dirName = record.get(Dir.DIR_NAME).toString();
                    if (!dirName.startsWith(lastCommonPrefix)) {
                        return true;
                    }
                    if (dirCursor.hasNext()) {
                        dirCursor.getNext();
                    } else {
                        return false;
                    }
                }
            }else {
                return true;
            }
        }

        return false;
    }

    private Boolean hasNextWithDelimiter() throws S3ServerException{

        if (lastCommonPrefix == null) {
            if (curCursor != null
                    || hisCursor != null) {
                return true;
            }
        } else {
            while (curCursor != null){
                if (!curCursor.getCurrent().get(ObjectMeta.META_KEY_NAME).toString().startsWith(lastCommonPrefix)) {
                   return true;
                }
                if (curCursor.hasNext()) {
                    curCursor.getNext();
                }else {
                    curCursor = null;
                }
            }
        }
        return false;
    }

    private Boolean hasNextWithNoDelimiter() throws S3ServerException{
        if (curCursor != null
                || hisCursor != null){
            return true;
        }
        return false;
    }

    public List<String> getCurKeys() throws S3ServerException{
        count = 0;
        index = 0;
        this.curList         = new ArrayList<>();
        this.curKeys         = new ArrayList<>();
        this.curVersionIds   = new ArrayList<>();
        while(count < MAX_NUMBER){
            FilterRecord curResult = new FilterRecord();
            BSONObject record = curCursor.getCurrent();
            if ((Boolean)record.get(ObjectMeta.META_DELETE_MARKER)) {
                RawVersion deleteMarker = new RawVersion(record, encodingType, true, owner);
                curResult.setRecordType(FilterRecord.DELETEMARKER);
                curResult.setDeleteMarker(deleteMarker);
            }else {
                Version version = new Version(record, encodingType, true, owner);
                curResult.setRecordType(FilterRecord.VERSION);
                curResult.setVersion(version);
            }
            curKeys.add(record.get(ObjectMeta.META_KEY_NAME).toString());
            curVersionIds.add((long)record.get(ObjectMeta.META_VERSION_ID));
            curList.add(curResult);
            count++;

            if (curCursor.hasNext()){
                curCursor.getNext();
            }else {
                curCursor = null;
                break;
            }
        }

        return curKeys;
    }

    public void setHisCursor(QueryDbCursor hisCursor) throws S3ServerException{
        this.hisCursor = hisCursor;
        if (this.hisCursor != null) {
            if (this.hisCursor.hasNext()) {
                this.hisCursor.getNext();
            } else {
                this.hisCursor = null;
            }
        }
    }
}
