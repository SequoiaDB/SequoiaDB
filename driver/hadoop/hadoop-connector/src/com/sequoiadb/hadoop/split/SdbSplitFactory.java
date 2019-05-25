package com.sequoiadb.hadoop.split;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.mapreduce.InputSplit;
import org.apache.hadoop.mapreduce.JobContext;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.FileSplit;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.hadoop.util.SdbConnAddr;
import com.sequoiadb.hadoop.util.SequoiadbConfigUtil;

/**
 * 
 * 
 * @className閿涙瓔dbSplit
 * 
 * @author閿涳拷gaoshengjie
 * 
 * @createtime:2013楠烇拷2閺堬拷1閺冿拷娑撳﹤宕�0:46:36
 * 
 * @changetime:TODO
 * 
 * @version 1.0.0
 * 
 */
public class SdbSplitFactory {
	private static final Log log = LogFactory.getLog(SdbSplitFactory.class);

	/**
	 * 
	 * @content:depend the method of scan ,return different split;
	 * 
	 * @param jobContext
	 * @return
	 * 
	 * @exception
	 * @since 1.0.0
	 */
	public static List<InputSplit> getSplits(JobContext jobContext) {

		Configuration conf = jobContext.getConfiguration();

		String urls = SequoiadbConfigUtil.getInputURL(conf);
		SdbConnAddr[] sdbConnAddrs = SequoiadbConfigUtil.getAddrList(urls);
		
		String user = SequoiadbConfigUtil.getInputUser(conf);
		String passwd = SequoiadbConfigUtil.getInputPasswd(conf);
		
		String preferedInstance=SequoiadbConfigUtil.getPreferenceInstance(conf);
		String collectionName = SequoiadbConfigUtil.getInCollectionName(conf);
		String collectionSpaceName = SequoiadbConfigUtil.getInCollectionSpaceName(conf);
		String queryStr = SequoiadbConfigUtil.getQueryString(conf);
		String selectorStr = SequoiadbConfigUtil.getSelectorString(conf);
		log.debug("test sdbConnAddrs whether it can connect or not ");
		Sequoiadb sdb = null;
		BaseException lastException = null;
		boolean flag = false;
		for (int i = 0; i < sdbConnAddrs.length; i++) {
			try {
				sdb = new Sequoiadb(sdbConnAddrs[i].getHost(),
						sdbConnAddrs[i].getPort(), user, passwd);
				
				if (preferedInstance.equalsIgnoreCase("slave")){
					preferedInstance = "S";
				}else if(preferedInstance.equalsIgnoreCase("master")){
					preferedInstance = "M";
				}else if(preferedInstance.equalsIgnoreCase("anyone")){
					preferedInstance = "A";
				}else if(preferedInstance.equalsIgnoreCase("node1") ||
						 preferedInstance.equalsIgnoreCase("node2") ||
						 preferedInstance.equalsIgnoreCase("node3") ||
						 preferedInstance.equalsIgnoreCase("node4") ||
						 preferedInstance.equalsIgnoreCase("node5") ||
						 preferedInstance.equalsIgnoreCase("node6") ||
						 preferedInstance.equalsIgnoreCase("node7"))
				{
					preferedInstance = preferedInstance.substring(4, 5);
				}else{
					log.warn("conf set 'preferedInstance' = " + preferedInstance + ", this type is undefine, use preferedInstance = 'slave'");
					preferedInstance = "S";
				}
				sdb.setSessionAttr(new BasicBSONObject("PreferedInstance",preferedInstance));
				break;
			} catch (BaseException e) {
				lastException = e;
			}
		}
		if (sdb == null) {
			throw lastException;
		}

		log.debug("start get data blocks");

		if (collectionSpaceName == null || collectionName == null) {
			throw new IllegalArgumentException(
					"collectionSpaceName and collectionName must have value");
		}
		
		if(!sdb.isCollectionSpaceExist(collectionSpaceName)){
			throw new IllegalArgumentException(
					"collectionSpaceName not exist");
		}
		
		if(!sdb.getCollectionSpace(collectionSpaceName).isCollectionExist(collectionName)){
			throw new IllegalArgumentException(
					"collectionName not exist");
		}
		DBCollection collection = sdb.getCollectionSpace(collectionSpaceName).getCollection(collectionName);
    	BSONObject queryBson = null;
    	BSONObject selectorBson = null;
    	BSONObject orderbyBson = null;	
    	if ( queryStr != null){  
    		try {
    			queryBson = (BSONObject) JSON.parse( queryStr );
			} catch (Exception e) {
				queryBson = null;
				log.warn("query string is error");
			}
    	}
    	if ( selectorStr != null){ 
    		try {
    			selectorBson = (BSONObject) JSON.parse( selectorStr );
    			selectorBson.put("_id", null);
			} catch (Exception e) {
				selectorBson = null;
				log.warn("selector string is error");
			}
    	}
    	log.debug("explain");
    	DBCursor explainCurl=collection.explain(queryBson, selectorBson, null, null, 0, 0, 0,new BasicBSONObject("Run",false));
    	Set<String> subCls=new HashSet<String>();
    	while(explainCurl.hasNext()){
    		BSONObject explainBson=explainCurl.getNext();	
    		
    		BasicBSONList bsonlist = (BasicBSONList) explainBson.get("SubCollections");
    		if(bsonlist == null)
    		{
    			subCls.add((String)explainBson.get("Name"));
    		}else
    		{
    			for(int i=0;i<bsonlist.size();i++)
    			{
    				BSONObject bson = (BSONObject) bsonlist.get(i);
    				subCls.add((String) bson.get("Name"));
    			}
    		}

    	}
    	log.debug("input split");
    	List<InputSplit> splits = new ArrayList<InputSplit>();
    	Iterator<String> clName=subCls.iterator();
    	while(clName.hasNext()){
    		String temp=clName.next();
    		String[] items=temp.split("\\.");
    		collection = sdb.getCollectionSpace(items[0]).getCollection(items[1]);
    		collect(collection, splits, items[0], items[1]);
    	}   	

		log.debug("inputsplit  size is"+splits.size());		
		return splits;
	}
	
	private static void collect(DBCollection collection,List<InputSplit> splits, String collectionSpaceName, String collectionName){	
		DBCursor cursor = collection.getQueryMeta(null, null, null, 0, -1, 0);
		while (cursor.hasNext()) {
			BSONObject obj = cursor.getNext();
			log.debug("meta record:" + obj.toString());

			String hostname = (String) obj.get("HostName");
			int port = Integer.parseInt((String) obj.get("ServiceName"));
			String scanType = (String) obj.get("ScanType");

			if ("ixscan".equals(scanType)) {
				String indexName = (String) obj.get("IndexName");
				int indexLID = (Integer) obj.get("IndexLID");
				int direction = (Integer) obj.get("Direction");

				BasicBSONList indexBlockList = (BasicBSONList) obj
						.get("Indexblocks");

				for (Object objBlock : indexBlockList) {
					if (objBlock instanceof BSONObject) {
						BSONObject indexBlock = (BSONObject) objBlock;
						BSONObject startKey = (BSONObject) indexBlock
								.get("StartKey");
						BSONObject endKey = (BSONObject) indexBlock
								.get("EndKey");
					}
				}

			}

			if ("tbscan".equals(scanType)) {
				BasicBSONList blockList = (BasicBSONList) obj.get("Datablocks");
				
				int i = 0;
				for (Object objBlock : blockList) {
					if (objBlock instanceof Integer) {
						Integer blockId = (Integer) objBlock;
						splits.add(new SdbBlockSplit(new SdbConnAddr(hostname,
								port), scanType, blockId, collectionSpaceName, collectionName));
					}
				}
			}

		}
	}
}

