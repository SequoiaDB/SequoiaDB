package com.sequoiadb.builddata;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

public class BuildUtils {

    public static void write2File(String path, String content) throws IOException {
        File f = new File(path);
        FileWriter out = null;
        try {
            out = new FileWriter(f, true);
            out.write(content);
        } finally {
            out.close();
        }
    }

    public static void createFile(String path) throws IOException {
        File f = new File(path);
        if (f.exists()) {
            f.delete();
        }
        f.createNewFile();
    }
}
