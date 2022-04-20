package de.hsos.pdieteri;

import java.net.StandardSocketOptions;
import java.util.Arrays;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.HashMap;
import java.util.Map;

public class Main {
    //https://www.techiedelight.com/validate-ip-address-java/
    private static final String IPV4_REGEX ="^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\." +
                    "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\." +
                    "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\." +
                    "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$";
    public static void main(String[] args) {

        boolean comandCheck = false;
        String adresse = "";
        String chunkSize = "";
        String fileName = "";
        if(args.length < 3){System.out.println("Geben Sie Adresse, Chunkksize und Datei an");}
        if(Pattern.compile(IPV4_REGEX).matcher(args[0]).matches()){adresse = args[0];
        }else{
            System.out.println("Invalid IP address");
        }
        if(args[1].matches("[0-9]+")) {chunkSize = args[1];}else{System.out.println("Invalid chunk size");}
        fileName = args[2];
        //Initial Client
	    UdpClient client = new UdpClient(adresse, chunkSize, fileName);
        //Initial massage
        Long chunkSizeLong = Long.parseLong(chunkSize);
        System.out.println(chunkSizeLong);
        //Initial writeToServer
        Hsosstp.initX(client, chunkSizeLong, fileName);




        //Initial readFromServer
        do{
            String read = client.readFromServer();
            String[] arguments = read.split(";");
            System.out.println("UdpClient::Main::readFromServer:  "+ arguments[0]);
            String command = arguments[0];
            if(command != null){
                if(command.equals("HSOSSTP_INITX")) {
                    chunkSizeLong = Long.parseLong(arguments[1]);
                    fileName = arguments[2];
                    comandCheck = Hsosstp.initX(client, chunkSizeLong, fileName);
                }
                if(command.equals("HSOSSTP_SIDXX")) {
                    int ssid = Integer.parseInt(arguments[1]);
                    comandCheck = Hsosstp.sidXX(client, ssid);
                }
                if(command.equals("HSOSSTP_GETXX")) {
                    Long chnunkNr = Long.parseLong(arguments[1]);
                    comandCheck = Hsosstp.getXX(client, chnunkNr);
                }
                if(command.equals("HSOSSTP_DATAX")){
                    long chunkNo = Long.parseLong(arguments[1]);
                    long actualChunkSize = Long.parseLong(arguments[2]);;
                    String data = arguments[3];
                    // static boolean dataX(UdpClient client, long chunkNo, long actualChunkSize, String data)
                    comandCheck = Hsosstp.dataX(client, chunkNo, actualChunkSize,data);
                }
                if(command.equals("HSOSSTP_ERROR")){
                    System.out.println("HSOSSTP_ERROR");
                    System.out.println("Error: " + arguments[1]);
                }
           }else{
                System.out.println("Protocol Error: Main");
                comandCheck = false;
            }
        }while (comandCheck);
        System.out.println("Client Ende");
        client.close();
    }



























}
