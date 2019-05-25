package com.sequoiadb.hadoop.io;


import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.io.NullWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.RecordWriter;
import org.apache.hadoop.mapreduce.TaskAttemptContext;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.hadoop.util.SdbConnAddr;

public class SequoiadbWriter<K, V> extends RecordWriter<K, V> {
	private static final Log log = LogFactory.getLog(SequoiadbWriter.class);
	
	static class BsonDeal{
		private BSONObject bson;
		public BsonDeal( BSONObject _bson ){
			this.bson = _bson;
		}
		public void setBson( BSONObject _bson ){
			this.bson = _bson;
		}
		public BSONObject getBson(){
			return this.bson;
		}
		public BSONObject getBsonWithoutId(){
			BSONObject newBson = new BasicBSONObject();
			Set keySet = this.bson.keySet();
			java.util.Iterator it = keySet.iterator();
			while( it.hasNext() ){
				String key=it.next().toString();
				if ( key.equalsIgnoreCase("_id") ){
					continue;
				}
				newBson.put(key, this.bson.get(key));
			}
			return newBson;
		}
		public ObjectId getBsonId(){
			
			Set keySet = this.bson.keySet();
			java.util.Iterator it = keySet.iterator();
			while( it.hasNext() ){
				String key=it.next().toString();
				if ( key.equalsIgnoreCase("_id") ){
					return (ObjectId) this.bson.get("_id");
				}
			}
			return null;
		}
	}

	private DBCollection dbCollection;
	private Sequoiadb sequoiadb;
	private List<BSONObject> lstBsonBuffer = null;
	private int bulkNum;
	private String writeType = null;

	public SequoiadbWriter(String collectionSpaceName, String collectionName,
			SdbConnAddr sdbConnAddr, String user, String passwd, int bulkNum, String writeType) {
		super();
		this.sequoiadb = new Sequoiadb(sdbConnAddr.getHost(),
				sdbConnAddr.getPort(), user, passwd);
		
		CollectionSpace space=null;
		if(collectionSpaceName == null)
			throw new IllegalArgumentException(" the output collection space is null");
		if(collectionName == null)
			throw new IllegalArgumentException(" the output collection is null");
		
		if(sequoiadb.isCollectionSpaceExist(collectionSpaceName)){
			space = sequoiadb.getCollectionSpace(collectionSpaceName);	
		}else{
			sequoiadb.createCollectionSpace(collectionSpaceName);
		}
		
		if(space.isCollectionExist(collectionName)){
			this.dbCollection=space.getCollection(collectionName);
		}else{
			this.dbCollection=space.createCollection(collectionName);
		}
		
		this.lstBsonBuffer = new ArrayList<BSONObject>(bulkNum);
		this.bulkNum = bulkNum;
		if ( writeType.equalsIgnoreCase("bulkinsert") || writeType.equalsIgnoreCase("upsert") ){
			this.writeType = writeType;
		}else{
			log.warn("writeType != bulkinsert and writeType != upsert,use default value");
			this.writeType = "upsert";
		}
		log.debug("writeType = " + this.writeType);
	}
	
	public SequoiadbWriter(String collectionSpaceName, String collectionName,
			SdbConnAddr sdbConnAddr, String user, String passwd, String writeType) {
		super();
		this.sequoiadb = new Sequoiadb(sdbConnAddr.getHost(),
				sdbConnAddr.getPort(), user, passwd);
		
		CollectionSpace space=null;
		if(sequoiadb.isCollectionSpaceExist(collectionSpaceName)){
			space = sequoiadb.getCollectionSpace(collectionSpaceName);	
		}else{
			sequoiadb.createCollectionSpace(collectionSpaceName);
		}
		
		if(space.isCollectionExist(collectionName)){
			this.dbCollection=space.getCollection(collectionName);
		}else{
			this.dbCollection=space.createCollection(collectionName);
		}
		
		
		if ( writeType.equalsIgnoreCase("bulkinsert") || writeType.equalsIgnoreCase("upsert") ){
			this.writeType = writeType;
		}else{
			log.warn("writeType != bulkinsert and writeType != upsert,use default value");
			this.writeType = "upsert";
		}
		log.debug("writeType = " + this.writeType);
	}

	@Override
	public void close(TaskAttemptContext arg0) throws IOException,
			InterruptedException {
		if( this.writeType.equalsIgnoreCase("bulkinsert") ){
			if(lstBsonBuffer.size()>0){
				this.dbCollection.bulkInsert(lstBsonBuffer, DBCollection.FLG_INSERT_CONTONDUP);
				lstBsonBuffer.clear();
			}
		}
		if (this.sequoiadb != null) {
			this.sequoiadb.disconnect();
		}
	}

	@Override
	public void write(K key, V value) throws IOException, InterruptedException {
		BSONObject bson = null;

		if (value != null) {
			if (value instanceof BSONWritable) {
				bson = ((BSONWritable) value).getBson();
			} else if (value instanceof BSONObject) {
				bson = (BSONObject) value;
			} else {
				try {
					bson = BasicBSONObject.typeToBson(value);
				} catch (Exception e) {
					log.error("Failed convert value to bson", e);
				}
			}
		}
		
		if (key != null && !(key instanceof NullWritable)) {
			if (key instanceof Text) {
				bson.put("_id", new ObjectId(((Text) key).toString()));
			} else if (key instanceof ObjectId) {
				bson.put("_id", key);
			} else {
				bson.put("_id", new ObjectId(key.toString()));
			}
		}
		
		if ( this.writeType.equalsIgnoreCase("bulkinsert") ){
			if (lstBsonBuffer.size() < bulkNum) {
				lstBsonBuffer.add(bson);
			} else {
				this.dbCollection.bulkInsert(lstBsonBuffer, DBCollection.FLG_INSERT_CONTONDUP);
				lstBsonBuffer.clear();
				lstBsonBuffer.add(bson);
			}
		}else if ( this.writeType.equalsIgnoreCase("upsert") ){
			BsonDeal bsonDeal = new BsonDeal( bson );
			bson = bsonDeal.getBsonWithoutId();
			
			BSONObject bson_rule = new BasicBSONObject();
			BSONObject bson_query = new BasicBSONObject();
			bson_query.put("_id", bsonDeal.getBsonId());
			bson_rule.put("$set", bson);
			this.dbCollection.upsert(bson_query, bson_rule, null);
		}
	}
}
