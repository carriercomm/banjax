/*
*  Part of regex_ban plugin: is the abstract class that banjax is inherited from
*
*  Vmon: May 2013: Initial version.
*/
#ifndef BANJAX_FILTER_H
#define BANJAX_FILTER_H

#include <assert.h>
#include <libconfig.h++>

#include "banjax_common.h"
#include "transaction_muncher.h"
#include "filter_list.h"
#include "ip_database.h"

class BanjaxFilter;
class FilterResponse
{
public:
   enum ResponseType {
        GO_AHEAD_NO_COMMENT,
        I_RESPOND,
        NO_WORRIES_SERVE_IMMIDIATELY,
        CALL_OTHER_FILTERS,
        CALL_ME_ON_RESPONSE
   };

  unsigned int response_type;
  void* response_data;
  typedef char* (BanjaxFilter::*ResponseGenerator) (const TransactionParts& transactionp_parts, const FilterResponse& response_info);

  ResponseGenerator response_generator;

  FilterResponse(unsigned int cur_response_type = GO_AHEAD_NO_COMMENT, void* cur_response_data = NULL, ResponseGenerator response_gen_func = NULL)
    : response_type(cur_response_type), response_data(cur_response_data), response_generator(response_gen_func)
  {}

};
class BanjaxFilter;

typedef FilterResponse (BanjaxFilter::*FilterTaskFunction) (const TransactionParts& transactionp_parts);

struct  FilterTask
{
  BanjaxFilter* filter;
  FilterTaskFunction task;

  FilterTask(BanjaxFilter* cur_filter, FilterTaskFunction cur_task)
    : filter(cur_filter), task(cur_task) {}

  //Default constructor just to make everthing null
  FilterTask()
    : filter(NULL),
      task(NULL) {}
      
};

class BanjaxFilter
{
 protected:
  IPDatabase* ip_database;

  /**
     It should be overriden by the filter to load its specific configurations
     
     @param banjax_dir the directory which contains banjax config files
     @param cfg the object that contains the configuration of the filter
  */
  virtual void load_config(libconfig::Setting& cfg) {(void) cfg;assert(0);};
  
 public:
  const unsigned int BANJAX_FILTER_ID;
  const std::string BANJAX_FILTER_NAME;

  /**
     this is a set of pointer to functions that the filter
     needs to be called on each queue, we are basically
     re-wrting the virtual table, but that makes more sense 
     for now.
  */
  typedef char* (BanjaxFilter::*ResponseGenerator) (const TransactionParts& transactionp_parts, const FilterResponse& response_info);
  enum ExecutionQueue {
    HTTP_START,
    HTTP_REQUEST,
    HTTP_READ_CACHE,
    HTTP_SEND_TO_ORIGIN,
    HTTP_RESPONSE,
    HTTP_CLOSE,
    TOTAL_NO_OF_QUEUES
  };
  
  FilterTaskFunction queued_tasks[BanjaxFilter::TOTAL_NO_OF_QUEUES];
  
  /**
     A disabled filter won't run, 
     Banjax calls the execution function of an enabled filter
     A controlled filter can be run in request of other filters
   */
  enum Status {
    disabled,
    enabled,
    controlled,
  };

  Status filter_status;
    
  /**
     receives the db object need to read the regex list,
     subsequently it reads all the regexs

  */
 BanjaxFilter(const std::string& banjax_dir, const libconfig::Setting& main_root, unsigned int child_id, std::string child_name)
    : BANJAX_FILTER_ID(child_id),
      BANJAX_FILTER_NAME(child_name),
    queued_tasks()
  {
    (void) banjax_dir; (void) main_root; ip_database = NULL;
  }

  /**
     Filter should call this function cause nullify in constructor does not 
     work :( then setting their functions
   */
  virtual void set_tasks()
  {
    for(unsigned int i = 0; i < BanjaxFilter::TOTAL_NO_OF_QUEUES; i++) {
      queued_tasks[i] = NULL;
    }
  }

  /**
     destructor: just to tell the compiler that the destructor is
     virtual
  */
  virtual ~BanjaxFilter()
    {
    }

  /**
     needs to be overriden by the filter
     It returns a list of ORed flags which tells banjax which fields
     should be retrieved from the request and handed to the filter.
  */
  virtual uint64_t requested_info() = 0;

  /**
     needs to be overriden by the filter
     It returns a list of ORed flags which tells banjax which fields
     should be retrieved from the response and handed to the filter.
  */
  virtual uint64_t response_info() {return 0;}

  /**
     needs to be overriden by the filter
     It returns a list of ORed flags which tells banjax which fields
     should be retrieved from the request and handed to the filter.

     @return if return the response_type of FilterResponse is 
     GO_AHEAD_NO_COMMENT then it has no effect on the transacion

     TODO: the return should be able to tell banjax to call another
     filter to run specific action. For example if botsniffer think 
     that the ip might be bot then should be able to ask the challenger
     to challenge it.

  */
  virtual FilterResponse execute(const TransactionParts& transaction_parts) = 0;
  
  /**
     The functoin will be called if the filter reply with I_RESPOND
   */
  virtual char* generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
  { 
    //Just in case that the filter has nothing to do with the response
    //we should not make them to overload this
    //but shouldn't be called anyway.
    (void) transaction_parts; (void) response_info;
    TSDebug(BANJAX_PLUGIN_NAME, "You shouldn't have called me at the first place.");
    assert(NULL);
  }

  /**
     Overload if you want this filter be called during the response
     the filter should return CALL_ME_ON_RESPONSE when it is executed 
     during request
   */
  virtual FilterResponse execute_on_response(const TransactionParts& transaction_parts)
  {
    (void) transaction_parts;
    return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
  }

};
  
#endif /* regex_manager.h */

