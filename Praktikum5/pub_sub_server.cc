/* Pub-Sub-Server
 * Getestet unter Ubuntu 20.04 64 Bit / g++ 9.3
 */

#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <set>
#include <openssl/sha.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <sstream>

// Diese Includes werden generiert.
#include "pub_sub.grpc.pb.h"
#include "pub_sub_deliv.grpc.pb.h"
#include "pub_sub_config.h"

// Notwendige gRPC Klassen.
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

// Diese Klassen sind in den .proto Dateien definiert.
using pubsub::EmptyMessage;
using pubsub::Message;

using pubsub::PubSubDelivService;
using pubsub::PubSubService;
using pubsub::ReturnCode;
using pubsub::SubscriberAddress;
using pubsub::Topic;

using pubsub::PubSubParam;
using pubsub::SessionId;
using pubsub::UserName;
int get_sessionId()
{
  static int nonce = 0;
  return nonce++;
}
/* Hashfunktionen mit verschiedenen Parametern */
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

// Implementierung des Service
class PubSubServiceImpl final : public PubSubService::Service
{
  // TODO: Channel topic und Subscribers für diesen Server merken
  Topic topic;
  std::list<SubscriberAddress *> subscribers;
  //<username> - <ssid>
  std::map<std::string, std::string> users;
  //<session id> - <username >
  std::map<int, std::string> sessions;
  //<username>
  std::set<std::string> log;

  static std::string stringify(const SubscriberAddress &adr)
  {
    std::string s = adr.ip_address() + ":";
    s += std::to_string(adr.port());
    return s;
  }

  bool checkAuth(std::string service, std::string hash_, SessionId sid)
  {
    std::map<int, std::string>::iterator itSession = sessions.find(sid.id());
    if (itSession == sessions.end())
      return false; // die Session ist nicht registiert

    std::set<std::string>::iterator itName = log.find(itSession->second);
    if (itName == log.end())
      return false; // ist nicht eingeloggt

    std::map<std::string, std::string>::iterator itUsers = users.find(itSession->second);

    std::stringstream ss;
    ss << itSession->first << ";" << service << ";" << itUsers->second;
    std::cout <<"checkAuth: " <<ss.str() << std::endl;
    std::string hash = ss.str();
    //hash = hash_sha(hash);
    //char* h = hash_sha((char*)hash_.c_str());
    std::cout << "hash:" << hash << " --- hash_:" << hash_ << std::endl;
    if (hash != hash_)
      return false; // falsche hash daten

    return true;
  }

  Status validate(ServerContext *context, const PubSubParam *request, ReturnCode *response)
  {
    // char hashed [256];
    // sprintf(hashed, "%s;%s", ssid.id(), "");
    SessionId ssid = request->sid();
    std::map<int, std::string>::iterator sessionsItr = sessions.find(ssid.id());
    if(sessionsItr == sessions.end())
    {
      std::cout << "Zeile 116: Fehler: sessionsItr == sessions.end()\n" ;
      response->set_value(ReturnCode::SESSION_INVALID);
      return Status::CANCELLED;
    }
    std::cout << sessionsItr->second << std::endl;
    std::set<std::string>::iterator logItr = log.find(sessionsItr->second);
    if (logItr != log.end())
    {
      sessions.erase(sessionsItr);
       std::cout << "Zeile 123: Fehler: logItr == log.end()\n" ;
      response->set_value(ReturnCode::USER_ALREADY_LOGGED_IN);
      return Status::CANCELLED;
    }
    std::map<std::string, std::string>::iterator userItr = users.find(sessionsItr->second);
    if (userItr == users.end())
    {
      sessions.erase(sessionsItr);
      std::cout << "Zeile 130: Fehler: userItr == users.end()\n" ;
      response->set_value(ReturnCode::USER_NOT_EXISTS);
      return Status::CANCELLED;
    }
    std::stringstream ss;
    ss << sessionsItr->first << ";;" << userItr->second;
    char *hash = (char *)ss.str().c_str();
    hash = hash_sha(hash);
     std::cout << ss.str() << "-----"<<hash << std::endl;
    if (request->hash_string() != hash)
    {
      std::cout << "Zeile 139: Fehler: request->hash_string() != hash \n" ;
      response->set_value(ReturnCode::WRONG_PASSWORD);
      return Status::CANCELLED;
    }
    log.insert(userItr->first);
    response->set_value(ReturnCode::OK);
    return Status::OK;
  }


  Status invalidate(ServerContext *context, const SessionId *request, ReturnCode *response)
  {
    std::map<int, std::string>::iterator itSession = sessions.find(request->id());
    if (itSession == sessions.end())
    { // die Session ist nicht registiert
      response->set_value(ReturnCode::SESSION_INVALID);
      return Status::OK;
    }
    std::set<std::string>::iterator itName = log.find(itSession->second);

    log.erase(itName);
    sessions.erase(itSession);
    response->set_value(ReturnCode::OK);
    return Status::OK;
  }

  Status subscribe(ServerContext *context, const PubSubParam *request,
                   ReturnCode *reply)

  {
    if (!checkAuth("subscribe", request->hash_string(), request->sid()))
    {
      reply->set_value(ReturnCode::WRONG_HASH_FOR_SESSION);
      return Status::OK;
    }
    // TODO: Client registrieren und Info ausgeben

    SubscriberAddress subscriberRequest = request->optaddress();
    std::string receiver = stringify(subscriberRequest);
    if (subscribers.size() >= subscribers.max_size())
    {
      reply->set_value(reply->CANNOT_REGISTER);
      std::cout << "sub cannceld\n"; 
      return Status::CANCELLED;
    }

    for (SubscriberAddress *sub : subscribers)
    {
      if (subscriberRequest.ip_address() == sub->ip_address() && subscriberRequest.port() == sub->port())
      {
        std::cout << "Already Subscribed\n";
        reply->set_value(reply->CLIENT_ALREADY_REGISTERED);
        std::cout << "sub cannceld\n"; 
        return Status::CANCELLED;
      }
    }
    SubscriberAddress *newSub = new SubscriberAddress();
    newSub->set_ip_address(subscriberRequest.ip_address());
    newSub->set_port(subscriberRequest.port());
    subscribers.push_back(newSub);
    std::cout << "New Subscriber: " << newSub->ip_address() << " - " << newSub->port() << std::endl;
    reply->set_value(reply->OK);
    return Status::OK;
  }

  Status unsubscribe(ServerContext *context, const PubSubParam *request,
                     ReturnCode *reply)
  {
    if (!checkAuth("unsubscribe", request->hash_string(), request->sid()))
    {
      reply->set_value(ReturnCode::WRONG_HASH_FOR_SESSION);
      return Status::OK;
    }
    SubscriberAddress subscriberRequest = request->optaddress();

    // TODO: Client austragen und Info ausgeben
    for (auto it = subscribers.begin(); it != subscribers.end(); it++)
    {
      SubscriberAddress *sub = *it;
      if (subscriberRequest.ip_address() == sub->ip_address() && subscriberRequest.port() == sub->port())
      {
        subscribers.erase(it);
        std::cout << "Subscriber: <" << sub->ip_address() << " - " << sub->port() << " left" << std::endl;
        reply->set_value(reply->OK);
        return Status::OK;
      }
    }
    reply->set_value(reply->CANNOT_UNREGISTER);
    std::cout << "User: " << subscriberRequest.ip_address() << " not found" << std::endl;

    return Status::OK;
  }

  void handle_status(const std::string operation, Status &status)
  {
    // Status auswerten -> deliver() gibt keinen Status zurück,k deshalb nur RPC Fehler melden.
    if (!status.ok())
    {
      std::cout << "[ RPC error: " << status.error_code() << " (" << status.error_message()
                << ") ]" << std::endl;
    }
  }

  Status publish(ServerContext *context, const PubSubParam *request,
                 ReturnCode *reply)
  {
    std::cout <<"publish\n";
    // TODO: Nachricht an alle Subscriber verteilen
    // for (subscriber in subscribers) {
    //    status = subscriber.deliver(request,  reply);
    //    handle_status(status, reply);
    // }

    if (!checkAuth("publish", request->hash_string(), request->sid()))
    {
      reply->set_value(ReturnCode::WRONG_HASH_FOR_SESSION);
      return Status::OK;
    }
    Message massageRequest = request->optmessage();

    std::stringstream ss;
    ss << "[Topic:" << topic.topic() << "]:" << massageRequest.message();
    std::string str_message = ss.str();
    std::cout << str_message << "\n";
    std::unique_ptr<PubSubDelivService::Stub> stub;

    for (SubscriberAddress *sub : subscribers)
    {
      std::stringstream str_sub;
      str_sub << sub->ip_address() << ":" << sub->port();
      Message message = Message();
      EmptyMessage emptyMessage;
      ClientContext clientContext;
      message.set_message(str_message);
      std::cout << str_sub.str() << std::endl;
      std::unique_ptr<PubSubDelivService::Stub> stub;
      ss.str("");
      ss.clear();

      for (SubscriberAddress *check : subscribers)
      {
        ss << check->ip_address() << ":" << check->port();
        // std::cout << ss.str() << "test \n";
        stub = PubSubDelivService::NewStub(grpc::CreateChannel(ss.str(), grpc::InsecureChannelCredentials()));
        stub->deliver(&clientContext, message, &emptyMessage);
        stub.release();
      }
    }
    return Status::OK;
  }

  Status get_session(ServerContext *context, const UserName *request, SessionId *response) override
  {
    response->set_id(get_sessionId());
    sessions.insert(std::pair<int, std::string>(response->id(), request->name()));
    return Status::OK;
  }
  Status set_topic(ServerContext *context, const PubSubParam *request, ReturnCode *reply)
  {
    Topic topicRequest = request->opttopic();

    std::cout << request->hash_string() << std::endl;

    if (!checkAuth("topic", request->hash_string(), request->sid()))
    {
      reply->set_value(ReturnCode::WRONG_HASH_FOR_SESSION);
      return Status::OK;
    }
    if (topicRequest.passcode() != topic.passcode())
    {
      std::cout << "Wrong Passcode" << std::endl;
      reply->set_value(reply->CANNOT_SET_TOPIC);
      return Status::OK;
    }
    topic.set_topic(topicRequest.topic());
    std::cout << "new Topic: [" << topic.topic() << "] set\n";
    reply->set_value(ReturnCode::OK);
    return Status::OK;
  }

public:
  PubSubServiceImpl()
  {
    // TODO: Topic initialisieren
    topic.set_topic("<no topic set>");
    topic.set_passcode(PASSCODE);

    std::string filename = "/home/ladmin/Schreibtisch/Praktikum 5/Hashing/hashes.txt";
    std::ifstream input(filename);

    if (input)
    {
      std::string user;
      std::string hash;
      while (!input.eof())
      {
        input >> user;
        input >> hash;
        users.insert(std::pair<std::string, std::string>(user, hash));
      }
      for (std::map<std::string, std::string>::iterator it = users.begin(); it != users.end(); ++it)
      {
        std::cout << "User: " << it->first << " Passwort: " << it->second << "\n";
      }
    }
    else
    {
      std::cerr << "Datei beim Oeffnen der Datei " << filename << "\n";
    }
  }
};
void RunServer()
{
  // Server auf dem lokalen Host starten.
  // std::string server_address(PUBSUB_SERVER_IP);
  std::string server_address("0.0.0.0"); // muss der lokale Rechner sein
  server_address += ":";
  server_address += std::to_string(PUBSUB_SERVER_PORT); // Port könnte umkonfiguriert werden

  PubSubServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Server starten ohne Authentifizierung
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Registrierung als synchroner Dienst
  builder.RegisterService(&service);
  // Server starten
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "[ Server launched on " << server_address << " ]" << std::endl;

  // Warten auf das Ende Servers. Das muss durch einen anderen Thread
  // ausgeloest werden.  Alternativ kann der ganze Prozess beendet werden.
  server->Wait();
}

int main(int argc, char **argv)
{
  // Server starten
  RunServer();
  return 0;
}