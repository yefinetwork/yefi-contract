#include <safekeep.hpp>

void safekeep::setcycletime( const uint64_t& time )
{
   require_auth( ADMIN );
   
   check( time > 0, "Positive integer time must be set" );

   sysinfo_singleton sys_info( get_self(), get_self().value );
   if ( sys_info.exists() ) {
      auto sinfo = sys_info.get();
      check( sinfo.cycle_time != time, "Can't set same cycle time" );
      sinfo.cycle_time = time;
      sys_info.set( sinfo, get_self() );
   } else {
      sys_info.set( sysinfo{ time }, get_self() );
   }
}

void safekeep::addtoken( const name& contract ,const symbol& sym )
{
   require_auth( ADMIN );

   check( is_account( contract ), "contract account does not exist" );
   check( sym.is_valid(), "invalid symbol name" );

   tokens_table tokens( get_self(), get_self().value );
   auto existing = false;
   for( auto it = tokens.begin(); it != tokens.end(); it++ ) {
      if ( it->contract == contract && it->sym == sym ) {
         existing = true;
         break;
      }
   }

   check( !existing, "token is already exists" );

   tokens.emplace( get_self(), [&]( auto& t ) {
      t.id       = tokens.available_primary_key();
      t.contract = contract;
      t.sym      = sym;
   });
}

void safekeep::removetoken( const uint64_t& id )
{
   require_auth( ADMIN );

   tokens_table tokens( get_self(), get_self().value );
   auto token = tokens.require_find( id, "token of the id is not exists" );
   tokens.erase( token );
}

void safekeep::withdraw( const name& user, const uint64_t& starttime )
{
   require_auth( user );
   
   records_table records( get_self(), user.value );
   auto record = records.require_find( starttime, "record of the starttime is not exists" );
   check( !record->repeat, "can't withdraw when repeat is open" );
   check( current_time_point().sec_since_epoch() > record->endtime, "It is not due and cannot be retrieved" );

   action( permission_level{ get_self(), "active"_n },
            record->contract,
            "transfer"_n,
            std::make_tuple( get_self(), user, record->quantity, std::string( "withdraw token" ) )
         ).send();

   records.erase( record );
}

void safekeep::changerepeat( const name& user, const uint64_t& starttime, const bool& repeat )
{
   require_auth( user );

   records_table records( get_self(), user.value );
   auto record = records.require_find( starttime, "record of the starttime is not exists" );
   check( record->repeat != repeat, "can't change to the same repeat" );
   if ( repeat ) {
      check( record->endtime >= current_time_point().sec_since_epoch(), "It can't be re opened after the end time" );
      records.modify( record, same_payer, [&]( auto& r ) {
         r.repeat = true;
      });
   } else {
      records.modify( record, same_payer, [&]( auto& r ) {
         while ( r.endtime < current_time_point().sec_since_epoch() ) {
            r.endtime += r.cycle_time;
         }
         r.repeat  = false;
      });
   }
   
}

void safekeep::transfer_handler( const name& from, const name& to, const asset& quantity, const string& memo )
{
   if( to != get_self() ) return;
   name contract = get_first_receiver();
   symbol sym = quantity.symbol;
   check( sym.is_valid(), "invalid symbol name" );

   tokens_table tokens( get_self(), get_self().value );
   auto existing = false;
   for( auto it = tokens.begin(); it != tokens.end(); it++ ) {
      if ( it->contract == contract && it->sym == sym ) {
         existing = true;

         sysinfo_singleton sys_info( get_self(), get_self().value );
         auto sinfo = sys_info.get();
         
         records_table records( get_self(), from.value );
         records.emplace( get_self(), [&]( auto& r ) {
            r.starttime    = current_time_point().sec_since_epoch();
            r.endtime      = current_time_point().sec_since_epoch() + sinfo.cycle_time;
            r.cycle_time   = sinfo.cycle_time;
            r.contract     = contract;
            r.quantity     = quantity;
            r.repeat       = memo == "1";
         });
         break;
      }
   }

   check( existing, "This token is not supported" );
}
