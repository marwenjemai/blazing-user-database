// BUD v0.0.1 BETA by Slice
// BUD v0.0.1 BETA by Slice

#include <a_samp>
#include <a_http>

 // :D

#define BUD::      BUD_
#define opt.       opt_
#define private    stock static
#define global     stock

 // :D!

#define BUD_Notice(%0)       ( printf( "BUD - Notice: " %0 ) )
#define BUD_Warning(%0)      ( printf( "BUD - Warning: " %0 ) )
#define BUD_Error(%0)        ( printf( "BUD - Error: " %0 ) )

 // This is so the constants can be defined before inclusion.

#if !defined BUD_USE_WHIRLPOOL
	#define BUD_USE_WHIRLPOOL  (false)
#endif

#if !defined BUD_MAX_DATABASE_NAME
	#define BUD_MAX_DATABASE_NAME  16
#endif

#if !defined BUD_MAX_COLUMNS
	#define BUD_MAX_COLUMNS  12
#endif

#if !defined BUD_MAX_COLUMN_NAME
	#define BUD_MAX_COLUMN_NAME  16
#endif

#if !defined BUD_CHECK_STRING_LENGTH
	#define BUD_CHECK_STRING_LENGTH  (false)
#endif

#if !defined BUD_MAX_ENTRY_STRING
	#define BUD_MAX_ENTRY_STRING  64
#endif

#if !defined BUD_MULTIGET_MAX_ENTRIES
	#define BUD_MULTIGET_MAX_ENTRIES 16
#endif

#if !defined BUD_MULTISET_MAX_ENTRIES
	#define BUD_MULTISET_MAX_ENTRIES 16
#endif

#if !defined BUD_MULTISET_MAX_STRING_SIZE
	#define BUD_MULTISET_MAX_STRING_SIZE BUD::MAX_ENTRY_STRING
#endif

#if !defined BUD_MULTISET_BUFFER_SIZE
	#define BUD_MULTISET_BUFFER_SIZE 2048
#endif

#define DEFAULT_DATABASE_NAME  "bud.db" // No need to change this; do it via the settings instead.

#define BUD_VERSION_MINOR  0
#define BUD_VERSION_MAJOR  1
#define BUD_VERSION_BUILD  2

#define INVALID_DB           (DB:0)
#define BUD_NO_RESULT        (DBResult:0)
#define BUD_INVALID_UID      (-1)
#define BUD_INVALID_RESULTS  (-1)

enum BUD::e_COLUMN_TYPES
{
	BUD::TYPE_NUMBER,
	BUD::TYPE_FLOAT,
	BUD::TYPE_STRING,
	BUD::TYPE_BINARY
};

enum BUD::e_OPTIONS
{
	opt.Database,
	opt.Asynchronous,
	opt.KeepAliveTime,
	opt.IntEntryDefault,
	opt.FloatEntryDefault,
	opt.DatabaseOpenTimeOut,
	opt.CheckForUpdates
};

enum BUD::e_SORT_ORDER
{
	BUD::SORT_ASC,
	BUD::SORT_DESC,
	BUD::SORT_ASCENDING = 0,
	BUD::SORT_DESCENDING
};

// No need to change this either; use BUD::VerifyColumn( column[ ], type ) instead.
#define DEFAULT_COLUMNS  "uid INTEGER PRIMARY KEY, name TEXT, passhash BLOB"

private
	 bool:g_bIsInitialized = false,
	      g_szDatabaseName[ BUD::MAX_DATABASE_NAME ] = "bud.db",
	      g_iColumnCount,
	      g_szColumnName[ BUD::MAX_COLUMNS ][ BUD::MAX_COLUMN_NAME ],
	   DB:g_dbKeptAlive = INVALID_DB,
	      g_iKeepAlive = 2000,
	      g_iKeepAliveTimer = -1,
	 bool:g_bAsynchronous = true,
	      g_iIntEntryDefault = 0,
	Float:g_fFloatEntryDefault = 0.0,
	      g_iDatabaseOpenTimeOut = 3000,
	 bool:g_bCheckForUpdates = true
;

// Prevent it from causing issues if WP_Hash is defined before or after this.

#if ( BUD::USE_WHIRLPOOL )
	#if defined WP_Hash
		#define WhirlpoolHash WP_Hash
	#else
		native WhirlpoolHash( buffer[ ], len = sizeof( buffer ), const str[ ] ) = WP_Hash;
	#endif
#endif

// Close the database connection when it has been unused for g_iKeepAlive ms

private bool:BUD::GetDB( )
{
	if ( g_iKeepAliveTimer == -1 )
	{
		new
			iStartTick = GetTickCount( )
		;
		
		do
			g_dbKeptAlive = db_open( g_szDatabaseName );
		while ( g_dbKeptAlive == INVALID_DB && GetTickCount( ) - iStartTick < g_iDatabaseOpenTimeOut );
		
		if ( g_dbKeptAlive == INVALID_DB )
		{
			BUD::Error( "Unable to open the database \"%s\".", g_szDatabaseName );
			
			return false;
		}
		else
		{
			if ( g_bAsynchronous )
				db_query( g_dbKeptAlive, "PRAGMA synchronous = 0" );
			else
				db_query( g_dbKeptAlive, "PRAGMA synchronous = 3" );
		}
	}
	else
	{
		KillTimer( g_iKeepAliveTimer );
	}
	
	g_iKeepAliveTimer = SetTimer( "BUD_CloseDB", g_iKeepAlive, false );
	
	return true;
}

forward BUD::CloseDB( );
public  BUD::CloseDB( )
{
	g_iKeepAliveTimer = -1;
	
	db_close( g_dbKeptAlive );
	
	g_dbKeptAlive = INVALID_DB;
}

global BUD::Setting( BUD::e_OPTIONS:iSetting, _:... )
{
	switch ( iSetting )
	{
		case opt.Database:
		{
			if ( g_bIsInitialized )
			{
				BUD::Warning( "opt.Database has to be set before BUD::Initialize( ) is called." );
				
				return;
			}
			
			for ( new i = 0; i < sizeof( g_szDatabaseName ); i++ )
			{
				g_szDatabaseName[ i ] = getarg( 1, i );
				
				if ( g_szDatabaseName[ i ] == EOS )
					break;
			}
		}
		
		case opt.Asynchronous:
		{
			new
				bool:bNewSetting = getarg( 1 ) ? true : false
			;
			
			if ( g_bAsynchronous != bNewSetting )
			{
				g_bAsynchronous = bNewSetting;
				
				if ( g_dbKeptAlive != INVALID_DB )
				{
					if ( g_bAsynchronous )
						db_query( g_dbKeptAlive, "PRAGMA synchronous = 0" );
					else
						db_query( g_dbKeptAlive, "PRAGMA synchronous = 3" );
				}
			}
		}
		
		case opt.KeepAliveTime:
		{
			g_iKeepAlive = max( 0, getarg( 1 ) );
		}
		
		case opt.IntEntryDefault:
		{
			g_iIntEntryDefault = getarg( 1 );
		}
		
		case opt.FloatEntryDefault:
		{
			g_fFloatEntryDefault = getarg( 1 );
		}
		
		case opt.DatabaseOpenTimeOut:
		{
			g_iDatabaseOpenTimeOut = getarg( 1 );
		}
		
		case opt.CheckForUpdates:
		{
			if ( g_bIsInitialized )
			{
				BUD::Warning( "opt.CheckForUpdates has to be set before BUD::Initialize( ) is called." );
				
				return;
			}
			
			g_bCheckForUpdates = getarg( 1 ) ? true : false;
		}
		
		default:
		{
			BUD::Warning( "Unknown setting ID passed in BUD::Setting (%d).", _:iSetting );
		}
	}
}

#if !( BUD::USE_WHIRLPOOL )
	#if !defined MAX_PASSWORD_LENGTH
		#define MAX_PASSWORD_LENGTH  (64)
		#define UNDEFINE_MAX_PASSWORD_LENGTH (true)
	#else
		#define UNDEFINE_MAX_PASSWORD_LENGTH (false)
	#endif
	
	stock BUD::chrfind(needle, haystack[], start = 0) // Y_Less
	{
		while (haystack[start]) if (haystack[start++] == needle) return start - 1;
		return -1;
	}
	
	stock BUD::JSCHash(const __pass[],__passhash[MAX_PASSWORD_LENGTH + 1]) // By JSC (Y_Less's dad); ported by Y_Less
	{
			static
					__charset[] = \"4YLi6pOX)Mudvbc_IFVB/8HZ\2r(fGjaN0oU9C1Wywnq*smKQRxJDhkAS|53EzglT7tPe",
					__css = 69;
			new
					__j = strlen(__pass);
			new
					__sum = __j,
					__tmp = 0,
					__i,
					__mod;
			for (__i = 0; __i < MAX_PASSWORD_LENGTH || __i < __j; __i++)
			{
					__mod = __i % MAX_PASSWORD_LENGTH;
					__tmp = (__i >= __j) ? __charset[(7 * __i) % __css] : __pass[__i];
					__sum = (__sum + BUD::chrfind(__tmp, __charset) + 1) % __css;
					__passhash[__mod] = __charset[(__sum + __passhash[__mod]) % __css];
			}
			__passhash[MAX_PASSWORD_LENGTH] = '\0';
	}
#else
	#define UNDEFINE_MAX_PASSWORD_LENGTH (false)
#endif

global bool:BUD::Initialize( )
{
	if ( g_bIsInitialized )
	{
		BUD::Notice( "Initialization aborted; BUD is already initialized." );
		
		return true;
	}
	
	#if ( BUD::USE_WHIRLPOOL )
		new szBuffer[ 129 ];
		
		WhirlpoolHash( szBuffer, _, "I like cookies." );
		
		if ( strcmp( szBuffer, "516A210D9C04CFE36568BEDA5287977572CCC618126B188B02F0BCFB92EE5B7EBA9F3D6DE2DE8FC0BB4C9E6111B93CF6482CCCD5639A94F125CFF7B829275933" ) )
			BUD::Warning( "Whirlpool isn't behaving as expected; this might cause problems with the authentication." );
	#endif
	
	if ( !g_szDatabaseName[ 0 ] )
	{
		BUD::Warning( "The database name was not specified; default will be applied (\"" DEFAULT_DATABASE_NAME "\")." );
		
		g_szDatabaseName = DEFAULT_DATABASE_NAME;
	}
	
	if ( !fexist( g_szDatabaseName ) )
	{
		BUD::Notice( "The database \"%s\" doesn't exist; it will be created.", g_szDatabaseName );
		
		if ( !BUD::CreateDatabase( ) )
			return false;
	}
	
	BUD::ReloadTableInfo( );
	BUD::IntegrityCheck( );
	
	if ( g_bCheckForUpdates )
		BUD::CheckForUpdates( );
	
	g_bIsInitialized = true;
	
	printf( "BUD v" #BUD::VERSION_MAJOR "." #BUD::VERSION_MINOR "." #BUD::VERSION_BUILD " BETA loaded." );
	
	return true;
}

global bool:BUD::Exit( )
{
	if ( g_iKeepAliveTimer != -1 )
	{
		KillTimer( g_iKeepAliveTimer );
		
		BUD::CloseDB( );
	}
	
	g_bIsInitialized = false;
}

private BUD::IntegrityCheck( )
{
	if ( !BUD::GetDB( ) )
		return;
	
	new
		DBResult:dbrResult
	;
	
	dbrResult = db_query( g_dbKeptAlive, "PRAGMA integrity_check" );
	
	if ( dbrResult )
	{
		new
			iRow,
			szField[ 64 ]
		;
		
		do
		{
			db_get_field( dbrResult, 0, szField, sizeof( szField ) - 1 );
			
			if ( iRow == 0 && !strcmp( "ok", szField ) )
				break;
			else
				BUD::Warning( "Database integrity check says: %s", szField );
			
			iRow++;
		}
		while ( db_next_row( dbrResult ) );
		
		db_free_result( dbrResult );
	}
}

private BUD::CreateDatabase( )
{
	if ( !BUD::GetDB( ) )
		return false;
	
	db_query( g_dbKeptAlive, "CREATE TABLE users ( " DEFAULT_COLUMNS " )" );
	
	return true;
}

private BUD::ReloadTableInfo( )
{
	new
		DBResult:dbrResult
	;
	
	if ( !BUD::GetDB( ) )
		return;
	
	dbrResult = db_query( g_dbKeptAlive, "PRAGMA table_info( 'users' )" );
	
	g_iColumnCount = 0;
	
	if ( dbrResult )
	{
		new
			szColumnName[ BUD::MAX_COLUMN_NAME ]
		;
		
		do
		{
			db_get_field( dbrResult, 1, szColumnName, BUD::MAX_COLUMN_NAME - 1 );
			
			memcpy( g_szColumnName[ g_iColumnCount++ ], szColumnName, .numbytes = ( BUD::MAX_COLUMN_NAME * ( cellbits / 8 ) ) );
		}
		while ( db_next_row( dbrResult ) );
		
		db_free_result( dbrResult );
	}
	else
		BUD::Warning( "Failed to get the table info from \"%s\"; some columns could be missing.", g_szDatabaseName );
}

global BUD::VerifyColumn( const szColumnName[ ], BUD::e_COLUMN_TYPES:iType, { _,Float }:... )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szColumnName ) >= BUD::MAX_COLUMN_NAME )
			return false;
	#endif
	
	if ( !( BUD::e_COLUMN_TYPES:0 <= iType <= BUD::e_COLUMN_TYPES ) )
	{
		BUD::Error( "Invalid type given to BUD::VerifyColumn." );
		
		return;
	}
	
	if ( iType == BUD::TYPE_BINARY )
	{
		BUD::Error( "The binary type is not yet supported." );
		
		return;
	}
	
	if ( !g_iColumnCount )
	{
		BUD::Notice( "Because the table info wasn't retrieved, columns cannot be verified." );
		
		return;
	}
	
	new
		bool:bColumnCreated = false
	;
	
recheck:
	
	for ( new i = 0; i < g_iColumnCount; i++ )
	{
		if ( !strcmp( g_szColumnName[ i ], szColumnName, true ) )
			return;
	}
	
	if ( !bColumnCreated )
	{
		BUD::Notice( "The column \"%s\" doesn't exist; attempting to create it.", szColumnName );
		
		if ( !BUD::GetDB( ) )
			return;
		
		new
			szQuery[ 64 + BUD::MAX_COLUMN_NAME + ( BUD::MAX_ENTRY_STRING * 2 ) ]
		;
		
		switch ( iType )
		{
			case BUD::TYPE_NUMBER:
			{
				new
					iDefaultValue
				;
				
				if ( numargs( ) != 3 )
					iDefaultValue = 0;
				else
					iDefaultValue = getarg( 2 );
				
				format( szQuery, sizeof( szQuery ), "ALTER TABLE `users` ADD COLUMN `%s` INTEGER DEFAULT( %d )", szColumnName, iDefaultValue );
			}
			
			case BUD::TYPE_FLOAT:
			{
				new
					Float:fDefaultValue
				;
				
				if ( numargs( ) != 3 )
					fDefaultValue = 0.0;
				else
					fDefaultValue = Float:getarg( 2 );
				
				format( szQuery, sizeof( szQuery ), "ALTER TABLE `users` ADD COLUMN `%s` REAL DEFAULT( %f )", szColumnName, fDefaultValue );
			}
			
			case BUD::TYPE_STRING:
			{
				new
					szDefaultValue[ BUD::MAX_ENTRY_STRING * 2 ]
				;
				
				if ( numargs( ) == 3 )
				{
					new
						i
					;
					
					do
					{
						szDefaultValue[ i ] = getarg( 2, i );
					}
					while ( szDefaultValue[ i++ ] && i < BUD::MAX_ENTRY_STRING );
				
					szDefaultValue[ BUD::MAX_ENTRY_STRING - 1 ] = EOS;
					
					BUD::EscapeSqlString( szDefaultValue );
				}
				
				format( szQuery, sizeof( szQuery ), "ALTER TABLE `users` ADD COLUMN `%s` TEXT DEFAULT( '%s' )", szColumnName, szDefaultValue );
			}
			
			default:
			{
				return;
			}
		}
		
		db_free_result(
			db_query( g_dbKeptAlive, szQuery )
		);
		
		bColumnCreated = true;
		
		BUD::ReloadTableInfo( );
		
		goto recheck;
	}
	
	BUD::Error( "Failed to create the column \"%s\"; this could be because of an invalid column name.", szColumnName );
}

global bool:BUD::IsNameRegistered( const szName[ ] )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szName ) >= MAX_PLAYER_NAME )
			return false;
	#endif
	
	if ( !BUD::GetDB( ) )
		return true; // Better return true here to prevent any very circumstantial hijacking.
	
	new
		         szQuery[ 44 + MAX_PLAYER_NAME ],
		DBResult:dbrResult,
		    bool:bIsRegistered = false
	;
	
	format( szQuery, sizeof( szQuery ), "SELECT `uid` FROM `users` WHERE `name` = '%s'", szName );
	
	dbrResult = db_query( g_dbKeptAlive, szQuery );
	
	if ( dbrResult )
	{
		if ( db_num_rows( dbrResult ) == 1 )
			bIsRegistered = true;
		
		db_free_result( dbrResult );
	}
	
	return bIsRegistered;
}

global bool:BUD::RegisterName( const szName[ ], const szPassword[ ] )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szName ) >= MAX_PLAYER_NAME )
			return false;
	#endif
	
	if ( BUD::IsNameRegistered( szName ) )
		return false;
	
	new
		#if ( BUD::USE_WHIRLPOOL )
					szPasshash[ 129 ],
		         szQuery[ 64 + MAX_PLAYER_NAME + sizeof( szPasshash ) ] = "INSERT INTO users( `name`, `passhash` ) VALUES( '",
		#else
					szPasshash[ MAX_PASSWORD_LENGTH + 1 ],
		         szQuery[ 64 + MAX_PLAYER_NAME + MAX_PASSWORD_LENGTH + 1 ] = "INSERT INTO users( `name`, `passhash` ) VALUES( '",
		#endif
		DBResult:dbrResult
	;
	
	if ( !BUD::GetDB( ) )
		return false;
	
	#if ( BUD::USE_WHIRLPOOL )
		WhirlpoolHash( szPasshash, _, szPassword );
	#else
		BUD::JSCHash( szPassword, szPasshash );
	#endif
	
	
	strcat( szQuery, szName );
	
	#if ( BUD::USE_WHIRLPOOL )
		strcat( szQuery, "', x'" );
	#else
		strcat( szQuery, "', '" );
	#endif
	
	strcat( szQuery, szPasshash );
	strcat( szQuery, "')" );
	
	dbrResult = db_query( g_dbKeptAlive, szQuery );
	
	if ( dbrResult )
		db_free_result( dbrResult );
	
	if ( BUD::IsNameRegistered( szName ) )
		return true;
	
	return false;
}

global BUD::UnregisterName( const szName[ ] )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szName ) >= MAX_PLAYER_NAME )
			return false;
	#endif
	
	if ( !BUD::IsNameRegistered( szName ) )
		return false;
	
	if ( !BUD::GetDB( ) )
		return false;
	
	new
		   szQuery[ 64 + MAX_PLAYER_NAME ] = "DELETE FROM `users` WHERE `name`='"
	;
	
	strcat( szQuery, szName );
	strcat( szQuery, "'" );
	
	db_query( g_dbKeptAlive, szQuery );
	
	if ( BUD::IsNameRegistered( szName ) )
		return false;
	
	return true;
}

global bool:BUD::CheckAuth( const szName[ ], const szPassword[ ] )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szName ) >= MAX_PLAYER_NAME )
			return false;
	#endif
	
	if ( !BUD::GetDB( ) )
		return false;
	
	new
		#if ( BUD::USE_WHIRLPOOL )
		         szPasshash[ 129 ],
		         szQuery[ 56 + MAX_PLAYER_NAME + sizeof( szPasshash ) ] = "SELECT `uid` FROM `users` WHERE `name`='",
		#else
		         szPasshash[ MAX_PASSWORD_LENGTH + 1 ],
		         szQuery[ 56 + MAX_PLAYER_NAME + MAX_PASSWORD_LENGTH + 1 ] = "SELECT `uid` FROM `users` WHERE `name`='",
		#endif
		DBResult:dbrResult,
		    bool:bAuthenticated = false
	;
	
	#if ( BUD::USE_WHIRLPOOL )
		WhirlpoolHash( szPasshash, _, szPassword );
	#else
		BUD::JSCHash( szPassword, szPasshash );
	#endif
	
	strcat( szQuery, szName );
	
	#if ( BUD::USE_WHIRLPOOL )
		strcat( szQuery, "' AND `passhash`=x'" );
	#else
		strcat( szQuery, "' AND `passhash`='" );
	#endif
	
	strcat( szQuery, szPasshash );
	strcat( szQuery, "'" );
	
	dbrResult = db_query( g_dbKeptAlive, szQuery );
	
	if ( dbrResult )
	{
		if ( db_num_rows( dbrResult ) == 1 )
			bAuthenticated = true;
		
		db_free_result( dbrResult );
	}
	
	return bAuthenticated;
}

global BUD::GetNameUID( const szName[ ] )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szName ) >= BUD::MAX_PLAYER_NAME )
			return BUD::INVALID_UID;
	#endif
	
	if ( !BUD::GetDB( ) )
		return BUD::INVALID_UID;
	
	new
		         szQuery[ 42 + MAX_PLAYER_NAME ],
		DBResult:dbrResult
	;
	
	format( szQuery, sizeof( szQuery ), "SELECT `uid` FROM `users` WHERE `name`='%s'", szName );
	
	dbrResult = db_query( g_dbKeptAlive, szQuery );
	
	if ( dbrResult )
	{
		new
			iUID = BUD::INVALID_UID
		;
		
		if ( db_num_rows( dbrResult ) == 1 )
		{
			db_get_field( dbrResult, 0, szQuery, sizeof( szQuery ) - 1 );
			
			iUID = strval( szQuery );
		}
		
		db_free_result( dbrResult );
		
		return iUID;
	}
	
	return BUD::INVALID_UID;
}

global Float:BUD::GetFloatEntry( iUID, const szColumn[ ] )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szColumn ) >= BUD::MAX_COLUMN_NAME )
			return g_fFloatEntryDefault;
	#endif
	
	if ( !BUD::GetDB( ) )
		return g_fFloatEntryDefault;
	
	new
		         szQuery[ 44 + BUD::MAX_COLUMN_NAME ],
		DBResult:dbrResult
	;
	
	format( szQuery, sizeof( szQuery ), "SELECT `%s` FROM `users` WHERE `uid`='%d'", szColumn, iUID );
	
	dbrResult = db_query( g_dbKeptAlive, szQuery );
	
	if ( dbrResult )
	{
		if ( db_num_rows( dbrResult ) == 1 )
			db_get_field( dbrResult, 0, szQuery, sizeof( szQuery ) - 1 );
		else
			szQuery[ 0 ] = EOS;
		
		db_free_result( dbrResult );
		
		if ( szQuery[ 0 ] )
			return floatstr( szQuery );
	}
	
	return g_fFloatEntryDefault;
}

global BUD::GetIntEntry( iUID, const szColumn[ ] )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szColumn ) >= BUD::MAX_COLUMN_NAME )
			return g_iIntEntryDefault;
	#endif
	
	if ( !BUD::GetDB( ) )
		return g_iIntEntryDefault;
	
	new
		         szQuery[ 44 + BUD::MAX_COLUMN_NAME ],
		DBResult:dbrResult
	;
	
	format( szQuery, sizeof( szQuery ), "SELECT `%s` FROM `users` WHERE `uid`='%d'", szColumn, iUID );
	
	dbrResult = db_query( g_dbKeptAlive, szQuery );
	
	if ( dbrResult )
	{
		if ( db_num_rows( dbrResult ) == 1 )
			db_get_field( dbrResult, 0, szQuery, sizeof( szQuery ) - 1 );
		else
			szQuery[ 0 ] = EOS;
		
		db_free_result( dbrResult );
		
		if ( szQuery[ 0 ] )
			return strval( szQuery );
	}
	
	return g_iIntEntryDefault;
}

global BUD::GetStringEntry( iUID, const szColumn[ ], szOutput[ ], iSize = sizeof( szOutput ) )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szColumn ) >= BUD::MAX_COLUMN_NAME )
		{
			szOutput[ 0 ] = EOS;
			
			return;
		}
	#endif
	
	if ( !BUD::GetDB( ) )
	{
		szOutput[ 0 ] = EOS;
		
		return;
	}
	
	new
		         szQuery[ 44 + BUD::MAX_COLUMN_NAME ],
		DBResult:dbrResult
	;
	
	format( szQuery, sizeof( szQuery ), "SELECT `%s` FROM `users` WHERE `uid`='%d'", szColumn, iUID );
	
	dbrResult = db_query( g_dbKeptAlive, szQuery );
	
	if ( dbrResult )
	{
		if ( db_num_rows( dbrResult ) == 1 )
			db_get_field( dbrResult, 0, szOutput, iSize - 1 );
		else
			szOutput[ 0 ] = EOS;
		
		db_free_result( dbrResult );
	}
}

global bool:BUD::MultiGet( iUID, const szTypeDefinitions[ ], { _, Float }:... )
{
	new
		iEntries = numargs( ) - 2
	;
	
	if ( iEntries & 0b1 )
	{
		BUD::Error( "A parameter is missing in BUD::MultiGet; expecting a variable in the end.", numargs( ) - 2 );
		
		return false;
	}
	
	iEntries /= 2;
	
	if ( iEntries > BUD::MULTIGET_MAX_ENTRIES )
	{
		BUD::Error( "Too many entires passed to BUD::MultiGet; entires: %d, limit: " #BUD::MULTIGET_MAX_ENTRIES ".", iEntries );
		
		return false;
	}
	
	new
		iNumTypeDefinitions,
		iTypeDefinitions[ BUD::MULTIGET_MAX_ENTRIES char ],
		iTypeLengths[ BUD::MULTIGET_MAX_ENTRIES ]
	;
	
	for ( new i = 0, l = strlen( szTypeDefinitions ); i < l; i++ )
	{
		switch ( szTypeDefinitions[ i ] )
		{
			case 'i', 'd', 'f':
			{
				iTypeDefinitions{ iNumTypeDefinitions++ } = szTypeDefinitions[ i ];
			}
			
			case 's':
			{
				if ( szTypeDefinitions[ i + 1 ] == '[' && szTypeDefinitions[ i + 2 ] )
				{
					++i;
					
					new
						szLength[ 11 ],
						iEndingBracket = strfind( szTypeDefinitions, "]", .pos = i + 2 )
					;
					
					if ( iEndingBracket != -1 )
					{
						strmid( szLength, szTypeDefinitions, i + 1, iEndingBracket );
						
						iTypeLengths[ iNumTypeDefinitions ] = strval( szLength );
						
						if ( iTypeLengths[ iNumTypeDefinitions ] <= 0 )
						{
							BUD::Error( "Invalid string length given in the type definitions for BUD::MultiGet: %d.", iTypeLengths[ iNumTypeDefinitions ] );
							
							return false;
						}
						
						iTypeDefinitions{ iNumTypeDefinitions++ } = 's';
						
						i = iEndingBracket;
						
						continue;
					}
				}
				
				BUD::Error( "String size has to be passed in the type definitions for BUD::MultiGet; example: \"iffs[32]ii\".", iNumTypeDefinitions, iEntries );
				
				return false;
			}
			
			default:
			{
				BUD::Error( "Unknown type definition passed to BUD::MultiGet; expected: i/s/f, given: %c.", szTypeDefinitions[ i ] );
				
				return false;
			}
		}
	}
	
	if ( iEntries != iNumTypeDefinitions )
	{
		BUD::Error( "The number of type definitions doesn't match the number of entries passed to BUD::MultiGet; typedefs: %d, entries: %d.", iNumTypeDefinitions, iEntries );
		
		return false;
	}
	
	if ( !BUD::GetDB( ) )
		return false;
	
	new
		szColumn[ BUD::MAX_COLUMN_NAME ],
		iColumnArg,
		szQuery[ 48 + ( BUD::MULTIGET_MAX_ENTRIES * ( BUD::MAX_COLUMN_NAME + 4 ) ) ] = "SELECT `"
	;
	
	for ( new iEntry = 0, i; iEntry < iEntries; iEntry++ )
	{
		iColumnArg = 2 + iEntry * 2;
		i = 0;
		
		do
			szColumn[ i ] = getarg( iColumnArg, i );
		while ( szColumn[ i++ ] );
		
		#if ( BUD::CHECK_STRING_LENGTH )
			if ( strlen( szColumn ) >= BUD::MAX_COLUMN_NAME )
				return false;
		#endif
		
		strcat( szQuery, szColumn );
		
		if ( iEntry < iEntries - 1 )
			strcat( szQuery, "`,`" );
	}
	
	strcat( szQuery, "` FROM `users` WHERE `uid`=" );
	
	new
		         szUID[ 11 ],
		DBResult:dbrResult
	;
	
	valstr( szUID, iUID );
	
	strcat( szQuery, szUID );
	
	dbrResult = db_query( g_dbKeptAlive, szQuery );
	
	if ( dbrResult )
	{
		new
			iRows = db_num_rows( dbrResult )
		;
		
		if ( iRows == 1 )
		{
			new
				iFields = db_num_fields( dbrResult )
			;
			
			if ( iEntries != iFields )
				BUD::Warning( "The number of entries requested doesn't match the given number; requested: %d, given: %d", iEntries, iFields );
			
			for ( new iField = 0; iField < iFields; iField++ )
			{
				if ( iTypeDefinitions{ iField } == 's' )
					db_get_field( dbrResult, iField, szQuery, iTypeLengths[ iField ] - 1 );
				else
					db_get_field( dbrResult, iField, szQuery, sizeof( szQuery ) - 1 );
				
				switch ( iTypeDefinitions{ iField } )
				{
					case 'i', 'd':
					{
						setarg( 2 + iField * 2 + 1, .value = strval( szQuery ) );
					}
					
					case 'f':
					{
						setarg( 2 + iField * 2 + 1, .value = _:floatstr( szQuery ) );
					}
					
					case 's':
					{
						new
							i
						;
						
						do
							setarg( 2 + iField * 2 + 1, i, szQuery[ i ] );
						while ( szQuery[ i++ ] );
					}
				}
			}
		}
		else if ( iRows > 1 )
		{
			BUD::Error( "Multiple rows fetched from one UID; UID: %d, rows: %d", iUID, iRows );
			
			db_free_result( dbrResult );
			
			return false;
		}
		
		db_free_result( dbrResult );
		
		return true;
	}
	
	return false;
}

global bool:BUD::MultiSet( iUID, const szTypeDefinitions[ ], { _, Float }:... )
{
	new
		iEntries = numargs( ) - 2
	;
	
	if ( iEntries & 0b1 )
	{
		BUD::Error( "A parameter is missing in BUD::MultiSet; expecting a variable/value in the end.", numargs( ) - 2 );
		
		return false;
	}
	
	iEntries /= 2;
	
	if ( iEntries > BUD::MULTISET_MAX_ENTRIES )
	{
		BUD::Error( "Too many entires passed to BUD::MultiSet; entires: %d, limit: " #BUD::MULTISET_MAX_ENTRIES ".", iEntries );
		
		return false;
	}
	
	new
		iNumTypeDefinitions = strlen( szTypeDefinitions )
	;
	
	if ( iEntries != iNumTypeDefinitions )
	{
		BUD::Error( "The number of type definitions doesn't match the number of entries passed to BUD::MultiSet; typedefs: %d, entries: %d.", iNumTypeDefinitions, iEntries );
		
		return false;
	}
	
	for ( new i = 0; i < iNumTypeDefinitions; i++ )
	{
		switch ( szTypeDefinitions[ i ] )
		{
			case 'i', 'd', 'f', 's': { }
			default:
			{
				BUD::Error( "Unknown type definition passed to BUD::MultiSet; expected: i/s/f, given: %c.", szTypeDefinitions[ i ] );
				
				return false;
			}
		}
	}
	
	if ( !BUD::GetDB( ) )
		return false;
	
	new
		szColumn[ BUD::MAX_COLUMN_NAME ],
		iColumnArg,
		szQuery[ BUD::MULTISET_BUFFER_SIZE ] = "UPDATE `users` SET ",
		szValue[ BUD::MULTISET_MAX_STRING_SIZE * 2 ]
	;
	
	for ( new iEntry = 0, i; iEntry < iEntries; iEntry++ )
	{
		iColumnArg = 2 + iEntry * 2;
		i = 0;
		
		do
			szColumn[ i ] = getarg( iColumnArg, i );
		while ( szColumn[ i++ ] );
		
		++iColumnArg;
		
		#if ( BUD::CHECK_STRING_LENGTH )
			if ( strlen( szColumn ) >= BUD::MAX_COLUMN_NAME )
				return false;
		#endif
		
		strcat( szQuery, "`" );
		strcat( szQuery, szColumn );
		strcat( szQuery, "`=" );
		
		switch ( szTypeDefinitions[ iEntry ] )
		{
			case 'i', 'd':
			{
				valstr( szValue, getarg( iColumnArg ) );
			}
			
			case 'f':
			{
				format( szValue, sizeof( szValue ), "%f", Float:getarg( iColumnArg ) );
			}
			
			case 's':
			{
				i = 0;
				
				do
					szValue[ i ] = getarg( iColumnArg, i );
				while ( szValue[ i++ ] || i >= BUD::MULTISET_MAX_STRING_SIZE );
				
				szValue[ BUD::MULTISET_MAX_STRING_SIZE - 1 ] = EOS;
				
				BUD::EscapeSqlString( szValue );
				
				strcat( szQuery, "'" );
			}
		}
		
		strcat( szQuery, szValue );
		
		if ( szTypeDefinitions[ iEntry ] == 's' )
			strcat( szQuery, "'" );
		
		if ( iEntry < iEntries - 1 )
			strcat( szQuery, ", " );
	}
	
	valstr( szValue, iUID );
	
	strcat( szQuery, " WHERE `uid`=" );
	strcat( szQuery, szValue );
	
	db_query( g_dbKeptAlive, szQuery );
	
	return true;
}

global bool:BUD::SetIntEntry( iUID, const szColumn[ ], iInput )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szColumn ) >= BUD::MAX_COLUMN_NAME )
			return false;
	#endif
	
	if ( !BUD::GetDB( ) )
		return false;
	
	new
		szQuery[ 58 + BUD::MAX_COLUMN_NAME ]
	;
	
	format( szQuery, sizeof( szQuery ), "UPDATE `users` SET `%s`=%d WHERE `uid`=%d", szColumn, iInput, iUID );
	
	db_query( g_dbKeptAlive, szQuery );
	
	return true;
}

global bool:BUD::SetFloatEntry( iUID, const szColumn[ ], Float:fInput )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szColumn ) >= BUD::MAX_COLUMN_NAME )
			return false;
	#endif
	
	if ( !BUD::GetDB( ) )
		return false;
	
	new
		szQuery[ 78 + BUD::MAX_COLUMN_NAME ]
	;
	
	format( szQuery, sizeof( szQuery ), "UPDATE `users` SET `%s`=%f WHERE `uid`=%d", szColumn, fInput, iUID );
	
	db_query( g_dbKeptAlive, szQuery );
	
	return true;
}

global bool:BUD::SetStringEntry( iUID, const szColumn[ ], const szInput[ ], iSize = sizeof( szInput ) )
{
	#if ( BUD::CHECK_STRING_LENGTH )
		if ( strlen( szColumn ) >= BUD::MAX_COLUMN_NAME || strlen( szInput ) >= BUD::MAX_ENTRY_STRING )
			return false;
	#endif
	
	if ( !BUD::GetDB( ) )
		return false;
	
	new
		szQuery[ 44 + BUD::MAX_COLUMN_NAME + ( BUD::MAX_ENTRY_STRING * 2 ) ],
		szUID[ 11 ]
	;
	
	memcpy( szQuery, szInput, .numbytes = ( iSize * ( cellbits / 8 ) )+4 );
	
	BUD::EscapeSqlString( szQuery );
	
	valstr( szUID, iUID );
	
	strins( szQuery, "UPDATE `users` SET `", 0 );
	strins( szQuery, szColumn, 20 );
	strins( szQuery, "`='", 20 + strlen( szColumn ) );
	strcat( szQuery, "' WHERE `uid`=" );
	strcat( szQuery, szUID );
	
	db_query( g_dbKeptAlive, szQuery );
	
	return true;
}

global BUD::EscapeSqlString( szString[ ], iEnclosingChar = '\'', iSize = sizeof( szString ) )
{
	new
		iLength = strlen( szString ),
		szInsert[ 2 ]
	;
	
	szInsert[ 0 ] = iEnclosingChar;
	
	for ( new i = 0; i <= iLength; i++ )
	{
		if ( szString[ i ] == iEnclosingChar )
		{
			
			if ( i > iLength - 1 )
			{
				szString[ i ] = EOS;
				
				break;
			}
			else
			{
				if ( iLength >= iSize - 1 )
				{
					szString[ iLength - 1 ] = EOS;
					
					--iLength;
				}
				
				strins( szString, szInsert, i, iSize );
				
				++iLength;
				++i;
			}
		}
	}
}

global DBResult:BUD::RunQuery( szQuery[ ], bool:bStoreResult )
{
	if ( !BUD::GetDB( ) )
		return BUD::NO_RESULT;
	
	new
		DBResult:dbrResult = db_query( g_dbKeptAlive, szQuery )
	;
	
	if ( dbrResult )
	{
		if ( bStoreResult )
		{
			SetTimerEx( "BUD_FreeResult", 0, false, "i", _:dbrResult );
			
			return dbrResult;
		}
		else
			db_free_result( dbrResult );
	}
	
	return BUD::NO_RESULT;
}

global DBResult:BUD::RunQueryEx( const szFormat[ ], bool:bStoreResult, { _, Float }:... )
{
	if ( !BUD::GetDB( ) )
		return BUD::NO_RESULT;
	
	static const
		STATIC_ARGUMENTS = 2;
	
	new
		     iNumArgs = numargs( ) - STATIC_ARGUMENTS,
		     iParams = iNumArgs * ( cellbits / 8 ),
		     szQuery[ 256 ],
		     szNewFormat[ sizeof( szQuery ) ],
		bool:bIsFormatCopied = false,
		     iPos = -1,
		     iArg = STATIC_ARGUMENTS,
		     i,
		bool:bSkipArguments[ 32 char ],
		     iSkippedArguments
	;
	
	goto jump_in_the_action__rambo_style;
	
	do
	{
		if ( iPos == 0 || ( !bIsFormatCopied && szFormat[ iPos - 1 ] != '%' ) || ( bIsFormatCopied && szNewFormat[ iPos - 1 ] != '%' )  )
		{
			if ( ( !bIsFormatCopied && szFormat[ iPos + 1 ] == 's' ) || ( bIsFormatCopied && szNewFormat[ iPos + 1 ] == 's' ) )
			{
				if ( !bIsFormatCopied )
				{
					bIsFormatCopied = true;
					
					memcpy( szNewFormat, szFormat, _, ( strlen( szFormat ) + 1 ) * 4 );
				}
				
				
				i = 0;
				
				do
					szQuery[ i ] = getarg( iArg, i );
				while ( szQuery[ i++ ] );
				
				BUD::EscapeSqlString( szQuery );
				
				strdel( szNewFormat, iPos, iPos + 2 );
				
				--iPos;
				
				strins( szNewFormat, szQuery, iPos+1 );
				
				iPos += i - 1;
				
				bSkipArguments{ iArg } = true;
				
				++iSkippedArguments;
			}
			
			++iArg;
		}
		
jump_in_the_action__rambo_style:
		
		++iPos;
		
		if ( bIsFormatCopied )
			iPos = strfind( szNewFormat, "%", .pos = iPos );
		else
			iPos = strfind( szFormat, "%", .pos = iPos );
	}
	while ( iPos != -1 );
	
	if ( iParams - ( iSkippedArguments + 1 ) * ( cellbits / 8 ) )
	{
		new
			iArgStart,
			iArgEnd
		;
		
		iArg = 1;
		
		#emit CONST.alt    bStoreResult
		#emit LCTRL        5
		#emit ADD
		#emit STOR.S.pri   iArgStart
		#emit LOAD.S.alt   iParams
		#emit ADD
		#emit STOR.S.pri   iArgEnd
		
		do
		{
			#emit LOAD.I
			#emit PUSH.pri
			
			if ( bSkipArguments{ iNumArgs + STATIC_ARGUMENTS - ++iArg } )
			{
				#emit POP.alt
			}
			
			iArgEnd -= ( cellbits / 8 );
			
			#emit LOAD.S.pri   iArgEnd
		}
		while ( iArgEnd > iArgStart );
		
		if ( bIsFormatCopied )
		{
			#emit PUSH.ADR     szNewFormat
		}
		else
		{
			#emit PUSH.S       szFormat
		}
		
		#if ( sizeof( szQuery ) != 256 )
			#error Update the number below this line to match the above number.
		#endif
		
		#emit PUSH.C     256
		#emit PUSH.ADR   szQuery
		
		iParams += ( ( cellbits / 8 ) * 3 );
		iParams -= iSkippedArguments * ( cellbits / 8 );
		
		#emit PUSH.S     iParams
		#emit SYSREQ.C   format
		
		iParams += ( cellbits / 8 );
		iParams += iSkippedArguments * ( cellbits / 8 );
		
		#emit LCTRL        4
		#emit LOAD.S.alt   iParams
		#emit ADD
		#emit SCTRL        4
	}
	
	printf( szQuery );
	
	return BUD::NO_RESULT;
}

forward BUD::FreeResult( DBResult:dbrResult );
public  BUD::FreeResult( DBResult:dbrResult )
{
	db_free_result( dbrResult );
}

global BUD::GetSortedData( brResults[ ][ ], const szColumnName[ ], BUD::e_SORT_ORDER:iSortOrder = BUD::SORT_DESC, iResults = sizeof( brResults ) )
{
	if ( !BUD::GetDB( ) )
		return BUD::INVALID_RESULTS;
	
	new
		         szQuery[ 320 ],
		DBResult:dbrResult,
		         iReturn = BUD::INVALID_RESULTS
	;
	
	format( szQuery, sizeof( szQuery ), "SELECT `uid`, `%s` FROM `users` ORDER BY `%s` %s LIMIT %d", szColumnName, szColumnName, ( BUD::SORT_ASC == iSortOrder ) ? ( "ASC" ) : ( "DESC" ), iResults );
	
	dbrResult = db_query( g_dbKeptAlive, szQuery );
	
	if ( dbrResult )
	{
		if ( db_num_rows( dbrResult ) > 0 )
		{
			new
				iIndex
			;
			
			do
			{
				db_get_field( dbrResult, 0, szQuery, sizeof( szQuery ) - 1 );
				
				brResults[ iIndex ][ 0 ] = strval( szQuery );
				
				db_get_field( dbrResult, 1, szQuery, sizeof( szQuery ) - 1 );
				
				brResults[ iIndex ][ 1 ] = strval( szQuery );
				
				++iIndex;
			}
			while ( db_next_row( dbrResult ) && iIndex < iResults );
			
			iReturn = iIndex;
		}
		
		db_free_result( dbrResult );
	}
	
	return iReturn;
}

global bool:BUD::GetNamesForSortedData( brResults[ ][ ], iResults, szaNames[ ][ ], iMaxName = sizeof( szaNames[ ] ) )
{
	if ( !BUD::GetDB( ) )
		return false;
	
	new
		         szQuery[ 192 ] = "SELECT `name` FROM `users` WHERE `uid` IN (",
		         szUID[ 11 ],
		DBResult:dbrResult
	;
	
	for ( new i = 0; i < iResults; i++ )
	{
		valstr( szUID, brResults[ i ][ 0 ] );
		
		strcat( szQuery, szUID );
		
		if ( i + 1 < iResults )
			strcat( szQuery, "," );
	}
	
	strcat( szQuery, ")" );
	
	dbrResult = db_query( g_dbKeptAlive, szQuery );
	
	if ( dbrResult )
	{
		if ( db_num_rows( dbrResult ) > 0 )
		{
			new
				iIndex
			;
			
			do
			{
				db_get_field( dbrResult, 0, szaNames[ iIndex ], iMaxName - 1 );
				
				++iIndex;
			}
			while ( db_next_row( dbrResult ) && iIndex < iResults );
		}
		
		db_free_result( dbrResult );
		
		return true;
	}
	
	return false;
}

#define BUD_Results:%0<%1> %0[%1][2]

// Inspired by Y_Less!

private BUD::CheckForUpdates( )
{
	HTTP( 0xFA7E, HTTP_GET, "spelsajten.net/bud_version.php?version=0x" #BUD_VERSION_MINOR #BUD_VERSION_MAJOR #BUD_VERSION_BUILD, "", "BUD_HTTPResponse" );
}

forward BUD::HTTPResponse( iIndex, iResponseCode, szData[ ] );
public  BUD::HTTPResponse( iIndex, iResponseCode, szData[ ] )
{
	if ( iResponseCode == 200 )
	{
		switch ( szData[ 0 ] )
		{
			case '0': return; // Up-to-date
			case '1': BUD::Notice( "There is a new update available." );
			case '2': BUD::Warning( "There is a new version out; upgrading is strongly recommended." );
			case '3': // New version available; upgrading has to be done.
			{
				new
					iTick = GetTickCount( )
				;
				
				BUD::Error( "This version is outdated; you have to upgrade your script because of security reasons." );
				
				SetGameModeText( "LOOK AT THE SERVER LOG" );
				
				while ( GetTickCount( ) - iTick < 3000 ) {}
				
				SendRconCommand( "exit" );
			}
		}
	}
	else
	{
		if ( iResponseCode < 100 )
			BUD::Notice( "Unable to check for updates; internal error code %d.", iResponseCode );
		else
			BUD::Notice( "Unable to check for updates; HTTP error code %d.", iResponseCode );
	}
}

#if ( BUD::USE_WHIRLPOOL && UNDEFINE_MAX_PASSWORD_LENGTH )
	#undef MAX_PASSWORD_LENGTH
#endif

#undef UNDEFINE_MAX_PASSWORD_LENGTH
#undef INVALID_DB
#undef DEFAULT_COLUMNS
#undef DEFAULT_DATABASE_NAME
#undef BUD_Notice
#undef BUD_Warning
#undef BUD_Error
#undef private
#undef global