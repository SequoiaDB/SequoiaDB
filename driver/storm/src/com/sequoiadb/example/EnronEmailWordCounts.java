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

			// Create a mapper from documents emitted from the spout to the ones
			// we wish to process
			SequoiaObjectGrabber mapsRecordToTuples = new SequoiaObjectGrabber() {
				@Override
				public List<Object> map(BSONObject object) {
					// The tuple we are returning
					List<Object> tuple = new ArrayList<Object>();

					tuple.add(object.get("_id"));
					tuple.add(object.get("body"));

					// Return the mapped object
					return tuple;
				}

				@Override
				public String[] fields() {
					return new String[] { "_id", "body" };
				}

			};

			// Set up
			SequoiaCappedCollectionSpout spout = new SequoiaCappedCollectionSpout(
					host, port, userName, password, srcCsName, srcClName,
					mapsRecordToTuples);

			// Create a word counting bold
			IBasicBolt wordCountingBolt = new IBasicBolt() {
				@Override
				public void prepare(Map map, TopologyContext topologyContext) {
				}

				@Override
				public void execute(Tuple tuple,
						BasicOutputCollector basicOutputCollector) {
					// Grab the _id field so we can pass it on for a save later
					Object _id = tuple.getValueByField("_id");
					// Grab the text body
					String body = tuple.getStringByField("body");
					// Clean up the text split it and count
					String[] Words = body.replace("\n\n", " ").split(" ");

					// Count the number of words in this tuple
					Map<String, Integer> wordNumsMap = new HashMap<String, Integer>();
					for (String word : Words) {
						if (wordNumsMap.containsKey(word)) {
							Integer num = wordNumsMap.get(word) + 1;
							wordNumsMap.put(word, num);
						} else {
							wordNumsMap.put(word, 1);
						}
					}

					// spout the all of word's number
					for (Entry<String, Integer> entry : wordNumsMap.entrySet()) {
						// Create the tuple result
						List<Object> resultTuple = new ArrayList<Object>();
						resultTuple.add(entry.getKey());
						resultTuple.add(entry.getValue());

						// Emit the result
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

			// Update Query for document
			UpdateQueryCreator updateQueryCreator = new UpdateQueryCreator() {
				@Override
				public BSONObject createQuery(Tuple tuple) {
					// Pick the document based on the _id passed in
					BSONObject query = new BasicBSONObject();
					query.put("word", tuple.getValueByField("word"));

					return query;
				}
			};

			// Field mapper
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

			// Create a mongodb update bolt that will update documents adding
			// the
			// wordcount variable
			SequoiaUpdateBolt mongoUpdateBolt = new SequoiaUpdateBolt(host,
					port, userName, password, dstCsName, dstClName,
					updateQueryCreator, mapper);

			// Build a topology
			TopologyBuilder builder = new TopologyBuilder();
			// Set the spout
			builder.setSpout("sequoiadbspout", spout, 1);
			// Add the bolt to count the number of words in each email
			builder.setBolt("wordscount", wordCountingBolt, 20)
					.shuffleGrouping("sequoiadbspout");
			// Save the word count back to the db
			builder.setBolt("sequoiadbbolt", mongoUpdateBolt, 10)
					.shuffleGrouping("wordscount");

			// Set debug config

			Config config = new Config();
			config.setDebug(false);

			config.setNumWorkers(1);

			// Submit a job
			StormSubmitter.submitTopology("enron", config,
					builder.createTopology());

		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
