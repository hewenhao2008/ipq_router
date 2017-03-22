#include <stdio.h>
#include <sqlite3.h>
#include "zb_db.h"

sqlite3 *db;

int db_get_endpoints(char *ManufactureName, char *ModelIdentifier, zb_uint8 endp_addr[])
{
	char sql_cmd[256];
	char *sql_comment = "select zigbee_endpoint.endpoint_addr from zigbee_endpoint, zigbee_module \
		where zigbee_endpoint.ModuleIndex == zigbee_module.ModuleIndex and \
		  zigbee_module.ManufacturerName == '%s' and \
		  zigbee_module.ModelIdentifier == '%s'";
		  
	int ret;
	char **dbResult;
	int nRow, nCols;
	int r,c;
	char *errMsg;

	snprintf(sql_cmd, 256, sql_comment, ManufactureName, ModelIdentifier);
	ZB_DBG(">sql:%s\n", sql_cmd);

	ret = sqlite3_get_table(db, sql_cmd, &dbResult, &nRow, &nCols, &errMsg);
	if( ret != SQLITE_OK)
	{
		sqlite3_free_table( dbResult );
		ZB_ERROR("!!!! sqlite3_get_table return fail. ret=%d\n", ret);
		return 0;
	}
	
	if(nRow == 0)	//No record found.
	{
		sqlite3_free_table( dbResult );
		ZB_DBG("---- No record found in db. nRow=%d ----\n", nRow);
		return 0;
	}

	//total result: (nRow+1)*nCols
	for( r=0; r<nRow + 1; r++)
	{
		ZB_DBG("Record Row: %d\n", r );
		
		for(c=0; c<nCols; c++)
		{
			if( r==0) //colNames
			{
				ZB_DBG("ColName:  %s", dbResult[c]);
			}
			else  //col values.
			{
				ZB_DBG("ColValue: %s", dbResult[nCols*r + c]);				
			}

			if( r >=1)	//The first col is endp_addr
				endp_addr[r-1] = strtoul(dbResult[nCols*r], NULL, 0);
		}		
	}
	
	sqlite3_free_table( dbResult );
	return nRow;	
}

int init_zigbee_db()
{
	int ret;
	
	ret = sqlite3_open("/root/zigbee_dev.db", &db);
	if(ret )
	{
		ZB_ERROR("Falied to open dev. error:%s", sqlite3_errmsg(db) );
		return -1;
	}

	return 0;
}


