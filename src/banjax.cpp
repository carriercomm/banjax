/*
 * Banjex is an ATS plugin that:
 *                    enforce regex bans 
 *                    store logs in a mysql db
 *                    run SVM on the log result
 *                    send a ban request to swabber in case of banning.
 *
 * Copyright (c) 2013 eQualit.ie under GNU AGPL v3.0 or later
 * 
 * Vmon: June 2013 Initial version
 */ 

#include <ts/ts.h>
//NULL not defined in c++
#include <cstddef>
#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <iomanip>
using namespace std;

#include <re2/re2.h>
#include <zmq.hpp>

#include "util.h"
#include "banjax_continuation.h"

#include "regex_manager.h"
#include "challenge_manager.h"
#include "white_lister.h"
#include "bot_sniffer.h"

#include "banjax.h"
#include "swabber_interface.h"
#include "ats_event_handler.h"

extern TSCont Banjax::global_contp;

extern const string Banjax::CONFIG_FILENAME = "banjax.conf";

extern Banjax* ATSEventHandler::banjax;
/**
   Read the config file and create filters whose name is
   mentioned in the config file. If you make a new filter
   you need to add it inside this function
*/
void
Banjax::filter_factory(const string& banjax_dir, const libconfig::Setting& main_root)
{
  unsigned int filter_count = main_root.getLength();
  
  for(unsigned int i = 0; i < filter_count; i++) {
    string cur_filter_name = main_root[i].getName();
    BanjaxFilter* cur_filter;
    if (cur_filter_name == REGEX_BANNER_FILTER_NAME) {
      cur_filter = new RegexManager(banjax_dir, main_root, &ip_database);
    } else if (cur_filter_name == CHALLENGER_FILTER_NAME){
      cur_filter = new ChallengeManager(banjax_dir, main_root);
    } else if (cur_filter_name == WHITE_LISTER_FILTER_NAME){
      cur_filter = new WhiteLister(banjax_dir, main_root);
    } else if (cur_filter_name == BOT_SNIFFER_FILTER_NAME){
      cur_filter = new BotSniffer(banjax_dir, main_root);
    } else {
      //unrecognized filter, warning and pass
      TSDebug(BANJAX_PLUGIN_NAME, "I do not recognize filter %s requested in the config", cur_filter_name.c_str());
      continue;
    }

    for(unsigned int i = BanjaxFilter::HTTP_START; i < BanjaxFilter::TOTAL_NO_OF_QUEUES; i++) {
      if (cur_filter->queued_tasks[i]) {
        TSDebug(BANJAX_PLUGIN_NAME, "active task %s %u", cur_filter->BANJAX_FILTER_NAME.c_str(), i);
        task_queues[i].push_back(FilterTask(cur_filter,cur_filter->queued_tasks[i]));
      }
    }

    filters.push_back(cur_filter);
  }
}

Banjax::Banjax()
  :all_filters_requested_part(0)
{
  //Everything is static in ATSEventHandle so it is more like a namespace
  //than a class (we never instatiate from it). so the only reason
  //we have to create this object is to set the static reference to banjax into 
  //ATSEventHandler, it is somehow the acknowledgementt that only one banjax 
  //object can exist
  ATSEventHandler::banjax = this;

  /* create an TSTextLogObject to log blacklisted requests to */
  TSReturnCode error = TSTextLogObjectCreate(BANJAX_PLUGIN_NAME, TS_LOG_MODE_ADD_TIMESTAMP, &log);
  if (!log || error == TS_ERROR) {
    TSDebug(BANJAX_PLUGIN_NAME, "error while creating log");
  }
  
  TSDebug(BANJAX_PLUGIN_NAME, "in the beginning");
  
  global_contp = TSContCreate(ATSEventHandler::banjax_global_eventhandler, ip_database.db_mutex);

  BanjaxContinuation* cd = (BanjaxContinuation *) TSmalloc(sizeof(BanjaxContinuation));
  cd = new(cd) BanjaxContinuation(NULL); //no transaction attached to this cont
  TSContDataSet(global_contp, cd);

  cd->contp = global_contp;

  TSHttpHookAdd(TS_HTTP_TXN_START_HOOK, global_contp);

  //creation of filters happen here
  read_configuration();

  //now Get rid of inactives events
  for(unsigned int cur_queue = BanjaxFilter::HTTP_START; cur_queue < BanjaxFilter::TOTAL_NO_OF_QUEUES; cur_queue++, ATSEventHandler::banjax_active_queues[cur_queue] = task_queues[cur_queue].empty() ? false : true);

  //Ask each filter what part of http transaction they are interested in
  for(list<BanjaxFilter*>::iterator cur_filter = filters.begin(); cur_filter != filters.end(); cur_filter++) {
    all_filters_requested_part |= (*cur_filter)->requested_info();
    all_filters_response_part |= (*cur_filter)->response_info();
  }

}

void
Banjax::read_configuration()
{
  // Read the file. If there is an error, report it and exit.
  string sep = "/";
  string banjax_dir = TSPluginDirGet() + sep + BANJAX_PLUGIN_NAME;
  string absolute_config_file = /*TSInstallDirGet() + sep + */ banjax_dir + sep+ CONFIG_FILENAME;

  try
  {
    cfg.readFile(absolute_config_file.c_str());
  }
  catch(const libconfig::FileIOException &fioex)
  {
    TSDebug(BANJAX_PLUGIN_NAME, "I/O error while reading config file %s.", CONFIG_FILENAME.c_str());
    return;
  }
  catch(const libconfig::ParseException &pex)
  {
    TSDebug(BANJAX_PLUGIN_NAME, "Parse error while reading the config file");
    return;
  }

  filter_factory(banjax_dir, (const libconfig::Setting&)cfg.getRoot());

}

/* Global pointer that keep track of banjax global object */
Banjax* p_banjax_plugin;

void
TSPluginInit(int argc, const char *argv[])
{

  (void) argc; (void)argv;
  TSPluginRegistrationInfo info;

  info.plugin_name = (char*) BANJAX_PLUGIN_NAME;
  info.vendor_name = (char*) "eQualit.ie";
  info.support_email = (char*) "info@deflect.ca";

  if (TSPluginRegister(TS_SDK_VERSION_3_0, &info) != TS_SUCCESS) {
    TSError("Plugin registration failed. \n");
  }

  if (!check_ts_version()) {
    TSError("Plugin requires Traffic Server 3.0 or later\n");
    return;
  }

  /* create the banjax object that control the whole procedure */
  p_banjax_plugin = (Banjax*)TSmalloc(sizeof(Banjax));
  p_banjax_plugin = new(p_banjax_plugin) Banjax;

}
