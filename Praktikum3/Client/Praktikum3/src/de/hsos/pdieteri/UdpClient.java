package de.hsos.pdieteri;

import javax.xml.crypto.Data;
import java.io.IOException;
import java.net.*;
import java.util.Arrays;

public class UdpClient {

    private InetAddress address;
    private DatagramSocket clientSocket;
    final int port = 8999;
    UdpClient(String adress,String chunkSize, String fileName){
        try {
            address = InetAddress.getByName(adress);
            clientSocket = new DatagramSocket();

        } catch (SocketException e) {
            System.err.print("SocketExeptopn");
            e.printStackTrace();
        } catch (UnknownHostException e) {
            System.err.print("UnknownHostException");
            e.printStackTrace();
        }

    }
    public String readFromServer(){
        byte[] receiveData = new byte[65565];
        try {
            DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
            System.out.println("UdpClient::readFromServer::before receive");
            clientSocket.receive(receivePacket);
          //  System.out.println("UdpClient::readFromServer::receive: " + Arrays.toString(receivePacket.getData()));
            System.out.println("UdpClient::readFromServer::after receive");
            System.out.printf("In Data: %.250s%n", new String(receivePacket.getData(),0, receivePacket.getLength()),"UTF8");

            return new String(receivePacket.getData(), 0, receivePacket.getLength(), "UTF-8");
        } catch (SocketException e) {
            System.out.println("SocketException UdpClient::readFromServer");
        } catch (IOException e) {
            System.out.println("IOException UdpClient::readFromServer");
        }
        return null;
    }

    public boolean writeToServer(String massage){
        byte[] sendData = new byte[65565];
        sendData = massage.getBytes();
        try {

            DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, address, port);
            System.out.println("UdpClient::writeToServer::before send");
            clientSocket.send(sendPacket);
            System.out.println("UdpClient::writeToServer::send: " + Arrays.toString(sendPacket.getData()) + "length: " + sendPacket.getLength());
            System.out.println("UdpClient::writeToServer::after send");
        } catch(SocketException e) {
            System.err.println("SocketException UdpClient::writeToServer");
        } catch (IOException e) {
            System.err.println("IOException UdpClient::writeToServer");
        }

        return true;
    }

    public void close(){
        clientSocket.close();
    }
}
