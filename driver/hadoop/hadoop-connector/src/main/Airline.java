package main;

import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.apache.commons.cli.BasicParser;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import com.sequoiadb.hadoop.io.BSONWritable;
import com.sequoiadb.hadoop.mapreduce.SequoiadbInputFormat;
import com.sequoiadb.hadoop.mapreduce.SequoiadbOutputFormat;
import com.sequoiadb.hadoop.util.SequoiadbConfigUtil;

public class Airline {
	private static Log log = LogFactory.getLog(Airline.class);

	private static Options options = null;
	private static CommandLineParser parser = null;
	private static HelpFormatter help = null;

	private static void helpPrint() {
		help = new HelpFormatter();
		help.printHelp("OptionsTip", options);
		System.exit(1);
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {

		String sdbUrls = "localhost:11810";
		String inSpaceName = null;
		String inCollectionName = null;
		String outSpaceName = null;
		String outCollectionName = null;

		String bulk_num = "512";
		String ignore = null;
		String insertType = null;

		try {
			parser = new BasicParser();
			options = new Options();

			options.addOption("H", "help", false,
					"Print this usage information");
			options.addOption("u", "urls", true,
					"sequoiadb url list (default localhost:11810)");
			options.addOption(null, "incs", true,
					"The collection space name of input");
			options.addOption(null, "incl", true,
					"The collection name of input");
			options.addOption(null, "outcs", true,
					"The collection space name of output");
			options.addOption(null, "outcl", true,
					"The collection name of output");
			options.addOption("n", "bulkNum", true,
					"BSON bulk number (default 512)");
			options.addOption("i", "ignore", false,
					"ignore the error data (default : false)");
			options.addOption("t", "type", true,
					"choose insert type , 'insert'/'upsert' (default insert)");

			CommandLine cl = parser.parse(options, args);

			if (0 == args.length || cl.hasOption("H")) {
				helpPrint();
			}

			
			if (cl.hasOption('u')) {
				sdbUrls = cl.getOptionValue('u');
			}

		
			if (cl.hasOption("incs")) {
				inSpaceName = cl.getOptionValue("incs");
			} else {
				helpPrint();
			}

			if (cl.hasOption("incl")) {
				inCollectionName = cl.getOptionValue("incl");
			} else {
				helpPrint();
			}

			if (cl.hasOption("outcs")) {
				outSpaceName = cl.getOptionValue("outcs");
			} else {
				helpPrint();
			}

			if (cl.hasOption("outcl")) {
				outCollectionName = cl.getOptionValue("outcl");
			} else {
				helpPrint();
			}



			if (cl.hasOption("n")) {
				bulk_num = cl.getOptionValue("n");
				String reg = "[^0-9]";
				Pattern pattern = Pattern
						.compile(reg, Pattern.CASE_INSENSITIVE);
				Matcher matcher = pattern.matcher(bulk_num);

				if (matcher.find()) {
					System.out.println("bulk_num only can be number");
					helpPrint();
				}
			}

			ignore = "false";
			if (cl.hasOption("i")) {
				ignore = "true";
			}

			insertType = "insert";
			if (cl.hasOption("t")) {
				insertType = cl.getOptionValue("t");
				if (!(insertType.equalsIgnoreCase("insert") || insertType
						.equalsIgnoreCase("upsert"))) {
					System.out
							.println("insertType only can be 'insert' or 'upsert'");
					helpPrint();
				}
			}

		} catch (Exception e) {
			helpPrint();
		}
		
		Configuration conf = new Configuration();
		Configuration.addDefaultResource("hdfs-default.xml");
		Configuration.addDefaultResource("hdfs-site.xml");
		Configuration.addDefaultResource("mapred-default.xml");
		Configuration.addDefaultResource("mapred-site.xml");
		conf.set("fs.hdfs.impl", org.apache.hadoop.hdfs.DistributedFileSystem.class.getName());
		conf.set(SequoiadbConfigUtil.JOB_INPUT_URL, sdbUrls);
		conf.set(SequoiadbConfigUtil.JOB_OUTPUT_URL, sdbUrls);
		conf.set(SequoiadbConfigUtil.JOB_IN_COLLECTIONSPACE, inSpaceName);
		conf.set(SequoiadbConfigUtil.JOB_IN_COLLECTION, inCollectionName);
		conf.set(SequoiadbConfigUtil.JOB_OUT_COLLECTIONSPACE, outSpaceName);
		conf.set(SequoiadbConfigUtil.JOB_OUT_COLLECTION, outCollectionName);
		conf.set(SequoiadbConfigUtil.JOB_OUT_BULKNUM, bulk_num);
		
		
		
		try {
			Job job = new Job(conf);
			job.setJarByClass(Airline.class);
			job.setJobName("Airline");
			job.setMapperClass(AirlineMapper.class);
			job.setReducerClass(AirlineReducer.class);	

			
			job.setOutputKeyClass(Text.class);		
			job.setOutputValueClass(BSONWritable.class);
			
			job.setMapOutputKeyClass(Text.class);
			job.setMapOutputValueClass(BSONWritable.class);
			job.setNumReduceTasks(12);
			job.setInputFormatClass(SequoiadbInputFormat.class);
			FileOutputFormat.setOutputPath(job,new Path("/tmp"));
			job.setOutputFormatClass(SequoiadbOutputFormat.class);
			job.waitForCompletion(true);
		} catch (IOException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
		}

	}


}
