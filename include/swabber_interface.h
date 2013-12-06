/*
 * functions to communicate with swabber to ban the ips detected as botnet 
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL V3.0 or later
 * 
 * Vmon: June 2013
 */

#ifndef SWABBER_INTERFACE_H
#define SWABBER_INTERFACE_H

#include <zmq.hpp>
#include <fstream>

#include "ip_database.h"
class SwabberInterface
{
 protected:
  static const std::string SWABBER_SERVER;
  static const std::string SWABBER_PORT;
  static const std::string SWABBER_BAN;

  static const std::string BAN_IP_LOG;

  static const unsigned int SWABBER_MAX_MSG_SIZE;

  //socket stuff
  zmq::context_t context;
  zmq::socket_t socket;

  std::ofstream ban_ip_list;

  //lock for writing into the socket
  TSMutex swabber_mutex;

  //to forgive ips after being banned
  IPDatabase* ip_database;

 public:
  //Error list
  enum SWABBER_ERROR {
    CONNECT_ERROR,
    SEND_ERROR
  };

  /**
     initiating the interface
  */
  SwabberInterface(IPDatabase* global_ip_db);

  /** 
     Destructor: closes and release the publication channell
   */
  ~SwabberInterface();

  /**
     Asks Swabber to ban the bot ip

     @param bot_ip the ip address to be banned
     @param banning_reason the reason for the request to be stored in the log
  */
  void ban(std::string bot_ip, std::string banning_reason);
  
};

#endif /*db_tools.h*/




