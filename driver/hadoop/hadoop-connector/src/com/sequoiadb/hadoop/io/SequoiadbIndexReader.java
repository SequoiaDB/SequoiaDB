package com.sequoiadb.hadoop.io;

import java.io.IOException;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.mapreduce.InputSplit;
import org.apache.hadoop.mapreduce.RecordReader;
import org.apache.hadoop.mapreduce.TaskAttemptContext;
import org.bson.BSONObject;

public class SequoiadbIndexReader  extends RecordReader<Object, BSONWritable>{
	
	public SequoiadbIndexReader(InputSplit inputSplit, Configuration conf){
		
	}
	
	@Override
	public void close() throws IOException {
		
	}

	@Override
	public Object getCurrentKey() throws IOException, InterruptedException {
		return null;
	}

	@Override
	public BSONWritable getCurrentValue() throws IOException,
			InterruptedException {
		return null;
	}

	@Override
	public float getProgress() throws IOException, InterruptedException {
		return 0;
	}

	@Override
	public void initialize(InputSplit arg0, TaskAttemptContext arg1)
			throws IOException, InterruptedException {
		
	}

	@Override
	public boolean nextKeyValue() throws IOException, InterruptedException {
		return false;
	}

}
