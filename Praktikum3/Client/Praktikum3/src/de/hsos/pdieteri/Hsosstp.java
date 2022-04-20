package de.hsos.pdieteri;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.Random;

public class Hsosstp {
    private static String filename;
    private static long chunkSize; //c++ int64
    private static int sessionKey;
    private static int start;
    private static RandomAccessFile file;
    static boolean initX(UdpClient client, long chunkSize, String filename){

        Hsosstp.chunkSize = chunkSize;
        Hsosstp.filename = filename;
        try{
            Hsosstp.file = new RandomAccessFile(filename,"rw");
            if(file.readBoolean()){ System.out.println("HSOSSTP::initX: file opened"); }

        }catch (FileNotFoundException e) {
            System.err.println("FileNotFoundExceptopn Hsosstp::initX()");
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        String init = String.format("HSOSSTP_INITX;%d;%s",chunkSize,filename);
        System.out.println("initx:  "+init);
        return client.writeToServer(init);
    }
    static boolean sidXX(UdpClient client, int sessionKey){
        Hsosstp.sessionKey = sessionKey;
        String sid = String.format("HSOSSTP_SIDXX;%d",sessionKey);
        System.out.println("HSOSSTP::sidXX:  "+sid);
        int _start = 0;
        getXX(client,_start);
        return true;
    }
    static boolean getXX(UdpClient client, long start){
        String begin = String.format("HSOSSTP_GETXX;%d;%s",Hsosstp.sessionKey,start);
        System.out.println("HSOSSTP::getXX:  "+begin);
        return client.writeToServer(begin);
    }
    static boolean dataX(UdpClient client, long chunkNo, long actualChunkSize, String data){
        try {
            //setz den file-pointer an die erste stelle oder an die stelle zum weiterlesen anhand der chunkNo.
            Hsosstp.file.seek(chunkNo * Hsosstp.chunkSize);
            Hsosstp.file.writeBytes(data);
            if (actualChunkSize == chunkSize) {
                getXX(client, chunkNo + 1);
                return true;
            }else{
                Hsosstp.file.close();
                return false;
            }
        } catch (IOException e) {
            System.err.println("IOExecptopm Hsosstp::dataX()");
            return false;
        }
    }

    static boolean error(UdpClient client, String errCode){
        if(errCode.equals("FNF")) {
            System.err.println("angeforderte Datei kann nicht gefunden werden Hsosstp::error()");
        }else if(errCode.equals("CNF")){
            System.err.println("angeforderter Chunk kann nicht transferiert werden Hsosstp::error()");
        }else if(errCode.equals("NOS")){
            System.err.println("keine Session vorhanden Hsosstp::error()");
        }
        return false;
    }

}
