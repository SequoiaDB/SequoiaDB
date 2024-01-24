package com.sequoiadb.chmProjectCreator;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map.Entry;

import org.apache.log4j.Logger;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;

import com.google.gson.Gson;
import com.sequoiadb.util.SDBMessageHelper;

public class ProjectCreator {
    private static Logger logger = Logger.getLogger(ProjectCreator.class);
    
    public ProjectCreator(){
    }
    
    private void writeEnter(OutputStreamWriter writer) throws IOException{
        writer.write("\r\n");
    }
    
    public void wrtieGerneralHeader(OutputStreamWriter writer, String inputDir, 
            String outputDir, String outputFile, String title,
            String topicFile) throws IOException{
        for( int i = 0; i < ProjectTemplate.GENERAL.length; i++ ){
            String content = ProjectTemplate.GENERAL[i];
            if ( i == ProjectTemplate.general_title_index ){
                content += title;
            }
            else if ( i == ProjectTemplate.general_rootdir_index ){
                content += inputDir + File.separator;
            }
            else if ( i == ProjectTemplate.general_default_topic_index ){
                content += topicFile;
            }
            else if ( i == ProjectTemplate.general_compiled_file_index ){
                content += (outputDir + File.separator + outputFile);
            }
            else if ( i == ProjectTemplate.general_webhelp_output_folder_index ){
                content += (outputDir + File.separator + title + File.separator);
            }
            else if ( i == ProjectTemplate.general_singlehtml_output_folder_index ){
                content += (outputDir + "singlehtml" + File.separator);
            }
            
            writer.write(content);
            writeEnter(writer);
        }
        
        writeEnter(writer);
    }
    
    public void wrtieCHMSetting(OutputStreamWriter writer, String title, String topicFile) throws IOException{
        for( int i = 0; i < ProjectTemplate.CHMSetting.length; i++ ){
            String content = ProjectTemplate.CHMSetting[i];
            if ( i == ProjectTemplate.chmsetting_title_index ){
                content += title;
            }
            else if ( i == ProjectTemplate.chmsetting_topic_index ){
                content += topicFile;
            }
            
            writer.write(content);
            writeEnter(writer);
        }
        
        writeEnter(writer);
    }
    
    public void parseTOC(String inputDir, String tocfile, List<title> titleList) throws IOException {
        StringBuffer sb = new StringBuffer();
        BufferedReader bufferReader = null;    
        InputStreamReader reader = null;
        InputStream input = null;
        String line = null;
        try {
            input = new FileInputStream(tocfile);
            reader = new InputStreamReader(input, "UTF-8");
            bufferReader = new BufferedReader(reader);

            while((line = bufferReader.readLine()) != null){
                sb.append(line);
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        finally{
            Util.closeAllInputReader(bufferReader, reader, input);
        }
        
        //System.out.println(sb.toString());
        BSONObject obj = (BSONObject) JSON.parse(sb.toString());
        BSONObject newObj = compineInputAndTOC(inputDir, obj);
        parseObj(newObj, titleList, 0, "");
    }
    
    private void parseObj(BSONObject obj, List<title> titleList, int level,
            String parentUrl) {
        BasicBSONList contents = (BasicBSONList)obj.get("contents");
        //System.out.println(contents);

        for (int i = 0; i<contents.size(); i++){
            boolean isDir = false;
            int status = 0;
            String file = "";
            String savedUrl = "";
            BasicBSONObject item = (BasicBSONObject)contents.get("" + i);
            String title = (String)item.get("cn");
            if (item.containsField("dir")){
                isDir = true;
                status = 2;
                file = (String)item.get("dir");
                savedUrl = "";
            }
            else{
                file = (String)item.get("file");
                if (parentUrl.equals("")){
                    savedUrl = file + ".html";
                }
                else{
                    savedUrl = parentUrl + File.separator + file + ".html";
                }
            }
            
            title t = new title(title, level, savedUrl, status,
                    Util.increaseNum());
            titleList.add(t);
            
            if (isDir){
                String tmpParent = "";
                if (parentUrl.equals("")){
                    tmpParent = file;
                }
                else{
                    tmpParent = parentUrl + File.separator + file;
                }
                
                parseObj(item, titleList, level+1, tmpParent);
            }
        }
    }
    
    private BasicBSONList getFileToc(String dir) throws IOException {
        BasicBSONList bsonList = new BasicBSONList();
        List<String> fileList = ProjectHelper.getChildrenList(new File(dir));

        if ( fileList.size() > 0 ) {
            addExtraFile(0, dir, fileList, bsonList);
        }

        return bsonList;
    }
    
    private void addExtraFile(int startIndex, String dir, List<String> files, 
            BasicBSONList newContents) throws IOException {
        for(int i=0; i<files.size(); i++) {
            String index = new Integer(startIndex + i).toString();
            String fileName = files.get(i);
            String fullFileName = dir + File.separator + fileName;
            
            BSONObject fileItem = new BasicBSONObject();
            File f = new File(fullFileName);
            if (f.isDirectory()) {
                BasicBSONList bsonList = getFileToc(fullFileName);
                fileItem.put("id", 0);
                fileItem.put("dir", fileName);
                fileItem.put("cn", fileName);
                fileItem.put("en", fileName);
                fileItem.put("contents", bsonList);
            }
            else {
                fileItem.put("id", 0);
                String fileNameWithoutType = fileName.substring(0, fileName.lastIndexOf("."));
                fileItem.put("file", fileNameWithoutType);
                fileItem.put("cn", fileNameWithoutType);
                fileItem.put("en", fileNameWithoutType);
            }
            
            newContents.put(index, fileItem);
        }
    }
    
    public BSONObject compineInputAndTOC(String inputDir, BSONObject tocObj) throws IOException{
        BSONObject newTocObj = new BasicBSONObject();
        
        BasicBSONList contents = (BasicBSONList)tocObj.get("contents");
        BasicBSONList newContents = new BasicBSONList();
        
        List<String> children = ProjectHelper.getChildrenList(new File(inputDir));
        for (int i=0; i<contents.size(); i++){
            String index = new Integer(i).toString();
            BasicBSONObject item = (BasicBSONObject)contents.get(index);
            boolean isDir = false;
            String fileName = null;
            if (item.containsField("dir")){
                isDir = true;
                fileName = (String)item.get("dir");
            }
            else{
                fileName = (String)item.get("file") + ".html";
            }

            children.remove(fileName);
            BSONObject newItem = new BasicBSONObject();
            newItem.put("id", item.get("id"));
            newItem.put("cn", item.get("cn"));
            newItem.put("en", item.get("en"));
            if (isDir) {
                String newDir = inputDir + File.separator + fileName;
                BSONObject tmpObj = compineInputAndTOC(newDir, item);
                newItem.put("contents", tmpObj.get("contents"));
                newItem.put("dir", item.get("dir"));
            }
            else{
                newItem.put("file", item.get("file"));
            }
            
            newContents.put(index, newItem);
        }
        
        if (children.size() > 0) {
            addExtraFile(contents.size(), inputDir, children, newContents);
        }
        
        newTocObj.put("contents", newContents);
        return newTocObj;
    }

    public void writeTopics(OutputStreamWriter writer, List<title> list) throws IOException{
        writer.write(ProjectTemplate.topics);
        writeEnter(writer);
        writer.write(ProjectTemplate.titlelist + "=" + list.size());
        writeEnter(writer);
        
        for( int i = 0; i < list.size(); i++ ){
            title t = list.get(i);
            String content = ProjectTemplate.titlelist_title + "." + i + "=" +
                    t.getTitle();
            writer.write(content);
            writeEnter(writer);
            
            content = ProjectTemplate.titlelist_level + "." + i + "=" +
                    t.getLevel();
            writer.write(content);
            writeEnter(writer);
            
            content = ProjectTemplate.titlelist_url + "." + i + "=" +
                    t.getUrl();
            //output.write(content.getBytes("gb2312"));
            writer.write(content);
            //output.write(content.getBytes("gbk"));
            writeEnter(writer);
            
            content = ProjectTemplate.titlelist_icon + "." + i + "=" +
                    t.getIcon();
            writer.write(content);
            writeEnter(writer);
            
            content = ProjectTemplate.titlelist_status + "." + i + "=" +
                    t.getStatus();
            writer.write(content);
            writeEnter(writer);
            
            content = ProjectTemplate.titlelist_keywords + "." + i + "=" +
                    t.getKeywords();
            writer.write(content);
            writeEnter(writer);
            
            content = ProjectTemplate.titlelist_contextnumber + "." + i + "=" +
                    t.getContextNumber();
            writer.write(content);
            writeEnter(writer);
            
            content = ProjectTemplate.titlelist_applytemp + "." + i + "=" +
                    t.getApplyTemp();
            writer.write(content);
            writeEnter(writer);
            
            content = ProjectTemplate.titlelist_expanded + "." + i + "=" +
                    t.getExpand();
            writer.write(content);
            writeEnter(writer);
            
            content = ProjectTemplate.titlelist_kind + "." + i + "=" +
                    t.getKind();
            writer.write(content);
            writeEnter(writer);
        }
    }
    
    public void doCreate(ProjectHelper helper){
        String inputDir = helper.getInputDir();
        String outputDir = helper.getOutputDir();
        String title = helper.getTitle();
        String toc = helper.getTocfile();

        logger.info("start to create chm project");
        
        String outputProject = outputDir + File.separator + title + ".wcp";
        String outputChm = title + ".chm";
        String topicFile = "release_note.html";

        FileOutputStream file = null;
        OutputStreamWriter writer = null;
        try {
            file = new FileOutputStream(outputProject);
            writer = new OutputStreamWriter(file, "UTF-8");
            
    
            this.wrtieGerneralHeader(writer, inputDir, outputDir, outputChm, 
                    title, topicFile);
            this.wrtieCHMSetting(writer, title, topicFile);
            
            List<title> list = new ArrayList<title>();
            this.parseTOC(inputDir, toc, list);
    
            this.writeTopics(writer, list);
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        finally{
            Util.closeAllOutputWriter(writer, file);
        }
    }
    
    public static void main(String args[]) throws IOException{
        ProjectHelper helper = new ProjectHelper();
        int result = helper.parseArgs(args);
        if(0 != result){
            return;
        }
        
        ProjectCreator creator = new ProjectCreator();
        creator.doCreate(helper);
    }
}

