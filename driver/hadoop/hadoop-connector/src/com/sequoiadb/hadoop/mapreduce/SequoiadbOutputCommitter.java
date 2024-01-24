package com.sequoiadb.hadoop.mapreduce;

import java.io.IOException;

import org.apache.hadoop.mapreduce.JobContext;
import org.apache.hadoop.mapreduce.OutputCommitter;
import org.apache.hadoop.mapreduce.TaskAttemptContext;



public class SequoiadbOutputCommitter extends OutputCommitter {

	@Override
	public void abortTask(TaskAttemptContext arg0) throws IOException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void commitTask(TaskAttemptContext arg0) throws IOException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public boolean needsTaskCommit(TaskAttemptContext arg0) throws IOException {
		// TODO Auto-generated method stub
		return true;
	}

	@Override
	public void setupJob(JobContext arg0) throws IOException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setupTask(TaskAttemptContext arg0) throws IOException {
		// TODO Auto-generated method stub
		
	}

}
