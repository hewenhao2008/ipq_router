#include "common_def.h"
#include "zb_data_type.h"

static zb_uint8 attributeSizes[] =
{
	  // Used ZCL attribute type sizes
	  ZCL_BITMAP16_ATTRIBUTE_TYPE, 2, 
	  ZCL_BITMAP24_ATTRIBUTE_TYPE, 3, 
	  ZCL_BITMAP32_ATTRIBUTE_TYPE, 4, 
	  ZCL_BITMAP48_ATTRIBUTE_TYPE, 6, 
	  ZCL_BITMAP64_ATTRIBUTE_TYPE, 8, 
	  ZCL_BITMAP8_ATTRIBUTE_TYPE, 1, 
	  ZCL_BOOLEAN_ATTRIBUTE_TYPE, 1, 
	  ZCL_DATA8_ATTRIBUTE_TYPE, 1, 
	  ZCL_ENUM16_ATTRIBUTE_TYPE, 2, 
	  ZCL_ENUM8_ATTRIBUTE_TYPE, 1, 
	  ZCL_IEEE_ADDRESS_ATTRIBUTE_TYPE, 8, 
	  ZCL_INT16S_ATTRIBUTE_TYPE, 2, 
	  ZCL_INT16U_ATTRIBUTE_TYPE, 2, 
	  ZCL_INT24S_ATTRIBUTE_TYPE, 3, 
	  ZCL_INT24U_ATTRIBUTE_TYPE, 3, 
	  ZCL_INT32S_ATTRIBUTE_TYPE, 4, 
	  ZCL_INT32U_ATTRIBUTE_TYPE, 4, 
	  ZCL_INT48U_ATTRIBUTE_TYPE, 6, 
	  ZCL_INT56U_ATTRIBUTE_TYPE, 7, 
	  ZCL_INT8S_ATTRIBUTE_TYPE, 1, 
	  ZCL_INT8U_ATTRIBUTE_TYPE, 1, 
	  ZCL_SECURITY_KEY_ATTRIBUTE_TYPE, 16, 
	  ZCL_UTC_TIME_ATTRIBUTE_TYPE, 4, 
	  
};

//return how many bytes extracted.
zb_uint8 zb_attr_get_simple_val(zb_uint8 dataType, zb_uint8 *datacont, zb_uint8 *cont) 
{
  zb_uint8 i;
  zb_uint8 len;
  
  if( dataType == ZCL_CHAR_STRING_ATTRIBUTE_TYPE) //string format.
  {
  	//The first byte indicate the length of the string.
  	len = *datacont;

  	++datacont;

  	memcpy(cont, datacont, len);

  	return len + 1;	//include the first byte(len)
  }
  
  for (i = 0; (i+1) < sizeof(attributeSizes); i+=2) 
  {
    if (attributeSizes[i] == dataType) 
    {
      len = attributeSizes[i + 1];
      memcpy(cont, datacont, len);
      return len;
    }
  }

  return 0;
}




