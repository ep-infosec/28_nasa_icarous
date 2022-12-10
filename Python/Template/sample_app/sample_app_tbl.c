//
// Created by {{AUTHOR}} on {{DATE}}
//

#include "cfe.h"
#include "cfe_tbl_filedef.h"  /* Required to obtain the CFE_TBL_FILEDEF macro definition */

#include "{{APP_NAME}}_tbl.h"

{{APP_NAME}}Table_t {{APP_NAME}}_TblStruct = {

};


/*
** The macro below identifies:
**    1) the data structure type to use as the table image format
**    2) the name of the table to be placed into the cFE Table File Header
**    3) a brief description of the contents of the file image
**    4) the desired name of the table image binary file that is cFE compatible
*/
CFE_TBL_FILEDEF({{APP_NAME}}_TblStruct, {{APP_NAMEU}}.{{APP_NAME}}Table, {{APP_NAME}} parameters, {{APP_NAME}}_tbl.tbl )
