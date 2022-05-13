/* Pub-Sub-Client
 * Implementiert eine interaktive Shell, in die Kommandos eingegeben werden können.
 * Getestet unter Ubuntu 20.04 64 Bit / g++ 9.3
 * @hje
 */

#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <stdio.h>

#include <grpcpp/grpcpp.h>
#include <openssl/sha.h>
//hash methode
#include "Hashing/sha_hashing.c"

// Diese Includes werden generiert.
#include "pub_sub.grpc.pb.h"
#include "pub_sub_common.grpc.pb.h"
#include "pub_sub_config.h"

#include <unistd.h>

// Notwendige gRPC Klassen im Client.
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

// Diese Klassen sind in den .proto Dateien definiert.
using pubsub::EmptyMessage;
using pubsub::Message;
using pubsub::SubscriberAddress;
using pubsub::PubSubService;
using pubsub::ReturnCode;
using pubsub::Topic;

using pubsub::UserName;
using pubsub::SessionId;
using pubsub::PubSubParam;
/**** Dies muss editiert werden! ****/
char receiverExecFile[] = RECEIVER_EXEC_FILE;

/* TODO: noch notwendig? */
void trim(std::string &s)
{
    /* erstes '\n' durch '\0' ersetzen */
    for (int i = 0; i < s.length(); i++)
    {
        if (s[i] == '\n')
        {
            s[i] = '\0';
            break;
        }
    }
}
  /* Hashfunktionen mit verschiedenen Parametern */
  /*
  char *hash_sha(char *str)
  {
    unsigned char result[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char *)str, strlen(str), result);
    char hash_val[96];
    hash_val[0] = '\0';
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
      char tmp[3];
      sprintf(tmp, "%02x", result[i]);
      strcat(hash_val, tmp);
    }
    // printf ("hashval: %s\n", hash_val);
    return strdup(hash_val);
  }
*/
// Argumente für Aufruf: der Client kann mit --target aufgerufen werden.
class Args
{
public:
    std::string target;
    
    Args(int argc, char **argv)
    {
        target = PUBSUB_SERVER_IP; 
        target += ":";
        target += std::to_string (PUBSUB_SERVER_PORT);

        // Endpunkt des Aufrufs ueber --target eingestellt?
        std::string arg_str("--target");
        for (int i = 1; i < argc; i++)
        {
            std::string arg_val = argv[i];
            size_t start_pos = arg_val.find(arg_str);
            if (start_pos != std::string::npos)
            {
                start_pos += arg_str.size();
                if (arg_val[start_pos] == '=')
                {
                    target = arg_val.substr(start_pos + 1);
                }
                else
                {
                    std::cout << "Error: set server address via --target=" << std::endl;
                    std::cout << target << " will be used instead." << std::endl;
                }
            }
        }
    }
};

static std::string get_receiver_ip() {
    // Hier wird eine statisch konfigurierte Adresse zurueck gegeben. 
    // Diese koennte auch dynamisch ermittelt werden. 
    // Dann aber: alle Adapter und die dafuer vorgesehenen IP Adresse durchgehen; 
    // eine davon auswaehlen. Das funktioniert aber auch nur im lokalen Netz.
    // Was ist wenn NAT verwendet wird? Oder Proxies? 
    return PUBSUB_RECEIVER_IP;
}
static int get_receiver_port() {
    return PUBSUB_RECEIVER_PORT;
}

class PubSubClient
{
private:
    void print_prompt (const Args& args) {
        std::cout << "Pub / sub server is: " << args.target << std::endl;
    }
    
    void print_help()
    {
        std::cout << "Client usage: \n";
        std::cout << "     'quit' to exit;\n";
        std::cout << "     'set_topic' to set new topic;\n";
        std::cout << "     'validate' to login\n";
        std::cout << "     'invalidate' to log off\n";
        std::cout << "     'subscribe' subscribe to server & register / start receiver;\n";
        std::cout << "     'unsubscribe' from this server & terminate receiver.\n";
    }

    static std::string stringify(pubsub::ReturnCode_Values value)
    {
        /* TODO: Hier sollte eine passende Status-Ausgabe generiert werden! */
        switch(value){
            case 0: return "OKAY";
            case 1: return "cant register new client";
            case 2: return "a client is registered";
            case 3: return "cant unregister client";
            case 4: return "cant set topic";
            case 5: return "no hash for session";
            case 6: return "wrong hash for session";
            case 7: return "user already logged in";
            case 8: return "session is invalid";
            case 9: return "password is wrong";
            case 10: return "user not exists";
            case 11: return "unknown Error";

        }
        return "UNKNOWN";
    }

    void handle_status(const std::string operation, Status &status, ReturnCode &reply)
    {
        // Status auswerten
        if (status.ok())
        {
            std::cout << operation << " -> " << stringify(reply.value()) << std::endl;
        }
        else
        {
            std::cout << "RPC error: " << status.error_code() << " (" << status.error_message()
                      << ")" << std::endl;
        }
    }
    char* build_hash(int id,char* name, char* userPasswdHash){
        char* hash;
        std::string nr = std::to_string(id); 
        strcat(hash,(char*)nr.c_str());
        strcat(hash,name);
        strcat(hash,userPasswdHash);
        return hash_sha(hash);
    }
public:
    PubSubClient(std::shared_ptr<Channel> channel)
        : stub_(PubSubService::NewStub(channel))
    {
    }
    
    void run_shell(const Args& args)
    {
        /* PID der Receiver Console */
        int rec_pid = -1;

        print_prompt(args);
        print_help();
        std::string topic = "[no Topic]";
        std::string cmd;
       

        do
        {
            std::cout <<topic << "> ";
            // Eingabezeile lesen
            getline(std::cin, cmd);
            // std::cin >> cmd;
            if (cmd.length() == 0)
                break;

            trim(cmd);
            if(cmd.compare("validate") == 0){
                std::string username;
                std::string passwd; 
                std::stringstream ss;

                std::cout << "username> ";
                getline(std::cin, username);
                trim(username);


                name.set_name(username);

                ClientContext cc;
                stub_->get_session(&cc, name, &sessionID);
                std::cout << "\n Session ID: " << sessionID.id() <<  "\n";


                std::cout << "password>";
                getline(std::cin, passwd);
                trim(passwd);

                ss << username << ";" << passwd;
                std::string  str = ss.str();
                char* c_str = new char[str.length() + 1];
                strcpy(c_str, str.c_str());
                authentication = hash_sha(c_str);
                delete c_str;
                std::cout << authentication << "\n";
                session_hash = authentication;
                ss.str("");
                ss.clear();
                ss << sessionID.id() << ";;"<< authentication;
                char* hash = (char*)ss.str().c_str();
                hash = hash_sha(hash);
                std::cout << ss.str() << "-----"<<hash << std::endl;

                PubSubParam pubSubParam = PubSubParam();
                pubSubParam.set_allocated_sid(&sessionID);
                pubSubParam.set_hash_string(hash);
                ClientContext context;
                ReturnCode replay;

                Status status = stub_->validate(&context,pubSubParam,&replay);

                this->handle_status("validate()", status, replay);
                pubSubParam.release_sid();
                
            }else if (cmd.compare("invalidate") == 0){
               ClientContext context;
               ReturnCode replay = ReturnCode();
               Status status = stub_->invalidate(&context,sessionID, &replay);
               this->handle_status("invalidate()",status,replay);

            }else if (cmd.compare("set_topic") == 0)
            {

                std::cout << "enter topic> ";
                // std::cin >> topic;
                getline(std::cin, topic);
                trim(topic);
                // Passcode einlesen, damit topic gesetzt werden darf.
                std::string passcode;
                std::cout << "enter passcode> ";
                getline(std::cin, passcode);
                trim(passcode);

                /* TODO: Hier den Request verschicken und Ergebnis auswerten! */
                // Platzhalter fuer Request, Kontext & Reply.
                // Muss hier lokal definiert werden, 
                // da es sonst Probleme mit der Speicherfreigabe gibt.
                Topic* request = new Topic();
                ReturnCode reply = ReturnCode();
                 // Kontext kann die barbeitung der RPCs beeinflusst werden. Wird nicht genutzt. 
                ClientContext context;
               
                // TODO: Topic fuer Server vorbereiten ...
                std::cout << topic << "--" << passcode << std::endl;
                request->set_topic(topic);
                request->set_passcode(passcode);

              
                //           ------------------param--------------------------------
                //                             ----------hash_string----------------
                //                                                  -authentication-
                // set_topic(session_id;topic;HASH(session_id;topic;HASH(user;pwd)))

                SessionId *ssid = new SessionId();
                ssid->set_id(sessionID.id());
                std::stringstream ss;
                std::cout << sessionID.id() << ";topic;" <<authentication << "\n";
                ss << sessionID.id() << ";topic;" << authentication;
                std::string hash_string = ss.str();
                           
            
                param.set_hash_string(hash_string);

                std::cout << hash_string << std::endl;

                param.set_allocated_opttopic(request);
                param.set_allocated_sid(ssid);

                Status status = stub_->set_topic(&context,param,&reply);

                // Status / Reply behandeln

                this->handle_status("set_topic()", status, reply);
            }
            else if (cmd.compare("subscribe") == 0)
            {              
                /* Ueberpruefen, ob Binary des Receivers existiert */
                if (access(receiverExecFile, X_OK) != -1)
                {
                    /* Receiver starten */
                    if ((rec_pid = fork()) < 0)
                    {
                        std::cerr << "Cannot create process for receiver!\n";
                    }
                    else if (rec_pid == 0)
                    {
                        /* Der Shell-Aufruf */
                        /* xterm -fa 'Monospace' -fs 12 -T Receiver -e ...pub_sub_deliv */
                        /* kann nicht 1:1 uebertragen werden. Bei Aufruf via exec() */
                        /* verhaelt sich das Terminal anders. */
                        /* Alternative: Aufruf von xterm ueber ein Shell-Skript. */
                        /* Allerdings haette man dann 2 Kind-Prozesse. */
                        execl("/usr/bin/xterm", "Receiver", "-fs", "14", receiverExecFile, (char *)NULL);
                        /* -fs 14 wird leider ignoriert! */
                        exit(0); /* Kind beenden */
                    }
                    
                    /* TODO: Hier den Request verschicken und Ergebnis auswerten! */
                    /* Platzhalter wie oben lokal erstellen ... */

                    // TODO: Receiver Adresse setzen ...
                    SubscriberAddress* request = new SubscriberAddress();
                    request->set_ip_address(get_receiver_ip());
                    request->set_port(get_receiver_port());
    
                    param.set_allocated_optaddress(request);
                    SessionId* ssid = new SessionId();
                    ssid->set_id(sessionID.id());
                    param.set_allocated_sid(ssid);

                    std::stringstream ss;
                    std::cout << sessionID.id() << ";subscribe;" << authentication << "\n";
                    ss << sessionID.id() << ";subscribe;" << authentication;
                    std::string hash_string = ss.str();
                    
                    param.set_hash_string(hash_string);
                    std::cout << "subscribe: " << ss.str() << std::endl; 
                    ReturnCode reply;
                    ClientContext context;
                    // TODO: RPC abschicken ...
                    Status status =  stub_->subscribe(&context,param,&reply);
                    // TODO: Status / Reply behandeln ...

                    this->handle_status("subscribe()",status, reply);
                }
                else
                {
                    std::cerr << "Cannot find message receiver executable!\n";
                    std::cerr << "Press <return> to continue";
                    char c = getc(stdin);
                    continue;
                }
            }
            else if ((cmd.compare("quit") == 0) ||
                     (cmd.compare("unsubscribe") == 0))
            {
                /* Receiver console beenden */
                if (rec_pid > 0)
                {
                    if (kill(rec_pid, SIGTERM) != 0)
                        std::cerr << "Cannot terminate message receiver!\n";
                    else
                        rec_pid = -1;
                }
                /* Bei quit muss ebenfalls ein unsubscribe() gemacht werden. */
                               
                
                /* TODO: Hier den Request verschicken und Ergebnis auswerten! */
                /* Platzhalter wie oben lokal erstellen ... */

                // TODO: Receiver Adresse setzen ...
                SubscriberAddress request;
                request.set_ip_address(get_receiver_ip());
                request.set_port(get_receiver_port());
                ReturnCode reply;
                ClientContext context;
                // TODO: RPC abschicken ...

                std::stringstream ss;
                 std::cout << sessionID.id() << ";unsubscribe;" <<authentication << "\n";
                ss << sessionID.id() << ";unsubscribe;" << authentication;
                std::string hash_string = ss.str();
               
                param.set_hash_string(hash_string);

                Status status = stub_->unsubscribe(&context, param,&reply);
                // TODO: Status / Reply behandeln ...

                this->handle_status("unsubscribe()",status, reply);
                /* Shell beenden nur bei quit */
                if (cmd.compare("quit") == 0)
                    break; /* Shell beenden */
            }
            else  /* kein Kommando -> publish() aufrufen */
            {
                /* TODO: Hier den Request verschicken und Ergebnis auswerten! */
                /* Platzhalter wie oben lokal erstellen ... */

                ClientContext context;
               /* std::string message;
                std::cout << "<enter message> ";
                // std::cin >> topic;
                getline(std::cin, message);
                trim(message);
                */

                Message* request = new Message();
                ReturnCode reply = ReturnCode();

                request->set_message(cmd);
                param.set_allocated_optmessage(request);

                std::stringstream ss;
                ss << sessionID.id() << ";publish;" << authentication;
                std::string hash_string = ss.str();
                param.set_hash_string(hash_string);
              
                //request.set_message(cmd);
                // TODO: RPC abschicken ...
                std::cout << "Test outPut for request.message(): "<< cmd  << "\t "<< request->message() << "\n";


                Status status = stub_->publish(&context, param, &reply);
                this->handle_status("publish()", status, reply);
            }
        } while (1);
    }

private:
    std::unique_ptr<PubSubService::Stub> stub_;
     char* authentication; 
       PubSubParam param;
       std::string session_hash;
       SessionId sessionID; // = SessionId();
       UserName name;
};

int main(int argc, char **argv)
{
    // Einlesen der Argumente. Der Endpunkt des Aufrufs
    // kann über die Option --target eingestellt werden.
    Args args(argc, argv);

    PubSubClient client(grpc::CreateChannel(
        args.target, grpc::InsecureChannelCredentials()));

    client.run_shell(args);

    return 0;
}
