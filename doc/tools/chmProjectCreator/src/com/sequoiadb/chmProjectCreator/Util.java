package com.sequoiadb.chmProjectCreator;

import java.io.BufferedReader;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;

public class Util {
    private static int increasingNum = 1000;
    public static void closeAllInputReader(BufferedReader bufferReader, InputStreamReader reader,
            InputStream input ){
        if (null != bufferReader){
            try {
                bufferReader.close();
            }
            catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
        
        if (null != reader){
            try {
                reader.close();
            }
            catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
        
        if (null != input){
            try {
                input.close();
            }
            catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }
    
    public static int increaseNum(){
        increasingNum++;
        return increasingNum;
    }

    public static void closeAllOutputWriter(OutputStreamWriter writer,
            FileOutputStream file) {
        if (null != writer){
            try {
                writer.close();
            }
            catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
        
        if (null != file){
            try {
                file.close();
            }
            catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }
}
