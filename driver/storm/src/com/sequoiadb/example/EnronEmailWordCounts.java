package com.sequoiadb.example;

import backtype.storm.Config;
import backtype.storm.StormSubmitter;
import backtype.storm.task.TopologyContext;
import backtype.storm.topology.BasicOutputCollector;
import backtype.storm.topology.IBasicBolt;
import backtype.storm.topology.OutputFieldsDeclarer;
import backtype.storm.topology.TopologyBuilder;
import backtype.storm.tuple.Fields;
import backtype.storm.tuple.Tuple;
import com.sequoiadb.base.*;
import com.sequoiadb.core.SequoiaObjectGrabber;
import com.sequoiadb.core.StormSequoiaObjectGrabber;
import com.sequoiadb.core.UpdateQueryCreator;
import com.sequoiadb.spout.SequoiaCappedCollectionSpout;
import com.sequoiadb.bolt.SequoiaUpdateBolt;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import org.apache.log4j.Logger;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.mortbay.util.ajax.JSON;

public class EnronEmailWordCounts {
	static Logger LOG = Logger.getLogger(EnronEmailWordCounts.class);

	public static void main(String[] args) throws UnknownHostException {
		try {
			if (args.length != 8) {
				System.out.println("Usage: <sdb host> <sdb port> <username> <password> <src csname> <src clname> <dst csname> <dst clname>");
				return;
			}
			
			String host = args[0];
			int port = Integer.valueOf(args[1]);
			String userName = args[2];
			String password = args[3];
			String srcCsName = args[4];
			String srcClName = args[5];
			String dstCsName = args[6];
			String dstClName = args[7];

			SequoiaObjectGrabber mapsRecordToTuples = new SequoiaObjectGrabber() {
				@Override
				public List<Object> map(BSONObject object) {
					List<Object> tuple = new ArrayList<Object>();

					tuple.add(object.get("_id"));
					tuple.add(object.get("body"));

					return tuple;
				}

				@Override
				public String[] fields() {
					return new String[] { "_id", "body" };
				}

			};

			SequoiaCappedCollectionSpout spout = new SequoiaCappedCollectionSpout(
					host, port, userName, password, srcCsName, srcClName,
					mapsRecordToTuples);

			IBasicBolt wordCountingBolt = new IBasicBolt() {
				@Override
				public void prepare(Map map, TopologyContext topologyContext) {
				}

				@Override
				public void execute(Tuple tuple,
						BasicOutputCollector basicOutputCollector) {
					Object _id = tuple.getValueByField("_id");
					String body = tuple.getStringByField("body");
					String[] Words = body.replace("\n\n", " ").split(" ");

					Map<String, Integer> wordNumsMap = new HashMap<String, Integer>();
					for (String word : Words) {
						if (wordNumsMap.containsKey(word)) {
							Integer num = wordNumsMap.get(word) + 1;
							wordNumsMap.put(word, num);
						} else {
							wordNumsMap.put(word, 1);
						}
					}

					for (Entry<String, Integer> entry : wordNumsMap.entrySet()) {
						List<Object> resultTuple = new ArrayList<Object>();
						resultTuple.add(entry.getKey());
						resultTuple.add(entry.getValue());

						basicOutputCollector.emit(resultTuple);
					}
				}

				@Override
				public void cleanup() {
				}

				@Override
				public void declareOutputFields(
						OutputFieldsDeclarer outputFieldsDeclarer) {
					outputFieldsDeclarer.declare(new Fields("word", "count"));
				}

				@Override
				public Map<String, Object> getComponentConfiguration() {
					return null;
				}
			};

			UpdateQueryCreator updateQueryCreator = new UpdateQueryCreator() {
				@Override
				public BSONObject createQuery(Tuple tuple) {
					BSONObject query = new BasicBSONObject();
					query.put("word", tuple.getValueByField("word"));

					return query;
				}
			};

			StormSequoiaObjectGrabber mapper = new StormSequoiaObjectGrabber() {
				@Override
				public BSONObject map(BSONObject object, Tuple tuple) {
					BSONObject updator = new BasicBSONObject();
					
					BSONObject countObj = new BasicBSONObject();
					countObj.put("count", tuple.getIntegerByField("count"));
					updator.put("$inc", countObj);
					
					
					BSONObject wordSetObj = new BasicBSONObject();
					BSONObject wordObj = new BasicBSONObject();
					wordObj.put("word", tuple.getStringByField("word"));
					updator.put("$set", wordObj);
					
					return updator;
				}
			};

			SequoiaUpdateBolt mongoUpdateBolt = new SequoiaUpdateBolt(host,
					port, userName, password, dstCsName, dstClName,
					updateQueryCreator, mapper);

			TopologyBuilder builder = new TopologyBuilder();
			builder.setSpout("sequoiadbspout", spout, 1);
			builder.setBolt("wordscount", wordCountingBolt, 20)
					.shuffleGrouping("sequoiadbspout");
			builder.setBolt("sequoiadbbolt", mongoUpdateBolt, 10)
					.shuffleGrouping("wordscount");


			Config config = new Config();
			config.setDebug(false);

			config.setNumWorkers(1);

			StormSubmitter.submitTopology("enron", config,
					builder.createTopology());

		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
