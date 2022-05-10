#pragma once
#include <tchar.h>
#include "config.h"

const AcString CLEANBLOCNAME_PATH = strToAcStr( VERSION_FOLDER_STR ) + _T( "BDX\\CLEANBLOCNAME.txt" );
const AcString CLDPOLY3D_PATH = strToAcStr( VERSION_FOLDER_STR ) + _T( "BDX\\ CLDPOLY3D.txt" );
const AcString GMT_CHECK_ERROR_PATH =  strToAcStr( VERSION_FOLDER_STR ) + _T( "BDX\\GMTCHECKERROR.txt" );
const AcString GMT_CHECK_ERROR_WALL_PATH = strToAcStr( VERSION_FOLDER_STR ) + _T( "BDX\\GMTCHECKERROR_WALL.txt" );
const AcString GMT_CHECK_ERROR_EDGE_PATH = strToAcStr( VERSION_FOLDER_STR ) + _T( "BDX\\GMTCHECKERROR_EDGE.txt" );
const AcString RAIL_CORRECTSCALES = strToAcStr( VERSION_FOLDER_STR ) + _T( "RAIL\\CORRECTSCALES.txt" );

const AcString KDN_ELEMENT_LAYER_PATH = strToAcStr( VERSION_FOLDER_STR ) + _T( "BDX\\KDNELEMENT.txt" );
const AcString KDN_DESCRIPTION_PATH = strToAcStr( VERSION_FOLDER_STR ) + _T( "BDX\\KDNDESCRIPTION.txt" );
const AcString KDN_TEXT_LAYER_PATH = strToAcStr( VERSION_FOLDER_STR ) + _T( "BDX\\KDNLAYERTEXT.txt" );

#define BLOC_NAME_MEPAUTO "MEPAUTO"
#define MANQUE_EPAISSEUR_LAYER _T("Manque_Epaisseur")
#define ERREUR_SENS_LAYER _T("Erreur_Sens")
#define PATATE_RADIUS 1.0
#define TEXT_HEIGHT 0.3

#ifndef M_PI_2
    #define M_PI_2     1.57079632679489661923
#endif

#ifndef MSG_ASK_XY
    #define MSG_ASK_XY _T( "\nSpécifiez les valeurs XY du point à insérer: " )
#endif

#ifndef MSG_ASK_XY_VALDIATE
    #define MSG_ASK_XY_VALDIATE _T( "\nSpécifiez les valeurs XY du point à insérer ou valider [Z courant = %.3f]: " )
#endif

#ifndef MSG_ASK_Z
    #define MSG_ASK_Z _T( "\nSpécifiez la valeur Z du point à insérer [Z courant = %.3f]: " )
#endif

#ifndef MSG_ASK_Z_VALDIATE
    #define MSG_ASK_Z_VALDIATE _T( "\nSpécifiez la valeur Z du point à insérer  ou valider [Z courant = %.3f]: " )
#endif
