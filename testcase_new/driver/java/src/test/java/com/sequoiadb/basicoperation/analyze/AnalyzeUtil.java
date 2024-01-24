package com.sequoiadb.basicoperation.analyze;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

class Explain {
    private DBCollection dbcl;
    private BSONObject matcher, selector, hint, orderBy, options;
    private long returnNum, skipRows;
    private int flag;

    private String result;

    public String execute() {
        DBCursor cur = dbcl.explain( matcher, selector, orderBy, hint, skipRows,
                returnNum, flag, options );
        StringBuilder stringBuilder = new StringBuilder();
        while ( cur.hasNext() ) {
            stringBuilder.append( cur.getNext().toString() );
        }
        result = stringBuilder.toString();
        return result;
    }

    public boolean isQueryUseTbscan() {
        if ( result == null ) {
            execute();
        }
        return !result.contains( "ixscan" ) && result.contains( "tbscan" );
    }

    public boolean isQueryUseIxscan() {
        if ( result == null ) {
            execute();
        }
        return result.contains( "ixscan" ) && !result.contains( "tbscan" );
    }

    private Explain( Builder builder ) {
        this.dbcl = builder.dbcl;
        this.matcher = builder.matcher;
        this.selector = builder.selector;
        this.hint = builder.hint;
        this.flag = builder.flag;
        this.orderBy = builder.orderBy;
        this.options = builder.options;
        this.returnNum = builder.returnNum;
        this.skipRows = builder.skipRows;
    }

    public String getExplainResult() {
        return result;
    }

    public static class Builder {
        private DBCollection dbcl;
        private BSONObject matcher, selector, hint, orderBy, options;
        private long returnNum = -1, skipRows = 0;
        private int flag = 0;

        public Builder( DBCollection dbcl ) {
            this.dbcl = dbcl;
        }

        public Builder( SdbClWarpper cl ) {
            this.dbcl = cl.getRealCl();
        }

        public Builder matcher( BSONObject m ) {
            this.matcher = m;
            return this;
        }

        public Builder selector( BSONObject o ) {
            this.selector = o;
            return this;
        }

        public Builder hint( BSONObject o ) {
            this.hint = o;
            return this;
        }

        public Builder orderBy( BSONObject o ) {
            this.orderBy = o;
            return this;
        }

        public Builder options( BSONObject o ) {
            this.options = o;
            return this;
        }

        public Builder returnNum( long num ) {
            this.returnNum = num;
            return this;
        }

        public Builder skipRows( long num ) {
            this.skipRows = num;
            return this;
        }

        public Builder flag( int f ) {
            this.flag = f;
            return this;
        }

        public Explain build() {
            return new Explain( this );
        }
    }
}

interface SdbWarpperInterface {
    public SdbCsWarpper createCS( SdbCsProperties cs );

    public void dropCSIfExist( String csName );

    public boolean isStandalone();

    public List< ReplicaGroup > getDataRG();

    public SdbClWarpper createCL( SdbClProperties sdbClProperties );

    public void dropCL( SdbClWarpper cl );
}

class SdbWarpper extends Sequoiadb implements SdbWarpperInterface {

    public SdbWarpper( String connString, String username, String password )
            throws BaseException {
        super( connString, username, password );
    }

    public SdbWarpper( String connString ) throws BaseException {
        super( connString, "", "" );
    }

    public SdbWarpper( String connString, String username, String password,
            ConfigOptions options ) throws BaseException {
        super( connString, username, password, options );
    }

    public SdbWarpper( List< String > connStrings, String username,
            String password, ConfigOptions options ) throws BaseException {
        super( connStrings, username, password, options );
    }

    public SdbWarpper( String host, int port, String username, String password )
            throws BaseException {
        super( host, port, username, password );
    }

    public SdbWarpper( String host, int port, String username, String password,
            ConfigOptions options ) throws BaseException {
        super( host, port, username, password, options );
    }

    public void dropCSIfExist( String csName ) {
        if ( this.isCollectionSpaceExist( csName ) ) {
            this.dropCollectionSpace( csName );
        }
    }

    public void dropCSIfExist( SdbCsProperties cs ) {
        dropCSIfExist( cs.getCsName() );
    }

    public boolean isStandalone() {
        return CommLib.isStandAlone( this );
    }

    public List< ReplicaGroup > getDataRG() {
        ArrayList< String > rgNames = CommLib.getDataGroupNames( this );
        List< ReplicaGroup > rgs = new ArrayList<>( 10 );
        for ( String rgName : rgNames ) {
            rgs.add( this.getReplicaGroup( rgName ) );
        }
        return rgs;
    }

    public SdbCsWarpper createCS( SdbCsProperties cs ) {
        CollectionSpace dbcs = null;
        if ( cs.getOptions() != null ) {
            dbcs = this.createCollectionSpace( cs.getCsName(),
                    cs.getOptions() );
        } else {
            dbcs = this.createCollectionSpace( cs.getCsName() );
        }
        return new SdbCsWarpper( dbcs );
    }

    public SdbClWarpper createCL( SdbClProperties sdbClProperties ) {
        CollectionSpace cs = this
                .getCollectionSpace( sdbClProperties.getCs().getCsName() );
        BSONObject options = new BasicBSONObject();
        if ( sdbClProperties.getShardingKey() != null ) {
            options.put( "ShardingKey", sdbClProperties.getShardingKey() );
        }
        if ( sdbClProperties.getShardingType() != null ) {
            options.put( "ShardingType", sdbClProperties.getShardingType() );
        }
        if ( sdbClProperties.getPartition() != null ) {
            options.put( "Partition", sdbClProperties.getPartition() );
        }
        if ( sdbClProperties.isAutoSplit() != null ) {
            options.put( "AutoSplit", sdbClProperties.isAutoSplit() );
        }
        if ( sdbClProperties.isEnsureShardingIndex() != null ) {
            options.put( "EnsureShardingIndex",
                    sdbClProperties.isEnsureShardingIndex() );
        }
        if ( sdbClProperties.isCompressed() != null ) {
            options.put( "Compressed", sdbClProperties.isCompressed() );
        }
        if ( sdbClProperties.getCompressionType() != null ) {
            options.put( "CompressionType",
                    sdbClProperties.getCompressionType() );
        }
        if ( sdbClProperties.getGroup() != null ) {
            options.put( "Group", sdbClProperties.getGroup() );
        }
        if ( sdbClProperties.isAutoIndexId() != null ) {
            options.put( "AutoIndexId", sdbClProperties.isAutoIndexId() );
        }
        if ( sdbClProperties.isMainCL() != null ) {
            options.put( "IsMainCL", sdbClProperties.isMainCL() );
        }
        if ( sdbClProperties.getReplSize() != null ) {
            options.put( "ReplSize", sdbClProperties.getReplSize() );
        }
        DBCollection cl = cs.createCollection( sdbClProperties.getClName(),
                options );
        return new SdbClWarpper( cl );
    }

    @Override
    public void dropCL( SdbClWarpper cl ) {
        this.getCollectionSpace( cl.getCSName() )
                .dropCollection( cl.getName() );
    }
}

class SdbClWarpper {
    SdbClWarpper( DBCollection cl ) {
        this._cl = cl;
    }

    public DBCollection getRealCl() {
        return _cl;
    }

    public String getName() {
        return _cl.getName();
    }

    public String getFullName() {
        return _cl.getFullName();
    }

    public String getCSName() {
        return _cl.getCSName();
    }

    public Sequoiadb getSequoiadb() {
        return _cl.getSequoiadb();
    }

    public CollectionSpace getCollectionSpace() {
        return _cl.getCollectionSpace();
    }

    public void setMainKeys( String[] keys ) throws BaseException {
        _cl.setMainKeys( keys );
    }

    public Object insert( BSONObject insertor ) throws BaseException {
        return _cl.insert( insertor );
    }

    public Object insert( String insertor ) throws BaseException {
        return _cl.insert( insertor );
    }

    public void insert( List< BSONObject > insertor, int flag )
            throws BaseException {
        _cl.insert( insertor, flag );
    }

    public void insert( List< BSONObject > insertor ) throws BaseException {
        _cl.insert( insertor );
    }

    public < T > void save( T type, Boolean ignoreNullValue, int flag )
            throws BaseException {
        _cl.save( type, ignoreNullValue, flag );
    }

    public < T > void save( T type, Boolean ignoreNullValue )
            throws BaseException {
        _cl.save( type, ignoreNullValue );
    }

    public < T > void save( T type ) throws BaseException {
        _cl.save( type );
    }

    public < T > void save( List< T > type, Boolean ignoreNullValue, int flag )
            throws BaseException {
        _cl.save( type, ignoreNullValue, flag );
    }

    public < T > void save( List< T > type, Boolean ignoreNullValue )
            throws BaseException {
        _cl.save( type, ignoreNullValue );
    }

    public < T > void save( List< T > type ) throws BaseException {
        _cl.save( type );
    }

    public void ensureOID( boolean flag ) {
        _cl.ensureOID( flag );
    }

    public boolean isOIDEnsured() {
        return _cl.isOIDEnsured();
    }

    @Deprecated
    public void bulkInsert( List< BSONObject > insertor, int flag )
            throws BaseException {
        _cl.bulkInsert( insertor, flag );
    }

    public void delete( BSONObject matcher ) throws BaseException {
        _cl.delete( matcher );
    }

    public void delete( String matcher ) throws BaseException {
        _cl.delete( matcher );
    }

    public void delete( String matcher, String hint ) throws BaseException {
        _cl.delete( matcher, hint );
    }

    public void delete( BSONObject matcher, BSONObject hint )
            throws BaseException {
        _cl.delete( matcher, hint );
    }

    public void update( DBQuery query ) throws BaseException {
        _cl.update( query );
    }

    public void update( BSONObject matcher, BSONObject modifier,
            BSONObject hint ) throws BaseException {
        _cl.update( matcher, modifier, hint );
    }

    public void update( BSONObject matcher, BSONObject modifier,
            BSONObject hint, int flag ) throws BaseException {
        _cl.update( matcher, modifier, hint, flag );
    }

    public void update( String matcher, String modifier, String hint )
            throws BaseException {
        _cl.update( matcher, modifier, hint );
    }

    public void update( String matcher, String modifier, String hint, int flag )
            throws BaseException {
        _cl.update( matcher, modifier, hint, flag );
    }

    public void upsert( BSONObject matcher, BSONObject modifier,
            BSONObject hint ) throws BaseException {
        _cl.upsert( matcher, modifier, hint );
    }

    public void upsert( BSONObject matcher, BSONObject modifier,
            BSONObject hint, BSONObject setOnInsert ) throws BaseException {
        _cl.upsert( matcher, modifier, hint, setOnInsert );
    }

    public void upsert( BSONObject matcher, BSONObject modifier,
            BSONObject hint, BSONObject setOnInsert, int flag )
            throws BaseException {
        _cl.upsert( matcher, modifier, hint, setOnInsert, flag );
    }

    public DBCursor explain( BSONObject matcher, BSONObject selector,
            BSONObject orderBy, BSONObject hint, long skipRows, long returnRows,
            int flag, BSONObject options ) throws BaseException {
        return _cl.explain( matcher, selector, orderBy, hint, skipRows,
                returnRows, flag, options );
    }

    public DBCursor query() throws BaseException {
        return _cl.query();
    }

    public DBCursor query( DBQuery matcher ) throws BaseException {
        return _cl.query( matcher );
    }

    public DBCursor query( BSONObject matcher, BSONObject selector,
            BSONObject orderBy, BSONObject hint ) throws BaseException {
        return _cl.query( matcher, selector, orderBy, hint );
    }

    public DBCursor query( BSONObject matcher, BSONObject selector,
            BSONObject orderBy, BSONObject hint, int flag )
            throws BaseException {
        return _cl.query( matcher, selector, orderBy, hint, flag );
    }

    public DBCursor query( String matcher, String selector, String orderBy,
            String hint ) throws BaseException {
        return _cl.query( matcher, selector, orderBy, hint );
    }

    public DBCursor query( String matcher, String selector, String orderBy,
            String hint, int flag ) throws BaseException {
        return _cl.query( matcher, selector, orderBy, hint, flag );
    }

    public DBCursor query( String matcher, String selector, String orderBy,
            String hint, long skipRows, long returnRows ) throws BaseException {
        return _cl.query( matcher, selector, orderBy, hint, skipRows,
                returnRows );
    }

    public DBCursor query( BSONObject matcher, BSONObject selector,
            BSONObject orderBy, BSONObject hint, long skipRows,
            long returnRows ) throws BaseException {
        return _cl.query( matcher, selector, orderBy, hint, skipRows,
                returnRows );
    }

    public DBCursor query( BSONObject matcher, BSONObject selector,
            BSONObject orderBy, BSONObject hint, long skipRows, long returnRows,
            int flags ) throws BaseException {
        return _cl.query( matcher, selector, orderBy, hint, skipRows,
                returnRows, flags );
    }

    public BSONObject queryOne( BSONObject matcher, BSONObject selector,
            BSONObject orderBy, BSONObject hint, int flag )
            throws BaseException {
        return _cl.queryOne( matcher, selector, orderBy, hint, flag );
    }

    public BSONObject queryOne() throws BaseException {
        return _cl.queryOne();
    }

    public DBCursor getIndexes() throws BaseException {
        return _cl.getIndexes();
    }

    public DBCursor queryAndUpdate( BSONObject matcher, BSONObject selector,
            BSONObject orderBy, BSONObject hint, BSONObject update,
            long skipRows, long returnRows, int flag, boolean returnNew )
            throws BaseException {
        return _cl.queryAndUpdate( matcher, selector, orderBy, hint, update,
                skipRows, returnRows, flag, returnNew );
    }

    public DBCursor queryAndRemove( BSONObject matcher, BSONObject selector,
            BSONObject orderBy, BSONObject hint, long skipRows, long returnRows,
            int flag ) throws BaseException {
        return _cl.queryAndRemove( matcher, selector, orderBy, hint, skipRows,
                returnRows, flag );
    }

    public DBCursor getIndex( String name ) throws BaseException {
        return _cl.getIndex( name );
    }

    public void createIndex( String name, BSONObject key, boolean isUnique,
            boolean enforced, int sortBufferSize ) throws BaseException {
        _cl.createIndex( name, key, isUnique, enforced, sortBufferSize );
    }

    public void createIndex( IndexProperties indexProperties ) {
        _cl.createIndex( indexProperties.getIndexName(),
                indexProperties.getIndexKey(), indexProperties.isUnique(),
                indexProperties.isEnforced(),
                indexProperties.getSortBufferSize() );

    }

    public void createIndex( String name, String key, boolean isUnique,
            boolean enforced, int sortBufferSize ) throws BaseException {
        _cl.createIndex( name, key, isUnique, enforced, sortBufferSize );
    }

    public void createIndex( String name, BSONObject key, boolean isUnique,
            boolean enforced ) throws BaseException {
        _cl.createIndex( name, key, isUnique, enforced );
    }

    public void createIndex( String name, String key, boolean isUnique,
            boolean enforced ) throws BaseException {
        _cl.createIndex( name, key, isUnique, enforced );
    }

    public void createIdIndex( BSONObject options ) throws BaseException {
        _cl.createIdIndex( options );
    }

    public void dropIdIndex() throws BaseException {
        _cl.dropIdIndex();
    }

    public void dropIndex( String name ) throws BaseException {
        _cl.dropIndex( name );
    }

    public long getCount() throws BaseException {
        return _cl.getCount();
    }

    public long getCount( String matcher ) throws BaseException {
        return _cl.getCount( matcher );
    }

    public long getCount( BSONObject matcher ) throws BaseException {
        return _cl.getCount( matcher );
    }

    public long getCount( BSONObject matcher, BSONObject hint )
            throws BaseException {
        return _cl.getCount( matcher, hint );
    }

    public void split( String sourceGroupName, String destGroupName,
            BSONObject splitCondition, BSONObject splitEndCondition )
            throws BaseException {
        _cl.split( sourceGroupName, destGroupName, splitCondition,
                splitEndCondition );
    }

    public void split( String sourceGroupName, String destGroupName,
            double percent ) throws BaseException {
        _cl.split( sourceGroupName, destGroupName, percent );
    }

    public long splitAsync( String sourceGroupName, String destGroupName,
            BSONObject splitCondition, BSONObject splitEndCondition )
            throws BaseException {
        return _cl.splitAsync( sourceGroupName, destGroupName, splitCondition,
                splitEndCondition );
    }

    public long splitAsync( String sourceGroupName, String destGroupName,
            double percent ) throws BaseException {
        return _cl.splitAsync( sourceGroupName, destGroupName, percent );
    }

    public DBCursor aggregate( List< BSONObject > objs ) throws BaseException {
        return _cl.aggregate( objs );
    }

    public DBCursor getQueryMeta( BSONObject matcher, BSONObject orderBy,
            BSONObject hint, long skipRows, long returnRows, int flag )
            throws BaseException {
        return _cl.getQueryMeta( matcher, orderBy, hint, skipRows, returnRows,
                flag );
    }

    public void attachCollection( String subClFullName, BSONObject options )
            throws BaseException {
        _cl.attachCollection( subClFullName, options );
    }

    public void detachCollection( String subClFullName ) throws BaseException {
        _cl.detachCollection( subClFullName );
    }

    public void alterCollection( BSONObject options ) throws BaseException {
        _cl.alterCollection( options );
    }

    public DBCursor listLobs() throws BaseException {
        return _cl.listLobs();
    }

    public DBLob createLob() throws BaseException {
        return _cl.createLob();
    }

    public DBLob createLob( ObjectId id ) throws BaseException {
        return _cl.createLob( id );
    }

    public DBLob openLob( ObjectId id, int mode ) throws BaseException {
        return _cl.openLob( id, mode );
    }

    public DBLob openLob( ObjectId id ) throws BaseException {
        return _cl.openLob( id );
    }

    public void removeLob( ObjectId lobId ) throws BaseException {
        _cl.removeLob( lobId );
    }

    public void truncateLob( ObjectId lobId, long length )
            throws BaseException {
        _cl.truncateLob( lobId, length );
    }

    public void truncate() throws BaseException {
        _cl.truncate();
    }

    public void pop( BSONObject options ) throws BaseException {
        _cl.pop( options );
    }

    private DBCollection _cl;
}

class SdbCsWarpper {
    private CollectionSpace _cs;

    SdbCsWarpper( CollectionSpace cs ) {
        this._cs = cs;
    }

    public String getName() {
        return _cs.getName();
    }

    public Sequoiadb getSequoiadb() {
        return _cs.getSequoiadb();
    }

    public DBCollection getCollection( String collectionName )
            throws BaseException {
        return _cs.getCollection( collectionName );
    }

    public boolean isCollectionExist( String collectionName )
            throws BaseException {
        return _cs.isCollectionExist( collectionName );
    }

    public List< String > getCollectionNames() throws BaseException {
        return _cs.getCollectionNames();
    }

    public DBCollection createCollection( String collectionName )
            throws BaseException {
        return _cs.createCollection( collectionName );
    }

    public DBCollection createCollection( String collectionName,
            BSONObject options ) {
        return _cs.createCollection( collectionName, options );
    }

    public void drop() throws BaseException {
        _cs.drop();
    }

    public void dropCollection( String collectionName ) throws BaseException {
        _cs.dropCollection( collectionName );
    }
}

class SdbCsProperties {
    private String csName;
    private BSONObject options;

    public String getCsName() {
        return csName;
    }

    public BSONObject getOptions() {
        return options;
    }

    SdbCsProperties( String csName, BSONObject options ) {
        this.csName = csName;
        this.options = options;
    }

    SdbCsProperties( String csName ) {
        this.csName = csName;
    }
}

class SdbClProperties {
    private String clName;

    private BSONObject shardingKey;
    private String shardingType;
    private Integer partition;
    private Integer replSize;
    private Boolean compressed;
    private String compressionType;
    private Boolean isMainCL;
    private Boolean autoSplit;
    private String group;
    private Boolean autoIndexId;
    private Boolean ensureShardingIndex;
    private SdbCsProperties cs;

    public String getClName() {
        return clName;
    }

    public BSONObject getShardingKey() {
        return shardingKey;
    }

    public String getShardingType() {
        return shardingType;
    }

    public Integer getPartition() {
        return partition;
    }

    public Integer getReplSize() {
        return replSize;
    }

    public Boolean isCompressed() {
        return compressed;
    }

    public String getCompressionType() {
        return compressionType;
    }

    public Boolean isMainCL() {
        return isMainCL;
    }

    public Boolean isAutoSplit() {
        return autoSplit;
    }

    public String getGroup() {
        return group;
    }

    public Boolean isAutoIndexId() {
        return autoIndexId;
    }

    public Boolean isEnsureShardingIndex() {
        return ensureShardingIndex;
    }

    public SdbCsProperties getCs() {
        return cs;
    }

    private SdbClProperties( Builder builder ) {
        clName = builder.clName;
        shardingKey = builder.shardingKey;
        shardingType = builder.shardingType;
        partition = builder.partition;
        replSize = builder.replSize;
        compressed = builder.compressed;
        compressionType = builder.compressionType;
        isMainCL = builder.isMainCL;
        autoSplit = builder.autoSplit;
        group = builder.group;
        autoIndexId = builder.autoIndexId;
        ensureShardingIndex = builder.ensureShardingIndex;
        cs = builder.cs;
    }

    public static Builder newBuilder( SdbCsProperties cs, String clName ) {
        return new Builder( cs, clName );
    }

    public static final class Builder {
        private String clName;
        private BSONObject shardingKey;
        private String shardingType = null;
        private Integer partition = null;
        private Integer replSize = null;
        private Boolean compressed = null;
        private String compressionType = null;
        private Boolean isMainCL = null;
        private Boolean autoSplit = null;
        private String group;
        private Boolean autoIndexId = null;
        private Boolean ensureShardingIndex = null;
        public SdbCsProperties cs;

        private Builder( SdbCsProperties cs, String clName ) {
            this.cs = cs;
            this.clName = clName;
        }

        public Builder clName( String val ) {
            clName = val;
            return this;
        }

        public Builder shardingKey( BSONObject val ) {
            shardingKey = val;
            return this;
        }

        public Builder shardingType( String val ) {
            shardingType = val;
            return this;
        }

        public Builder partition( int val ) {
            partition = val;
            return this;
        }

        public Builder replSize( int val ) {
            replSize = val;
            return this;
        }

        public Builder compressed( boolean val ) {
            compressed = val;
            return this;
        }

        public Builder compressionType( String val ) {
            compressionType = val;
            return this;
        }

        public Builder isMainCL( boolean val ) {
            isMainCL = val;
            return this;
        }

        public Builder autoSplit( boolean val ) {
            autoSplit = val;
            return this;
        }

        public Builder group( String val ) {
            group = val;
            return this;
        }

        public Builder autoIndexId( boolean val ) {
            autoIndexId = val;
            return this;
        }

        public Builder ensureShardingIndex( boolean val ) {
            ensureShardingIndex = val;
            return this;
        }

        public SdbClProperties build() {
            return new SdbClProperties( this );
        }
    }
}

class IndexProperties {
    private String indexName;
    private BSONObject indexKey;
    private boolean isUnique;
    private boolean enforced;
    private int sortBufferSize;

    public String getIndexName() {
        return indexName;
    }

    public BSONObject getIndexKey() {
        return indexKey;
    }

    public boolean isUnique() {
        return isUnique;
    }

    public boolean isEnforced() {
        return enforced;
    }

    public int getSortBufferSize() {
        return sortBufferSize;
    }

    private IndexProperties( Builder builder ) {
        indexName = builder.indexName;
        indexKey = builder.indexKey;
        isUnique = builder.isUnique;
        enforced = builder.enforced;
        sortBufferSize = builder.sortBufferSize;
    }

    public static Builder newBuilder( String indexName, BSONObject indexKey ) {
        return new Builder( indexName, indexKey );
    }

    public static final class Builder {
        private String indexName;
        private BSONObject indexKey;
        private boolean isUnique = false;
        private boolean enforced = false;
        // SdbConstants.IXM_SORT_BUFFER_DEFAULT_SIZE;
        private int sortBufferSize = 64;

        private Builder( String indexName, BSONObject indexKey ) {
            this.indexName = indexName;
            this.indexKey = indexKey;
        }

        public Builder isUnique( boolean val ) {
            isUnique = val;
            return this;
        }

        public Builder enforced( boolean val ) {
            enforced = val;
            return this;
        }

        public Builder sortBufferSize( int val ) {
            sortBufferSize = val;
            return this;
        }

        public IndexProperties build() {
            return new IndexProperties( this );
        }
    }
}

public class AnalyzeUtil {
    public static String getRandomString( int length ) {
        String str = "adcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length; i++ ) {
            int number = random.nextInt( str.length() );
            sb.append( str.charAt( number ) );
        }
        return sb.toString();
    }
}