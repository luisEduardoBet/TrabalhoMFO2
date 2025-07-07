#include <assert.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

#include "bank.hpp"

using json = nlohmann::json;
using namespace std;

enum class Action {
  Init,
  Deposit,
  Withdraw,
  Transfer,
  BuyInvestment,
  SellInvestment,
  Unknown
};

Action stringToAction(const std::string &actionStr) {
  static const std::unordered_map<std::string, Action> actionMap = {
      {"init", Action::Init},
      {"deposit_action", Action::Deposit},
      {"withdraw_action", Action::Withdraw},
      {"transfer_action", Action::Transfer},
      {"buy_investment_action", Action::BuyInvestment},
      {"sell_investment_action", Action::SellInvestment}};

  auto it = actionMap.find(actionStr);
  if (it != actionMap.end()) {
    return it->second;
  } else {
    return Action::Unknown;
  }
}

int int_from_json(json j) {
  string s = j["#bigint"];
  return stoi(s);
}

map<string, int> balances_from_json(json j) {
  map<string, int> m;
  for (auto it : j["#map"]) {
    m[it[0]] = int_from_json(it[1]);
  }
  return m;
}

map<int, Investment> investments_from_json(json j) {
  map<int, Investment> m;
  for (auto it : j["#map"]) {
    m[int_from_json(it[0])] = {.owner = it[1]["owner"],
                               .amount = int_from_json(it[1]["amount"])};
  }
  return m;
}


void printBankMap(BankState &bank_map, string s){
  cout << "-----------------------" << s << "---------------" << endl; 
  cout<<  "balances: [ "; 
  for(auto it: bank_map.balances){
    cout << "{" << it.first << ":" << it.second << "},";  
  };  
  cout << "]" << endl;

  cout<< "investiments: [ "; 
  for(auto it: bank_map.investments){
    cout << "{ id:" << it.first << ", owner:" << it.second.owner << ", amount:" << it.second.amount << "},";
  }
  cout << "]" << endl;

  cout << "next_id: " << bank_map.next_id << endl; 

}



BankState bank_state_from_json(json state) {
  map<string, int> balances = balances_from_json(state["balances"]);
  map<int, Investment> investments =
      investments_from_json(state["investments"]);
  int next_id = int_from_json(state["next_id"]);
  return {.balances = balances, .investments = investments, .next_id = next_id};
}


int main() {
  for (int i = 0; i < 10000; i++) {
    cout << "Trace #" << i << endl;
    std::ifstream f("traces/out" + to_string(i) + ".itf.json");
    json data = json::parse(f);

    // Estado inicial: começamos do mesmo estado incial do trace
    BankState bank_state =
        bank_state_from_json(data["states"][0]["bank_state"]);

    auto states = data["states"];
    for (auto state : states) {
      string action = state["mbt::actionTaken"];
      json nondet_picks = state["mbt::nondetPicks"];

      string error = "";

      // Próxima transição
      switch (stringToAction(action)) {
      case Action::Init: {
        cout << "initializing" << endl;
        break;
      }
      case Action::Deposit: {
        string depositor = nondet_picks["depositor"]["value"];
        int amount = int_from_json(nondet_picks["amount"]["value"]);
        cout << endl << "ACTION: deposit(" << depositor << "," << amount << ")"<< endl;

        error = deposit(bank_state, depositor, amount);

        break;
      }

      case Action::Transfer: {
        string sender  =  nondet_picks["sender"]["value"];
        string receiver = nondet_picks["receiver"]["value"]; 
        int amount =  int_from_json(nondet_picks["amount"]["value"]);

        cout << endl << "ACTION: transfer(" << sender << "," << receiver << "," << amount << ")" << endl;

        error = transfer(bank_state, sender, receiver, amount);

        break;
      }

      case Action:: Withdraw:{

        string withdrawer =  nondet_picks["withdrawer"]["value"];
        int amount =  int_from_json(nondet_picks["amount"]["value"]);

        cout << endl << "ACTION: withdraw (" << withdrawer << "," << amount <<")"<< endl; 

        error = withdraw(bank_state, withdrawer, amount);

        break; 

      }


      case Action:: BuyInvestment:{

        string buyer =  nondet_picks["buyer"]["value"];
        int amount =  int_from_json(nondet_picks["amount"]["value"]);


        cout << endl << "ACTION: buy_investiment (" << buyer << "," << amount <<")"<< endl; 


        error = buy_investment(bank_state, buyer, amount);

        break;

      } 

      case Action:: SellInvestment:{

        string seller =  nondet_picks["seller"]["value"];
        int id =  int_from_json(nondet_picks["id"]["value"]);


        cout << endl << "ACTION: sell_investiment (" << seller << "," << id <<")"<< endl; 


        error = sell_investment(bank_state, seller, id);

        break;

      } 

      default: {
        cout << "TODO: fazer a conexão para as outras ações. Ação: " << action << endl;
        error = "";
        break;
      }
      }


    BankState expected_bank_state = bank_state_from_json(state["bank_state"]);

    
    //Print dos Estados dos Bancos

    printBankMap(bank_state, "Current");
    cout << endl; 
    printBankMap(expected_bank_state, "Expected");



    string expected_error = string(state["error"]["tag"]).compare("Some") == 0
                              ? state["error"]["value"]
                              : "";

    
    
    if(error !=  expected_error){
      cout<< "-------------ERRORS ------------------" << endl; 
      cout << "Error: " << error << endl; 
      cout << "Expected Error: " << expected_error << endl;  
      return 0;
    }; 

    }
  }
  return 0;
}



// Para rodar:  g++ -I lib teste.cpp && ./a.out 