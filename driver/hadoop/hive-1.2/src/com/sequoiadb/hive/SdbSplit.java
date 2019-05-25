package com.sequoiadb.hive;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.mapred.FileInputFormat;
import org.apache.hadoop.mapred.FileSplit;
import org.apache.hadoop.mapred.InputSplit;
import org.apache.hadoop.mapred.JobConf;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SdbSplit extends FileSplit implements InputSplit {
	private String collectionSpaceName;
	private String collectionName;
	public static final Log LOG = LogFactory.getLog(SdbSplit.class.getName());

	private static final String[] EMPTY_ARRAY = new String[] {};

	private String dbHostName = "";
	private int dbPort = -1;
	private String dbUserName = "";
	private String dbPasswd = "";

	public SdbSplit() {
		super((Path) null, 0, 0, EMPTY_ARRAY);

	}

	public SdbSplit(String host, int port, String dbUserName, String dbPasswd, String collectionSpaceName,
			String collectionName, Path dummyPath) {
		super(dummyPath, 0, 0, EMPTY_ARRAY);
		this.dbHostName = host;
		this.dbPort = port;
		this.dbUserName = dbUserName;
		this.dbPasswd = dbPasswd;
		this.collectionSpaceName = collectionSpaceName;
		this.collectionName = collectionName;
	}

	@Override
	public void readFields(final DataInput input) throws IOException {
		LOG.debug("enter sdbSplit readFields");
		super.readFields(input);
		this.dbHostName = input.readUTF();
		this.dbPort = input.readInt();
		this.dbUserName = input.readUTF();
		this.dbPasswd = input.readUTF();
		this.collectionSpaceName=input.readUTF();
		this.collectionName=input.readUTF();

		LOG.debug("go out sdbSplit readFields");
	}

	@Override
	public void write(final DataOutput output) throws IOException {
		LOG.debug("enter sdbSplit write");
		super.write(output);
		output.writeUTF(this.dbHostName);
		output.writeInt(this.dbPort);
		output.writeUTF(this.dbUserName);
		output.writeUTF(this.dbPasswd);
		output.writeUTF(this.collectionSpaceName);
		output.writeUTF(this.collectionName);
		
		LOG.debug("go out sdbSplit write");
	}


	
	public String getSdbHost (){
		return this.dbHostName;
	}
	public int getSdbPort (){
		return this.dbPort;
	}
	public String getSdbUser (){
		return this.dbUserName;
	}
	public String getSdbPasswd (){
		return this.dbPasswd;
	}

	@Override
	public long getLength() {
		return super.getLength();
	}

	/* Data is remote for all nodes. */
	@Override
	public String[] getLocations() throws IOException {
		return new String[] { this.dbHostName };
	}

	@Override
	public String toString() {
		return "SdbSplit [collectionSpaceName=" + collectionSpaceName
				+ ", collectionName=" + collectionName + ", sdbAddr=" + this.dbHostName
				+ ":" + this.dbPort 
				+ "]";
	}

	public static InputSplit[] getSplits(JobConf conf, int numSplits) {
		LOG.debug("Entry getSplits function");

		final Path[] tablePaths = FileInputFormat.getInputPaths(conf);

		Sequoiadb sdb = null;
		BaseException lastException = null;

		String dbUserName = ConfigurationUtil.getUserName(conf);
		String dbPasswd = ConfigurationUtil.getPasswd(conf);
		String dbAddrs = ConfigurationUtil.getDBAddr(conf);
		
		List<String> addrList = ConfigurationUtil.getDBAddrs (dbAddrs);
		LOG.debug ("addrList = " + addrList.toString());                             
	      LOG.debug ("dbUserName = " + dbUserName);                                    
	      LOG.debug ("dbPasswd = " + dbPasswd);
		sdb = new Sequoiadb (addrList, dbUserName, dbPasswd, null);
		
		if (sdb == null) {
			LOG.error("Connect coords error:" + lastException);
			throw lastException;
		}

		String spaceName = null;
		String clName = null;
		
		if (ConfigurationUtil.getCsName(conf) == null){
			spaceName = ConfigurationUtil.getHiveDataBaseName(conf);
		}else{
			spaceName = ConfigurationUtil.getCsName(conf);
		}
		if (ConfigurationUtil.getClName(conf) == null){
			clName = ConfigurationUtil.getHiveTableName(conf);
		}else{
			clName = ConfigurationUtil.getClName(conf);
		}
		
		StringBuilder snapCondBuilder = new StringBuilder();
		
		String snapCond = snapCondBuilder.append("{Name:\"").append(spaceName)
				.append('.').append(clName).append("\"}").toString();
		
		List<InputSplit> splits = new LinkedList<InputSplit>();
		
		DBCollection collection = sdb.getCollectionSpace(spaceName)
				.getCollection(clName);
		
		DBCursor explainCurl = collection.explain(null, null, null, null, 0, 0,
				0, new BasicBSONObject("Run", false));
		
		Set<String> subCls = new HashSet<String>();
		
		while (explainCurl.hasNext()) {
			BSONObject explainBson = explainCurl.getNext();
			String nodeName = (String) explainBson.get("NodeName");
			String hostName = nodeName.split(":")[0];
			int port = Integer.parseInt(nodeName.split(":")[1]);
			try {
				List<BSONObject> list = (List<BSONObject>) explainBson
						.get("SubCollections");
				for (int i = 0; i < list.size(); i++) {
					String totalName = (String) list.get(i).get("Name");
					String[] items = totalName.split("\\.");
					SdbSplit split=new SdbSplit(hostName, port, dbUserName, dbPasswd, items[0], items[1],
							tablePaths[0]);
					splits.add(split);
				}
			} catch (Exception e) {
				splits.add(new SdbSplit(hostName, port, dbUserName, dbPasswd, spaceName, clName,
						tablePaths[0]));
			}
		}
		sdb.disconnect();
		return splits.toArray(new InputSplit[splits.size()]);
	}

	public String getCollectionSpaceName() {
		return collectionSpaceName;
	}

	public void setCollectionSpaceName(String collectionSpaceName) {
		this.collectionSpaceName = collectionSpaceName;
	}

	public String getCollectionName() {
		return collectionName;
	}

	public void setCollectionName(String collectionName) {
		this.collectionName = collectionName;
	}
	
}
