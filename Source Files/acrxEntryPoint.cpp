#include "acrxEntryPoint.h"

void initApp()
{
    // Blocs
    initCmd( "CORRECTSCALE", cmdCorrectScale );
    initCmd( "CGNAME", cmdCGName );
    initCmd( "ROBINET", cmdRobinet );
    initCmd( "CLEANBLOCNAME", cmdCleanBlockName );
    initCmd( "REPLACEBLOCKDYN", cmdReplaceBlockDyn );
    initCmd( "FMAPCOPYBLOCKLAYER", cmdCopyBlockLayer );
    initCmd( "SETBLOCKSCALE", cmdSetBlockScale );
    initCmd( "DRAWCONTOURBLOCK", cmdDrawContourBlock );
    initCmd( "PROJOI", cmdProjectJoi ); //Stephane 07/07/2021 15:42
    initCmd( "LEVELUPBLOCKTEXT", cmdLevelUpBlockText ); //DInahasina  17/11/2021
    
    // Polylignes
    initCmd( "ADDVERTEX3D", cmdDiscretizePoly );
    initCmd( "POLY3DADDPTXYZ", cmdAddPointXYZPoly3d );
    initCmd( "CHECKALONEVERTEX", cmdIsolerVertex );
    initCmd( "INTERSECTVERTEX", cmdIntersectVertex );
    initCmd( "ARROUNDPOLYS", cmdArroundPolys );
    initCmd( "ARROUNDVERTEX", cmdArroundVertex ); //Andry 26/11/2021
    
    // RTE
    initCmd( "EXPORTRTE", cmdExportPoint ); // Mamy [#4]
    
    // Points topo
    initCmd( "INSERTOPOALLVERTEX", cmdAddBlocOnAllVertex );
    initCmd( "SETPTOPOPOSITION", cmdSetPTOPOPosition );
    initCmd( "CHECKTOPOATTR", cmdCheckTopoAttr ); //Stephane 27/07/2021
    
    // Export
    initCmd( "EXPORTNUMPATATEINDALLE", cmdExportDalle );
    initCmd( "PRMERGEPHOTOCSV", cmdMergePhotoCsv );
    
    // Textes
    initCmd( "FMAPCHANGELAYOFTXT", cmdChangeLayerOfText );
    initCmd( "ROADNAME", cmdRoadName );
    initCmd( "TEXTWITHPOLY", cmdOrientTxtWithPoly );
    initCmd( "CONTROLBLOCTXT", cmdControlBlocTxt );
    
    // Palette
    initCmd( "CLEANGTCIMAGEFOLDER", cmdCleanImgGtcFolder ); //Stephane 17/06/2021 12:51
    
    // KDN
    initCmd( "KDNCHECKSEUIL", cmdKdnCheckSeuil ); //Stephane 22/07/2021
    initCmd( "KDNCORRECTSEUIL", cmdKdnCorrectSeuil ); //Stephane 23/07/2021
    initCmd( "KDNPOLYPONCTTEXT", cmdAddPolyPonctText );
    initCmd( "ARRONDIRH", cmdArrondirH );
    
    // SIEML
    initCmd( "SIEMLROTATE", cmdSiemlRotate );
    initCmd( "SIEMLREGARDS", cmdSiemlRegard ); //Ando 21/04/2022
    initCmd( "SIEMLGOUTT", cmdSiemlGoutt ); //Ando 21/04/2022
    initCmd( "SMDIV", cmdSMDiv ); //Ando 21/04/2022
    
    // Brest
    initCmd( "ORIENTEBLOCKBREST", cmdOrientBrestBlock );
    
    // Nuage
    initCmd( "SETVIEWPORTEP", cmdSetViewPortEp );
    initCmd( "COPYPCD", cmdExportCopyPcd );
    initCmd( "DIVPCJ", cmdDivPCJ );
    initCmd( "DRAWPCJ", cmdDrawPcj ); //Stephane 09/07/2021
    initCmd( "FMAPFILTERPCJ", cmdFilterPcj );
    initCmd( "TABLINEAIRE", cmdTabLineaire ); //Nils 01/10/21
    
    // BIM
    initCmd( "BIMETIQUETER", cmdBimEtiqueter );
    initCmd( "BIMGENERERFB", cmdBimGenererFb ); //Andry 27/10/2021
    
    // Sytral
    initCmd( "SYTRALSEUIL", cmdSytralSeuil ); // Marielle 18/10/2021
    initCmd( "SYTRALSEUILCONTROL", cmdSytralSeuilControl ); // Marielle 15/11/2021
    initCmd( "SYTRALTOPO", cmdSytralTopo ); // Marielle 19/10/2021
    initCmd( "SYTRALGRILLE", cmdSytralGrille ); // Marielle 19/10/2021
    initCmd( "SYTRALAVALOIR", cmdSytralAvaloir ); // Marielle 19/10/2021
    initCmd( "SYTRALFILEDEAUZ", cmdSytralFileDeauZ );//Stephane 20/10/2021
    initCmd( "SYTRALPOLY", cmdSytralAddBlocTopo );//Stephane 20/10/2021
    initCmd( "SYTRALTOPOSEUL", cmdSytralCheckAloneBlocTopo ); //Stephane 20/10/2021
    initCmd( "SYTRALUNROTATE", cmdSytralUnrotate ); //Dinahasina 25/10/2021
    initCmd( "SYTRALTOPOALT", cmdSytralTopoAlt );//Stephane 26/10/2021
    initCmd( "SYTRALACCROCHETOPO", cmdSytralAccrocheTopo ); //Stephane 24/11/2021
    
    // Autres
    initCmd( "ESCAFLECHE", cmdEscaFleche ); //Stephane 26/07/2021
    initCmd( "GMTCHECKERROR", cmdGMTCheckError );
    
    // IGN
    initCmd( "IGNEXPORTCENTER", cmdIGNExportCenter );//Marielle 19/11/2021
    
    // Plateforme
    initCmd( "CENTREDWG", cmdGetDwgCenter ); //Dinahasina 24/01/2022
    
    // QRT
    initCmd( "QRTLISTSURF", cmdQrtListSurf );
    
    // Tabulations
    initCmd( "DELTAB", cmdDelTab ); //Marielle 01/04/2022
    
    // Statique
    initCmd( "MAJNGFALT", cmdMajngfAlt ); //Dinahasina 13/04/2022
    initCmd( "MAJNGFREF", cmdMajngfRef ); //Dinahasina 13/04/2022
}

void initCmd( AcString sCmdName, GcRxFunctionPtr FunctionAddr )
{
    //Declarer le global name
    AcString sGlobalName = sGroupName + "_" + sCmdName;
    
    //Creer le nom
    acedRegCmds->addCommand(
        sGroupName,
        sGlobalName,
        sCmdName,
        ACRX_CMD_MODAL + ACRX_CMD_USEPICKSET + ACRX_CMD_REDRAW,
        FunctionAddr );
}

void unloadApp()
{
    acedRegCmds->removeGroup( sGroupName );
}

extern "C" AcRx::AppRetCode
acrxEntryPoint( AcRx::AppMsgCode msg, void* appId )
{
    switch( msg )
    {
    
        case AcRx::kInitAppMsg:
            acrxDynamicLinker->unlockApplication( appId );
            acrxDynamicLinker->registerAppMDIAware( appId );
            
            initApp();
            
            acrxBuildClassHierarchy();
            
            break;
            
        case AcRx::kUnloadAppMsg:
            unloadApp();
            
            break;
    }
    
    return AcRx::kRetOK;
}