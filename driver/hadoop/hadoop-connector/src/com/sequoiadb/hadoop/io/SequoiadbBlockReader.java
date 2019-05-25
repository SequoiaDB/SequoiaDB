package com.sequoiadb.hadoop.io;

import java.io.IOException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.mapreduce.InputSplit;
import org.apache.hadoop.mapreduce.RecordReader;
import org.apache.hadoop.mapreduce.TaskAttemptContext;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.hadoop.split.SdbBlockSplit;
import com.sequoiadb.hadoop.util.SequoiadbConfigUtil;

/**
 * 
 * 
 * @className閿涙瓔equoiadbReader
 * 
 * @author閿涳拷gaoshengjie
 * 
 * @createtime:2013楠烇拷2閺堬拷0閺冿拷娑撳宕�:33:34
 * 
 * @changetime:TODO
 * 
 * @version 1.0.0
 * 
 */
public class SequoiadbBlockReader extends RecordReader<Object, BSONWritable> {
	private static final Log log = LogFactory.getLog(SequoiadbBlockReader.class);
    private BSONObject current;
    private final SdbBlockSplit sdbBlockSplit;
    private final Sequoiadb sequoiadb;
    private final DBCursor cursor;
    private long seen = 0;
    private long total;
	
      public SequoiadbBlockReader(InputSplit inputSplit, Configuration conf){
    	if(inputSplit==null||!(inputSplit instanceof SdbBlockSplit)){
    		throw new IllegalArgumentException("the inputsplit is not SdbBlockSplit" );
    	}
    	
		String user = SequoiadbConfigUtil.getInputUser(conf);
		String passwd = SequoiadbConfigUtil.getInputPasswd(conf);
		
    	
		String queryStr = SequoiadbConfigUtil.getQueryString(conf);
		String selectorStr = SequoiadbConfigUtil.getSelectorString(conf);
		
    	this.sdbBlockSplit=(SdbBlockSplit)inputSplit;
    	   	
    	if(sdbBlockSplit.getSdbAddr()==null){
    		throw new IllegalArgumentException(" the SdbBlockSplit.sdbaddr is null");
    	}
    	
    	String collectionName = this.sdbBlockSplit.getCollectionName();
		String collectionSpaceName = this.sdbBlockSplit.getCollectionSpaceName();
		
		
		if(collectionSpaceName == null)
			throw new IllegalArgumentException(" the input collection space is null");
		if(collectionName == null)
			throw new IllegalArgumentException(" the collection is null");

    	this.sequoiadb = new Sequoiadb(this.sdbBlockSplit.getSdbAddr().getHost(), this.sdbBlockSplit.getSdbAddr().getPort(),user,passwd);
    	CollectionSpace collectionSpace=sequoiadb.getCollectionSpace(collectionSpaceName);
    	if(collectionSpace==null){
    		throw new IllegalArgumentException(" the CS not exists");
    	}
    	
    	DBCollection dbCollection=collectionSpace.getCollection(collectionName);
    	if(dbCollection==null){
    		throw new IllegalArgumentException(" the CL not exists");
    	}	  	
    	
    	BSONObject hint=new BasicBSONObject();
    	
    	BSONObject meta=new BasicBSONObject();
    	BasicBSONList blocks=new BasicBSONList();
    	blocks.add(this.sdbBlockSplit.getDataBlockId());
    	meta.put("Datablocks",blocks);
    	meta.put("ScanType",this.sdbBlockSplit.getScanType());
    	hint.put("$Meta", meta);
    	BSONObject queryBson = null;
    	BSONObject selectorBson = null;
    	BSONObject orderbyBson = null;

    	if ( queryStr != null){  //閺嶇厧绱￠張澶愭６妫帮拷
    		try {
    			queryBson = (BSONObject) JSON.parse( queryStr );
			} catch (Exception e) {
				log.warn("query string is error");
				queryBson = null;
			}
    		log.debug( "queryBson = " + queryBson.toString() );
    	}
    	if ( selectorStr != null){ //閺嶇厧绱￠張澶愭６妫帮拷
    		try {
    			selectorBson = (BSONObject) JSON.parse( selectorStr );
    			selectorBson.put("_id", null);
			} catch (Exception e) {
				log.warn("selector string is error");
				selectorBson = null;
				
			}
    		log.debug( "selectorBson = " + selectorBson.toString() );
    	}

    	this.cursor=dbCollection.query( queryBson, 
    			                        selectorBson,
    			                        orderbyBson,
    			                        hint,
    			                        0,
    			                        -1);	
    }
    
	@Override
	public Object getCurrentKey() throws IOException, InterruptedException {
		return this.current.get("_id");
	}

	@Override
	public BSONWritable getCurrentValue() throws IOException,
			InterruptedException {
		return new BSONWritable(this.current);
	}

	@Override
	public float getProgress() throws IOException, InterruptedException {
		if(this.cursor.hasNext()){
			return 0f;
		}else{
			return 1f;
		}
	}

	@Override
	public void initialize(InputSplit arg0, TaskAttemptContext arg1)
			throws IOException, InterruptedException {
		log.debug("begin to read the inputsplit");
	}

	@Override
	public boolean nextKeyValue() throws IOException, InterruptedException {
            if ( !this.cursor.hasNext() ){
            	log.debug("this inputsplit read over");
                return false;
            }
            this.current = this.cursor.getNext();
            this.seen++;
            return true;
	}

	@Override
	public void close() throws IOException {
		if(cursor!=null){
			cursor.close();
		}
		if(this.sequoiadb!=null){
			this.sequoiadb.disconnect();
		}
	}

}

