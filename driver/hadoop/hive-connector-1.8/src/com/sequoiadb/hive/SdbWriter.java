package com.sequoiadb.hive;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Random;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hive.ql.exec.FileSinkOperator.RecordWriter;
import org.apache.hadoop.hive.serde2.io.ByteWritable;
import org.apache.hadoop.hive.serde2.io.DoubleWritable;
import org.apache.hadoop.hive.serde2.io.ShortWritable;
import org.apache.hadoop.io.BooleanWritable;
import org.apache.hadoop.io.FloatWritable;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.MapWritable;
import org.apache.hadoop.io.NullWritable;
import org.apache.hadoop.io.Writable;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.*;

public class SdbWriter implements RecordWriter {
	
	public static final Log LOG = LogFactory.getLog(SdbWriter.class.getName());
	
	static private int bufferMaxSize = 1500;
	private Sequoiadb sdb = null;
	private DBCollection collection = null;
	private List<BSONObject> objectBuffer = new ArrayList<BSONObject>(
			bufferMaxSize * 2);

	// private List<BSONObject> objectBuffer = new LinkedList<BSONObject>();

	public SdbWriter(String connAddr, String spaceName, String colName,
			int RecoredNum) {
		
		LOG.debug("Entry:SdbWriter");
		
		if (RecoredNum > 0)
		{
			bufferMaxSize = RecoredNum;
		}

		SdbConnAddr[] addrList = ConfigurationUtil.getAddrList(connAddr);
		if (addrList == null || addrList.length == 0) {
			throw new IllegalArgumentException("The argument "
					+ ConfigurationUtil.DB_ADDR + " must be set.");
		}

		InetAddress localAddr = null;
		try {
			localAddr = InetAddress.getLocalHost();
			LOG.debug(localAddr.getHostAddress());
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			LOG.error(e.getMessage());
		}

		//Get all localtion address.
		ArrayList<SdbConnAddr> localAddrList = new ArrayList<SdbConnAddr>();
		for (int i = 0; i < addrList.length; i++) {
			if (addrList[i].getHost().equals(localAddr.getHostAddress()) || addrList[i].getHost().equals(localAddr.getHostName())) {
				localAddrList.add(addrList[i]);
			}
		}

		//If not any local address
		if (localAddrList.isEmpty()) {
			for (int i = 0; i < addrList.length; i++) {
				localAddrList.add(addrList[i]);
			}
		}
		
		Random rand = new Random();
		int i = rand.nextInt(localAddrList.size());
		
		LOG.debug("i:" + i + "localAddrList:" + localAddrList.get(i).toString());

		this.sdb = new Sequoiadb(localAddrList.get(i).getHost(), localAddrList
				.get(i).getPort(), null, null);
		CollectionSpace space = null;
		try{
			if( sdb.isCollectionSpaceExist(spaceName) )
				space = sdb.getCollectionSpace(spaceName);
			else
				space = sdb.createCollectionSpace(spaceName);
		}catch(BaseException e){
			LOG.error(e.getMessage());
		}
		
		try{
			if( space.isCollectionExist(colName) )
				collection = space.getCollection(colName);
			else
				collection = space.createCollection(colName);
		}catch(BaseException e){
			LOG.error(e.getMessage());
		}
	}

	@Override
	public void close(boolean abort) throws IOException {

		// flush buffer.
		if (objectBuffer.size() > 0) {
			collection.bulkInsert(objectBuffer,
					DBCollection.FLG_INSERT_CONTONDUP);
			objectBuffer.clear();
		}

		if (sdb != null)
			sdb.disconnect();
	}

	@Override
	public void write(Writable w) throws IOException {
		MapWritable map = (MapWritable) w;
		BSONObject dbo = new BasicBSONObject();
		for (final Map.Entry<Writable, Writable> entry : map.entrySet()) {
			// System.err.println("Write: key=" + entry.getKey().toString()
			// + ", val=" + entry.getValue().toString());
			String key = entry.getKey().toString();
			// if ("id".equals(key)) {
			// key = "_id";
			// }
			dbo.put(key.toLowerCase(), getObjectFromWritable(entry.getValue()));
		}
		
		objectBuffer.add(dbo);
		if (objectBuffer.size() >= bufferMaxSize &&
			collection != null) {
			collection.bulkInsert(objectBuffer,
					DBCollection.FLG_INSERT_CONTONDUP);
			objectBuffer.clear();
		}
	}

	private Object getObjectFromWritable(Writable w) {
		if (w instanceof IntWritable) {
			// int
			return ((IntWritable) w).get();
		} else if (w instanceof ShortWritable) {
			// short
			return ((ShortWritable) w).get();
		} else if (w instanceof ByteWritable) {
			// byte
			return ((ByteWritable) w).get();
		} else if (w instanceof BooleanWritable) {
			// boolean
			return ((BooleanWritable) w).get();
		} else if (w instanceof LongWritable) {
			// long
			return ((LongWritable) w).get();
		} else if (w instanceof FloatWritable) {
			// float
			return ((FloatWritable) w).get();
		} else if (w instanceof DoubleWritable) {
			// double
			return ((DoubleWritable) w).get();
		} else if (w instanceof NullWritable) {
			// null
			return null;
		} else {
			// treat as string
			return w.toString();
		}

	}

}
