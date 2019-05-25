package com.sequoiadb.bolt;

import java.util.Map;

import org.apache.log4j.Logger;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.core.StormSequoiaObjectGrabber;
import com.sequoiadb.exception.BaseException;

import backtype.storm.task.OutputCollector;
import backtype.storm.task.TopologyContext;
import backtype.storm.topology.OutputFieldsDeclarer;
import backtype.storm.topology.base.BaseRichBolt;
import backtype.storm.tuple.Tuple;

public abstract class SequoiaBoltBase extends BaseRichBolt {

	private static final long serialVersionUID = 8636365605902629088L;
	private static Logger LOG = Logger.getLogger(SequoiaBoltBase.class);

	protected Map map;
	protected TopologyContext topologyContext;
	protected OutputCollector outputCollector;

	protected String host;
	protected int port;
	protected String userName;
	protected String password;
	protected String dbName;
	protected String collectionName;
	protected StormSequoiaObjectGrabber mapper;
	
	protected Sequoiadb sdb;
	protected CollectionSpace space;
	protected DBCollection collection;

	/**
	 * @fn SequoiaBoltBase(String host, int port, String userName,
			String password, String dbName, String collectionName,
			StormSequoiaObjectGrabber mapper)
	 * @brief construct
	 * @param host
	 *            The sequoiadb
	 * @param port
	 *            CollectionSpace handle
	 * @param userName
	 *            The user name for sequoiadb
	 * @param password
	 *            The password for sequoiadb
	 * @param dbName
	 *            The Collection space name for sequoiadb
	 * @param collectionNames
	 *            The collection name list
	 * @param mapper
	 *            map tuple to BSONObject
	 */
	public SequoiaBoltBase(String host, int port, String userName,
			String password, String dbName, String collectionName,
			StormSequoiaObjectGrabber mapper) {
		this.host = host;
		this.port = port;
		this.userName = userName;
		this.password = password;
		this.dbName = dbName;
		this.collectionName = collectionName;
		this.mapper = mapper;
	}

	@Override
	public abstract void execute(Tuple tuple);

	@Override
	public void prepare(Map map, TopologyContext context, OutputCollector collector) {
		this.map = map;
		this.topologyContext = context;
		this.outputCollector = collector;
		
		try {
			Sequoiadb sdb = new Sequoiadb(host, port, userName, password);
			space = sdb.getCollectionSpace(dbName);
			
			collection = space.getCollection(collectionName);
		} catch (BaseException e) {
			LOG.error("Failed to get Collection:" + e);
			throw new RuntimeException(e);
		}
	}

	@Override
	public abstract void declareOutputFields(OutputFieldsDeclarer declarer);

	/**
	 * @fn afterExecuteTuple(Tuple tuple)
	 * @brief Lets you handle any additional emission you wish to do
	 * @param tuple
	 *            The emit tuple
	 */
	public abstract void afterExecuteTuple(Tuple tuple);

	@Override
	public abstract void cleanup();

}
