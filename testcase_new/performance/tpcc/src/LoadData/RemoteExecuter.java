import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.List;

/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:RemoteExecuter.java
 * @author wangwenjing Date:2017-3-27 10:00:28
 * @version 1.00
 */

public class RemoteExecuter implements Runnable {

    private String scriptFile;
    private Process remoteProc;
    private List< String > cmdLine = new ArrayList< String >();

    public RemoteExecuter( String scriptFile, String address, String user,
            List< String > parameters ) {
        this.scriptFile = scriptFile;
        cmdLine.add( "ssh" );

        String sshAddress = String.format( "%s@%s", user, address );
        cmdLine.add( sshAddress );
        cmdLine.add( "python" );
        cmdLine.add( "-" ) ;

        for ( int i = 0; i < parameters.size(); ++i ) {
            cmdLine.add( parameters.get( i ) );
        }
    }

    private void execProc( String scriptFile, List< String > cmdLine ) {
        try {
            ProcessBuilder pb = new ProcessBuilder( cmdLine );
            pb.redirectError( ProcessBuilder.Redirect.INHERIT );

            remoteProc = pb.start();
            BufferedReader scriptReader = new BufferedReader( new FileReader(
                    scriptFile ) );
            BufferedWriter scriptWriter = new BufferedWriter(
                    new OutputStreamWriter( remoteProc.getOutputStream() ) );

            String line;
            while ( ( line = scriptReader.readLine() ) != null ) {
                scriptWriter.write( line );
                scriptWriter.newLine();
            }

            scriptWriter.close();
            scriptReader.close();
        } catch ( Exception e ) {
            System.out.println( "remote exec:" + e.getMessage() );
            e.printStackTrace();
            System.exit( 1 );
        }
    }

    public String getResult() {
        if ( remoteProc != null && remoteProc.getErrorStream() != null){
        BufferedReader errReader = new BufferedReader( new InputStreamReader(
                remoteProc.getErrorStream() ) );
        try {
            String prevLine = "";
            while ( true ){
               String line = errReader.readLine();
               if ( line == null )
                  break;
               System.out.println( line );
               prevLine = line;
            }
            return prevLine;
        } catch ( IOException e ) {

            // TODO Auto-generated catch block
            e.printStackTrace();
        }
       }
        return "";
    }

    @Override
    public void run() {

        // TODO Auto-generated method stub
        execProc( this.scriptFile, this.cmdLine );
        BufferedReader reader = new BufferedReader( new InputStreamReader(
                remoteProc.getInputStream() ) );

        while ( true ) {
            String line;
            try {
                line = reader.readLine();
                if ( line == null )
                    break;

                System.out.println( line );
            } catch ( IOException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }

}
