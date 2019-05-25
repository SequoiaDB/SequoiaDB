package com.sequoiadb.mapreduce;

import java.io.IOException;
import java.util.Iterator;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.NullWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Mapper.Context;
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.mapreduce.lib.output.TextOutputFormat;
import org.apache.hadoop.mapreduce.Reducer;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.hadoop.io.BSONWritable;
import com.sequoiadb.hadoop.mapreduce.SequoiadbInputFormat;
import com.sequoiadb.hadoop.mapreduce.SequoiadbOutputFormat;

public class HdfsSequoiadbMR {
	static class MobileMapper extends  Mapper<LongWritable,Text,Text,IntWritable>{
		private static final IntWritable ONE=new IntWritable(1);
		@Override
		protected void map(LongWritable key, Text value, Context context)
				throws IOException, InterruptedException {
			String valueStr=value.toString();
			
			String mobile_prefix=valueStr.split(",")[3].substring(0,3);
			context.write(new Text(mobile_prefix), ONE);
		}
		
	}
	
	static class MobileReducer extends Reducer<Text, IntWritable, NullWritable, BSONWritable>{

		@Override
		protected void reduce(Text key, Iterable<IntWritable> values,Context context)
				throws IOException, InterruptedException {
				Iterator<IntWritable> iterator=values.iterator();
				long sum=0;
				while(iterator.hasNext()){
					sum+=iterator.next().get();
				}
				BSONObject bson=new BasicBSONObject();
				bson.put("prefix", key.toString());
				bson.put("count", sum);
				context.write(null,new BSONWritable(bson));
		}
		
	}
	
	
	
	public static void main(String[] args) throws IOException, InterruptedException, ClassNotFoundException {
		if(args.length<1){
			System.out.print("please set input path ");
			System.exit(1);
		}
		Configuration conf=new Configuration();
		conf.addResource("sequoiadb-hadoop.xml");
		Job job=Job.getInstance(conf);
		job.setJarByClass(HdfsSequoiadbMR.class);
		job.setJobName("HdfsSequoiadbMR");
		job.setInputFormatClass(TextInputFormat.class);
		job.setOutputFormatClass(SequoiadbOutputFormat.class);
		TextInputFormat.setInputPaths(job, new Path(args[0]));

		job.setMapperClass(MobileMapper.class);	
		job.setReducerClass(MobileReducer.class);
		
		job.setMapOutputKeyClass(Text.class);
		job.setMapOutputValueClass(IntWritable.class);
		
		job.setOutputKeyClass(NullWritable.class);		
		job.setOutputValueClass(BSONWritable.class);
		
		job.waitForCompletion(true);
	}
}
