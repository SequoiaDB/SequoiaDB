package com.sequoiadb.hadoop.util;

import java.util.Map.Entry;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.conf.Configured;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.util.Tool;
/**
 * 
 * 
 * @className：SequoiadbTool
 *
 * @author： gaoshengjie
 *
 * @createtime:2013年12月10日 下午3:05:54
 *
 * @changetime:TODO
 *
 * @version 1.0.0 
 *
 */
public class SequoiadbTool extends Configured implements Tool {
	private static final Log log = LogFactory.getLog( SequoiadbTool.class );
	
	private String jobname;
	
	public String getJobname() {
		return jobname;
	}

	public void setJobname(String jobname) {
		this.jobname = jobname;
	}

	@Override
	public int run(String[] arg0) throws Exception {
		final Configuration conf=getConf();
		log.info("create a conf "+conf+" on {"+this.getClass()+"? as job named:"+this.getJobname());
		
		for(final Entry<String,String> entry:conf){
			log.debug(String.format("%s=%s\n",entry.getKey(),entry.getValue()));
		}
		
		final Job job=new Job(conf,this.getJobname());
		job.setJarByClass(this.getClass());
		final Class mapper=SequoiadbConfigUtil.getMapper(conf);
		log.debug("mapper class:"+mapper);
		job.setMapperClass(mapper);
		
		final Class combiner=SequoiadbConfigUtil.getCombiner(conf);
		if(combiner!=null){
			job.setCombinerClass( combiner );
			log.debug("combiner class:"+combiner);
		}
		
		final Class reducer=SequoiadbConfigUtil.getReducer(conf);
		job.setReducerClass(reducer);
		
        job.setOutputFormatClass( SequoiadbConfigUtil.getOutputFormat( conf ) );
        job.setOutputKeyClass( SequoiadbConfigUtil.getOutputKey( conf ) );
        job.setOutputValueClass( SequoiadbConfigUtil.getOutputValue( conf ) );
        job.setInputFormatClass( SequoiadbConfigUtil.getInputFormat( conf ) );
        Class mapOutputKeyClass = SequoiadbConfigUtil.getMapperOutputKey(conf);
        Class mapOutputValueClass = SequoiadbConfigUtil.getMapperOutputValue(conf);

        if(mapOutputKeyClass != null){
            job.setMapOutputValueClass(mapOutputKeyClass);
        }
        if(mapOutputValueClass != null){
            job.setMapOutputValueClass(mapOutputValueClass);
        }
		
        final boolean verbose=SequoiadbConfigUtil.isJobVerbose( conf );
		
        final boolean background=SequoiadbConfigUtil.isJobBackground( conf );
		
        try {
			if(background){
                log.info( "Setting up and running MapReduce job in background." );
                job.submit();
                return 0;				
			}else{
                log.info( "Setting up and running MapReduce job in foreground, will wait for results.  {Verbose? "
                        + verbose + "}" );
              return job.waitForCompletion(true)? 0 : 1;
          }
		} catch (Exception e) {
            log.error( "Exception while executing job... ", e );
            return 1;
		}
	}
	

}
