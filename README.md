Banjax
======

Apache Traffic Server Plugin performing various anti-DDoS measures

Copyright 2013 eQualit.ie

Banjax is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see `<http://www.gnu.org/licenses/>`.

Installation
============

Installation from so file
-------------------------
Requirements:

Following standard Debian packages are required by banjax

     apt-get install libzmq libconfig++

You also need mercurial to get Google re2:

    apt-get install mercurial

Google re2 regex engine:

        hg clone https://re2.googlecode.com/hg re2
        cd re2
        make test
        make install
        make testinstall

Copy banjax.so file in:

     /usr/local/trafficserver/modules

Copy banjax.conf file in:

     /usr/local/trafficserver/modules/banjax/

Add a line

    banjax.so

to 

    /usr/local/trafficserver/conf/plugins.conf


Installation from source
------------------------
Staging branch always contain the most recent semi stable source. Master branch is laging far behind the current development.

Banjax is using automake frame work, to make and install banjax, cd into banjax directory, assuming that the plugin directory of the traffic server is

    /usr/local/trafficserver/modules

You need the following dev deb package to compile banjax in addition to libre2 described above:

    apt-get install build-essential git mercurial libzmq-dev libconfig++-dev


You also need google test framework to be installed

    wget http://googletest.googlecode.com/files/gtest-1.7.0.zip
    unzip gtest-1.7.0.zip
    cd gtest-1.7.0
    ./configure
    make
    cp lib/.libs/libgtest*.a /usr/lib

invoke following sequences of command

    libtoolize
    aclocal
    automake --add-missing
    autoconf
    ./configure --libdir=/usr/local/trafficserver/modules
    make
    make install

HACK:
if configure complains that libre2 isn't there but you are sure 
you have already installed it, then

    cp libre2.pc /usr/lib/pkgconfig/

Testing:
--------
running unittests in banjax/test will run the unittests corresponding to banjax filters.

Debugging
----------
If you would like to debug banjax in gdb you need to configure it as follow:
       ./configure --libdir=/usr/local/trafficserver/modules CXXFLAGS="-g2 -O0"

Available Filters
=================
Currently 4 filters are implemented in Banjax. One can configure each filter behavior in banjax.conf. The configuration of each filter should start with the name of the filter followed by colon, then the configuration will stays between {} followed by semi-colon. 

The order that each filter configuration appears in banjax.conf matters and  determine the order that banjax.conf run the filter. For example, there is no point to put white_lister filter at the end.

white_lister
------------
White listed IPs do not go through any other filter configured passed white_lister. For example the ip address of  monitoring programs such as nagios needs to be white listed, so bot stoppers such as challenger does not prevent them from their duty.

White listed ips need to be entered separately in double quotation in white_listed_ips array, separated by comma.

---------------
    white_lister :
    {
      white_listed_ips = ( "127.0.0.1", 
                           "5.135.71.96" );
    };

regex_banner:
-------------
regex_banner bans each request based on the specific rate they are matching regexes.

Each request consists of the following parts

METHOD uri host UA

such as

GET http://wiki/ equalit.ie "Firefox 1.0.1"

Currently the hit rate computed globally in the sense that it is  number of time that a request matches each regexes, hence for example if a request matches two different regexes, it counts as two hits.

To configure a new regex, you need to add a new rule in {} to the banned_regexes array. All parts of the rule is separated by semi-colon from each-other.

rule: human readable explanation of what rule is about. It is *not* optional.
regex: a regex to match the whole request. Pay attention that you need to put \\ when you are intending to have one \ in the regex. Also there are characters which do not get matched to "." so you need to use "[\\s\\S]" instead.

interval: The span of time in seconds when you want banjax keep record of the hit.

hits_per_interval: the number of hit per interval that results in banning 

Banjax bans based on 1/1000*hit_per_interval per millisecond rate. Hence it does not wait for number of hits to reach hits_per_interval, as soon as an ip reach the rate of 1/1000*hit_per_interval per millisecond it will get banned.

If you want to ban a specific regex in first apperance you need to set hits_per_interval = 0

Sample Attacks:
---------------
* If the bot always requesting "http://host.com/vmon" you can use the first rule.
* If a bot requesting pages with rate of higher than 100 request per minute and you want to ban any IP with higher requests than that use the second rule.

---------------
    regex_banner :
    {
      banned_regexes = ( { rule = "too much veggie monster";
                           regex = ".*vmon[\\s\\S]*";
                           interval = 1;
                           hits_per_interval = 0;
                         },
                         { rule = "dos";
                           regex = "[\\s\\S]*";
                           interval = 60;
                           hits_per_interval = 100;
                         }
                       );
    };

Challenger:
-----------
Challenger serves different challenges to confirm the legitimacy of the client. Currently partial inverse SHA256 or Captcha puzzles are supported. The hash solution of the puzzle is also sent along side with the ip of the requester. It is mainly meant to be a cache busting prevention mechanism.

key: is the string from which MAC key that is used to authenticate the cookie is being used. MAC prevents the attacker from tampering with the challenge or reuse its solution for different bots.

difficulty: it determines the number of leading zeros that the  SHA256(solution) at least should have in binary representation. Hence adding this value by 1 doubles the difficulty of the challenge. However in current version only multiple of four bits is supported as difficulty hence it can be 4,8,16,... each one is 16 times harder than proceeding one.

For each host then the user needs to specifies the following parameters:

name: the host name as it appears in the url, "www.host.com" and "host.com" are treated as two different hosts and needs two separate config records.

validity_period: determine how often the challenger should re-challenge a client in seconds. When under cache-busting attack, it is advisable to decrease this number. Also because the solution is cookie based each client will be challenged once for each website. 

challenge_type: can be "captcha" or "sha_inverse" at the moment.

challenge: the name of the html file that contains the challenge. by default it is "captcha.html" and "solver.html", however user can copy these files (residing in modules/banjax/) to costumize the appearance for example for localization.

no_of_fails_to_ban (optional): If specifies, challenger reports the ip to swabber, if the ip asks for the challenge this many times and fails to solve it (ip needs two request per captcha challenges).

Sample Attacks:
---------------
* If www.equalit.ie is under cache busting attack and you want to prevent the bots from reaching the origin you can use the first set of host rules, so challenger serves captcha before reaching to the origin. Note that you need a new set of host rule for equalit.ie if both www.equalit.ie and equalit.ie are resolving to your website.

* If wiki.deflect.ca is being attacked by a botnet that is not able to run Java Script or you would like to slow down each bot request by making them solve a problem before being served, at the same time you want to ban anybody who failed to solve 10 problems, you can use the second rule. 

---------------
    challenger :
    {
      key = "thisisakey";
      difficulty = 4; #only multiple of 4 is allowed                                                                                                                                                        
      hosts = (
             { name = "www.equalit.ie";
               challenge_type = "captcha";
               challenge = "captcha.html";
               validity_period = 120;
             },
             { name = "wiki.deflect.ca";
               challenge_type = "sha_inverse";
               challenge = "solver.html";
               no_of_fails_to_ban = 10;
               validity_period = 120;}
            )
    };


BotSniffer
==========
Bot sniffer reports the detail of each transaction to BotBanger to test the requester against a pre-learned model to see if the behavior of the ip resembles a bot. The only config is the zmq socket port where BotSniffer should publish the log into:

Sample Attacks:
---------------
* If you have Learn2ban model for your attack and Botbanger is running on your edge listening to port 22621 then you can add the following to banjax.conf to inform BotBanger about the requests to the edge

---------------
    bot_sniffer :
    {
       botbanger_port = 22621;
    };

How To Write A Filter
=====================

To add your own new filter to banjax you need to follow these steps:

1) Inherit from BanjaxFilter.
2) Add the constant representing your filter to the FilterIDType enum in filter_list.h e.g. 
   WHITE_LISTER_FILTER_ID

3) Add the constant filter name in filter_list.h, e.g.:
   const std::string WHITE_LISTER_FILTER_NAME = "white_lister";

4) Write the constructor to set the filter name and id.

5) Override the load_config and execute to load the config of your filter and execute the operation your filter suppose to do. Override 

6) Override requested_info to return the flags of all parts your filter need to analysis the request. for example:
  uint64_t requested_info() { return 
      TransactionMuncher::IP     |
      TransactionMuncher::METHOD |
      TransactionMuncher::URL    |
      TransactionMuncher::HOST   |
      TransactionMuncher::UA;}    

6) If it is needed override generate_response to generate your response instead of serving the request.

7) in Banjax::filter_factory add the new name to the loop

8) include the filter header file in banjax.cpp

9) add the cpp file to the src/Makefile.am

If your filter needs to keep state
----------------------------------
You need to ask banjax to allocate space for your filter in ip_db. To do so add your filter id to filter_to_column array in ip_database.h:

const FilterIDType filter_to_column[] = {
  REGEX_BANNER_FILTER_ID
  };

You'll get 16 bytes of memory for each ip, if you need more memory to keep the state you need to allocate it yourself and store a pointer to it in ip_database.

You need to edit the constructor of the filter to receive a pointer to the ip_database:

    RegexManager(const std::string& banjax_dir, const libconfig::Setting& main_root, IPDatabase* global_ip_database)
    
And you need to store it in ip_database member variable of BanjaxFilter (the parent of your filter) for further use. Finally you need edit 

    Banjax::filter_factory
    
and tell banjax to send the pointer to your filter upon creation.

How does regex_banner works
---------------------------
Regex Banner uses the following method to keep the approximate rate of hit of each IP without storing every instance of hit. 

Regex banner stores the hit rate in hit/millisecond and keep the rate for the given interval and the time stamp for the beginning of the interval. Once a new hit comes in:

If the beginning time stamp till the hit time stamps is larger than the interval then it will subtracts (time stamp - now)/interval* rate from the rate.

Independently it sums the current rate with 1/(interval*1000) (rate computed in times per millisecond).
