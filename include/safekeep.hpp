#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/dispatcher.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>

#include <string>
#include <stdio.h>

#define ADMIN    "admin.test"_n   // administrator

using namespace std;
using namespace eosio;

/**
 * 
 */
class [[eosio::contract("safekeep")]] safekeep : public contract {
   public:
      using contract::contract;

      /**
       * Set system cycle time
       *
       * @param time - the cycle time.
       */
      [[eosio::action]]
      void setcycletime( const uint64_t& time );

      /**
       * Add token supported by the system
       *
       * @param contract - contract name of the token
       * @param sym - symbol of the token
       */
      [[eosio::action]]
      void addtoken( const name& contract ,const symbol& sym );

      /**
       * Remove the token supported by the system
       *
       * @param id - id of the token in the table
       */
      [[eosio::action]]
      void removetoken( const uint64_t& id );

      /**
       * Withdrawal token after completion
       *
       * @param user - the user account name
       * @param starttime - record starttime
       */
      [[eosio::action]]
      void withdraw( const name& user, const uint64_t& starttime );

      /**
       * Modify multiple switch
       *
       * @param user -the user's account name
       * @param starttime - record starttime
       * @param repeat - repeat status of the record
       */
      [[eosio::action]]
      void changerepeat( const name& user, const uint64_t& starttime, const bool& repeat );

      // Transfer callback method for other contracts
      [[eosio::on_notify("*::transfer")]] 
      void transfer_handler(const name& from, const name& to, const asset& quantity, const string& memo );

      // Judge whether the record exists
      static bool is_record_exist( const name& contract_account, const name& user, const uint64_t& starttime )
      {
         records_table records( contract_account, user.value );
         const auto& record = records.find( starttime );
         return record != records.end();
      }

      
      using setcycletime_action = eosio::action_wrapper<"setcycletime"_n, &safekeep::setcycletime>;
      using addtoken_action = eosio::action_wrapper<"addtoken"_n, &safekeep::addtoken>;
      using removetoken_action = eosio::action_wrapper<"removetoken"_n, &safekeep::removetoken>;
      using withdraw_action = eosio::action_wrapper<"withdraw"_n, &safekeep::withdraw>;
      using changerepeat_action = eosio::action_wrapper<"changerepeat"_n, &safekeep::changerepeat>;

   private:

      /**
       * system config info
       */
      struct [[eosio::table]] sysinfo {
         uint64_t  cycle_time;
      };
      
      /**
       * suported tokens
       */
      struct [[eosio::table]] token {
         uint64_t                   id;
         name                       contract;
         symbol                     sym;

         uint64_t        primary_key()const { return id; }
      };

      /**
       * the keep records table
       */
      struct [[eosio::table]] record {
         uint64_t                starttime;
         uint64_t                endtime;
         uint64_t                cycle_time;
         name                    contract;
         asset                   quantity;
         bool                    repeat;

         uint64_t        primary_key()const { return starttime; }
      };

      typedef singleton< "sysinfo"_n, sysinfo > sysinfo_singleton;
      typedef multi_index< "tokens"_n, token > tokens_table;
      typedef multi_index< "records"_n, record > records_table;
};

