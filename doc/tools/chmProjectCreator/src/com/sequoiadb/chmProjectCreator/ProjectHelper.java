package com.sequoiadb.chmProjectCreator;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.LinkedList;
import java.util.List;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.log4j.Logger;

public class ProjectHelper {
    private static Logger logger = Logger.getLogger(ProjectHelper.class);
    private String inputDir;
    private String outputDir;
    private String title;
    private String tocfile;

    public String getInputDir() {
        return inputDir;
    }

    public String getOutputDir() {
        return outputDir;
    }

    public String getTitle() {
        return title;
    }
    
    public String getTocfile() {
        return tocfile;
    }
    
    public int parseArgs(String args[]) throws IOException{
        Options opt = new Options();
        opt.addOption("h", "help", false, "help");
        opt.addOption("i", "inputDir", true, "input html directory");
        opt.addOption("o", "outputDir", true, "output project dir");
        opt.addOption("t", "title", true, "output file title");
        opt.addOption("c", "toc", true, "json configure file");
        CommandLineParser parser = new GnuParser();
        CommandLine cl = null;
        try{
            cl = parser.parse(opt, args);
        }
        catch(ParseException e){
            new HelpFormatter().printHelp("chmProjectCreator", opt);
            return -1;
        }
        
        if (cl.hasOption("h")){
            new HelpFormatter().printHelp("chmProjectCreator", opt);
            return -2;
        }
        
        if (!cl.hasOption("i")){
            System.err.println("input html directory must be specified!");
            return -1;
        }
        
        if (!cl.hasOption("o")){
            System.err.println("output project directory must be specified!");
            return -1;
        }
        
        if (!cl.hasOption("t")){
            System.err.println("title must be specified!");
            return -1;
        }
        
        if (!cl.hasOption("c")){
            System.err.println("jscon configure file must be specified!");
            return -1;
        }
        
        inputDir = cl.getOptionValue("i");
        File f = new File(inputDir);
        inputDir = f.getCanonicalPath();
        logger.info("input file=" + inputDir);
        
        outputDir = cl.getOptionValue("o");
        f = new File(outputDir);
        outputDir = f.getCanonicalPath();
        logger.info("output file=" + outputDir);
        
        title = cl.getOptionValue("t");
        
        tocfile = cl.getOptionValue("c");
        f = new File(tocfile);
        tocfile = f.getCanonicalPath();
        logger.info("toc file=" + tocfile);
        
        return 0;
    }
    
    public static List<String> getFullFileList(File dir, String files[]) throws IOException{
        List<String> fullFile = new LinkedList<String>();
        for (int i=0; i<files.length; i++) {
            fullFile.add(files[i]);
        }
        
        return fullFile;
    }
    
    public static List<String> getChildrenList(File f) throws IOException{
        if (f.isFile()) {
            return null;
        }

        String files[] = f.list(new FilenameFilter() {
            @Override
            public boolean accept(File dir, String name) {
                try {
                    File tmpFile = new File( dir.getCanonicalPath() + 
                            File.separator+ name);
                    if (tmpFile.isDirectory()){
                        return true;
                    }
                }
                catch (IOException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                if (name.endsWith("html")){
                    return true;
                }
                else{
                    return false;
                }
            }
        });
        
        return getFullFileList(f, files);
    }
}
