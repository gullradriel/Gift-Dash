/**\file states_management.c
 *
 *  level file for hacks
 *
 *\author Castagnier Mickaël aka Gull Ra Driel
 *
 *\version 1.0
 *
 *\date 29/12/2021 
 *
 */

#include "nilorea/n_str.h"
#include "states_management.h"
#include "cJSON.h"


int load_app_state( char *state_filename , size_t *WIDTH , size_t *HEIGHT , bool *fullscreen , char **bgmusic , double *drawFPS , double *logicFPS )
{
	__n_assert( state_filename , return FALSE );

	if( access( state_filename , F_OK ) != 0 )
	{
		n_log( LOG_INFO , "no app state %s to load !" , state_filename );
		return FALSE ;
	}

	N_STR *data = NULL ;
	data = file_to_nstr( state_filename );
	if( !data )
	{ 
		n_log( LOG_ERR , "Error reading file %s, defaults will be used" , state_filename );
		return FALSE;
	}

	cJSON *monitor_json = cJSON_Parse( _nstr( data ) );
	if (monitor_json == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		n_log( LOG_ERR , "%s: Error before: %s, defaults will be used", state_filename , _str( error_ptr ) );
		cJSON_Delete( monitor_json );
		free_nstr( &data );
		return FALSE ;
	}

	cJSON *value = NULL ;
	value = cJSON_GetObjectItemCaseSensitive( monitor_json, "width" );
    if( cJSON_IsNumber( value ) ){ 
        (*WIDTH)  = value -> valueint ; 
    } else { 
        n_log( LOG_ERR , "width is not a number"); 
    }
	value = cJSON_GetObjectItemCaseSensitive( monitor_json, "height" );     
    if( cJSON_IsNumber( value ) ){ 
        (*HEIGHT) = value -> valueint ; 
    } else { 
        n_log( LOG_ERR , "height is not a number"); 
    }
	value = cJSON_GetObjectItemCaseSensitive( monitor_json, "fullscreen" ); 
    if( cJSON_IsNumber( value ) ){ 
        (*fullscreen) = value -> valueint ; 
    } else { 
        n_log( LOG_ERR , "fullscreen is not a number"); 
    }
	value = cJSON_GetObjectItemCaseSensitive( monitor_json, "bg-music" );   
    if( cJSON_IsString( value ) ){ 
        (*bgmusic) = strdup( value -> valuestring ); 
    } else { 
        n_log( LOG_ERR , "bg-music is not a string"); 
    }
	value = cJSON_GetObjectItemCaseSensitive( monitor_json, "drawFPS" ); 
    if( cJSON_IsNumber( value ) ){ 
        (*drawFPS) = value -> valuedouble ; 
    } else { 
        n_log( LOG_ERR , "drawFPS is not a number"); 
    }
	value = cJSON_GetObjectItemCaseSensitive( monitor_json, "logicFPS" ); 
    if( cJSON_IsNumber( value ) ){ 
        (*logicFPS) = value -> valuedouble ; 
    } else { 
        n_log( LOG_ERR , "logicFPS is not a number"); 
    }

	cJSON_Delete(monitor_json);
	free_nstr( &data );

	return TRUE ;
}
