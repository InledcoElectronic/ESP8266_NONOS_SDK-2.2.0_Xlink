/*
 * xlink_datapoint.c
 *
 *  Created on: 2018年3月28日
 *      Author: liruya
 */

#include "xlink_datapoint.h"

#include "../include/app_config.h"
#include "xlink_sdk.h"
#include "xlink.h"
#include "c_types.h"

#define DP_BUFFER_SIZE		2048

LOCAL uint8_t dp_buffer[DP_BUFFER_SIZE];

datapoint_t *p_datapoints[DATAPOINT_MAX_NUM];
xlink_onDatapointChanged_callback_t xlink_onDatapointChanged_cb;

bool xlink_datapoint_check( datapoint_t *pdp )
{
	if ( pdp == NULL || pdp->pdata == NULL ) {
		return false;
	}
	uint16_t len = pdp->length;
	switch ( pdp->type ) {
		case DP_TYPE_BYTE:
			if ( len == 1 )
			{
				return true;
			}
			break;
		case DP_TYPE_INT16:
		case DP_TYPE_UINT16:
			if ( len == 2 )
			{
				return true;
			}
			break;
		case DP_TYPE_INT32:
		case DP_TYPE_UINT32:
		case DP_TYPE_FLOAT:
			if ( len == 4 )
			{
				return true;
			}
			break;
		case DP_TYPE_INT64:
		case DP_TYPE_UINT64:
		case DP_TYPE_DOUBLE:
			if ( len == 8 )
			{
				return true;
			}
			break;
		case DP_TYPE_STRING:
			if ( len <= DATAPOINT_STR_MAX_LEN )
			{
				return true;
			}
			break;
		case DP_TYPE_BINARY:
			if ( len <= DATAPOINT_BIN_MAX_LEN )
			{
				return true;
			}
			break;
		default:
			break;
	}
	return false;
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init( uint8_t type, uint16_t len, uint8_t *pdata )
{
	datapoint_t *pdp = ( datapoint_t * ) os_zalloc( sizeof( datapoint_t ) );
	if ( pdp != NULL )
	{
		pdp->type = type;
		pdp->length = len;
		pdp->pdata = pdata;
		if ( xlink_datapoint_check( pdp ) == false )
		{
			os_free( pdp );
			pdp = NULL;
		}
	}
	return pdp;
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_byte( uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_BYTE, 1, pdata );
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_int16( uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_INT16, 2, pdata );
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_uint16( uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_UINT16, 2, pdata );
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_int32( uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_INT32, 4, pdata );
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_uint32( uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_UINT32, 4, pdata );
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_int64( uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_INT64, 8, pdata );
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_uint64( uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_UINT64, 8, pdata );
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_float( uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_FLOAT, 4, pdata );
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_double( uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_DOUBLE, 8, pdata );
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_string( uint8_t len, uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_STRING, len, pdata );
}

datapoint_t XLINK_FUNCTION *xlink_datapoint_init_binary( uint8_t len, uint8_t *pdata )
{
	return xlink_datapoint_init( DP_TYPE_BINARY, len, pdata );
}

uint16_t XLINK_FUNCTION xlink_datapoints_to_array( uint8_t *pdata )
{
	if( pdata == NULL )
	{
		return 0;
	}
	uint8_t i, j;
	uint16_t idx = 0;
	datapoint_t *pdp = NULL;
	for ( i = 0; i < DATAPOINT_MAX_NUM; i++ )
	{
		pdp = p_datapoints[i];
		if ( xlink_datapoint_check( pdp ) && pdp->length > 0 )
		{
			pdata[idx++] = i;
			pdata[idx++] = pdp->array[1];
			pdata[idx++] = pdp->array[0];
			switch ( pdp->type )
			{
				case DP_TYPE_BYTE:
				case DP_TYPE_INT16:
				case DP_TYPE_UINT16:
				case DP_TYPE_INT32:
				case DP_TYPE_UINT32:
				case DP_TYPE_INT64:
				case DP_TYPE_UINT64:
				case DP_TYPE_FLOAT:
				case DP_TYPE_DOUBLE:
					for ( j = pdp->length; j > 0; j-- )
					{
						pdata[idx++] = pdp->pdata[j-1];
					}
					break;
				case DP_TYPE_STRING:
				case DP_TYPE_BINARY:
					os_memcpy( pdata + idx, pdp->pdata, pdp->length );
					idx += pdp->length;
					break;
				default:
					break;
			}
		}
	}
	return idx;
}

void XLINK_FUNCTION xlink_array_to_datapoints( const uint8_t *pdata, uint16_t data_length )
{
	if ( pdata == NULL || data_length < 4 )
	{
		return;
	}
	bool flag = false;
	uint8_t i, j;
	uint8_t idx;
	uint8_t type;
	uint16_t len;
	datapoint_t *pdp = NULL;
	for ( i = 0; i < data_length; )
	{
		if ( pdata[i] >= DATAPOINT_MAX_NUM || ( data_length - i < 4 ) )
		{
			break;
		}
		idx = pdata[i];
		type = pdata[i+1]>>4;
		len = ((pdata[i+1]&0x0F)<<8)|pdata[i+2];
		pdp = p_datapoints[idx];
		if ( xlink_datapoint_check(pdp) == false || type != pdp->type )
		{
			break;
		}
		switch ( type )
		{
			case DP_TYPE_BYTE:
			case DP_TYPE_INT16:
			case DP_TYPE_UINT16:
			case DP_TYPE_INT32:
			case DP_TYPE_UINT32:
			case DP_TYPE_INT64:
			case DP_TYPE_UINT64:
			case DP_TYPE_FLOAT:
			case DP_TYPE_DOUBLE:
				if ( len == pdp->length )
				{
					for ( j = 0; j < len; j++ )
					{
						pdp->pdata[j] = pdata[i+2+len-j];
					}
					i += 3 + len;
					flag = true;
				}
				break;
			case DP_TYPE_STRING:
				if ( len > 0 && len <= DATAPOINT_STR_MAX_LEN )
				{
					pdp->length = len;
					os_memcpy( pdp->pdata, &pdata[3+i], len );
					i += 3 + len;
					flag = true;
				}
				break;
			case DP_TYPE_BINARY:
				if ( len > 0 && len <= DATAPOINT_BIN_MAX_LEN )
				{
					pdp->length = len;
					os_memcpy( pdp->pdata, &pdata[3+i], len );
					i += 3 + len;
					flag = true;
				}
				break;
			default:
				break;
		}
	}
	if ( flag && xlink_onDatapointChanged_cb != NULL )
	{
		xlink_onDatapointChanged_cb();
	}
}

void XLINK_FUNCTION xlink_datapoint_update_all()
{
	uint8_t *pdata = dp_buffer;
	uint16_t dp_length = xlink_datapoints_to_array( dp_buffer );
	xlink_update_datapoint_with_alarm( (const uint8_t **)&pdata, dp_length );
}

void XLINK_FUNCTION xlink_setOnDatapointChangedCallback( xlink_onDatapointChanged_callback_t callback )
{
	xlink_onDatapointChanged_cb = callback;
}
