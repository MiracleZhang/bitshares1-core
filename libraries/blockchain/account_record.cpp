#include <bts/blockchain/account_record.hpp>
#include <bts/blockchain/chain_interface.hpp>

namespace bts { namespace blockchain {

    bool account_record::is_delegate()const
    {
        return !signing_key_history.empty();
    }

    bool account_record::is_retracted()const
    {
        return active_key() == public_key_type();
    }

    address account_record::active_address()const
    {
        return address( active_key() );
    }

    void account_record::set_active_key( const time_point_sec now, const public_key_type& new_key )
    { try {
        FC_ASSERT( now != fc::time_point_sec() );
        active_key_history[ now ] = new_key;
    } FC_CAPTURE_AND_RETHROW( (now)(new_key) ) }

    public_key_type account_record::active_key()const
    {
        FC_ASSERT( !active_key_history.empty() );
        return active_key_history.rbegin()->second;
    }

    void account_record::set_signing_key( uint32_t block_num, const public_key_type& signing_key )
    {
        signing_key_history[ block_num ] = signing_key;
    }

    public_key_type account_record::signing_key()const
    {
        FC_ASSERT( is_delegate() );
        return signing_key_history.crbegin()->second;
    }

    address account_record::signing_address()const
    {
        return address( signing_key() );
    }

    void account_record::sanity_check( const chain_interface& db )const
    { try {
        FC_ASSERT( id > 0 );
        FC_ASSERT( !name.empty() );
        FC_ASSERT( !active_key_history.empty() );
    } FC_CAPTURE_AND_RETHROW( (*this) ) }

    oaccount_record account_record::lookup( const chain_interface& db, const account_id_type id )
    { try {
        return db.account_lookup_by_id( id );
    } FC_CAPTURE_AND_RETHROW( (id) ) }

    oaccount_record account_record::lookup( const chain_interface& db, const string& name )
    { try {
        return db.account_lookup_by_name( name );
    } FC_CAPTURE_AND_RETHROW( (name) ) }

    oaccount_record account_record::lookup( const chain_interface& db, const address& addr )
    { try {
        return db.account_lookup_by_address( addr );
    } FC_CAPTURE_AND_RETHROW( (addr) ) }

    void account_record::store( chain_interface& db, const account_id_type id, const account_record& record )
    { try {
        const oaccount_record prev_record = db.lookup<account_record>( id );
        if( prev_record.valid() )
        {
            if( prev_record->name != record.name )
                db.account_erase_from_name_map( prev_record->name );

            if( prev_record->owner_address() != record.owner_address() )
                db.account_erase_from_address_map( prev_record->owner_address() );

            if( fc::ripemd160::hash( prev_record->active_key_history ) != fc::ripemd160::hash( record.active_key_history ) )
            {
                for( const auto& item : prev_record->active_key_history )
                {
                    const public_key_type& active_key = item.second;
                    db.account_erase_from_address_map( address( active_key ) );
                }
            }

            if( prev_record->is_delegate() )
            {
                if( !record.is_delegate() || fc::ripemd160::hash( prev_record->signing_key_history )
                                             != fc::ripemd160::hash( record.signing_key_history ) )
                {
                    for( const auto& item : prev_record->signing_key_history )
                    {
                        const public_key_type& signing_key = item.second;
                        db.account_erase_from_address_map( address( signing_key ) );
                    }
                }
            }
        }

        db.account_insert_into_id_map( id, record );
        db.account_insert_into_name_map( record.name, id );
        db.account_insert_into_address_map( record.owner_address(), id );

        for( const auto& item : record.active_key_history )
        {
            const public_key_type& active_key = item.second;
            if( active_key != public_key_type() ) db.account_insert_into_address_map( address( active_key ), id );
        }

        if( record.is_delegate() )
        {
            for( const auto& item : record.signing_key_history )
            {
                const public_key_type& signing_key = item.second;
                if( signing_key != public_key_type() ) db.account_insert_into_address_map( address( signing_key ), id );
            }
        }
    } FC_CAPTURE_AND_RETHROW( (id)(record) ) }

    void account_record::remove( chain_interface& db, const account_id_type id )
    { try {
        const oaccount_record prev_record = db.lookup<account_record>( id );
        if( prev_record.valid() )
        {
            db.account_erase_from_id_map( id );
            db.account_erase_from_name_map( prev_record->name );
            db.account_erase_from_address_map( prev_record->owner_address() );

            for( const auto& item : prev_record->active_key_history )
            {
                const public_key_type& active_key = item.second;
                db.account_erase_from_address_map( address( active_key ) );
            }

            if( prev_record->is_delegate() )
            {
                for( const auto& item : prev_record->signing_key_history )
                {
                    const public_key_type& signing_key = item.second;
                    db.account_erase_from_address_map( address( signing_key ) );
                }
            }
        }
    } FC_CAPTURE_AND_RETHROW( (id) ) }

} } // bts::blockchain
