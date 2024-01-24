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





import com.sequoiadb.hadoop.io.BSONWritable;
import com.sequoiadb.hadoop.mapreduce.SequoiadbInputFormat;

public class SequoiadbHdfsMR {
	/**
	 * 
	 * @author gaoshengjie
	 *  read the data, count penple in a province
	 */
	static class ProvinceMapper extends Mapper<Object, BSONWritable,IntWritable,IntWritable>{
		private static final IntWritable ONE=new IntWritable(1);
		@Override
		protected void map(Object key, BSONWritable value, Context context)
				throws IOException, InterruptedException {
			BSONObject bson=value.getBson();
			int province=(Integer) bson.get("province_code");
			context.write(new IntWritable(province), ONE);
		}
			
	}
	
	static class ProvinceReducer extends Reducer<IntWritable,IntWritable,IntWritable,LongWritable>{

		@Override
		protected void reduce(IntWritable key, Iterable<IntWritable> values,
				Context context)
				throws IOException, InterruptedException {
			Iterator<IntWritable> iterator=values.iterator();
			long sum=0;
			while(iterator.hasNext()){
				sum+=iterator.next().get();
			}
			context.write(key,new LongWritable(sum));
		}

	}
	
	
	public static void main(String[] args) throws IOException, InterruptedException, ClassNotFoundException {
		if(args.length<1){
			System.out.print("please set  output path ");
			System.exit(1);
		}
		Configuration conf=new Configuration();
		conf.addResource("sequoiadb-hadoop.xml");
		Job job=Job.getInstance(conf);
		job.setJarByClass(SequoiadbHdfsMR.class);
		job.setJobName("SequoiadbHdfsMR");
		job.setInputFormatClass(SequoiadbInputFormat.class);
		job.setOutputFormatClass(TextOutputFormat.class);
		FileOutputFormat.setOutputPath(job, new Path(args[0]+"/result"));		
		job.setMapperClass(ProvinceMapper.class);	
		job.setReducerClass(ProvinceReducer.class);		
		job.setMapOutputKeyClass(IntWritable.class);
		job.setMapOutputValueClass(IntWritable.class);		
		job.setOutputKeyClass(IntWritable.class);		
		job.setOutputValueClass(LongWritable.class);		
		job.waitForCompletion(true);
	}
}
