#include "util/traverse_config_parser.hpp"
#include "util/json.hpp"
#include <iostream>
#include <fstream>
#include <boost/format.hpp>

using json = nlohmann::json;

#define MAX_RECUR_LEVEL 10
const std::string cfg_file = "config/traverse.json";

void traverse_config_decoder(TraverseTestCFG *tcfg, const json &db, const std::string &ttype, int t) {
  if(t > MAX_RECUR_LEVEL) {
    std::cerr << boost::format("Recursive more than %1% times when trying to decode a traverse config related to %2%") % MAX_RECUR_LEVEL % ttype << std::endl;
    return;
  }

  if(db[ttype].count("base"))
    traverse_config_decoder(tcfg, db, db[ttype]["base"].get<std::string>(), t+1);

  #define traverse_cfg_decode(name, type) \
      if(db[ttype].count(#name)) tcfg->name = db[ttype][#name].get<type>()

  traverse_cfg_decode(ntests,        uint32_t);
  traverse_cfg_decode(ntraverse,     uint32_t);
  traverse_cfg_decode(threshold,     uint32_t);
  traverse_cfg_decode(window,        uint32_t);
  traverse_cfg_decode(repeat,        uint32_t);
  traverse_cfg_decode(step,          uint32_t);
  traverse_cfg_decode(traverse_type, std::string);
}

traverse_func_t traverse_config_parser(const std::string& fn, const std::string& cfg, TraverseTestCFG *tcfg) {
  json db;
  std::ifstream db_file(fn);
  if(db_file.good()) {
    db_file >> db;
    db_file.close();
    // std::cout << db.dump(4) << std::endl;
  } else {
    std::cerr << "Fail to open config file " << fn << std::endl;
    return list_traverse(1,1);
  }

  if(!db.count("config") || !db["config"].count(cfg)) {
    std::cerr << boost::format("Fail to find the specific config `%1%'. ") % cfg << std::endl;
    return list_traverse(1,1);
  }

  std::string ttype = db["config"][cfg].get<std::string>();

  if(!db.count(ttype)) {
    std::cerr << boost::format("Fail to find the traverse type `%1%'. ") % ttype << std::endl;
    return list_traverse(1,1);
  }

  traverse_config_decoder(tcfg, db, ttype, 0);

  if(tcfg->traverse_type == "list")      return list_traverse(tcfg->window, tcfg->repeat);
  if(tcfg->traverse_type == "strategy")  return strategy_traverse(tcfg->window, tcfg->repeat, tcfg->step);
  if(tcfg->traverse_type == "round")     return round_traverse(tcfg->repeat);

  // should not reach here
  std::cerr << boost::format("Wrong traverse type `%1%'. ") % tcfg->traverse_type << std::endl;
  return list_traverse(1,1);
}
