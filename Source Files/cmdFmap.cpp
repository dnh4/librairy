#pragma once

#include <xlnt/xlnt.hpp>
#include "cmdFmap.h"
#include "blockEntity.h"
#include "progressBar.h"
#include "fmapMath.h"
#include "constFmap.h"
#include "boundingBox.h"
#include <set>
#include <regex>
#include <direct.h>

//------------------------------------------- <PDAL>
//#include <pdal/Reader.hpp>
//#include <pdal/PointTable.hpp>
//#include <pdal/PointView.hpp>
//#include <io/LasReader.hpp>
//#include <io/LasHeader.hpp>
//#include <pdal/Options.hpp>
//#include <pdal/StageFactory.hpp>
//#include <pdal/StageWrapper.hpp>
#include "entityLib.h"
#include <fstream>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

double g_minSliceWidth = 0.1;
int g_sliceIteration = 1;
int g_sliceScaleFactor = 2;
double g_ToleranceEnZ = 0.5;//50 cm
double g_anglePI_2_Tol = 0.1745;// 10 deg
double g_wallLenght_Max = 0.5;// 50cm
double g_edgeLenght_Max = 1.0;// 100cm
double g_tol = 0.01;
double g_distYTextBloc = 0.7;// 70 cm
double g_distYTextPoly3D = 0.1;// 70 cm
double g_distMinTab = 1.0;  // 1 m

//Structure pour compter le nombre de doublons dans la commande BIMETIQUETER
struct xlsBlock
{
    double c_x;
    double c_y;
    string etage;
    int nfois ;
};


void cmdCorrectScale()
{
    // Récupérer la sélection
    ads_name ssBlock;
    
    vector< AcString > layers;
    AcString blocsDef = getLayer( RAIL_CORRECTSCALES, true );
    int length = getSsBlock( ssBlock, "", blocsDef );
    
    if( !length )
        return;
        
    int count = 0;
    
    for( int i = 0; i < length; i++ )
    {
    
        // Récupérer le bloc
        AcDbBlockReference* block = getBlockFromSs( ssBlock, i, AcDb::kForWrite );
        
        AcGeScale3d scaleBlock = block->scaleFactors();
        double scale = max( scaleBlock.sx, scaleBlock.sy );
        
        if( scaleBlock.sx != scaleBlock.sy )
            count++;
            
        scaleBlock.sx =  scale;
        scaleBlock.sy =  scale;
        
        block->setScaleFactors( scaleBlock );
        
        // Fermer le bloc
        block->close();
    }
    
    acedSSFree( ssBlock );
    
    print( "Nombre de blocs dont l'échelle a été uniformisée : %d", count );
}

void cmdCGName()
{
    ads_name ssBlock;
    long lengthBlock = getSsAllBlock( ssBlock );
    
    if( lengthBlock == 0 )
        print( "Aucun bloc" );
        
    ErrorStatus es = eNotApplicable;
    
    for( int counter = 0; counter < lengthBlock; counter++ )
    {
        //RECUPERATION DES INFORMATIONS DANS LE BLOC
        AcDbBlockReference* block = getBlockFromSs( ssBlock, counter, AcDb::kForWrite );
        
        if( !block )
        {
            acutPrintf( _T( "\nSelection failed!" ) );
            acedSSFree( ssBlock );
            return ;
        }
        
        // On vérifie le nom du bloc
        AcString blckName = _T( "" );
        es = getDynamicBlockName( blckName, block );
        
        if( es != eOk )
        {
            acutPrintf( _T( "\nErreur lors de la recupération du nom de bloc" ) );
            acedSSFree( ssBlock );
            return ;
        }
        
        /// Vérification du nome de bloc
        if( blckName.compare( BLOC_NAME_MEPAUTO ) != 0 )
        {
            block->close();
            continue ;
        }
        
        //Recupérer les attributs du bloc
        AcString debutAtt = "";
        AcString finAtt = "";
        
        getStringAttributValue( block, _T( "DEBUT" ), debutAtt );
        getStringAttributValue( block, _T( "FIN" ), finAtt );
        
        // transformation des chaines en Chiffres
        AcString d = debutAtt;
        AcString f = finAtt;
        AcString temp = debutAtt.substr( 4, 7 );
        debutAtt = debutAtt.substr( 0, 3 ) + temp;
        
        temp = finAtt.substr( 4, 7 );
        finAtt = finAtt.substr( 0, 3 ) + temp;
        
        int  debut = -1;
        stringToInt( debutAtt, &debut );
        
        int  fin;
        stringToInt( finAtt, &fin );
        
        int valAltb = int( ( debut + fin ) / 2 );
        
        // NUMERO_PLAN
        AcString numPlanAtt = _T( "ALTAMETRIS_719_" );
        numPlanAtt.append( numberToAcString( valAltb ) );
        numPlanAtt.append( _T( "_LL01" ) );
        
        // OUVRAGE
        AcString ouvrageAtt = _T( "PK " );
        ouvrageAtt.append( d );
        ouvrageAtt.append( _T( " à " ) );
        ouvrageAtt.append( f );
        
        // On applique les nouvelles valeurs
        // NUMERO_PLAN
        setAttributValue( block, _T( "NUMERO_PLAN" ), numPlanAtt );
        
        // OUVRAGE
        setAttributValue( block, _T( "OUVRAGE" ), ouvrageAtt );
        
        block->close();
    }
}

void cmdRobinet()
{
    //Sélection des blocs
    ads_name blSelection;
    int nbBlocks = getSsBlock( blSelection );
    
    //Initialisation
    std::set<AcString> attSet; //comprend tous les noms distincts des attributs
    ProgressBar* progressBar = new ProgressBar( _T( "Mise à jour des attributs" ), nbBlocks );
    
    //Compteurs qui seront renvoyés à l'utilisateur dans la console
    int countCreate = 0;
    int countDelete = 0;
    int countUpdate = 0;
    
    //Preparation des attributs
    for( int i = 0; i < nbBlocks; i++ )
    {
        //Récupération de la référence i
        AcDbBlockReference* pBlkRef = getBlockFromSs( blSelection, i );
        
        if( !pBlkRef ) continue;
        
        //Récupération des noms des attributs de la référence
        vector<AcString> attListRef = getAttributesNamesListOfBlockRef( pBlkRef );
        
        //Insertion dans le set
        int attListRefSize = attListRef.size();
        
        for( int j = 0; j < attListRefSize; ++j )
            attSet.insert( attListRef[j] );
            
        //Récupération de la définition associée à la référence
        AcDbBlockTableRecord* pBlkDef = getBlockDef( pBlkRef );
        
        if( !pBlkDef )
        {
            pBlkRef->close();
            continue;
        }
        
        //Récupération des noms des attributs de la définition
        vector<AcString> attListDef = getAttributesNamesListOfBlockDef( pBlkDef );
        
        //Insertion dans le set
        int attListDefSize = attListDef.size();
        
        for( int j = 0; j < attListDefSize; j++ )
            attSet.insert( attListDef[j] );
            
        //Fermeture de la définition et de la référence
        pBlkDef->close();
        pBlkRef->close();
    }
    
    acutPrintf( _T( "\nNombre d'attributs distincts : %i" ), attSet.size() );
    
    //Choix de la sélection par l'utilisateur
    ACHAR* input; //par defaut, selectionner tous les attributs
    AcString initGet = _T( "*" );
    AcString prompt = _T( "\nChoix de l'attribut à modifier : \n[<*>" );
    std::set<AcString>::iterator it;
    std::set<AcString>::iterator itBegin = attSet.begin();
    std::set<AcString>::iterator itEnd = attSet.end();
    
    //Concaténation des options pour l'initGet et le prompt
    for( it = itBegin; it != itEnd; it++ )
    {
        initGet += _T( " " ) + *it;
        prompt += _T( "/" ) + *it;
    }
    
    prompt += _T( "] " );
    
    acedInitGet( 0, initGet );
    int res = acedGetFullKword( acStrToAcharPointeur( prompt ), input );
    
    if( res == RTNONE )
        input = _T( "*" );
    else if( res == RTCAN )
    {
        acedSSFree( blSelection );
        delete( progressBar );
        print( res );
        return;
    }
    
    acutPrintf( input );
    
    //Modifications des blocs en itérant sur toutes les références de la sélection
    for( int i = 0; i < nbBlocks; i++ )
    {
        AcDbBlockReference* pBlkRef = getBlockFromSs( blSelection, i, AcDb::kForWrite );
        AcDbBlockTableRecord* pBlkDef = getBlockDef( pBlkRef );
        std::map<AcString, AcDbObjectId> attRefMap = getAttributesRef( pBlkRef );
        std::map<AcString, AcDbObjectId> attDefMap = getAttributesDef( pBlkDef );
        
        //Update de la progress bar
        progressBar->moveUp( i + 1 );
        
        //Si l'utilisateur sélectionne tous les attributs
        if( areEqualCase( input, _T( "*" ) ) )
        {
            //Itération sur les attributs de la référence courante
            std::map<AcString, AcDbObjectId>::iterator itRef;
            std::map<AcString, AcDbObjectId>::iterator itRefBegin = attRefMap.begin();
            std::map<AcString, AcDbObjectId>::iterator itRefEnd = attRefMap.end();
            
            for( itRef = itRefBegin; itRef != itRefEnd; itRef++ )
            {
                AcString inputStr = itRef->first;
                std::map<AcString, AcDbObjectId>::iterator itDef;
                itDef = attDefMap.find( inputStr );
                
                //Si l'attribut ne figure pas dans la définition
                if( itDef == attDefMap.end() )
                {
                    //Effacer attribut de la référence
                    if( res = removeAtt( itRef->second ) )
                    {
                        acedSSFree( blSelection );
                        delete( progressBar );
                        print( res );
                        return;
                    }
                    
                    countDelete++;
                }
                
                //S'il figure dans la définition
                else
                {
                    //Update
                    if( res = updateAtt( pBlkRef, itRef->second, itDef->second ) )
                    {
                        acedSSFree( blSelection );
                        delete( progressBar );
                        print( res );
                        return;
                    }
                    
                    countUpdate++;
                }
            }
            
            int countUpdateCopy = countUpdate;
            
            //Itération sur les attributs de la définition associée à la référence courante
            std::map<AcString, AcDbObjectId>::iterator itDef;
            std::map<AcString, AcDbObjectId>::iterator itDefBegin = attDefMap.begin();
            std::map<AcString, AcDbObjectId>::iterator itDefEnd = attDefMap.end();
            
            for( itDef = itDefBegin; itDef != itDefEnd; itDef++ )
            {
                AcString inputStr = itDef->first;
                itRef = attRefMap.find( inputStr );
                
                //Si l'attribut ne figure pas dans la référence
                if( itRef == attRefMap.end() )
                {
                    //Créer attribut, set position, update propriétés
                    if( res = createAtt( pBlkRef, itDef->second ) )
                    {
                        acedSSFree( blSelection );
                        delete( progressBar );
                        print( res );
                        return;
                    }
                    
                    countCreate++;
                }
                
                //S'il figure dans la référence
                else
                {
                    //Update
                    if( res = updateAtt( pBlkRef, itRef->second, itDef->second ) )
                    {
                        acedSSFree( blSelection );
                        delete( progressBar );
                        print( res );
                        return;
                    }
                    
                    countUpdate++;
                }
            }
            
            countUpdate = countUpdateCopy + ( countUpdate - countUpdate ) / 2; //Petite arnaque ici car chaque attribut maj est visité deux fois
        }
        
        //Si l'utilisateur sélectionne un seul attribut
        else
        {
            std::map<AcString, AcDbObjectId>::iterator itDef;
            itDef = attDefMap.find( input );
            std::map<AcString, AcDbObjectId>::iterator itRef;
            itRef = attRefMap.find( input );
            
            //S'il figure dans la définition
            if( itDef != attDefMap.end() )
            {
            
                //Et dans la référence
                if( itRef == attRefMap.end() )
                {
                    //Ajouter l'attribut à la référence
                    if( res = createAtt( pBlkRef, itDef->second ) )
                    {
                        acedSSFree( blSelection );
                        delete( progressBar );
                        print( res );
                        return;
                    }
                    
                    countCreate++;
                }
                
                //Mais pas dans la référence
                else
                {
                    //Update
                    if( res = updateAtt( pBlkRef, itRef->second, itDef->second ) )
                    {
                        acedSSFree( blSelection );
                        delete( progressBar );
                        print( res );
                        return;
                    }
                    
                    countUpdate++;
                }
            }
            
            //S'il ne figure pas dans la définition
            else
            {
                //Mais qu'il est dans la référence
                if( itRef != attRefMap.end() )
                {
                    //Supprimer l'attribut de la référence
                    if( res = removeAtt( itRef->second ) )
                    {
                        acedSSFree( blSelection );
                        delete( progressBar );
                        print( res );
                        return;
                    }
                    
                    countDelete++;
                }
            }
        }
        
        //Fermeture des blocs
        pBlkRef->close();
        pBlkDef->close();
    }
    
    acedSSFree( blSelection );
    
    delete( progressBar );
    
    acutPrintf( _T( "\n%i attribut(s) supprimé(s), %i attribut(s) ajouté(s) et %i attribut(s) mis à jour" ), countDelete, countCreate, countUpdate ) ;
}

void cmdDiscretizePoly()
{
    //Message de selection
    print( "Sélectionner les polylignes 3D à discrétiser" );
    
    ads_name ss;
    
    //Recuperer le nombre de polyligne dans le dessin
    long length = getSsPoly3D( ss );
    
    //Initialiser les
    double defDist = 1.0;
    double distU;
    
    // Demander le pas de discretisation
    if( askForDouble( _T( "Valeur de discrétisation" ), defDist, distU ) == RTCAN )
    {
        acedSSFree( ss );
        return;
    }
    
    // Nombre de polylignes discrétisées
    int c = 0;
    
    //Boucler sur les polylines
    Acad::ErrorStatus es = Acad::eOk;
    AcGePoint3dArray ptAr;
    
    for( int i = 0; i < length; i++ )
    {
        //i-eme polyligne dans la selection
        AcDb3dPolyline* poly3D = getPoly3DFromSs( ss, i, AcDb::kForWrite );
        
        //Initialiser le pointarray du nouveau polyligne
        es = discretizePoly( ptAr,
                poly3D,
                distU );
                
        if( es != Acad::eOk )
        {
            poly3D->close();
            print( es );
            continue;
        }
        
        c++;
        
        // On met à jour les sommets de la polyligne
        modifyPolyline3d( poly3D, ptAr );
        
        poly3D->close();
    }
    
    //Liberer la selection
    acedSSFree( ss );
    
    // Fermer la base de donne du dessin
    //closeModelSpace( blockTable );
    
    print( "Nombre de polylignes discrtises : %i ", c );
}

void cmdCleanBlockName()
{
    if( !isFileExisting( CLEANBLOCNAME_PATH ) )
    {
        acutPrintf( _T( "\nAucun fichier de configuration trouvé %s" ), CLEANBLOCNAME_PATH );
        return;
    }
    
    /// Recuperation du premier nom du fichier de configuration
    vector<AcString> vecNameToDel = layerFileToLayerVector( CLEANBLOCNAME_PATH );
    
    if( vecNameToDel.size() == 0 )
    {
        print( _T( "Aucun nom dans le fichier %s" ), CLEANBLOCNAME_PATH );
        return;
    }
    
    AcString strToReplace = vecNameToDel[0];
    ads_name ssblock, ssblock1;
    long block = getSsAllBlock( ssblock,  "", "Ele_Div$0$" );
    
    block = getSsAllBlock( ssblock1 );
    vector<AcString> vecDuplicateBlockName;
    /// Recuperer la table des blocks
    // Vecteur contenant les noms du bloc dans le dessin
    vector <AcString> blockList;
    
    AcDbObjectIdArray idsArrays;
    
    // On recupère la table des blocs
    AcDbBlockTable* bTable = getBlockTable( AcDb::kForWrite );
    
    // Iterateur sur les blocs dans la table
    AcDbBlockTableIterator* bTableIter;
    bTable->newIterator( bTableIter );
    
    // Variable pour les caractères à supprimmer
    
    // On parcourt la liste des blocs dans la table
    for( bTableIter; !bTableIter->done(); bTableIter->step() )
    {
        // On recupère l'id du bloc
        AcDbObjectId blockId = bTableIter->getRecordId();
        
        // On recupère la définition de bloc à partir de son id
        AcDbObject* obj = NULL;
        
        Acad::ErrorStatus es = acdbOpenAcDbObject( obj, blockId, AcDb::kForWrite );
        
        if( Acad::eOk != es )
            continue;
            
        AcDbBlockTableRecord* bRec = AcDbBlockTableRecord::cast( obj );
        
        if( !bRec ) continue;
        
        // On recupère le nom du bloc
        AcString blcName;
        bRec->getName( blcName );
        
        // On ajoute le nom du bloc dans le vecteur s'il ne commence pas par *
        int pos = blcName.find( strToReplace );
        
        /// Si on Trouve on supprime
        int length = blcName.length();
        
        if( pos >= 0 )
        {
            AcString start = blcName.substr( pos );
            AcString end  = blcName.substr( pos + 3, length - 1 );
            AcString newBlcName = start.append( end );
            
            // On change le nom du bloc
            es = bRec->setName( newBlcName );
            
            /// Dans le cas ou le nouveau nom de bloc existe déjà on replace la définition de bloc dans les références de blocs
            if( es == eDuplicateRecordName )
            {
                idsArrays.append( blockId );
                vecDuplicateBlockName.push_back( blcName );
                
                
                //vecDuplicateBlockName.push_back( blcName );
                
                AcDbObjectIdArray ids;
                Acad::ErrorStatus err = bRec->getBlockReferenceIds( ids, true, false );
                
                if( err == eOk )
                {
                    int size = ids.size();
                    
                    // On boucle sur les référernces de blocs pour change la définition de bloc
                    for( int idx = 0; idx < size; idx++ )
                    {
                        AcDbObjectId id = ids[idx];
                        AcDbEntity* entity = NULL;
                        err = acdbOpenAcDbEntity( entity, id, AcDb::kForWrite );
                        
                        if( Acad::eOk != err )
                            continue;
                            
                        AcDbBlockReference* bRef = AcDbBlockReference::cast( entity );
                        
                        if( !bRef ) continue;
                        
                        bRef->setBlockTableRecord( bRec->id() );
                        bRef->close();
                    }
                }
            }
        }
        
        //on ferme la définition du bloc
        bRec->close();
    }
    
    //on supprime l'iterateur
    delete bTableIter;
    
    //on ferme l'objet
    bTable->close();
}

void cmdGMTCheckError()
{
    //secuFmapApp();
    
    // Verification fichier de configuration
    if( !isFileExisting( GMT_CHECK_ERROR_PATH ) )
    {
        print( _T( "\nfichier de configuration %s est introuvable" ), GMT_CHECK_ERROR_PATH );
        return;
    }
    
    // Verification fichier de configuration pour les calques de mur
    if( !isFileExisting( GMT_CHECK_ERROR_WALL_PATH ) )
    {
        print( _T( "\nfichier de configuration %s est introuvable" ), GMT_CHECK_ERROR_WALL_PATH );
        return;
    }
    
    // Verification fichier de configuration pour les calques de haie
    if( !isFileExisting( GMT_CHECK_ERROR_EDGE_PATH ) )
    {
        print( _T( "\nfichier de configuration %s est introuvable" ), GMT_CHECK_ERROR_EDGE_PATH );
        return;
    }
    
    // Recuperer les calques des polylignes concernés par la comamnde
    AcString layer = getLayer( GMT_CHECK_ERROR_PATH, true );
    AcString wallLayer = getLayer( GMT_CHECK_ERROR_WALL_PATH, true );
    AcString edgeLayer = getLayer( GMT_CHECK_ERROR_EDGE_PATH, true );
    
    // Selection des polylignes concernees
    ads_name ssPoly3d;
    //long lenght = getSsAllPoly3D(ads_name, layer);
    print( _T( "Sélectionner les polylignes à vérifier " ) );
    long lenght = getSsPoly3D( ssPoly3d, layer );
    
    if( lenght == 0 )
    {
        print( _T( "Aucune polyligne de calques : %s dans le dessin" ), layer );
        acedSSFree( ssPoly3d );
        return;
    }
    
    /// Creation des calques pour les patates
    AcCmColor red;
    red.setRGB( 255, 0, 0 );
    Acad::ErrorStatus er = createLayer( MANQUE_EPAISSEUR_LAYER, red );
    
    // Verification
    if( er != Acad::eOk )
    {
        //Afficher message
        print( "Impossible de créer le calque de patate" );
        acedSSFree( ssPoly3d );
        return;
    }
    
    er = createLayer( ERREUR_SENS_LAYER, red );
    
    // Verification
    if( er != Acad::eOk )
    {
        //Afficher message
        print( "Impossible de créer le calque de patate" );
        acedSSFree( ssPoly3d );
        return;
    }
    
    int ep_error_count = 0;
    int sens_error_count = 0;
    
    /// On boucle sur les polylignes
    for( int counter = 0; counter < lenght; counter++ )
    {
        AcDb3dPolyline* poly3D = getPoly3DFromSs( ssPoly3d, counter );
        
        if( !poly3D )
        {
            print( _T( "Erreur lors de l'ouverture de la polyligne; operation arrétée" ) );
            acedSSFree( ssPoly3d );
            return;
        }
        
        /// On boucle sur les 3 premiers sommets si possible
        AcDb3dPolylineVertex* vertex = NULL;
        AcDbObjectIterator* iterPoly3D = poly3D->vertexIterator();
        int vtxSize = 0;
        AcGePoint3d firstPt, secondPt, thirdPt;
        
        // Initialiser l'itérateur
        iterPoly3D->start();
        
        for( iterPoly3D->start(); !iterPoly3D->done(); iterPoly3D->step() )
        {
            // Ouvrir le premier sommet de la polyligne
            if( Acad::eOk != poly3D->openVertex( vertex, iterPoly3D->objectId(), AcDb::kForRead ) )
            {
                poly3D->close();
                print( _T( "Erreur lors de l'ouverture de la polyligne; operation arrétée" ) );
                acedSSFree( ssPoly3d );
                return;
            }
            
            if( vtxSize == 0 )
            {
                firstPt = vertex->position();
                vtxSize++;
            }
            
            else if( vtxSize == 1 )
            {
                secondPt = vertex->position();
                vtxSize++;
            }
            
            else if( vtxSize == 2 )
            {
                thirdPt = vertex->position();
                vtxSize++;
            }
            
            // Fermer le vertex
            vertex->close();
            vertex = NULL;
            
            if( vtxSize > 2 )
                break;
        }
        
        // On supprime l'iterator
        delete iterPoly3D;
        
        // Recuperation du calque de la polyligne
        AcString poly3dLayer = poly3D->layer();
        
        // On ferme la polyligne
        poly3D->close();
        
        /// Verification de la partie Epaisseur
        /// 1. Verificaton du nombre de sommet, donc la présent de l'epaisseur
        if( vtxSize != 3 )
        {
            ep_error_count++;
            // On met une patate et on passe à la polyligne suivante
            er = drawCircle( firstPt,
                    PATATE_RADIUS,
                    MANQUE_EPAISSEUR_LAYER );
            continue;
        }
        
        /// 2. Verification de l'angle formé par les 3 premiers sommets sur le plan XY
        AcGeVector3d vec1 = AcGeVector3d( firstPt.x - secondPt.x, firstPt.y - secondPt.y, 0 );
        AcGeVector3d vec2 = AcGeVector3d( thirdPt.x - secondPt.x, thirdPt.y - secondPt.y, 0 );
        
        // Calcul de  l'angle entre les deux veteurs
        double angleVec = vec1.angleTo( vec2 );
        
        // Calculer la différence par rapport à PI/2
        double angleDiff = abs( angleVec - M_PI_2 );
        
        // Verification si la diférence depasse les 10 degré
        if( angleDiff > g_anglePI_2_Tol )
        {
            ep_error_count ++;
            // On met une patate et on passe à la polyligne suivante
            er = drawCircle( firstPt,
                    PATATE_RADIUS,
                    MANQUE_EPAISSEUR_LAYER );
            continue;
        }
        
        /// 3. verification longeur de segment par rapport à la liste des calques
        
        double lenght2FirstPt =  round( getDistance2d( firstPt, secondPt ), 2 );
        
        bool isLenghtOk = false;
        
        if( wallLayer.find( poly3dLayer ) != -1 )
        {
            if( lenght2FirstPt > g_wallLenght_Max )
            {
                ep_error_count ++;
                // On met une patate et on passe à la polyligne suivante
                er = drawCircle( firstPt,
                        PATATE_RADIUS,
                        MANQUE_EPAISSEUR_LAYER );
                continue;
            }
        }
        
        else
        {
            if( lenght2FirstPt > g_edgeLenght_Max )
            {
                ep_error_count ++;
                // On met une patate et on passe à la polyligne suivante
                er = drawCircle( firstPt,
                        PATATE_RADIUS,
                        MANQUE_EPAISSEUR_LAYER );
                continue;
            }
        }
        
        
        /// 4. Sens de ligne;
        // Caclul du produit vectoriel
        AcGeVector3d crossVect = vec1.crossProduct( vec2 );
        
        // Verification du sens par rapport à l'axe Z
        double dotProd = crossVect.dotProduct( AcGeVector3d::kZAxis );
        
        // Si opposé à Z
        if( dotProd > 0 )
        {
            sens_error_count++;
            // On met une patate et on passe à la polyligne suivante
            er = drawCircle( firstPt,
                    PATATE_RADIUS,
                    ERREUR_SENS_LAYER );
        }
        
        continue;
        
    }
    
    acutPrintf( _T( "\nCommande terminée, erreur epaisseur = %d, erreur sens = %d" ), ep_error_count, sens_error_count );
    acedSSFree( ssPoly3d );
}


void cmdReplaceBlockDyn()
{
    //Selection sur les blocs
    ads_name ssBlockDyn;
    
    //Demander nom du block
    AcString blockName = askString( _T( "Entrer le nom du bloc: " ) );
    
    //Verifier
    if( blockName == "" )
    {
        print( "Commande annulée" );
        
        return;
    }
    
    //Recuperer tous les elements du dessin
    long lenBlock = getSelectionSet( ssBlockDyn, "X", "INSERT" );
    
    //Verifier
    if( lenBlock == 0 )
    {
        print( "Aucun block connu dans le dessin" );
        
        return;
    }
    
    int compt = 0;
    
    //Reference de bloc
    AcDbBlockReference* block3D = NULL;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), lenBlock );
    
    //Boucle sur les blocs
    for( int i = 0 ; i < lenBlock; i++ )
    {
        //Recuperer le i-eme block
        block3D = getBlockFromSs( ssBlockDyn, i, AcDb::kForWrite );
        
        //Verifier
        if( !block3D )
            continue;
            
        //Recuperer le nom du bloc
        AcString dynBlockName = _T( "" );
        getDynamicBlockName( dynBlockName, block3D );
        
        //Verifier
        if( dynBlockName != blockName )
        {
            //Fermer le bloc
            block3D->close();
            
            continue;
        }
        
        //Recuperer la liste des attributs du bloc
        map<AcString, AcString> attMap = getBlockAttWithValuesList( block3D );
        
        //Recuperer le calque du bloc
        AcString blockLayer = block3D->layer();
        
        AcStringArray propName;
        AcStringArray propValue;
        
        //Recuperer les proprietes
        Acad::ErrorStatus er = getBlockPropWithValueList( block3D,
                propName,
                propValue );
                
        //Inserer le nouveau bloc
        AcDbBlockReference* newBlock = insertFmapDynamicBlockReference( blockName,
                block3D->position(),
                propName,
                propValue,
                blockLayer,
                block3D->rotation(),
                block3D->scaleFactors().sx,
                block3D->scaleFactors().sy,
                block3D->scaleFactors().sz );
                
        //Boucle pour changer les attributs du bloc
        map<AcString, AcString>::iterator itAtt;
        
        for( itAtt = attMap.begin(); itAtt != attMap.end(); itAtt++ )
        {
            setAttributValue( newBlock,
                itAtt->first,
                itAtt->second );
        }
        
        //Replacer le bloc
        newBlock->setPosition( block3D->position() );
        
        //Fermer le bloc
        newBlock->close();
        
        //Supprimer l'ancien bloc
        block3D->erase();
        
        block3D->close();
        
        //Compter
        compt++;
        
        //Progresser la barre
        prog.moveUp( i );
    }
    
    //Liberer les selections
    acedSSFree( ssBlockDyn );
    
    //Afficher message
    print( "Nombre de blocs remplacés: %d", compt );
    
    return;
}


void cmdExportPoint()
{
    //Selection sur les blocs
    ads_name ssBloc;
    
    //Afficher message
    print( "Selectionner les blocs" );
    
    //Selection des blocs
    long lenBloc = getSsBlock( ssBloc, "", _T( "RTE" ) );
    
    //Verifier
    if( lenBloc == 0 )
    {
        print( "Selection vide, Sortie" );
        return;
    }
    
    //Vecteur de int
    vector<int> vectInt;
    int defValue = 0;
    int newValue = 0;
    
    //Demander le code à inserer dans le fichier
    while( RTCAN != askForInt( _T( "Entrer le code: (Entrée pour valider une valeur, Echap pour valider les valeurs)" ), defValue, newValue ) )
        vectInt.push_back( newValue );
        
    //Recuperer la taille
    int taille = vectInt.size();
    
    if( taille == 0 )
    {
        acedSSFree( ssBloc );
        return;
    }
    
    //Supprimer les doublons dans le vecteur
    sort( vectInt.begin(), vectInt.end() );
    vectInt.erase( unique( vectInt.begin(), vectInt.end() ), vectInt.end() );
    
    //Recuperer le dossier contenant le fichier
    AcString filePath = getCurrentFileFolder();
    
    //Recuperer le nom du fichier
    AcString fileName = getCurrentFileName();
    
    //Chemin vers le fichier de sortie
    AcString fileToExport = fileName + _T( ".csv" );
    
    //Si le fichier n'existe pas on le crée
    if( !isFileExisting( acStrToStr( fileToExport ) ) )
    {
        //Creer le fichier de controle
        std::ofstream file( acStrToStr( fileToExport ) );
        
        //Fermer le fichier
        file.close();
    }
    
    ProgressBar prog = ProgressBar( _T( "Progression" ), lenBloc );
    
    //Ouvrir le fichier csv
    ofstream fPointFile( acStrToStr( fileToExport ).c_str() );
    
    //Ajouter entete
    fPointFile << "CODE" << ";" << "X" << ";" << "Y" << ";" << "Z" << "\n";
    
    //Bloc
    AcDbBlockReference* bloc = NULL;
    
    //Boucle sur les blocs
    for( int i = 0; i < lenBloc; i++ )
    {
        //Recuperer le ieme bloc
        bloc = getBlockFromSs( ssBloc, i );
        
        if( !bloc )
            continue;
            
        //Recuperer le code
        AcString code = getAttributValue( bloc, _T( "CODE" ) );
        
        //Caster en int
        int a = stoi( acStrToStr( code ) );
        
        //Recuperer le point d'insertion du bloc
        AcGePoint3d posBlock = bloc->position();
        
        //Fermer le bloc
        bloc->close();
        
        //Verifier si on a le code ou non
        for( int j = 0; j < taille; j++ )
        {
            //Tester la valeur
            if( a == vectInt[j] )
            {
                //Ajouter dans le fichier
                fPointFile << a << ";" << std::fixed << std::setprecision( 6 ) << posBlock.x << ";" << std::fixed << std::setprecision( 6 ) << posBlock.y << ";" << posBlock.z << "\n";
            }
        }
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Fermer le fichier
    fPointFile.close();
    
    return;
}


void cmdExportDalle()
{
    //Selection sur les polylignes 2d
    ads_name ssPoly2d;
    
    //Selection sur les textes
    ads_name ssText;
    
    //Selection sur les patates
    ads_name ssCircle;
    
    //Demander de selectionner les textes
    print( "Selectionner les textes" );
    long lenText = getSsText( ssText );
    
    if( lenText == 0 )
        return;
        
    //Demander de selectionner les polyligne 2d
    print( "Selectionner les polylignes" );
    long lenPoly2d = getSsPoly2D( ssPoly2d );
    
    if( lenPoly2d == 0 )
    {
        acedSSFree( ssText );
        
        return;
    }
    
    //Demander de selectionner les patates
    print( "Selectionner les patates" );
    long lenPatate = getSsCircle( ssCircle );
    
    if( lenPatate == 0 )
    {
        acedSSFree( ssText );
        acedSSFree( ssPoly2d );
        return;
    }
    
    //ErrorStatus
    Acad::ErrorStatus er;
    
    //Recuperer le dossier contenant le fichier
    AcString filePath = getCurrentFileFolder();
    
    //Recuperer le nom du fichier
    AcString fileName = getCurrentFileName();
    
    //Chemin vers le fichier de sortie
    AcString fileToExport = fileName + _T( ".csv" );
    
    //Si le fichier n'existe pas on le crée
    if( !isFileExisting( acStrToStr( fileToExport ) ) )
    {
        //Creer le fichier de controle
        std::ofstream file( acStrToStr( fileToExport ) );
        
        //Fermer le fichier
        file.close();
    }
    
    //Vecteur de texte, poly2d,
    vector<AcDbExtents> vecPoly;
    vector<AcGePoint3d> vecCircle;
    
    //Remplir le vecteur
    AcDbPolyline* poly2d;
    AcDbExtents bbPoly;
    
    for( int i = 0; i < lenPoly2d; i++ )
    {
        poly2d = getPoly2DFromSs( ssPoly2d, i );
        
        if( er = poly2d->getGeomExtents( bbPoly ) )
        {
            print( er );
            return;
        }
        
        //Ajouter le bb dans le vecteur
        vecPoly.push_back( bbPoly );
        
        //Fermer la polyligne
        poly2d->close();
    }
    
    //Remplir les patate
    AcDbCircle* circle;
    
    for( int i = 0; i < lenPatate; i++ )
    {
        circle = getCircleFromSs( ssCircle, i );
        
        //Ajouter le centre du cercle dans le vecteur
        vecCircle.push_back( circle->center() );
        
        //Fermer le cercle
        circle->close();
    }
    
    //Structure dalle
    vector<Dalle> dalleVec;
    struct Dalle dalle;
    
    int maxZone = 0;
    
    //Boucler sur les textes
    AcDbText* text;
    
    for( int i = 0; i < lenText; i++ )
    {
        //Recuperer le i-eme texte
        text = getTextFromSs( ssText, i, AcDb::kForRead );
        
        //Recuperer le nom du zone
        AcString layerZone = text->layer();
        AcString zone = layerZone.substr( 1, 2 );
        
        //Ajouter dans le structure
        dalle.zoneDalle = stoi( acStrToStr( zone ) );
        
        if( dalle.zoneDalle > maxZone )
            maxZone = dalle.zoneDalle;
            
        //Recuperer le nom de la dalle
        AcString nameDalle = text->textString();
        
        //Ajouter dans le structure
        dalle.nomDalle = nameDalle;
        
        dalle.nombreDePatate = 0;
        
        //Boucle sur les polylignes
        for( int j = 0; j < vecPoly.size(); j++ )
        {
            //Tester si le texte est à l'interieur de la polyligne
            if( !isInBoundingBox( vecPoly[j], text->position() ) )
                continue;
                
            //Boucle sur les patate
            for( int k = 0; k < vecCircle.size(); k++ )
            {
                //Tester si la patate est à l'interieur de la polyligne
                if( isInBoundingBox( vecPoly[j], vecCircle[k] ) )
                {
                    dalle.nombreDePatate++;
                    
                    //Supprimer la patate dans le vecteur
                    vecCircle.erase( vecCircle.begin() + k );
                    
                    k--;
                }
            }
            
            //Supprimer le bounding box dans le vecteur
            vecPoly.erase( vecPoly.begin() + j );
            
            break;
        }
        
        //Ajouter le structure dans le vecteur
        dalleVec.push_back( dalle );
        
        //Fermer le texte
        text->close();
    }
    
    //Ouvrir le fichier csv
    ofstream fDalleFile( acStrToStr( fileToExport ).c_str() );
    
    //Ajouter entete
    fDalleFile << "Zone" << ";" << "Nom" << ";" << "Nombre de patate" << "\n";
    
    //Boucler sur le vecteur de structure
    for( int i = 1 ; i <= maxZone; i++ )
    {
        for( int j = 0; j < dalleVec.size(); j++ )
        {
            if( i == dalleVec[j].zoneDalle )
            {
                //Ajouter les infos dans le fichier csv
                fDalleFile << dalleVec[j].zoneDalle << ";" << acStrToStr( dalleVec[j].nomDalle ) << ";" << dalleVec[j].nombreDePatate << "\n";
                
                //Enlever l'infos dans le vecteur
                dalleVec.erase( dalleVec.begin() + j );
                
                j--;
            }
        }
    }
    
    //Fermer le fichier
    fDalleFile.close();
    
    //Liberer les selections
    acedSSFree( ssText );
    acedSSFree( ssPoly2d );
    acedSSFree( ssCircle );
    
    //Sortir
    return;
}


void cmdAddBlocOnAllVertex()
{
    //Déclaration d'une selection sur les polylignes
    ads_name ssPoly;
    
    //Afficher une message
    print( "Selectionner les polylignes" );
    
    //Récupérer les polylignes dans les calques dans le fichier texte
    long lengthPoly = getSsPoly3D( ssPoly, "" );
    
    //Tester si la selection est valide
    if( lengthPoly == 0 )
        return;
        
    //Nom du bloc
    AcString blockTopoName = _T( "" );
    
    //Demander d'entrer le nom du bloc à inserer
    blockTopoName = askString( _T( "Entrer le nom du bloc à insérer" ) );
    
    //Verifier
    if( blockTopoName == _T( "" ) )
    {
        print( "Commande annulée, Sortie" );
        
        acedSSFree( ssPoly );
        return ;
    }
    
    //Appeler la fonction addBlockTopoOnForgetedPolyVertex
    if( Acad::eOk != addTopoOnAllVertex( ssPoly, blockTopoName ) )
    {
        //Afficher une message
        print( "Impossible d'ajouter des points topos sur les sommets des polylignes" );
        
        //Libérer la selection
        acedSSFree( ssPoly );
        
        return;
    }
    
    //Sortir de la fonction
    return;
}

void cmdAddPointXYZPoly3d()
{
    // Declaration des variables
    ads_point point;
    ads_name poly3DSelection;
    
    // Demander au dessinateur de Sélectionner la FMPoly3D et ou insérer le point
    int es = acedEntSel( _T( "\nCliquez sur la polyligne 3d" ), poly3DSelection, point );
    
    if( es != RTNORM )
    {
        acutPrintf( _T( "\nProblème de sélection" ) );
        acedSSFree( poly3DSelection );
        return;
    }
    
    bool gotaPtXY = false;
    bool validatePtXY = false;
    bool gotaPtZ = false;
    bool validatePtZ = false;
    
    AcDbObjectId newVertexId;
    AcDbObjectId PrevVertexId;
    AcGePoint3d ptRef = adsToAcGe( point );
    AcDb3dPolyline* poly3D = NULL;
    AcDb3dPolylineVertex* oldVertex = NULL;
    // On récupère le point dans le SCU général
    getWcsPoint( &ptRef );
    
    // Récupere la poly3D
    AcDbObjectId id = AcDbObjectId::kNull;
    
    //On récupère l'Id de la pièce sélectionnée
    acdbGetObjectId( id, poly3DSelection );
    
    //On récupère un pointeur sur l'entity sélectionnée
    AcDbEntity* pEnt = NULL;
    acdbOpenAcDbEntity( pEnt, id, AcDb::kForWrite );
    
    if( pEnt )
    {
        // Vérification du type de class
        if( pEnt->isKindOf( AcDb3dPolyline::desc() ) )
            poly3D = AcDb3dPolyline::cast( pEnt );
        else
            pEnt->close();
    }
    
    if( !poly3D )
    {
        acutPrintf( _T( "\nProblème de sélection" ) );
        acedSSFree( poly3DSelection );
        return;
    }
    
    // Tester si le pointeur sur la  polyline3d est null
    if( poly3D )
    {
        long indexAfter = -1;
        bool isForArc;
        
        AcGePoint3d ptRefcopy = ptRef;
        AcGePoint3d ptRef3d;
        
        if( poly3D->getClosestPointTo( ptRef, AcGeVector3d::kZAxis, ptRef3d )  == eOk )
        {
        
            /// 1- recherche de l'index de point précédent
            if( ( searchPreviewVertextIdToPoint( poly3D, ptRef3d, PrevVertexId, g_tol ) != Acad::eOk ) || ( PrevVertexId.isNull() ) )
            {
                // Si on trouve pas le point précédent; On arrete l'operation
                acedSSFree( poly3DSelection );
                poly3D->close();
                return;
            }
            
            // On demande le nouveau sommet à insérer
            AcGePoint3d newPt;
            
            // On demande le point à insérer
            es = getWcsPoint( MSG_ASK_XY, &newPt, ptRef3d );
            
            while( ( es == RTNORM ) || ( es == RTNONE ) )
            {
                if( es == RTNORM )
                {
                    // Suppression du précédent nouveau point
                    if( gotaPtXY )
                    {
                        if( !validatePtXY )
                        {
                            /// Suppression de l'ancien point
                            if( Acad::eOk == poly3D->openVertex( oldVertex, newVertexId, AcDb::kForWrite ) )
                            {
                                oldVertex ->erase();
                                oldVertex->close();
                                oldVertex = NULL;
                            }
                        }
                        
                        else
                        {
                            /// Suppression de l'ancien point
                            if( Acad::eOk == poly3D->openVertex( oldVertex, newVertexId, AcDb::kForWrite ) )
                            {
                                AcGePoint3d pos = oldVertex->position();
                                pos.z = newPt.z;
                                newPt = pos;
                                oldVertex ->erase();
                                oldVertex->close();
                                oldVertex = NULL;
                            }
                        }
                    }
                    
                    /// 2- Ajout du nouveau sommet
                    AcDb3dPolylineVertex* newVertex = new AcDb3dPolylineVertex( newPt );
                    
                    if( poly3D->insertVertexAt( newVertexId, PrevVertexId, newVertex ) != eOk )
                    {
                        delete newVertex;
                        newVertex = NULL;
                        acedSSFree( poly3DSelection );
                        poly3D->close();
                        return;
                    }
                    
                    newVertex->close();
                    
                    redrawEntity( poly3D );
                    
                    ptRefcopy = newPt;
                    
                    if( !gotaPtXY )
                        gotaPtXY = true;
                        
                    if( validatePtXY )
                    {
                        if( !gotaPtZ )
                            gotaPtZ = true;
                    }
                }
                
                else
                {
                    /// Si validation
                    if( es == RTNONE )
                    {
                        if( !validatePtXY )
                            validatePtXY = true;
                        else
                        {
                            if( !validatePtZ )
                            {
                                validatePtZ = true;
                                break;
                            }
                        }
                    }
                    
                    else if( es == RTCAN )
                        break;
                }
                
                // On demande le point à insérer
                AcString strMsg;
                
                if( !validatePtXY )
                    strMsg.format( MSG_ASK_XY_VALDIATE, ptRefcopy.z );
                else if( !gotaPtZ )
                    strMsg.format( MSG_ASK_Z, ptRefcopy.z );
                else
                    strMsg.format( MSG_ASK_Z_VALDIATE, ptRefcopy.z );
                    
                es = getWcsPoint( strMsg, &newPt, ptRefcopy );
            }
            
            // Touche Echap
            if( es == RTCAN )
            {
                if( validatePtXY )
                {
                    /// Suppression de l'ancien point
                    if( Acad::eOk == poly3D->openVertex( oldVertex, newVertexId, AcDb::kForWrite ) )
                    {
                        oldVertex ->erase();
                        oldVertex->close();
                        oldVertex = NULL;
                    }
                }
                
                acutPrintf( _T( "Opération annulée \n" ) );
                acedSSFree( poly3DSelection );
                
                poly3D->close();
                return;
            }
            
            else if( es != RTNORM )
            {
                acutPrintf( _T( "Opération terminée \n" ) );
                acedSSFree( poly3DSelection );
                
                poly3D->close();
                return;
            }
            
            else
                acutPrintf( _T( "\nVous n'avez pas sélectionné d'objet FuturMap Poly3D" ) );
                
            poly3D->close();
        }
    }
    
    acedSSFree( poly3DSelection );
}


void cmdCopyBlockLayer()
{
    //Selection sur un bloc
    ads_name ssBloc1;
    
    //Demander de selectionner le premier bloc
    print( "Selectionner le premier bloc" );
    
    //Selection du bloc
    long lenBloc1 = getSsBlock( ssBloc1 );
    
    //Verifier
    if( lenBloc1 != 1 )
    {
        print( "Nombre de bloc différent de 1, Sortie" );
        acedSSFree( ssBloc1 );
        return;
    }
    
    //Demander de selectionner le second bloc
    print( "Selectionner le second bloc" );
    
    //Selection du bloc
    ads_name ssBloc2;
    long lenBloc2 = getSsBlock( ssBloc2 );
    
    //Verifier
    if( lenBloc2 == 0 )
    {
        print( "Nombre de bloc différent de 1, Sortie" );
        acedSSFree( ssBloc1 );
        return;
    }
    
    //Recuperer le bloc1
    AcDbBlockReference* block1 = getBlockFromSs( ssBloc1, 0, AcDb::kForRead );
    
    //Verifier
    if( !block1 )
    {
        //Afficher message
        print( "Impossible de recuperer les blocs, sortie" );
        acedSSFree( ssBloc1 );
        acedSSFree( ssBloc2 );
        return;
    }
    
    //Recuperer le nom du block 1
    AcString blockName;
    getBlockName( blockName, block1 );
    
    //Barre de progression
    ProgressBar* progressBar = new ProgressBar( _T( "Progression" ), lenBloc2 );
    
    //Boucle sur les autres bloc
    for( int i = 0; i < lenBloc2; i++ )
    {
        //Recuperer le bloc2
        AcDbBlockReference* block2 = getBlockFromSs( ssBloc2, i, AcDb::kForWrite );
        
        //Recuperer les infos du bloc2
        AcGePoint3d b2Pos = block2->position();
        AcGeScale3d scale = block2->scaleFactors();
        double rotation = block2->rotation();
        
        //Supprimer le bloc2
        block2->erase();
        block2->close();
        
        //Creer une reference de bloc
        AcDbBlockReference* newBlock = insertBlockReference( blockName,
                b2Pos,
                rotation,
                scale.sx,
                scale.sy,
                scale.sz );
                
        //Recuperer les propriétés de l'autre bloc
        newBlock->setPropertiesFrom( block1 );
        
        //Fermer le nouveau bloc
        newBlock->close();
        
        //Progresser
        progressBar->moveUp( i );
    }
    
    //Fermer l'ancien block
    block1->close();
    
    //Liberer les selections
    acedSSFree( ssBloc1 );
    acedSSFree( ssBloc2 );
}


void cmdLoadOrthoTiff()
{
    //Afficher message
    print( "Selectionner un fichier dans le dossier" );
    
    //Demander à l'utilisateur de selectionner un fichier dans le dossier
    AcString filePath = askForFilePath();
    int ind = filePath.findOneOfRev( _T( "\\" ) );
    filePath = filePath.substr( 0, ind );
    
    //Demander d'entrer le level
    int lev = 0;
    
    if( RTCAN == askForInt( _T( "Entrer le level (0 pour tout): " ), lev, lev ) )
    {
        //Afficher message
        print( "Commande annulée, Sortie" );
        
        return;
    }
    
    //Resultat
    vector<string> fileName;
    
    //Structure dirent
    struct dirent* entry;
    DIR* dir = opendir( acStrToStr( filePath ).c_str() );
    
    //Verifier
    if( dir == NULL )
        return;
        
    //Boucle pour recuperer les noms des fichiers dans le dossier
    while( ( entry = readdir( dir ) ) != NULL )
        fileName.push_back( entry->d_name );
        
    //Compteur
    int comp = 0;
    
    //Nettoyer le vecteur de string
    fileName = cleanVectorOfName( fileName );
    
    //Recuperer la taille du vecteur de nom de fichier
    int lenFileName = fileName.size();
    
    //Verifier
    if( lenFileName == 0 )
        return;
        
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Chargement des images" ), lenFileName );
    
    //Image
    AcDbRasterImage* image = NULL;
    
    //Boucle sur chaque nom de fichier
    for( int i = 0; i < lenFileName; i++ )
    {
        //Recuperer le premier nom
        AcString name = strToAcStr( fileName[i] );
        
        //Nettoyer le nom
        ind = name.findOneOfRev( _T( "." ) );
        name = name.substr( 0, ind );
        
        //Recuperer le chemin vers le fichier .tfw
        AcString fPath = filePath + _T( "\\" ) + name + _T( ".tfw" );
        
        //Ouvrir le fichier
        ifstream fFile( acStrToStr( fPath ) );
        
        //Ligne du fichier
        string line = "";
        
        //Creer un point
        AcGePoint3d ptInsert = AcGePoint3d::kOrigin;
        
        int c = 0;
        
        //Recuperer le point d'insertion de l'image
        while( getline( fFile, line ) )
        {
            if( c == 4 )
                ptInsert.x = stod( line );
                
            if( c == 5 )
                ptInsert.y = stod( line );
                
            c++;
        }
        
        //Fermer le fichier
        fFile.close();
        
        //Inserer l'image
        image = insertRasterImage( filePath + _T( "\\" ) + name + _T( ".tiff" ),
                ptInsert,
                name + _T( ".tiff" ) );
                
        //Verifier
        if( !image )
            continue;
            
        //Verifier le calque de l'image
        if( name.find( _T( "_level_1" ) ) != -1 )
        {
            createLayer( _T( "Fmap_ortho_Png1" ) );
            image->setLayer( _T( "Fmap_ortho_Png1" ) );
        }
        
        else if( name.find( _T( "_level_2" ) ) != -1 )
        {
            createLayer( _T( "Fmap_ortho_Png2" ) );
            image->setLayer( _T( "Fmap_ortho_Png2" ) );
        }
        
        else if( name.find( _T( "_level_3" ) ) != -1 )
        {
            createLayer( _T( "Fmap_ortho_Png3" ) );
            image->setLayer( _T( "Fmap_ortho_Png3" ) );
        }
        
        //Fermer l'image
        image->close();
        
        i++;
        
        //Compter
        comp++;
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Afficher message
    print( "Nombre de fichier tiff importé: %d", comp );
}


void cmdFilterPcj()
{
    //Afficher message
    print( "Selectionner le fichier .pcj dans le dossier" );
    
    //Demander à l'utilisateur de selectionner un fichier dans le dossier
    AcString filePath = askForFilePath( true,  _T( "pcj" ) );
    
    //Verifier
    if( filePath == _T( "" ) )
    {
        //Afficher message
        print( "Commande annulée, Sortie" );
        
        return;
    }
    
    //Recuperer le dossier ou on va mettre le fichier
    int ind = filePath.findOneOfRev( _T( "\\" ) );
    AcString outFolder = filePath.substr( 0, ind );
    
    //Vecteur qui va contenir tous les noms du fichier dans le dossier
    vector<string> fileName;
    
    //Structure dirent
    struct dirent* entry;
    DIR* dir = opendir( acStrToStr( outFolder ).c_str() );
    
    //Verifier
    if( dir == NULL )
        return;
        
    //Boucle pour recuperer les noms des fichiers dans le dossier
    while( ( entry = readdir( dir ) ) != NULL )
        fileName.push_back( entry->d_name );
        
    //Nettoyer le vecteur de nom
    fileName = cleanVectorOfName( fileName );
    
    //Recuperer la taille du vecteur de nom
    int taille = fileName.size();
    
    //Ouvrir le fichier pcj
    ifstream fFile( acStrToStr( filePath ) );
    
    //Ligne du fichier
    string line = "";
    
    //Vecteur resultat
    vector<string> result;
    
    //Boucle sur le fichier .pcj
    while( getline( fFile, line ) )
    {
        //Rechercher le pcd
        size_t found = line.find( ".pcd" );
        
        //Si on est sur l"entete on recupere l'entete
        if( found == std::string::npos )
            result.push_back( line );
            
        //Si on trouve on test si le .pcd est dans le dossier
        else
        {
            //Recuperer le nom du fichier
            string temp = line.substr( 0, found );
            
            //Boucle sur le vecteur nom du fichier
            for( int i = 0; i < taille; i++ )
            {
                //Rechercher
                if( fileName[i].find( temp ) != std::string::npos )
                    result.push_back( line );
            }
        }
    }
    
    //Fermer le fichier
    fFile.close();
    
    //Creer un fichier
    std::ofstream outFile( acStrToStr( outFolder + _T( "\\new_Project.pcj" ) ) );
    
    //Recuperer la taille du resultat
    taille = result.size();
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), taille );
    
    //Boucle sur le resultat
    for( int i = 0; i < taille ; i++ )
    {
        //Ajouter la ligne de string dans le nouveau fichier
        outFile << result[i];
        
        //Ajouter le à la ligne
        if( i != taille - 1 )
            outFile << "\n";
            
        //Progresser la barre
        prog.moveUp( i );
    }
    
    //Fermer le fichier
    outFile.close();
    
    //Supprimer l'ancien fichier
    int res = remove( filePath );
    
    //Afficher message
    print( "Commande terminée!" );
}


void cmdChangeLayerOfText()
{
    //Selection sur les textes
    ads_name ssText;
    
    //Afficher message
    print( "Selectionner les textes" );
    
    //Selection des entité
    long lenText = getSsObject( ssText );
    
    //Verifier
    if( lenText == 0 )
    {
        print( "Aucun texte recuperé" );
        return;
    }
    
    AcDbEntity* entity = NULL;
    
    //Texte
    AcDbMText* text = NULL;
    
    //Compteur
    int comp = 0;
    
    //Boucle sur les textes
    for( int i = 0; i < lenText; i++ )
    {
        entity = getEntityFromSs( ssText, i, AcDb::kForWrite );
        
        if( !entity )
            continue;
            
        if( entity->isKindOf( AcDbMText::desc() ) )
            text = AcDbMText::cast( entity );
        else
        {
            entity->close();
            continue;
        }
        
        //Verifier
        if( !text )
            continue;
            
        //Recuperer le calque du texte
        AcString layerText = text->layer();
        
        //Recuperer le calque du texte
        AcString newLayer = getLayerOfThisText( text->text(), layerText );
        
        if( newLayer != _T( "" ) )
        {
            text->setLayer( newLayer );
            comp++;
        }
        
        //Fermer le texte
        text->close();
    }
    
    //Liberer la selection
    acedSSFree( ssText );
    
    //Afficher message
    print( "Nombre de texte dont le calque a été changé : %d", comp );
}

void cmdArrondirH()
{
    int countTxtH = 0;
    
    //Selection sur les textes
    ads_name ssTxt;
    
    //Afficher message
    print( "Selectionner les textes pour lesquels il faut arrondir la hauteur" );
    
    //Selection des polylignes 3d
    long lenTxt = getSsMText( ssTxt, _T( "T-FERM-MUR-T,T-FERM-CLOTURE-T,T-BATI-EMPRISE_DUR-T,T-BATI-EMPRISE_LEGER-T,T-VEGE-ARBRE-T" ) );
    
    //Verifier
    if( lenTxt == 0 )
    {
        print( "Aucun texte a traiter, Sortie." );
        return;
    }
    
    // On parcourt les textes
    for( int i = 0; i < lenTxt; i++ )
    {
        AcDbMText* mtxtEntity = getMTextFromSs( ssTxt, i, AcDb::kForWrite );
        AcString acTxt = mtxtEntity->text();
        string txt = acStrToStr( acTxt );
        
        // On vérifie que le texte a bien une information hauteur
        int posH = txt.find( "h=" );
        
        if( posH >= txt.size() )
        {
            mtxtEntity->close();
            acedSSFree( ssTxt );
            return;
        }
        
        // On regarde jusqu'où cette information doit être prise en compte
        int posM = posH;
        
        for( int j = posH + 1; j < txt.size(); j++ )
        {
            string subTxt = txt.substr( j, 1 );
            
            if( subTxt == "m" )
            {
                posM = j;
                break;
            }
        }
        
        if( posM >= txt.size() )
        {
            print( "Ce texte n'a pas d'unité." );
            mtxtEntity->close();
            acedSSFree( ssTxt );
            return;
        }
        
        // On modifie la string avec la hauteur
        double h;
        AcString begOfString = acTxt.substr( posH + 2 );
        AcString endOfString = acTxt.substrRev( txt.size() - posM );
        AcString height = acTxt.substr( posH + 2, posM - posH - 2 );
        stringToDouble( height, &h );
        
        AcString layer = mtxtEntity->layer();
        
        if( layer == _T( "T-VEGE-ARBRE-T" ) )
        {
            double arrH = int( h * 10.0 + 0.5 ) / 10.0;
            
            AcString newH = numberToAcString( arrH, 2 );
            mtxtEntity->setContents( begOfString.concat( newH.concat( endOfString ) ) );
            
            countTxtH++;
        }
        
        else
        {
            double arrH = std::ceil( ( int )( 20 * h + 0.5 ) * 100.0 / 20.0 ) / 100.0;
            
            AcString newH = numberToAcString( arrH, 2 );
            mtxtEntity->setContents( begOfString.concat( newH.concat( endOfString ) ) );
            
            countTxtH++;
        }
        
        mtxtEntity->close();
    }
    
    print( "%i hauteurs arrondies", countTxtH );
    acedSSFree( ssTxt );
}

void cmdRoadName()
{
    //Selection sur les MTextes
    ads_name ssMTxt;
    
    //Afficher message
    print( "Selectionner les noms de rue MTexte" );
    
    //Selection des polylignes 3d
    long lenMTxt = getSsMText( ssMTxt, _T( "A-VOIE-CHAUSSEE-T" ) );
    
    // On parcourt les textes
    for( int i = 0; i < lenMTxt; i++ )
    {
        // On vérifie si le texte est en majuscule
        AcDbMText* mtxtEntity = getMTextFromSs( ssMTxt, i, AcDb::kForWrite );
        AcString acTxt = mtxtEntity->text();
        string txt = acStrToStr( acTxt );
        vector<string> words = splitString( txt, " " );
        
        for( int j = 0; j < words.size(); j++ )
        {
            bool isCapital = true;
            
            for( int k = 0; k < words[j].size(); k++ )
            {
                string letter = words[j].substr( k, 1 );
                char letterChar = *letter.c_str();
                
                if( isupper( letterChar ) && !isCapital )
                    words[j].replace( k, 1, std::string( 1, tolower( letterChar ) ) );
                    
                if( letterChar == '-' )
                    isCapital = true;
                else
                    isCapital = false;
            }
        }
        
        // On concatène les résultats
        string newContent = words[0];
        
        for( int j = 1; j < words.size(); j++ )
            newContent += " " + words[j];
            
        mtxtEntity->setContents( strToAcStr( newContent ) );
        mtxtEntity->close();
    }
    
    acedSSFree( ssMTxt );
    
    
    //Selection sur les textes
    ads_name ssTxt;
    
    //Afficher message
    print( "Selectionner les noms de rue Texte" );
    
    //Selection des polylignes 3d
    long lenTxt = getSsText( ssTxt, _T( "A-VOIE-CHAUSSEE-T" ) );
    
    // On parcourt les textes
    for( int i = 0; i < lenTxt; i++ )
    {
        // On vérifie si le texte est en majuscule
        AcDbText* txtEntity = getTextFromSs( ssTxt, i, AcDb::kForWrite );
        AcString acTxt = txtEntity->textString();
        string txt = acStrToStr( acTxt );
        vector<string> words = splitString( txt, " " );
        
        for( int j = 0; j < words.size(); j++ )
        {
            bool isCapital = true;
            
            for( int k = 0; k < words[j].size(); k++ )
            {
                string letter = words[j].substr( k, 1 );
                char letterChar = *letter.c_str();
                
                if( isupper( letterChar ) && !isCapital )
                    words[j].replace( k, 1, std::string( 1, tolower( letterChar ) ) );
                    
                if( letterChar == '-' )
                    isCapital = true;
                else
                    isCapital = false;
            }
        }
        
        // On concatène les résultats
        string newContent = words[0];
        
        for( int j = 1; j < words.size(); j++ )
            newContent += " " + words[j];
            
        txtEntity->setTextString( strToAcStr( newContent ) );
        txtEntity->close();
    }
    
    acedSSFree( ssTxt );
}

void cmdAddPolyPonctText()
{
    //Tester que le fichier existe
    if( !isFileExisting( KDN_ELEMENT_LAYER_PATH ) )
    {
        acutPrintf( _T( "\nAucun fichier de configuration %s trouvé" ), KDN_ELEMENT_LAYER_PATH );
        return;
    }
    
    if( !isFileExisting( KDN_DESCRIPTION_PATH ) )
    {
        acutPrintf( _T( "\nAucun fichier de configuration %s trouvé" ), KDN_DESCRIPTION_PATH );
        return;
    }
    
    if( !isFileExisting( KDN_TEXT_LAYER_PATH ) )
    {
        acutPrintf( _T( "\nAucun fichier de configuration % trouvé" ), KDN_TEXT_LAYER_PATH );
        return;
    }
    
    /// Verification qu'on a les même nombre d'element
    // 1. Liste element
    std::vector<AcString> vecEltLayer = layerFileToLayerVector( KDN_ELEMENT_LAYER_PATH );
    int sizeElt = vecEltLayer.size();
    
    // 2. Liste descritpion
    std::vector<AcString> vecDescritpion = fileLineToUTF8StringVector( KDN_DESCRIPTION_PATH );
    int sizeDesc = vecDescritpion.size();
    
    // 3. Liste calque texte
    std::vector<AcString> vecTextLayer = layerFileToLayerVector( KDN_TEXT_LAYER_PATH );
    int sizeTextLayer = vecTextLayer.size();
    
    if( sizeElt != sizeDesc )
    {
        acutPrintf( _T( "\nLes nombres de lignes dans les 3 fichiers de configurations doivent être les même : %s\n\t %s\n\t %s" ), KDN_ELEMENT_LAYER_PATH, KDN_DESCRIPTION_PATH, KDN_TEXT_LAYER_PATH );
        print( "Commande terminée" );
        return;
    }
    
    if( sizeElt != sizeTextLayer )
    {
        acutPrintf( _T( "\nLes nombres de lignes dans les 3 fichiers de configurations doivent être les même : %s\n\t %s\n\t %s" ), KDN_ELEMENT_LAYER_PATH, KDN_DESCRIPTION_PATH, KDN_TEXT_LAYER_PATH );
        print( "Commande terminée" );
        return;
    }
    
    /// Recuperation des elements à traiter par calque
    AcString strLayerToUse = getLayer( KDN_ELEMENT_LAYER_PATH, true );
    
    ads_name entitySs;
    long  lengthEnt = getSelectionSet( entitySs,
            _T( "X" ),
            "",
            strLayerToUse );
            
    if( lengthEnt == 0 )
    {
        acedSSFree( entitySs );
        print( "\nAucun entite selectionne" );
        return;
    }
    
    long nbBloc = 0;
    long nbPoly3D = 0;
    long nbCircle = 0;
    
    // Ouverture model sapce
    AcDbBlockTableRecord* blocktable = openModelSpace();
    
    ProgressBar prog = ProgressBar( _T( "\nElements traites" ),  lengthEnt );
    
    /// Parcours et traitemetn des entites
    for( int counter = 0; counter <  lengthEnt; counter++ )
    {
        AcDbEntity* entity =  getEntityFromSs( entitySs, counter, AcDb::kForWrite );
        
        if( !entity )
        {
            acutPrintf( _T( "\nErreur lors de l'ouverture de l'entite" ) );
            prog.moveUp( counter );
            continue;
        }
        
        /// 1. Verification du type
        // 1- Si c'est un bloc
        if( entity->isKindOf( AcDbBlockReference::desc() ) )
        {
            // a- Recuperation du point d'insertion du bloc
            AcDbBlockReference* block = AcDbBlockReference::cast( entity );
            AcGePoint3d blocPos = block->position();
            
            // b- Calcul du point d'insertion du texte
            blocPos.y = blocPos.y - g_distYTextBloc;
            
            // c- Recuperation du nom de calque et de la description du texte
            // c.1 recuperation de l'indice du calque dans la liste
            int idxElt = findAcStringInVector( vecEltLayer, block->layer() );
            AcString txtLayer = vecTextLayer[idxElt];
            createLayer( txtLayer );
            
            AcString txtValue = vecDescritpion[idxElt];
            
            AcDbText* text = new AcDbText( blocPos, txtValue );
            text->setLayer( txtLayer );
            text->setHeight( TEXT_HEIGHT );
            addToModelSpaceAlreadyOpened( blocktable, text );
            text->close();
            text = NULL;
            
            entity->close();
            entity = NULL;
            prog.moveUp( counter );
            nbBloc++;
            continue;
        }
        
        if( entity->isKindOf( AcDbCircle::desc() ) )
        {
            // a- Recuperation du point d'insertion du bloc
            AcDbCircle* circle = AcDbCircle::cast( entity );
            AcGePoint3d circlePos = circle->center();
            
            // b- Calcul du point d'insertion du texte
            circlePos.y = circlePos.y - g_distYTextBloc;
            
            // c- Recuperation du nom de calque et de la description du texte
            // c.1 recuperation de l'indice du calque dans la liste
            int idxElt = findAcStringInVector( vecEltLayer, circle->layer() );
            AcString txtLayer = vecTextLayer[idxElt];
            createLayer( txtLayer );
            
            AcString txtValue = vecDescritpion[idxElt];
            
            AcDbText* text = new AcDbText( circlePos, txtValue );
            text->setLayer( txtLayer );
            text->setHeight( TEXT_HEIGHT );
            addToModelSpaceAlreadyOpened( blocktable, text );
            text->close();
            text = NULL;
            
            entity->close();
            entity = NULL;
            prog.moveUp( counter );
            nbCircle++;
            continue;
        }
        
        // 2- Si c'est un poly 3D
        if( entity->isKindOf( AcDb3dPolyline::desc() ) )
        {
            // a- Recuperation du point d'insertion du bloc
            AcDb3dPolyline* poly3d = AcDb3dPolyline::cast( entity );
            
            // b- Calcul du point d'insertion du texte
            AcGePoint3d midPoly = AcGePoint3d::kOrigin;
            
            // b.1 On recupere le segment le plus long de la polyligne
            AcGePoint3d pt1, pt2, centerPoly;
            double maxLenght = -1;
            getMaxSeg( poly3d, pt1, pt2, maxLenght, centerPoly );
            
            // c - Recuperation du nom de calque et de la description du texte
            // c.1 recuperation de l'indice du calque dans la liste
            int idxElt = findAcStringInVector( vecEltLayer, poly3d->layer() );
            AcString txtLayer = vecTextLayer[idxElt];
            createLayer( txtLayer );
            
            AcString txtValue = vecDescritpion[idxElt];
            
            AcDbText* text = new AcDbText( midPoly, txtValue );
            text->setLayer( txtLayer );
            text->setHeight( TEXT_HEIGHT );
            addToModelSpaceAlreadyOpened( blocktable, text );
            AcDbExtents textExtnd;
            AcGePoint3dArray textBox;
            getBoundingBox( text, textExtnd, textBox );
            
            // On calcul le centre de la box du texte
            AcGePoint3d ptTxt1 = textBox[0];
            AcGePoint3d ptTxt2 = textBox[1];
            AcGePoint3d ptTxt3 = textBox[2];
            AcGePoint3d ptTxt4 = textBox[3];
            
            double distTxt12 = getDistance2d( ptTxt1, ptTxt2 );
            double distTxt14 = getDistance2d( ptTxt1, ptTxt4 );
            
            double diff = abs( maxLenght - distTxt12 );
            AcGeVector2d vecSegPol;
            
            if( pt1.x > pt2.x )
                vecSegPol = getVector2d( pt2, pt1 );
            else
                vecSegPol = getVector2d( pt1, pt2 );
                
            // b.1 On recupere la bounding box de la poly3d
            AcDbExtents poly3DExend;
            AcGePoint3dArray poly3dBox;
            ErrorStatus es = getBoundingBox( poly3d, poly3DExend, poly3dBox );
            
            // On calcul le centre de la box de la polyligne
            AcGePoint3d ptPoly1 = poly3dBox[0];
            AcGePoint3d ptPoly2 = poly3dBox[1];
            AcGePoint3d ptPoly3 = poly3dBox[2];
            AcGePoint3d ptPoly4 = poly3dBox[3];
            
            double dist14 = getDistance2d( ptPoly1, ptPoly4 );
            double dist12 = getDistance2d( ptPoly1, ptPoly4 );
            
            if( ( maxLenght > distTxt12 ) && ( diff > 0.01 ) )
            {
                // Calcul orientation
                AcGeVector2d vecX = AcGeVector2d::kXAxis;
                
                double angle = vecSegPol.angle();
                AcGeVector3d vecSegPol3d = AcGeVector3d( vecSegPol.x, vecSegPol.y, 0 );
                AcGeVector3d vecPerp = vecSegPol3d.perpVector();
                vecPerp.normalize();
                vecPerp = vecPerp * ( -dist14 / 2 ) ;
                
                AcGePoint3d mid14poly = AcGePoint3d( ( ptPoly1.x + ptPoly4.x ) / 2, ( ptPoly1.y + ptPoly4.y ) / 2, ( ptPoly1.z + ptPoly4.z ) / 2 );
                
                AcGePoint3d textPos = AcGePoint3d( ( pt1.x + pt2.x ) / 2, ( pt1.y + pt2.y ) / 2, 0 );
                text->setPosition( textPos );
                
                centerPoly.y = mid14poly.y - ( distTxt14 / 2 );
                AcGeVector3d vecTrans = vecSegPol3d.normalize();
                vecTrans = vecTrans * ( -distTxt12 / 2 );
                AcGeMatrix3d trans  = AcGeMatrix3d::translation( vecTrans );
                text->setPosition( centerPoly );
                text->setRotation( angle );
                text->transformBy( trans );
            }
            
            else
            {
                AcGePoint3d textPos = AcGePoint3d( ( pt1.x + pt2.x ) / 2, ( pt1.y + pt2.y ) / 2, 0 );
                text->setPosition( textPos );
            }
            
            text->close();
            text = NULL;
            
            entity->close();
            entity = NULL;
            prog.moveUp( counter );
            nbPoly3D++;
            continue;
        }
        
        // Si autre on ne fait rien
        entity->close();
        entity = NULL;
        prog.moveUp( counter );
        continue;
    }
    
    closeModelSpace( blocktable );
    
    acedSSFree( entitySs );
    print( "Commande terminee" );
    acutPrintf( _T( "\nNombre de Polylignes 3D traitees : %d" ), nbPoly3D );
    acutPrintf( _T( "\nNombre de Blocs traites : %d" ), nbBloc );
    acutPrintf( _T( "\nNombre de Circle traites : %d" ), nbCircle );
}

void cmdOrientTxtWithPoly()
{
    // Sélectionner les textes
    ads_name ssTxt;
    
    //Afficher message
    print( "Selectionner un MTexte a aligner" );
    
    //Selection des polylignes 3d
    long lenTxt = getSsOneMText( ssTxt, _T( "A-VOIE-CHAUSSEE_TROTTOIR-T,A-VOIE-CHAUSSEE-T,A-VOIE-CHAUSSEE_ACCOTEMENT-T,A-VOIE-CHAUSSEE_CHEMIN-T,A-VOIE-CHEMINEMENT_PIETON-T,E-ASEP-CANIVEAU-T,T-VEGE-ARBUST-T,T-VEGE-HERBE-T,T-VEGE-MASSIF-T,T-VEGE-FRICHE-T" ) );
    
    //Verifier
    if( lenTxt == 0 )
    {
        print( "Aucun texte a traiter, Sortie." );
        return;
    }
    
    // Sélectionner la polyligne
    ads_name ssPoly3d;
    
    //Afficher message
    print( "Selectionner la polyligne" );
    
    //Selection des polylignes 3d
    long lenPoly3d = getSsOnePoly3D( ssPoly3d, "" );
    
    //Verifier
    if( lenPoly3d == 0 )
    {
        print( "Aucune polyligne selectionnee, Sortie." );
        return;
    }
    
    
    AcDbMText* mtxtEntity = getMTextFromSs( ssTxt, 0, AcDb::kForWrite );
    AcDb3dPolyline* closestPoly = getPoly3DFromSs( ssPoly3d, 0, AcDb::kForRead );
    
    AcGePoint3d txtPt = mtxtEntity->location();
    
    // On cherche l'orientation de la polyligne 3d
    AcGePoint3d closestVertex, sdClosestVertex;
    closestPoly->getClosestPointTo( txtPt, closestVertex, true );
    
    // Parcourir les points de la polylignes pour trouver le vecteur directeur le plus proche
    AcDbObjectIterator* iterPoly3D = closestPoly->vertexIterator();
    
    // Initialiser l'itérateur
    AcDb3dPolylineVertex* vertex = NULL;
    iterPoly3D->start();
    bool hasToKeepNext = false;
    
    //Initialisation
    vector<AcGePoint3d> closestPts;
    closestPts.push_back( AcGePoint3d( 0, 0, 0 ) );
    closestPts.push_back( AcGePoint3d( 0, 0, 0 ) );
    
    vector<double> dists;
    dists.push_back( 1000 );
    dists.push_back( 1000 );
    
    for( iterPoly3D->start(); !iterPoly3D->done(); iterPoly3D->step() )
    {
        // Ouvrir le premier sommet de la polyligne
        if( Acad::eOk != closestPoly->openVertex( vertex, iterPoly3D->objectId(), AcDb::kForRead ) )
        {
            closestPoly->close();
            mtxtEntity->close();
            
            print( _T( "Erreur lors de l'ouverture de la polyligne; operation arrétée" ) );
            return;
        }
        
        AcGePoint3d vtxCoords = vertex->position();
        double distToPt = vtxCoords.distanceTo( txtPt );
        
        if( distToPt < dists[0] )
        {
            closestPts[1] = closestPts[0];
            closestPts[0] = vtxCoords;
            dists[1] = dists[0];
            dists[0] = distToPt;
        }
        
        else if( distToPt < dists[1] )
        {
            closestPts[1] = vtxCoords;
            dists[1] = distToPt;
        }
    }
    
    closestVertex = closestPts[0];
    sdClosestVertex = closestPts[1];
    
    AcGeVector3d direction = AcGeVector3d( closestVertex.x - sdClosestVertex.x, closestVertex.y - sdClosestVertex.y, closestVertex.z - sdClosestVertex.z );
    AcGeVector3d txtDirection = mtxtEntity->direction();
    
    if( txtDirection.dotProduct( direction ) < 0 )
        direction *= -1;
        
    mtxtEntity->setDirection( direction );
    
    mtxtEntity->close();
    closestPoly->close();
}


void cmdControlBlocTxt()
{
    //Recuperer les calques
    AcString layer = getLayer( _T( "C:\\Futurmap\\Outils\\GStarCAD2020\\BDX\\CONTROLBLOCTXT.txt" ), true );
    
    vector<string> vecLayer = splitString( acStrToStr( layer ), "," );
    
    //Recuperer la taille du vecteuru
    int taille = vecLayer.size();
    
    //Verifier
    if( taille == 0 )
    {
        //Afficher message
        print( "Fichier de calque vide, Sortie" );
        
        return;
    }
    
    //Vecteur de AcString
    vector<AcString> layerVec;
    
    //Recuperer le vecteur de string en AcString
    for( int i = 0; i < taille; i++ )
        layerVec.push_back( strToAcStr( vecLayer[i] ) );
        
    //Nombre de patate ajouté
    int patate = 0;
    
    //Couleur
    AcCmColor red, magenta;
    red.setRGB( 255, 0, 0 );
    magenta.setRGB( 255, 0, 255 );
    
    //Creer les calque
    createLayer( _T( "Manque_Texte" ), red );
    createLayer( _T( "Manque_Bloc" ), magenta );
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), taille );
    
    //Boucle sur les calques
    for( int i = 0; i < taille; i++ )
    {
        //Recuperer le i-eme calque du bloc
        AcString bLay = layerVec[i];
        
        //Recuperer pour celui du texte
        AcString tLay = layerVec[i];
        tLay = tLay.substr( 0, tLay.length() - 1 );
        tLay = tLay.append( _T( "T" ) );
        
        //Selection sur les bloc du calque
        ads_name ssBlock;
        
        //Selection sur les Mtextes du calque
        ads_name ssMText;
        
        //Selection sur les textes du calque
        ads_name ssText;
        
        //Recuperer tous les blocs du calque
        long lenBloc = getSsAllBlock( ssBlock, bLay );
        
        //Recupurer tous les Mtextes du calque
        long lenMTxt = getSsAllMText( ssMText, tLay );
        
        //Recuperer tous les textes du calque
        long lenTxt = getSsAllText( ssText, tLay );
        
        //Verifier si le nombre de texte et de bloc sont les même dans le calque
        if( lenBloc == ( lenMTxt + lenTxt ) )
        {
            //Liberer les selections
            acedSSFree( ssBlock );
            acedSSFree( ssMText );
            acedSSFree( ssText );
        }
        
        //Si on a des nombres differents
        else
        {
            //Verifier le calque
            patate += verifyLayer( ssBlock,
                    ssMText,
                    ssText,
                    lenBloc,
                    lenMTxt,
                    lenTxt );
                    
            //Liberer les selections
            acedSSFree( ssBlock );
            acedSSFree( ssMText );
            acedSSFree( ssText );
        }
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Afficher message
    print( "Nombre de patates ajoutés : %d", patate );
}


void cmdSetBlockScale()
{
    AcString blockToSelect = _T( "AFF03,AFF04,AFF09,AFF10,ASS01,ASS02,CH03,E_P01,E_P02,E_U01,E_U02,EAU09,ECL02,ECL10,EDF08,GAZ03,PIP03,PTT04,PTT05,PTT06,SIL06,SIL07,TCL02" );
    
    /// 1- Demande de selectionner les blocs
    // Selectionner les blocs à modifier
    ads_name ssblock, ssblockTopo;
    long lnBlock = getSsBlock( ssblock, "", _T( "AFF03,AFF04,AFF09,AFF10,ASS01,ASS02,CH03,E_P01,E_P02,E_U01,E_U02,EAU09,ECL02,ECL10,EDF08,GAZ03,PIP03,PTT04,PTT05,PTT06,SIL06,SIL07,TCL02" ) );
    
    
    // Verifier le nombre des selections
    if( acedSSLength( ssblock, &lnBlock ) != RTNORM
        || lnBlock == 0 )
    {
        // Afficher un message
        print( "Sélection vide!" );
        
        // Libérer la sélection
        acedSSFree( ssblock );
        
        // Sortie
        return;
    }
    
    
    /// 2- Parcourir la liste des blocs
    
    // Boucle sur la liste des blocs
    for( int iBlock = 0; iBlock < lnBlock; iBlock++ )
    {
        // Recupération de ième bloc
        AcDbBlockReference* block = getBlockFromSs( ssblock, iBlock, AcDb::kForWrite );
        
        if( !block )
            continue;
            
        // Recuperer l'echelle du bloc
        AcGeScale3d scale =  block->scaleFactors();
        double scaleX = scale.sx;
        double scaleY = scale.sy;
        double scaleZ = scale.sz;
        
        scaleX = roundUp( scaleX, 2, 5 );
        scaleY = roundUp( scaleY, 2, 5 );
        scaleZ = roundUp( scaleZ, 2, 5 );
        scale = AcGeScale3d( scaleX, scaleY, scaleZ );
        block->setScaleFactors( scale );
        
        // Exploser les blocs
        //Recuperer les points topo
        
        // Fermer
        block->close();
        
    }
    
    // 3- Vérifier l'échelle du bloc
    // 4- Modifier le point topo
    
    // Libération des selections
    
    acedSSFree( ssblock );
}


void cmdSetPTOPOPosition()
{
    ads_name ssblockTopo;
    long lnPtTopo = getSsAllBlock( ssblockTopo, "", _T( "PTOPO" ) );
    
    if( acedSSLength( ssblockTopo, &lnPtTopo ) != RTNORM
        || lnPtTopo == 0 )
    {
        acedSSFree( ssblockTopo );
        return;
    }
    
    ads_name ssblock;
    long lnBlock = getSsAllBlock( ssblock, "", _T( "AFF03,AFF04,AFF09,AFF10,ASS01,ASS02,CH03,E_P01,E_P02,E_U01,E_U02,EAU09,ECL02,ECL10,EDF08,GAZ03,PIP03,PTT04,PTT05,PTT06,SIL06,SIL07,TCL02" ) );
    
    if( acedSSLength( ssblock, &lnBlock ) != RTNORM
        || lnPtTopo == 0 )
    {
        acedSSFree( ssblockTopo );
        acedSSFree( ssblock );
        return;
    }
    
    int nbDepl = 0;
    
    // Boucler sur les points topo
    for( int iPtopo = 0; iPtopo < lnPtTopo; iPtopo++ )
    {
        AcDbBlockReference* blockTopo = getBlockFromSs( ssblockTopo, iPtopo, AcDb::kForWrite );
        
        if( !blockTopo )
            continue ;
            
        // Recuperer les coordonnées du bloc
        AcGePoint3d positionTopo = blockTopo->position();
        AcGePoint2d ptPttopo = getPoint2d( positionTopo );
        
        // Boucler sur les autres blocs
        
        
        double distance = 10000;
        AcGePoint3d position ;
        bool isBlockPos = false;
        
        for( int iBloc = 0; iBloc < lnBlock; iBloc++ )
        {
            AcDbBlockReference* block = getBlockFromSs( ssblock, iBloc, AcDb::kForWrite );
            
            if( !block )
                continue;
                
            AcGePoint3d blockPos = block->position();
            
            if( isEqual2d( positionTopo, blockPos ) )
            {
                isBlockPos = true;
                block->close();
                break;
            }
            
            
            AcDbVoidPtrArray entityArray;
            
            Acad::ErrorStatus es =  block->explode( entityArray );
            int size = entityArray.size();
            
            if( es != eOk || size == 0 )
            {
                block->close();
                continue;
            }
            
            for( int iEnt = 0; iEnt < size; iEnt++ )
            {
                AcDbEntity* ent = AcDbEntity::cast( ( AcRxObject* )entityArray[iEnt] );
                
                if( !ent )
                    continue;
                    
                // Verifier si l'entité est une polyligne
                if( ent->isKindOf( AcDbPolyline::desc() ) )
                {
                
                    AcDbPolyline* polyline = AcDbPolyline::cast( ent );
                    
                    if( !polyline )
                    {
                        ent->close();
                        continue;
                    }
                    
                    // Boucle sur les sommets de la polyline
                    int numVerts = polyline->numVerts();
                    
                    if( numVerts == 0 )
                    {
                        polyline->close();
                        continue;
                    }
                    
                    
                    
                    for( int iVtx = 0; iVtx < numVerts; iVtx++ )
                    {
                        AcGePoint2d point ;
                        polyline->getPointAt( iVtx, point );
                        double dist = getDistance2d( point, ptPttopo );
                        
                        if( distance > dist )
                        {
                            distance = dist;
                            position = AcGePoint3d( point.x, point.y, positionTopo.z );
                        }
                    }
                    
                    
                    polyline->close();
                }
                
                else if( ent->isKindOf( AcDbLine::desc() ) )
                {
                    AcDbLine* line = AcDbLine::cast( ent );
                    
                    if( !line )
                    {
                        ent->close();
                        continue;
                    }
                    
                    // Boucle sur les sommets de la polyline
                    AcGePoint3d ptstart, ptend;
                    line->getStartPoint( ptstart );
                    
                    double dist = getDistance2d( getPoint2d( ptstart ), ptPttopo );
                    
                    if( distance > dist )
                    {
                        distance = dist;
                        position = AcGePoint3d( ptstart.x, ptstart.y, positionTopo.z );
                    }
                    
                    line->getEndPoint( ptend );
                    dist = getDistance2d( getPoint2d( ptend ), ptPttopo );
                    
                    if( distance > dist )
                    {
                        distance = dist;
                        position = AcGePoint3d( ptend.x, ptend.y, positionTopo.z );
                    }
                    
                    
                    line->close();
                }
            }
            
            block->close();
        }
        
        if( isBlockPos )
        {
            blockTopo->close();
            continue;
        }
        
        // SEtter la position du bloc
        if( position == AcGePoint3d::kOrigin || distance > 0.05 )
        {
        
            blockTopo->close();
            continue;
        }
        
        blockTopo->setPosition( position );
        nbDepl++;
        
        blockTopo->close();
    }
    
    AcString message = numberToAcString( nbDepl ) + _T( " PTOPO deplacés" ) ;
    print( message );
    acedSSFree( ssblockTopo );
    acedSSFree( ssblock );
}


void cmdIsolerVertex()
{
    //Selection sur les polylignes 3d
    ads_name ssPoly3d;
    
    //Afficher message
    print( "Selectionner les polylignes 3D" );
    
    //Selection des polyligne
    long lenPoly3d = getSsPoly3D( ssPoly3d );
    
    //Verifier
    if( lenPoly3d == 0 )
    {
        //Afficher message
        print( "Aucune Polyligne dans la selection" );
        
        //Sortir
        return;
    }
    
    //Tolerance
    ads_real tolDef = 0.01;
    double tol = 0;
    
    //Demander la tolerance
    if( RTCAN == askForDouble( _T( "Entrer la tolerance" ), tolDef, tol ) )
    {
        //Afficher message
        print( "Commande annulée" );
        
        //Liberer la selection
        acedSSFree( ssPoly3d );
        
        return;
    }
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Préparation" ), lenPoly3d );
    
    //Tableau de point 3d
    AcGePoint3dArray ptArr;
    vector<double> xArr;
    
    //Polyligne 3d
    AcDb3dPolyline* poly3d = NULL;
    
    //Boucle sur les polyligne
    for( int i = 0; i < lenPoly3d; i++ )
    {
        //Tableau de point temporaire
        AcGePoint3dArray ptTemp;
        
        vector<double> xPos;
        
        //Recuperer le i-eme polyligne
        poly3d = getPoly3DFromSs( ssPoly3d, i );
        
        //Verifier
        if( poly3d )
        {
            //Recuperer les sommets de la polyligne
            getVertexesPoly( poly3d, ptTemp, xPos );
            
            //Ajouter les points dans le vecteur
            ptArr.append( ptTemp );
            xArr.insert( xArr.end(), xPos.begin(), xPos.end() );
            
            //Fermer la polyligne
            poly3d->close();
        }
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Creer le calque de patate
    AcCmColor red;
    red.setRGB( 255, 0, 0 );
    createLayer( _T( "PATATE_CIRCLE" ), red );
    
    //Recuperer la taille du tableau
    long lenVert = ptArr.size();
    
    //Trier les points
    sortList( ptArr, xArr );
    
    //Supprimer les doublons
    eraseDoublons( ptArr, tol );
    
    //Recuperer la nouvelle taille
    lenVert = ptArr.size();
    
    //Barre de progression
    prog = ProgressBar( _T( "Insertion des patates" ), lenVert );
    
    //Boucle pour ajouter les patates
    for( int i = 0; i < lenVert; i++ )
    {
        //Tracer un cercle
        drawCircle( ptArr[i], 1, _T( "PATATE_CIRCLE" ) );
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Liberer la selection
    acedSSFree( ssPoly3d );
}


void cmdIntersectVertex()
{
    // Demander au dessinateur de Sélectionner les polylignes 3D dont les intersections sont à chercher
    acutPrintf( _T( "\n Sélectionner les polylignes 3D" ) );
    
    ads_name ss;
    
    long length = getSsPoly3D( ss );
    
    if( !length )
    {
        // Liberer la selection
        acutPrintf( _T( "\n Aucune polyligne sélectionnée" ) );
        return;
    }
    
    //Demander la tolérance
    ads_real tolDef = 0.1;
    double tol = 0.1;
    
    if( RTCAN == askForDouble( _T( "Enter la valeur de la tolerance" ), tolDef, tol ) )
    {
        acedSSFree( ss );
        
        print( "Commande annulée" );
        
        return;
    }
    
    
    long int start = GetTickCount();
    
    /// 1. On vérifie avant tout si on a besoin de translation
    AcGePoint3d orgOfDistance = AcGePoint3d::kOrigin;
    AcGePoint3d nearestPoint;
    
    // Calcule la plus courte distance entre la 1ère polyligne et l'origine
    AcDb3dPolyline* firstPoly3D = getPoly3DFromSs( ss, 0, AcDb::kForWrite );
    firstPoly3D->getClosestPointTo( orgOfDistance, nearestPoint );
    AcGeVector3d vecOfTranslation = AcGeVector3d::kIdentity;
    
    std::vector<AcDbObjectId> poly3DIdVector;
    long intersectionCount = 0;
    
    // Fermer la 1ère polyligne
    firstPoly3D->close();
    
    // Créer la matrice de translation
    AcGeMatrix3d matrixOfTranslation = AcGeMatrix3d::translation( vecOfTranslation );
    
    // On sauvegarde tous les id des polylines sélectionnées
    for( long nPoly3dIndex = 0; nPoly3dIndex < length; nPoly3dIndex++ )
    {
        // Récuperer la poly3D
        if( AcDb3dPolyline* poly3D = writePoly3DFromSs( ss, nPoly3dIndex ) )
        {
            // On garde l'id
            poly3DIdVector.push_back( poly3D->objectId() );
            //On ferme la poly
            poly3D->close();
        }
    }
    
    //Creer le calque de patate
    AcCmColor red;
    red.setRGB( 255, 0, 0 );
    createLayer( _T( "PATATE_CIRCLE" ), red );
    
    long size = poly3DIdVector.size();
    
    // Initialistion de la barre de progression avec le nombre de polyline de la section plus 5 pour faire avancer la bare de progression lors des calculs sur de grand fichiers
    ProgressBar* prog = new ProgressBar( _T( "Polyline(s) 3D traitée(s)" ), size );
    
    // On parcours l'ensemble des poly3D
    for( long nPoly3dIndexPrimary = 0; nPoly3dIndexPrimary < size; nPoly3dIndexPrimary++ )
    {
        AcDbObjectId currentId = poly3DIdVector.at( nPoly3dIndexPrimary );
        
        for( long nPoly3dIndex = nPoly3dIndexPrimary + 1; nPoly3dIndex < length; nPoly3dIndex++ )
        {
            // On cherche les intersections
            AcGePoint3dArray intersectPoint3DArray;
            
            AcDbObjectId nextId = poly3DIdVector.at( nPoly3dIndex );
            
            // On cherche les intersections entre les 2 poly3D
            intersectTwoPoly3D( currentId, nextId, intersectPoint3DArray, tol );
            
            // On test si on a trouvé ou non au moin une intersection
            // Si aucune intersection on passe à la suivante
            if( intersectPoint3DArray.length() == 0 )
                continue;
                
            // On comptabilise les intersections trouvées
            intersectionCount += intersectPoint3DArray.length();
            
            // On insert les intersections
            insertNewVertToPoly3D( currentId, intersectPoint3DArray, _T( "PATATE_CIRCLE" ), tol );
            
            // On projette les intersections sur l'autre poly3D
            AcGePoint3dArray projectedIntersectPoint3DArray;
            
            projectToPoly3D( nextId, intersectPoint3DArray, projectedIntersectPoint3DArray, tol );
            
            // On insert les intersections sur l'autre poly3D
            insertNewVertToPoly3D( nextId, projectedIntersectPoint3DArray, _T( "PATATE_CIRCLE" ), tol );
            
            // On efface une fois utilisé
            intersectPoint3DArray.removeAll();
            projectedIntersectPoint3DArray.removeAll();
            
        }
        
        // incrementer la barre de progression
        prog->moveUp( nPoly3dIndexPrimary );
        
    }
    
    delete prog;
    
    // Liberer la selection
    acedSSFree( ss );
    
    //afficher les resultats
    acutPrintf( _T( "Nombre d'intersections trouvées : %d \n" ), intersectionCount );
    
    
    int end = GetTickCount();
    
    //afficher le temsp d'exécustion
    acutPrintf( _T( "Temps de calcul  : %d secondes\n" ), ( end - start ) / 1000 );
}


void cmdDrawContourBlock()
{
    //Selection sur les blocks
    ads_name ssBlock;
    
    //Afficher message
    print( "Selectionner les blocs" );
    
    //Selection des block
    long lenBlock = getSsBlock( ssBlock );
    
    //Verifier
    if( lenBlock == 0 )
    {
        //Afficher message
        print( "Commande annulée" );
        
        return;
    }
    
    //Chemin vers le fichier de contour
    AcString path = _T( "C:\\Futurmap\\Outils\\GStarCAD2020\\BDX\\POLYBLOCK.csv" );
    
    //Reference de block
    AcDbBlockReference* block = NULL;
    
    //Boucle sur les blocks
    for( int i = 0; i < lenBlock; i++ )
    {
        //Recuperer le ieme bloc
        block = getBlockFromSs( ssBlock, i );
        
        //Verifier
        if( !block )
            continue;
            
        //Recuperer le nom du bloc
        AcString blockName = "";
        getBlockName( blockName, block );
        
        //Tableau de point de la polyligne
        AcGePoint3dArray ptArr;
        
        //Recuperer les contours du bloc
        getPolyBlock( path, blockName, ptArr );
        
        //Dessiner le contour du bloc
        drawPolyBlock( block, ptArr );
        
        //Fermer le bloc
        block->close();
    }
    
    //Liberer la selectin
    acedSSFree( ssBlock );
}


void cmdMergePhotoCsv()
{
    //Demander de selectionner le fichier de sortie
    AcString filePath = askForFilePath( true, "csv", _T( "Selectionner le fichier de sortie" ) );
    
    //Verifier
    if( filePath == _T( "" ) )
    {
        //Afficher message
        print( "Commande annulée" );
        return;
    }
    
    //Recuperer le chemin vers le dossier
    AcString rootFolder = filePath;
    int ind = filePath.findRev( "\\" );
    rootFolder = rootFolder.substr( 0, ind );
    
    //Recuperer tous les nomms des dossiers et fichier dans le dossier root
    vector<string> fileNames = getSortedFileNamesInFolder( rootFolder );
    
    fileNames = cleanVectorOfName( fileNames );
    
    int taille = fileNames.size();
    
    if( taille == 0 )
    {
        print( "Aucun dossier à recuperer" );
        return;
    }
    
    //Vecteur qui va contenir tous les chemin vers les csv
    vector<AcString> csvPaths;
    
    //Boucle sur les dossiers
    for( int i = 0; i < taille; i++ )
    {
        //Recuperer le nom du fichier
        AcString file = strToAcStr( fileNames[i] );
        
        //Si c'est un fichier on passe
        if( file.findRev( "." ) != -1 )
            continue;
            
        //Recuperer le chemin du ieme fichier
        AcString path = rootFolder + _T( "\\" ) + file;
        
        //Recuperer les noms de tous les fichiers dans le dossier
        vector<string> csvFile = getFileNamesInFolder( path );
        
        csvFile = cleanVectorOfName( csvFile );
        
        int temp = csvFile.size();
        
        for( int j = 0; j < temp; j++ )
        {
            AcString rev = strToAcStr( csvFile[j] );
            
            //Tester si c'est un fichier csv
            if( rev.findRev( "External Orientation.csv" ) != -1 )
                csvPaths.push_back( path + _T( "\\" ) + rev );
        }
    }
    
    //Printer les chemins vers le csv
    taille = csvPaths.size();
    
    //Ouvrir le csv ou on va mettre les données
    ofstream outfile( filePath.c_str() );
    
    //ProgressBar
    ProgressBar prog = ProgressBar( _T( "Progression" ), taille );
    
    //Boucle sur les csv
    for( int i = 0; i < taille; i++ )
    {
        //Recuperer le chemin
        AcString p = csvPaths[i];
        
        //Ouvrir le ieme csv
        ifstream infile( acStrToStr( p ) );
        
        //Recuperer le prefixe
        int ind = p.findRev( "\\" );
        AcString pref = p.substr( 0, ind );
        pref += _T( "\\" );
        
        string line = "";
        
        //Recuperer la ligne du fichier
        while( getline( infile, line ) )
        {
            //Ajouter le prefixe
            line.insert( 0, acStrToStr( pref ) );
            
            //Ajouter la ligne dans le fichier de sortie
            outfile << line;
            
            //Ajouter une retour a la ligne
            if( !infile.eof() )
                outfile << "\n";
        }
        
        //Fermer le fichier
        infile.close();
        
        prog.moveUp( i );
    }
    
    //Fermer le fichier
    outfile.close();
    
    return;
}

void cmdExportCopyPcd()
{
    //Demander si on veut utiliser lineaire ou contour fermée
    bool useLinear = false;
    vector<AcString> vecChoice;
    vecChoice.push_back( "Contour_fermé" );
    vecChoice.push_back( "Lineaire" );
    
    AcString choice = _T( "" );
    choice = askToChoose( vecChoice, _T( "Selectionner le mode" ) );
    
    //Verifier
    if( choice == _T( "" ) )
    {
        print( "Commande annulée" );
        return;
    }
    
    //Si lineaire
    if( choice == _T( "L" ) || choice == _T( "l" ) )
    {
        //Print
        print( "Mode Linéaire" );
        
        //Appeler la commande divpcj
        acedCommand( RTSTR,
            _T( "DIVPCJ" ),
            RTNONE );
            
        return;
    }
    
    //Demander de selectionner le fichier pcj
    AcString pcjPath = askForFilePath( true, _T( "pcj" ), _T( "Selectionner le pcj" ) );
    
    //Verifier
    if( pcjPath == _T( "" ) )
    {
        //Afficher messsage
        print( "Commande annulée" );
        return;
    }
    
    //Recuperer le chemin vers le dossier du pcj
    int ind = pcjPath.findOneOfRev( _T( "\\" ) );
    AcString pcjFolder = pcjPath.substr( 0, ind + 1 );
    
    //Creer un dossier export dans le dossier du pcj
    AcString command = pcjFolder + _T( "\\Export" );
    mkdir( acStrToStr( command ).c_str() );
    
    //Creer un dossier reste dans le dossier export
    command = pcjFolder + _T( "\\Export\\Reste" );
    mkdir( acStrToStr( command ).c_str() );
    
    //Creer un fichier pcj dans export
    command = pcjFolder + _T( "\\Export\\Reste\\reste.pcj" );
    ofstream resteFile( acStrToStr( command ) );
    resteFile.close();
    
    //Couleur rouge
    AcCmColor red;
    red.setRGB( 255, 0, 0 );
    
    //Vecteur qui va contenir les lignes du pcd
    vector<string> pcjLineVec;
    
    //Vecteur qui va contenir la liste des pcd qui ont été exporté
    vector<string> exportedPcdVec;
    
    //Ouvrir le pcj
    ifstream pcjStream( acStrToStr( pcjPath ) );
    string pcjLine;
    
    //Boucle sur le fichier pcj
    while( getline( pcjStream, pcjLine ) )
    {
        //Ajouter la ligne dans le vecteur
        pcjLineVec.push_back( pcjLine );
        
        //Essayer de tracer la polyligne si on a les infos
        if( pcjLine.find( "pcd", 0 ) != -1 )
        {
            //Spliter la ligne
            vector<string> splittedLine = splitString( pcjLine, "\t" );
            
            //Recuperer bounding box du pcd
            AcDbExtents bb = AcDbExtents( AcGePoint3d( stod( splittedLine[1] ), stod( splittedLine[3] ), 0 ), AcGePoint3d( stod( splittedLine[2] ), stod( splittedLine[4] ), 0 ) );
            
            //Tracer le bounding box
            drawBoundingBox3d( bb, red );
        }
    }
    
    //Fermer le pcj
    pcjStream.close();
    
    //Demander de tracer les polylignes
    bool drawDalle = true;
    vector<DallePoly> vecDalle;
    
    if( !getDalleInfos( vecDalle ) )
    {
        //Afficher message
        print( "Aucune polyligne trouvé" );
        return;
    }
    
    //Recuperer la taille du vecteur de dalle
    int dalleSize = vecDalle.size();
    
    //Verifier
    if( dalleSize == 0 )
    {
        //Afficher message
        print( "Aucune polyligne dans le dessin" );
        return;
    }
    
    //Recuperer la taille du vecteur de pcj
    int pcjSize = pcjLineVec.size();
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "[Etape 1/2] Copie des fichiers" ), pcjSize );
    
    //Boucle sur les dalles
    for( int i = 0; i < dalleSize; i++ )
    {
        //Recuperer le i-eme dalle
        DallePoly refDalle = vecDalle[i];
        
        //Creer un dossier qui va contenir les pcd dans le dalle et creer le pcj
        command = pcjFolder + _T( "\\Export\\" ) + refDalle.dalleName;
        mkdir( acStrToStr( command ).c_str() );
        
        //Creer le fichier pcj correspondant
        ofstream dallePcjFile( acStrToStr( pcjFolder + _T( "\\Export\\" ) + refDalle.dalleName + _T( "\\" ) + refDalle.dalleName + _T( ".pcj" ) ), std::ios::app );
        
        //Boucle sur les lignes du fichier pcj
        for( int j = 0; j < pcjSize; j++ )
        {
            //Verifier si c'est une entete
            if( pcjLineVec[j].find( ".pcd", 0 ) == -1 )
            {
                dallePcjFile << pcjLineVec[j] << "\n";
                continue;
            }
            
            //Spliter la ligne
            vector<string> splittedLine = splitString( pcjLineVec[j], "\t" );
            
            //Recuperer le chemin vers le pcd
            AcString pcdPath = pcjFolder + strToAcStr( splittedLine[0] );
            
            //Creer un chemin temporaire qui pointe vers le rar
            AcString rarPath = pcdPath;
            rarPath.replace( _T( ".pcd" ), _T( ".rar" ) );
            
            //Recuperer le centre du pcd en (x,y)
            AcGePoint2d ptMin = AcGePoint2d( stod( splittedLine[1] ), stod( splittedLine[3] ) );
            AcGePoint2d ptMax = AcGePoint2d( stod( splittedLine[2] ), stod( splittedLine[4] ) );
            AcGePoint2d ptCenter = midPoint2d( ptMin, ptMax );
            
            //Recuperer la polyligne de la dalle
            AcDbEntity* ent = new AcDbEntity();
            
            if( Acad::eOk != acdbOpenAcDbEntity( ent, refDalle.idPoly, AcDb::kForRead ) )
                continue;
                
            AcDbPolyline* poly = AcDbPolyline::cast( ent );
            
            if( !poly )
                continue;
                
            //Creer une polyligne
            AcDbPolyline* polybb = new AcDbPolyline();
            polybb->addVertexAt( polybb->numVerts(), ptMin );
            polybb->addVertexAt( polybb->numVerts(), AcGePoint2d( ptMin.x, ptMax.y ) );
            polybb->addVertexAt( polybb->numVerts(), ptMax );
            polybb->addVertexAt( polybb->numVerts(), AcGePoint2d( ptMax.x, ptMin.y ) );
            polybb->setClosed( true );
            
            AcGePoint3dArray intersectArr;
            computeIntersection( poly, polybb, intersectArr );
            
            //Si le pcd est dans la polyligne
            if( isPointInPoly( poly, ptCenter ) || intersectArr.size() > 0 )
            {
                //Ajouter la ligne dans le fichier pcj
                dallePcjFile << pcjLineVec[j] << "\n";
                
                //Verifier si le fichier pcd existe
                if( isFileExisting( pcdPath ) )
                {
                    //Copier le pcd vers le dossier
                    AcString source = pcdPath;
                    AcString destination = pcjFolder + _T( "\\Export\\" ) + refDalle.dalleName + _T( "\\" ) + strToAcStr( splittedLine[0] );
                    
                    copyFileTo( source, destination );
                    
                    //Checker le vecteur
                    exportedPcdVec.push_back( pcjLineVec[j] );
                }
                
                else if( isFileExisting( rarPath ) )
                {
                    //Copier le rar
                    AcString source = rarPath;
                    AcString rarFile = strToAcStr( splittedLine[0] );
                    rarFile.replace( _T( ".pcd" ), _T( ".rar" ) );
                    AcString destination = pcjFolder + _T( "\\Export\\" ) + refDalle.dalleName + _T( "\\" ) + rarFile;
                    
                    copyFileTo( source, destination );
                    
                    //Checker le vecteur
                    exportedPcdVec.push_back( pcjLineVec[j] );
                }
                
                else
                    continue;
                    
                poly->close();
                
                drawRedCross( ptCenter );
            }
            
            else
                poly->close();
        }
        
        //Fermer le fichier
        dallePcjFile.close();
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Recuperer la taille des dalles restants
    int resteSize = exportedPcdVec.size();
    
    //Boucle sur les pcd restant
    for( int i = 0; i < resteSize; i++ )
    {
        //Recuperer le ieme pcd exporté
        string exp = exportedPcdVec[i];
        
        //Passer les entetes
        if( exp.find( ".pcd" ) == -1 )
            continue;
            
        //Boucle sur le pcj
        for( int j = 0; j < pcjLineVec.size(); j++ )
        {
            //Recuperer le pcd
            string orig = pcjLineVec[j];
            
            //Passer les entetes
            if( orig.find( ".pcd" ) == -1 )
                continue;
                
            //Si on a le meme
            if( exp == orig )
            {
                //On supprime
                pcjLineVec.erase( pcjLineVec.begin() + j );
                break;
            }
        }
    }
    
    //Ouvrir le fichier pcj
    ofstream restePcjFile( acStrToStr( pcjFolder + _T( "Export\\Reste\\reste.pcj" ) ), std::ios::app );
    
    long lineVecSize = pcjLineVec.size();
    
    //Barre de progression
    ProgressBar prog2 = ProgressBar( _T( "[Etape 2/2] Copie des pcd restants" ), lineVecSize );
    
    //Copier les fichiers
    for( int i = 0; i < pcjLineVec.size(); i++ )
    {
        //Recuper le ieme pcd restant
        string rest = pcjLineVec[i];
        
        //Verifier si c'est une entete
        if( rest.find( ".pcd" ) == -1 )
        {
            restePcjFile << pcjLineVec[i] << "\n";
            continue;
        }
        
        vector<string> splitRes = splitString( rest, "\t" );
        
        //Recuperer le centre du pcd en (x,y)
        AcGePoint2d ptMin = AcGePoint2d( stod( splitRes[1] ), stod( splitRes[3] ) );
        AcGePoint2d ptMax = AcGePoint2d( stod( splitRes[2] ), stod( splitRes[4] ) );
        AcGePoint2d ptCenter = midPoint2d( ptMin, ptMax );
        
        AcDbPoint* point = new AcDbPoint( getPoint3d( ptCenter ) );
        point->setColor( red );
        addToModelSpace( point );
        point->close();
        
        //Recuperer le chemin vers le pcd
        AcString source = pcjFolder + strToAcStr( splitRes[0] );
        AcString rarSource = source;
        rarSource.replace( _T( ".pcd" ), _T( ".rar" ) );
        AcString destination = pcjFolder + _T( "\\Export\\Reste\\" ) + strToAcStr( splitRes[0] );
        
        //Verifier si le fichier pcd existe
        if( isFileExisting( source ) )
        {
            //Copier le pcd vers le dossier
            AcString destination = pcjFolder + _T( "\\Export\\Reste\\" ) + strToAcStr( splitRes[0] );
            
            copyFileTo( source, destination );
        }
        
        else if( isFileExisting( rarSource ) )
        {
            //Copier le rar
            AcString rarFile = strToAcStr( splitRes[0] );
            rarFile.replace( _T( ".pcd" ), _T( ".rar" ) );
            AcString destination = pcjFolder + _T( "\\Export\\Reste\\" ) + rarFile;
            
            copyFileTo( rarSource, destination );
        }
        
        else
            continue;
            
        //Ecrire dans le pcj
        restePcjFile << pcjLineVec[i] << "\n";
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Fermer le pcj
    restePcjFile.close();
}


void cmdDivPCJ()
{
    Acad::ErrorStatus es;
    
    bool bXlsx = false;
    
    if( askYesOrNo( bXlsx, _T( "Selectionner un fichier Xlsx" ) ) == RTCAN )
    {
        print( "Commande annulée." );
        bXlsx = false;
        return;
    }
    
    AcString filePath;
    
    if( !bXlsx )
        divPcj();
        
        
    else
        divPcjXlsx();
}

void cmdCleanImgGtcFolder()
{
    //Demander de selectionner le fichier gtc
    AcString gtcFilePath = askForFilePath( true, "gtc", _T( "Selectionner le GTC" ) );
    
    //Verifier
    if( gtcFilePath == _T( "" ) )
    {
        //Afficher message
        print( "Selection vide" );
        return;
    }
    
    //Recuperer le dossier du gtc
    auto idx = gtcFilePath.findOneOfRev( _T( "\\" ) );
    AcString gtcFileFolder = gtcFilePath.substr( idx );
    
    //Creer un ifstream sur le fichier
    ifstream gtcFile( acStrToStr( gtcFilePath ) );
    
    //Vecteur de string
    vector<string> imagePath;
    
    //Ligne du fichier
    string line;
    
    //Boucle sur les lignes du fichier
    while( getline( gtcFile, line ) )
    {
        //Verifier la ligne
        if( line.find( "<Image Source=\"" ) != -1 )
            imagePath.push_back( line );
    }
    
    //Fermer le fichier gtc
    gtcFile.close();
    
    //Creer un ofstream
    ofstream logFile( acStrToStr( gtcFileFolder ) + "\\imageFiles.txt" );
    
    //Recuperer la taille du vecteur
    long vecSize = imagePath.size();
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Préparation au suppression" ), vecSize );
    
    //Nettoyer le vecteur de nom
    for( long i = 0; i < vecSize; i++ )
    {
        //Recuperer le ieme ligne
        line = imagePath[i];
        
        //Chercher seuluement le chemin de l'image
        if( line.find( "Source=\"\"" ) != -1 )
            continue;
            
        auto id1 = line.find( "Source=\"." );
        auto id2 = line.find( ".png" );
        
        if( id2 == -1 )
            id2 = line.find( ".PNG" );
            
        line = line.substr( id1 + 9, ( id2 + 3 ) - ( id1 + 8 ) );
        
        imagePath[i] = acStrToStr( gtcFileFolder ) + line;
        
        //Ajouter le chemine complet
        logFile << acStrToStr( gtcFileFolder ) + line << "\n";
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Fermer le logFile
    logFile.close();
    
    //Recuperer tous les fichiers dans le dossier imagecache
    AcString command = _T( "cd " ) + gtcFileFolder + _T( "\\ImageCache && dir /s /b /o:gn > allFile.txt" );
    
    //Lancer la commande
    system( acStrToStr( command ).c_str() );
    
    //Ouvrir le fichier crée
    ifstream allFile( acStrToStr( gtcFileFolder + _T( "\\ImageCache\\allFile.txt" ) ) );
    
    //Vecteur de string
    vector<string> allFileVec;
    
    //Boucler sur le fichier
    while( getline( allFile, line ) )
    {
        //Verifier si png
        if( line.find( ".png" ) != -1 || line.find( ".PNG" ) != -1 )
            allFileVec.push_back( line );
    }
    
    //Fermer le fichier
    allFile.close();
    
    //Taille de tous les fichier
    long allFileSize = allFileVec.size();
    
    //Compteur
    long comp = 0;
    
    //Barre de progression
    ProgressBar prog2 = ProgressBar( _T( "Suppression des images" ), allFileSize );
    
    //Boucle de suppression
    for( long i = 0; i < allFileSize; i++ )
    {
        //Recuperer le ieme ligne
        line = allFileVec[i];
        
        bool found = true;
        
        //Boucle sur les bons nom
        for( long j = 0; j < vecSize; j++ )
        {
            //Verifier
            if( line == imagePath[j] )
            {
                found = true;
                break;
            }
            
            else
                found = false;
        }
        
        if( !found )
        {
            remove( line.c_str() );
            comp++;
        }
        
        //Progresser
        prog2.moveUp( i );
    }
    
    //Afficher message
    print( "Nombre d'image supprimé : %d", comp );
}


void cmdProjectJoi()
{
    //Selection sur les blocs
    ads_name ssBloc;
    
    //Selection sur les polylignes 3d
    ads_name ssPoly3d;
    
    //Demander de selectionner les blocs
    long lenBloc = getSsBlock( ssBloc );
    
    //Verifier
    if( lenBloc == 0 )
    {
        //Afficher message
        print( "Commande annulée" );
        
        return;
    }
    
    //Demander de selectionner les polylignes 3d
    long lenPoly = getSsPoly3D( ssPoly3d );
    
    //Verifier
    if( lenPoly == 0 )
    {
        //Afficher message
        print( "Commande annulée" );
        
        //Liberer les selections
        acedSSFree( ssPoly3d );
        
        return;
    }
    
    //Compteur
    long comp = 0;
    
    //Reference de bloc
    AcDbBlockReference* block = NULL;
    
    //Polyligne 3d
    AcDb3dPolyline* poly3d = NULL;
    
    //Point de projection de la polylignes
    AcGePoint3d ptProj = AcGePoint3d::kOrigin;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), lenBloc );
    
    //Boucle sur les blocs
    for( int i = 0; i < lenBloc; i++ )
    {
        //Recuperer le bloc
        block = getBlockFromSs( ssBloc, i, AcDb::kForWrite );
        
        //Verifier le bloc
        if( !block )
            continue;
            
        //On vérifie le nom du bloc
        AcString blckName = _T( "" );
        
        if( eOk != getDynamicBlockName( blckName, block ) )
        {
            //Fermer le bloc
            block->close();
            
            continue;
        }
        
        // Vérification du nom de bloc
        if( blckName == _T( "Tra_Typ" ) ||
            blckName == _T( "Joi" ) )
        {
            //Recuperer le prop du bloc
            double length = getPropertyFromBlock( block,
                    _T( "Longueur" ) );
                    
            //Recuperer le point d'insertion du bloc
            AcGePoint3d ptIns = block->position();
            
            //Recuperer la matrice de transformation du bloc
            AcGeMatrix3d matB = block->blockTransform();
            
            //Recuperer le point du bloc à setter
            AcGePoint3d ptB = AcGePoint3d( 0, 1.435, 0 ).transformBy( matB );
            
            //Creer un vecteur le long du bloc
            AcGeVector3d vecBloc = getVector3d( ptIns, ptB );
            
            //Recuperer un vecteur perpendiculaire au vecteur du bloc
            AcGeVector3d vecBlocPerp = vecBloc.perpVector().normalize();
            
            //Boucle sur les polylignes
            for( int p = 0; p < lenPoly; p++ )
            {
                //Recuperer le p-eme polyligne
                poly3d = getPoly3DFromSs( ssPoly3d, p, AcDb::kForRead );
                
                //Verifier la polyligne
                if( !poly3d )
                    continue;
                    
                //Recuperer le point
                AcGeVector3d vecTemp = getVector3d( ptIns, ptB ).normalize() * length;
                AcGePoint3d ptTemp = ptIns + vecTemp;
                
                //Plaquer sur la polyligne
                poly3d->getClosestPointTo( ptTemp, ptProj );
                
                //Verifier
                if( ptProj == AcGePoint3d::kOrigin )
                    continue;
                    
                //Verifier si on a le meme point
                if( !isEqual2d( ptTemp, ptProj ) )
                {
                    //Fermer la polyligne
                    poly3d->close();
                    
                    continue;
                }
                
                //Fermer la polyligne
                poly3d->close();
                
                //Creer un vecteur vers le point de la polyligne
                AcGeVector3d vecPoly = getVector3d( ptIns, ptProj );
                
                //Calculer l'angle entre les deux vecteur
                double angle = vecBloc.angleTo( vecPoly, vecBlocPerp );
                
                //Creer une matrice de rotation
                AcGeMatrix3d rot = AcGeMatrix3d::rotation( angle, vecBlocPerp, ptIns );
                
                //Roter le bloc
                block->transformBy( rot );
                
                //Setter la nouvelle longueur
                setPropertyFromBlock( block, _T( "Longueur" ), vecPoly.length() );
                
                //Compter
                comp++;
                
                //Sortir du boucle
                break;
            }
        }
        
        //Fermer le bloc
        block->close();
        
        //Porgresser
        prog.moveUp( i );
    }
    
    //Afficher message
    print( "Nombre de bloc traité : %d", comp );
    
    //Liberer les selections
    acedSSFree( ssPoly3d );
    acedSSFree( ssBloc );
}

void cmdDrawPcj()
{
    //Demander de selectionner le fichier pcj
    AcString pcjPath = askForFilePath( true, _T( "pcj" ), _T( "Selectionner le fichier pcj" ),
            getCurrentFileFolder() );
            
    //Verifier
    if( pcjPath == _T( "" ) )
    {
        //Afficher message
        print( "Entrée vide" );
        
        return;
    }
    
    //Creer un ifstream
    ifstream pcjFile( acStrToStr( pcjPath ) );
    
    //Recuperer le nombre de ligne dans le fichier
    long numLine = getNumberOfLines( acStrToStr( pcjPath ) );
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), numLine );
    
    //Ligne di fichier
    string line;
    
    int i = 0;
    
    //Boucler sur les lignes du fichier
    while( getline( pcjFile, line ) )
    {
        //Splitter la ligne
        vector<string> splittedLine = splitString( line, "\t" );
        
        //Passer les entetes
        if( line.find( "pcd" ) == -1 )
            continue;
            
        //Recuperer les points
        AcDbExtents bb = AcDbExtents( AcGePoint3d( stod( splittedLine[1] ), stod( splittedLine[3] ), stod( splittedLine[5] ) ),
                AcGePoint3d( stod( splittedLine[2] ), stod( splittedLine[4] ), stod( splittedLine[6] ) ) );
                
        //Creer un texte
        AcDbText* txt = new AcDbText( midPoint3d( bb.minPoint(), bb.maxPoint() ), strToAcStr( splittedLine[0] ) );
        addToModelSpace( txt );
        txt->close();
        
        //Dessiner le boundingbox
        drawBoundingBox3d( bb );
        
        //Progresser
        prog.moveUp( i++ );
    }
    
    //Fermer le fichier
    pcjFile.close();
}

void cmdKdnCheckSeuil()
{
    //Selection sur les polylignes 2_SeuilPCRS
    ads_name ssPoly3d;
    
    //Selection sur les autres polylignes
    ads_name ssOtherPoly3d;
    
    //Recuperer tous les polylignes 2_SeuilPCRS
    long lenPoly3d = getSsAllPoly3D( ssPoly3d, _T( "2_SeuilPCRS" ) );
    
    //Verifier
    if( lenPoly3d == 0 )
    {
        //Afficher message
        print( "Impossible de recuperer les polylignes de 2_SeuilPCRS" );
        
        return;
    }
    
    //Recuperer les autres polylignes
    long lenOtherPoly3d = getSsAllPoly3D( ssOtherPoly3d, _T( "3_MurPCRS,3_Mur-BahutPCRS,3_Mur-SoutPCRS,3_Mur-ParapetPCRS,3_Mur-EnrochPCRS,3_Mur-CloturePCRS,5_HaiePCRS,2_FacadePCRS,1_LimiteVoiePCRS" ) );
    
    //Verifieri
    if( lenOtherPoly3d == 0 )
    {
        //Afficher message
        print( "Impossible de recuperer les autres polylignes" );
        
        //Liberer la selection
        acedSSFree( ssPoly3d );
        
        return;
    }
    
    //Creer le calque des patates
    AcCmColor red;
    red.setRGB( 255, 0, 0 );
    AcString patateLayer = _T( "PATATES_KDNCHECKSEUIL" );
    createLayer( patateLayer, red );
    
    //Polyligne
    AcDb3dPolyline* poly3D = NULL;
    AcDb3dPolyline* otherPoly3D = NULL;
    
    //Compteur
    int compt = 0;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), lenPoly3d );
    
    //Boucle sur les polylignes
    for( long i = 0; i < lenPoly3d; i++ )
    {
        //Recuperer le ieme polylignes
        poly3D = getPoly3DFromSs( ssPoly3d, i, AcDb::kForRead );
        
        //Verifier
        if( !poly3D )
            continue;
            
        //Booleen de test
        bool isOk = false;
        
        //Recuperer les extremités de la polyligne
        AcGePoint3d ptStart, ptEnd;
        poly3D->getStartPoint( ptStart );
        poly3D->getEndPoint( ptEnd );
        
        //Fermer la polyligne
        poly3D->close();
        
        //Boucle sur les autres polylignes
        for( long p = 0; p < lenOtherPoly3d; p++ )
        {
            //Recuperer le peme polyligne
            otherPoly3D = getPoly3DFromSs( ssOtherPoly3d, p, AcDb::kForRead );
            
            //Verifier
            if( !otherPoly3D )
                continue;
                
            //Points projeté sur la polyligne
            AcGePoint3d ptStartProj = AcGePoint3d::kOrigin;
            AcGePoint3d ptEndProj = AcGePoint3d::kOrigin;
            
            //Point projeté
            otherPoly3D->getClosestPointTo( ptStart, ptStartProj );
            otherPoly3D->getClosestPointTo( ptEnd, ptEndProj );
            
            //Point start et end de la polyligne
            AcGePoint3d ptOstart = AcGePoint3d::kOrigin;
            AcGePoint3d ptSstart = AcGePoint3d::kOrigin;
            otherPoly3D->getStartPoint( ptOstart );
            otherPoly3D->getEndPoint( ptSstart );
            
            //Verifier
            if( isEqual2d( ptStart, ptStartProj ) ||
                isEqual2d( ptEnd, ptEndProj ) ||
                isEqual2d( ptStart, ptOstart ) ||
                isEqual2d( ptEnd, ptSstart ) )
                isOk = true;
                
            //Fermer la polyligne
            otherPoly3D->close();
        }
        
        //Ajouter la patate
        if( !isOk )
        {
            //Tracer la patate
            drawCircle( midPoint3d( ptStart, ptEnd ), 1, patateLayer );
            
            //Compter
            compt += 2;
        }
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Liberer les selections
    acedSSFree( ssPoly3d );
    acedSSFree( ssOtherPoly3d );
    
    //Afficher message
    print( "Nombre de patate insérés : %d", compt );
}

void cmdKdnCorrectSeuil()
{
    //Selection sur les polylignes 2_SeuilPCRS
    ads_name ssPoly3d;
    
    //Recuperer tous les polylignes 2_SeuilPCRS
    long lenPoly3d = getSsAllPoly3D( ssPoly3d, _T( "2_SeuilPCRS" ) );
    
    //Verifier
    if( lenPoly3d == 0 )
    {
        //Afficher message
        print( "Impossible de recuperer les polylignes de 2_SeuilPCRS" );
        
        return;
    }
    
    //Creer le calque des patates
    AcCmColor yellow;
    yellow.setRGB( 255, 255, 0 );
    AcString patateLayer = _T( "PATATES_KDNCORRECTSEUIL" );
    createLayer( patateLayer, yellow );
    
    //Compteur
    int comp = 0;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), lenPoly3d );
    
    //Polyligne 3d
    AcDb3dPolyline* poly = NULL;
    
    //Boucle sur les selections des polylignes 3d
    for( int i = 0; i < lenPoly3d; i++ )
    {
        //Recuperer la polyligne
        poly = getPoly3DFromSs( ssPoly3d,
                i,
                AcDb::kForRead );
                
        //Verifier
        if( !poly )
            continue;
            
        //Recuperer le nombre de vertex de la polyligne
        int numVerts = getNumberOfVertex( poly );
        
        //Verifier
        if( numVerts > 2 )
        {
            //Recuperer le point ou on va ajouter une patate
            AcGePoint3d ptPatate;
            AcGePoint3d ptStart, ptEnd;
            
            poly->getStartPoint( ptStart );
            poly->getEndPoint( ptEnd );
            
            AcGePoint3d ptMidPoint = midPoint3d( ptStart, ptEnd );
            
            poly->getClosestPointTo( ptMidPoint, ptPatate );
            
            //Ajouter une patate
            drawCircle( ptPatate, 1, patateLayer );
            
            //Compter
            comp++;
        }
        
        //Fermer la polyligne
        poly->close();
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Liberer la slelection
    acedSSFree( ssPoly3d );
    
    //Afficher message
    print( "Nombre de patate ajouté : %d", comp );
}

void cmdEscaFleche()
{
    //Selection sur les polylignes
    ads_name ssPoly;
    
    //Recuperer les polylignes escalier du dessin
    long lenPoly = getSsAllPoly3D( ssPoly, _T( "TOPO_Bâtiments_Escaliers" ) );
    
    //Verifier
    if( lenPoly == 0 )
    {
        //Afficher message
        print( "Selection vide" );
        return;
    }
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), lenPoly );
    
    //Compteur
    int compt = 0;
    
    //Polyligne 3d
    AcDb3dPolyline* poly3d = NULL;
    
    //Boucle sur les polylignes
    for( long i = 0; i < lenPoly; i++ )
    {
        //Recuperer le ieme polyligne
        poly3d = getPoly3DFromSs( ssPoly, i, AcDb::kForRead );
        
        //Verifier
        if( !poly3d )
            continue;
            
        //Recuperer le milieu de la polyligne
        AcGePoint3d centroid;
        
        if( eOk != getCentroid( poly3d, centroid ) )
        {
            poly3d->close();
            continue;
        }
        
        //Mettre le centroide à 0
        centroid.z = 0;
        
        //Recuperer les sommets de la polyligne
        AcGePoint3dArray vtxs;
        vector<double> vecX;
        getVertexesPoly( poly3d,
            vtxs,
            vecX );
            
        //Recuperer la taille des vertexes
        int vtxsSize = vtxs.size();
        
        //Verifier
        if( vtxsSize == 0 )
        {
            poly3d->close();
            continue;
        }
        
        //Fermer la polyligne
        poly3d->close();
        
        //Vecteur pour calculer l'orientation du bloc
        AcGeVector2d vec;
        
        //Recuperer le trois premiers points
        AcGePoint3d pt1 = vtxs[0];
        AcGePoint3d pt2 = vtxs[1];
        AcGePoint3d pt3 = vtxs[2];
        
        //Recuperer le distance
        double dist12 = getDistance2d( pt1, pt2 );
        double dist23 = getDistance2d( pt2, pt3 );
        
        //Verifier
        if( dist12 > dist23 )
            vec = getVector2d( pt2, pt3 );
        else if( dist23 > dist12 )
            vec = getVector2d( pt1, pt2 );
        else
        {
            if( pt3.z > pt2.z && pt3.z > pt1.z )
                vec = getVector2d( pt2, pt3 );
            else
                vec = getVector2d( pt1, pt2 );
        }
        
        //Inserer le bloc sur le centroide
        AcDbBlockReference* block = insertBlockReference( _T( "ESC01" ),
                centroid,
                0 );
                
        //Verifier
        if( !block )
            continue;
            
        block->setRotation( vec.angle() - M_PI_2 );
        
        //Mettre le bloc dans le bon calque
        block->setLayer( _T( "TOPO_Bâtiments_Habillages" ) );
        
        //Fermer le block
        block->close();
        
        //Compter
        compt++;
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Liberer la selection
    acedSSFree( ssPoly );
    
    //Afficher message
    print( "Nombre de bloc ajouté : %d", compt );
}


void cmdCheckTopoAttr()
{
    //Selection sur les blocs
    ads_name ssBlock;
    
    //Selection sur les polylignes 3d
    ads_name ssPoly;
    
    //Recuperer les blocs
    long lenBloc = getSsAllBlock( ssBlock, _T( "TOPO_Points_Piqués" ), _T( "PTOPO" ) );
    
    //Recuperer les polylignes
    long lenPoly = getSsAllPoly3D( ssPoly );
    
    //Verifier
    if( lenBloc == 0 )
    {
        print( "Selection de bloc vide" );
        return;
    }
    
    //Creer une calque de patate
    AcString patateLayer = _T( "PATATES_TOPO_ATTR" );
    AcCmColor red;
    red.setRGB( 255, 0, 0 );
    createLayer( patateLayer, red );
    
    //Creer un plan de projection en 2d
    AcGePlane plane = AcGePlane( AcGePoint3d::kOrigin,
            AcGeVector3d::kXAxis,
            AcGeVector3d::kYAxis );
            
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), lenBloc );
    
    //Compteur
    int compt = 0;
    
    //Reference de bloc
    AcDbBlockReference* topo = NULL;
    
    //Boucle sur les blocs
    for( long b = 0; b < lenBloc; b++ )
    {
        //Recuperer le block
        topo = getBlockFromSs( ssBlock, b, AcDb::kForRead );
        
        //Verifier le bloc
        if( !topo )
            continue;
            
        //Tableau de pointeur
        AcDbVoidPtrArray entityArray;
        
        //Exploser le bloc
        Acad::ErrorStatus es = topo->explode( entityArray );
        
        if( es != eOk )
        {
            topo->close();
            continue;
        }
        
        //Recuperer l'attribut du bloc
        AcDbText* attr = getTopoAttr( entityArray );
        
        //Verifier
        if( !attr )
            continue;
            
        //Reference de bloc
        AcDbBlockReference* topoTemp = NULL;
        
        //Reboucle sur tous le blocs
        for( long ob = 0; ob < lenBloc; ob++ )
        {
            //Recuperer le block
            topoTemp = getBlockFromSs( ssBlock, ob, AcDb::kForRead );
            
            //Verifier le bloc
            if( !topoTemp || topoTemp->id() == topo->id() )
                continue;
                
            //Tableau de pointeur
            AcDbVoidPtrArray entityTempArray;
            
            //Exploser le bloc
            Acad::ErrorStatus es = topoTemp->explode( entityTempArray );
            
            if( es != eOk )
            {
                topoTemp->close();
                continue;
            }
            
            //Resultat
            AcDbText* tempAttr = getTopoAttr( entityTempArray );
            
            //Tableau de point qui va contenir les points d'intersection
            AcGePoint3dArray intersectArrayB;
            
            //Recuperer les intersections
            attr->intersectWith( tempAttr,
                AcDb::kOnBothOperands,
                plane,
                intersectArrayB );
                
            //Verifier
            if( intersectArrayB.size() > 0 )
            {
                //Ajouter une patate
                drawCircle( attr->position(),
                    1,
                    patateLayer );
                    
                //Compter
                compt++;
            }
            
            //Fermer l'attribut
            tempAttr->close();
            
            //Fermer le bloc
            topoTemp->close();
        }
        
        //Polyligne 3d
        AcDb3dPolyline* poly3d = NULL;
        
        //Boucle sur les polylinge
        for( long p = 0; p < lenPoly; p++ )
        {
            //Recuperer la polyligne
            poly3d = getPoly3DFromSs( ssPoly, p );
            
            //Verifier
            if( !poly3d )
                continue;
                
            //Tableau de point qui va contenir les points d'intersection
            AcGePoint3dArray intersectArrayP;
            
            //Recuperer les intersections
            poly3d->intersectWith( attr,
                AcDb::kOnBothOperands,
                plane,
                intersectArrayP );
                
            //Verifier
            if( intersectArrayP.size() > 0 )
            {
                //Ajouter une patate
                drawCircle( attr->position(),
                    1,
                    patateLayer );
                    
                //Compter
                compt++;
            }
            
            //Fermer la polyligne
            poly3d->close();
        }
        
        //Fermer le bloc
        topo->close();
        
        //Fermer l'attribut
        attr->close();
        
        //Progresser
        prog.moveUp( b );
    }
    
    //Liberer la selection
    acedSSFree( ssBlock );
    acedSSFree( ssPoly );
    
    //Afficher message
    print( "Nombre de patate ajoutés : %d", compt );
}


void cmdSiemlRotate()
{
    //Selectionner les points topos
    ads_name ssPointTopo, ssBlock;
    print( "Selectionner les points topos" );
    int nbPointTopo = getSsBlock( ssPointTopo, "", "FMAP_Elevation" );
    
    //Verifier il y a trois blocks
    if( nbPointTopo != 3 )
    {
        print( "Nombre de point topo incorecte" );
        return;
    }
    
    print( "Selectionner le block a inserer" );
    int blockNumber = getSsOneBlock( ssBlock, "", "FMAP_Regard_Ass_Rec,FMAP_GrilleAvaloir_Rec" );
    
    //Verifier q'ona un seul block
    if( blockNumber != 1 )
    {
        print( "Selection de block pas valide" );
        return;
    }
    
    //Recuperer les points d'insersions des trois points topos
    AcGePoint3dArray pointArray;
    
    for( int i = 0; i < 3; i++ )
    {
        AcDbBlockReference* pointTopos = getBlockFromSs( ssPointTopo, i, AcDb::kForRead );
        AcGePoint3d pPosition = pointTopos->position();
        pointArray.push_back( pPosition );
        pointTopos->close();
    }
    
    AcGePoint3d solo;
    pair<AcGePoint3d, AcGePoint3d> duo;
    double dist = DBL_MAX;
    double zMoyenne = DBL_MAX;
    
    //Recuperer le block a modifier
    AcDbBlockReference* blockRef = getBlockFromSs( ssBlock, 0, AcDb::kForWrite );
    AcGeScale3d scaleFactor = blockRef->scaleFactors();
    
    //Zhercher le z solo et le z moyenne
    for( int i = 0; i < 3; i++ )
    {
        int j, k;
        
        if( i == 0 ) { j = 1; k = 2; }
        
        else if( i == 1 ) { j = 2; k = 0; }
        
        else if( i == 2 ) { j = 0; k = 1; }
        
        if( abs( pointArray[i].z - pointArray[j].z ) < abs( dist ) )
        {
            dist = abs( pointArray[i].z - pointArray[j].z );
            zMoyenne = ( pointArray[i].z + pointArray[j].z ) / 2;
            duo = make_pair( pointArray[i], pointArray[j] );
            solo = pointArray[k];
        }
    }
    
    //Chercher l'origine du plan
    duo.first.z = zMoyenne;
    duo.second.z = zMoyenne;
    double angle1 = getVector2d( duo.first, duo.second ).angleTo( getVector2d( duo.first, solo ) );
    double angle2 = getVector2d( duo.second, duo.first ).angleTo( getVector2d( duo.second, solo ) );
    AcGePoint3d origin, uDir, vDir;
    
    if( angle1 > angle2 ) { origin = duo.first; uDir = duo.second; vDir = solo; }
    
    else { origin = duo.second; uDir = duo.first; vDir = solo; }
    
    double dist12d = getDistance2d( origin, uDir );
    double dist22d = getDistance2d( origin, vDir );
    double dist13d = origin.distanceTo( uDir );
    double dist23d = origin.distanceTo( vDir );
    
    double dist1X;
    double dist1Y;
    double dist2X;
    double dist2Y;
    dist1X = dist12d;
    dist2X = dist13d;
    dist1Y = dist22d;
    dist2Y = dist23d;
    //Creer le plan dont le block est dessiner
    AcGePlane plan = AcGePlane( origin, getVector3d( origin, uDir ), getVector3d( origin, vDir ) );
    
    AcGeVector3d vec = plan.normal();
    //vec.normalize();
    
    double scalex = dist2X / dist1X;
    double scaley = dist2Y / dist1Y;
    scaleFactor.sx *= scalex;
    scaleFactor.sy *= scaley;
    blockRef->setScaleFactors( scaleFactor );
    //scaleFactor = blockRef->scaleFactors();
    
    AcGePoint3d center = blockRef->position();
    print( center );
    blockRef->setNormal( vec );
    blockRef->setRotation( 0 );
    center.z = ( zMoyenne + solo.z ) / 2 ;
    blockRef->setPosition( center );
    print( center );
    //drawLine(center, AcGePoint3d(center.x + vec.x, center.y + vec.y, center.z + vec.z));
    blockRef->close();
    
    //Liberer les selections
    acedSSFree( ssPointTopo );
}

void cmdSetViewPortEp()
{
    //Enter l'epaisseur du coupe
    double epaisseur = 1.0;
    askForDouble( _T( "Entrer l'epaisseur de du viewPort" ), epaisseur, epaisseur );
    
    //Recuperer les viewports
    acedVports2VportTableRecords();
    AcDbObjectId activeViewPortId = acedActiveViewportId();
    AcDbObject* viewPortObj;
    acdbOpenAcDbObject( viewPortObj, activeViewPortId, AcDb::kForWrite );
    
    AcDbViewportTableRecord* viewport = AcDbViewportTableRecord::cast( viewPortObj );
    //Modifier le profondeurs du champ
    //Recuperer l'SCU
    AcGePoint3d origScu;
    AcGeVector3d xScu, yScu;
    viewport->getUcs( origScu, xScu, yScu );
    viewport->setTarget( origScu );
    
    viewport->setFrontClipEnabled( true );
    viewport->setBackClipEnabled( true );
    viewport->setFrontClipDistance( epaisseur / 2 );
    viewport->setBackClipDistance( -epaisseur / 2 );
    viewport->close();
}


void cmdBimEtiqueter()
{
    //Déclaration des variables utiles pour le traitement du fichier excel
    AcString file;          // Fichier excel
    string filename,        //Chemine vers le fichier
           s_coord_x,       //Coordonnée x
           s_coord_y,       //Coordonnée y
           s_famille,       //Famille
           s_niveau;        //Niveau
    double tolerance = 1.0; //Tolerance
    
    //Configuration
    AcString config;        // Chemin du fichier excel en AcString
    string configname;    // Chemin du fichier excel en string
    map<string, vector<string>> map_config; // Map qui va servir pour stocker les correspondances
    
    
    
    //Variables de conversion de types
    double d_coord_x;        // Coord x en double
    double d_coord_y;        //Coord y en double
    string bloc_name_to_xls; // Le nom du block à inserer dans le fichier excel
    
    //Variables gst
    vector<string> vec_test_name;  //Vecteur contenant les noms par correspondance du fichier config
    ads_name ssBlock;
    AcGePoint3d pt_xls = AcGePoint3d::kOrigin;
    AcDbBlockReference* nearest_block = NULL;
    AcDbBlockReference* temp_block = NULL;
    
    map<string, xlsBlock> struct_block;
    
    
    
    //Variables pour récuperer l'indice du cellule contenant Coord_x; Coord_y; Niveau; Famille
    int coordXId = -1;
    int coordYId = -2;
    int familleId = -3;
    int niveauId = -4;
    int fboId = -5;
    
    
    AcString temp_name_b; //Nom temporaire d'un block
    
    
    //Obtenir les informations concernant les fichiers excel et la tolerance
    askFileInfos( filename, configname, tolerance );
    
    //Verifier
    if( filename == "" || configname == "" || tolerance == 0 )
    {
        print( "Impossible de terminer l'opération." );
        print( "Commande annulée" );
        return;
    }
    
    //Obtenir les informations du fichier de configuration
    getConfigInfos( configname, map_config );
    
    vector<AcString> vec_blocName = getBlockList();
    
    //Créer un workbook excel
    xlnt::workbook wb_excel;
    
    
    try
    {
        //Charger le fichier excel
        wb_excel.load( filename );
    }
    
    catch( const xlnt::exception& e )
    {
        print( "Impossible de charger le fichier" );
        
        //Liberer la selection
        acedSSFree( ssBlock );
        
        return;
    }
    
    //Se positionner dans la feuille courante
    xlnt::worksheet ws = wb_excel.active_sheet();
    bool had_id = false; //Tester si tous les variables ont des données correctes
    
    //Récuperer le numéro de colonne de chaque titre
    for( auto row : ws.rows( false ) )
    {
        for( auto cell : row )
        {
            if( cell.to_string() == "Coord_X" )
                coordXId = cell.column_index();
                
            if( cell.to_string() == "Coord_Y" )
                coordYId = cell.column_index();
                
            if( cell.to_string() == "Famille" )
                familleId = cell.column_index();
                
            if( cell.to_string() == "Niveau" )
                niveauId = cell.column_index();
                
            if( cell.to_string() == "FB01 _ ID unique" )
                fboId = cell.column_index();
                
            if( coordXId != -1 && coordYId != -2 && familleId != -3 && niveauId != -4 && fboId != -5 )
            {
                had_id = true;
                break;
            }
        }
        
        if( had_id )
            break;
    }
    
    string temp = ws.cell( coordXId, 5 ).to_string();
    
    //Indice ligne
    int i = 5;
    s_niveau = "";
    
    while( temp.compare( "" ) != 0 )
    {
        //Reinitialiser vec_test_name
        vec_test_name.clear();
        
        //Récuperer les données du fichier excel
        s_coord_x = ws.cell( coordXId, i ).to_string();
        s_coord_y = ws.cell( coordYId, i ).to_string();
        s_famille = ws.cell( familleId, i ).to_string();
        s_niveau = ws.cell( niveauId, i ).to_string();
        
        //Si le niveau est vide on ne traite pas cette ligne
        if( s_niveau == "" )
        {
            //Incrementer la ligne
            i++;
            
            //Mettre à jour temp
            temp = ws.cell( coordXId, i ).to_string();
            continue;
        }
        
        //Rechercher le niveau dans s_niveau
        s_niveau = std::regex_replace( s_niveau, std::regex( "[^0-9]*" ), std::string( "$1" ) );
        
        //Conversion de types
        d_coord_x = stod( s_coord_x );
        d_coord_y = stod( s_coord_y );
        
        //Le point à partir duquel on effectue la precision
        pt_xls.x = d_coord_x;
        pt_xls.y = d_coord_y;
        pt_xls.z = 0;
        
        //Trouver le nom du block selon le vecteur de configuration map_config
        map<string, vector<string>>::iterator conf_iter = map_config.find( s_famille );
        
        for( ; conf_iter != map_config.end(); ++conf_iter )
        {
            //chercher la liste de noms correspondant
            if( conf_iter->first == s_famille )
            {
                vec_test_name = conf_iter->second;
                break;
            }
        }
        
        //Concatener tous le noms du bloc dans cette variable
        string concat_bloc = "";
        int size_blockname = vec_blocName.size();
        
        for( int s = 0; s < size_blockname; s++ )
        {
            //Stocker le s-ieme nom du block
            string nm = acStrToStr( vec_blocName[s] );
            
            //Récuperer le niméro du niveau
            string sbstr = nm.substr( 0, 7 );
            
            //Le nombre qui s'y trouve
            sbstr = std::regex_replace( sbstr, std::regex( "[^0-9]*" ), std::string( "$1" ) );
            
            int name_zize = vec_test_name.size();
            
            //Recuperer le nom du bloc en faisant une recherche avec vec_test_name
            for( int t = 0; t < name_zize; t++ )
            {
                if( nm.find( vec_test_name[t] ) != string::npos && sbstr == s_niveau )
                    concat_bloc += nm + ",";
            }
        }
        
        if( !concat_bloc.empty() )
            concat_bloc.pop_back();
        else
        {
            // Incrementer la ligne
            i++;
            
            //Mettre à jour temp
            temp = ws.cell( coordXId, i ).to_string();
            
            continue;
        }
        
        //Récuperer seulement les blocks dont le nom est dans conc_bloc
        AcString block_name = strToAcStr( concat_bloc );
        long bnb = getSsAllBlock( ssBlock, _T( "" ), block_name );
        
        //Récuperer le block le plus proche
        double dist = 10000;
        AcDbObjectId idDist = AcDbObjectId::kNull;
        AcDbEntity* ent = NULL;
        
        for( int q = 0; q < bnb; q++ )
        {
            //Recuperer le q-eme block
            temp_block = getBlockFromSs( ssBlock, q );
            
            //Verifier si temp_block n'est pas NULL
            if( !temp_block )
                continue;
                
            //Recuperer le nom du block
            getBlockName( temp_name_b, temp_block );
            string t_name = acStrToStr( temp_name_b );
            
            //Extraire la sous chaine contenant le niveau
            string nv_sbstr = t_name.substr( 0, 7 );
            string keep_name = "";
            
            //Le nombre qui s'y trouve
            nv_sbstr = std::regex_replace( nv_sbstr, std::regex( "[^0-9]*" ), std::string( "$1" ) );
            int name_zize = vec_test_name.size();
            
            //Comparer les config et le nom du block
            for( int r = 0; r < name_zize; r++ )
            {
                if( t_name.find( vec_test_name[r] ) != string::npos && nv_sbstr == s_niveau )
                {
                    //Distance entre le point dans le fichier excel et la position du block
                    double dist_temp = getDistance2d( pt_xls, temp_block->position() );
                    
                    //Recuperer le block qui a la distance la plus petite par rapport au coord dans le fichier excel
                    if( dist_temp < dist )
                    {
                        dist = dist_temp;
                        idDist = temp_block->id();
                        keep_name = t_name;
                    }
                    
                }
            }
            
            if( !idDist )
            {
                temp_block->close();
                continue;
            }
            
            //Fermer le block
            temp_block->close();
            
            //Ouvrir l'entité associée à idDist
            acdbOpenAcDbEntity( ent, idDist, AcDb::kForRead );
            
            //Verifier si l'entité n'est pas NULL
            if( !ent )
                continue;
                
            //Caster l'entité en block
            nearest_block = AcDbBlockReference::cast( ent );
            
            //verifier
            if( !nearest_block )
            {
                ent->close();
                continue;
            }
            
            //Valider par la tolerance
            if( dist <= tolerance )
            {
                //Tester si la celule est deja occupé
                string cell_val = ws.cell( fboId, i ).to_string();
                
                if( cell_val.compare( "" ) == 0 )
                {
                    //Inserer le nom du block
                    xlsBlock xlb;
                    
                    //Insertion dans le fichier excel
                    bloc_name_to_xls = keep_name;
                    ws.cell( fboId, i ).value( bloc_name_to_xls );
                    
                    //Rechercher si l'élement existe déjà dans struct_block
                    map < string, xlsBlock>::iterator it = struct_block.find( bloc_name_to_xls );
                    
                    if( it != struct_block.end() )
                        it->second.nfois += 1;
                    else
                    {
                        //Sinon on crée un clé pour ce block dans struct_block
                        xlb.c_x = nearest_block->position().x;
                        xlb.c_y = nearest_block->position().y;
                        xlb.etage = s_niveau;
                        xlb.nfois = 1;
                        struct_block.insert( make_pair( bloc_name_to_xls, xlb ) );
                    }
                    
                    
                }
                
                
                
                //Liberer la mémoire
                nearest_block->close();
                ent->close();
                continue;
            }
            
            //Libérer la mémoire
            if( nearest_block )
                nearest_block->close();
                
            if( ent )
                ent->close();
        }
        
        //Incrementer la ligne
        i++;
        
        //Mettre à jour temp
        temp = ws.cell( coordXId, i ).to_string();
    }
    
    
    //Mémorisation du fichier excel
    
    
    
    try
    {
        //Reperage de doublons dans le fichier excel
        AcGePoint3d pat_center = AcGePoint3d::kOrigin;
        AcDbBlockReference* bloc_doublon = NULL;
        AcString pat_layer;
        map<string, xlsBlock>::iterator st_block_iter = struct_block.begin();
        AcCmColor col;
        col.setRGB( 255, 165, 0 );
        
        for( ; st_block_iter != struct_block.end(); st_block_iter++ )
        {
        
            xlsBlock st_temp = st_block_iter->second;
            
            //Créer un calque
            if( st_temp.nfois > 1 )
            {
                //Recuperer le nom et définir le calque approprié
                string bl_name = st_block_iter->first;
                string repeated_str = to_string( st_temp.nfois ) + " fois";
                string laye = "DOUBLON_" + st_temp.etage;
                pat_layer = strToAcStr( laye );
                createLayer( pat_layer, col );
                
                //Point d'insertion du patate
                pat_center.x = st_temp.c_x;
                pat_center.y = st_temp.c_y;
                pat_center.z = 0;
                
                
                //Créer un bloc doublon à partir du gabarit du bloc
                bloc_doublon = insertBlockReference( _T( "doublon_bloc" ), pat_layer, pat_center );
                
                //Verifie si le bloc n'est pas NULL
                if( !bloc_doublon )
                {
                    print( "Le gabarit pour le traitement des doublons n'est pas chargé" );
                    break;
                }
                
                //Setter les attributs du bloc doublon
                setAttributValue( bloc_doublon, _T( "REPEATED" ), strToAcStr( repeated_str ) );
                setAttributValue( bloc_doublon, _T( "NAME" ), strToAcStr( bl_name ) );
                
                //Liberer la mémoire
                bloc_doublon->close();
            }
            
        }
        
        //Liberer la selection
        acedSSFree( ssBlock );
        
        //Sauvegarder les modifications
        wb_excel.save( filename );
        
        print( "Export terminé avec succès" );
    }
    
    catch( const xlnt::exception& e )
    {
        print( "Imporssible de sauvegarder les modifications, remplacer le fichier" );
        
        //Liberer la selection
        acedSSFree( ssBlock );
        
        return;
    }
}


void cmdOrientBrestBlock()
{
    //Selection sur les blocs
    ads_name ssBlock;
    
    AcString blockName = getLayer( _T( "C:\\Futurmap\\Outils\\GStarCAD2020\\BDX\\BREST.txt" ), true );
    
    //Demander de selectionner les blocs
    long lenBlock = getSsBlock( ssBlock, "", blockName );
    
    //Verifier
    if( lenBlock == 0 )
    {
        //Afficher message
        print( "Selection vide" );
        return;
    }
    
    //Bloc reference
    AcDbBlockReference* block = NULL;
    
    //Compteur
    long comp = 0;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), lenBlock );
    
    //Boucle sur les blocs
    for( long i = 0; i < lenBlock; i++ )
    {
        //Recuperer le block
        block = getBlockFromSs( ssBlock, i, AcDb::kForWrite );
        
        //Verifier
        if( !block )
            continue;
            
        double blocRot = block->rotation();
        
        //Prendre juste les bloc en 100grades
        if( !isTheSame( blocRot, 0, 0.001 ) )
        {
            block->close();
            continue;
        }
        
        //Sinon
        block->setRotation( blocRot - 0.015708 );
        block->close();
        
        //Compter
        comp++;
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Liberer la selection
    acedSSFree( ssBlock );
    
    //Afficher message
    print( "Nombre de bloc orienté : %d", comp );
}

void cmdTabLineaire()
{
    ///1. Sélectionner les tabulations
    ads_name ssTabs;
    print( "Sélectionner les tabulations du projet." );
    int nbTabs = getSsLine( ssTabs, _T( "FMAP_Tabulation" ) );
    
    ///2. Sélectionner les linéaires
    ads_name ssLineaires;
    print( "Sélectionner les linéaires du projet." );
    int nbLineaires = getSsPoly2D( ssLineaires );
    
    ///3. Trouver le linéaire de chaque tabulation
    int countTab = 0;
    
    for( int i = 0; i < nbTabs; i++ )
    {
        AcDbLine* tab = getLineFromSs( ssTabs, i, AcDb::kForWrite );
        
        // Itérer sur les linéaires
        for( int j = 0; j < nbLineaires; j++ )
        {
            AcDbPolyline* lin = getPoly2DFromSs( ssLineaires, j );
            
            /// On vérifie que le calque est bien celui d'un bloc
            AcString linLayer = lin->layer();
            
            if( linLayer.find( _T( "Bloc" ) ) != -1 )
            {
                // On cherche si cette tabulation intersecte ce linéaire
                AcGePlane projectionPlane = AcGePlane( AcGePoint3d::kOrigin, AcGeVector3d::kZAxis );
                AcGePoint3dArray intersectArray;
                lin->intersectWith( tab, AcDb::kOnBothOperands,
                    projectionPlane,
                    intersectArray );
                    
                // Si oui, on crée un calque TAB_Bloc_XX de même couleur que le calque Bloc_XX
                if( intersectArray.size() > 0 )
                {
                    AcString tabLayer = _T( "TAB_" ) + linLayer;
                    AcCmColor tabColor;
                    getColorLayer( tabColor, linLayer );
                    createLayer( tabLayer, tabColor );
                    tab->setLayer( tabLayer );
                    countTab++;
                    break;
                }
            }
            
            lin->close();
        }
        
        tab->close();
    }
    
    print( "%d tabulations affectées à un bloc.", countTab );
    print( "%d tabulations ignorées.", nbTabs - countTab );
}

void cmdSytralSeuil()
{
    // Selectionner les blocs seuils
    ads_name ssblockSeuil, ssblockPTopo;
    long lnblockSeuil( 0 ), lnBlockPt( 0 );
    print( "Selectionner les blocs seuils: " );
    lnblockSeuil = getSsBlock( ssblockSeuil, "", "ENTREE" );
    
    if( lnblockSeuil == 0 )
    {
        print( "Sélection vide." );
        acedSSFree( ssblockSeuil );
        return;
    }
    
    // Selectionner les blocs points Topos
    print( "Sélectionner les points topos des blocs Seuils:" );
    lnBlockPt = getSsBlock( ssblockPTopo, "TOPO_Points_Seuil_Piqués" );
    
    if( lnBlockPt == 0 )
    {
        print( "Sélection vide." );
        acedSSFree( ssblockPTopo );
        acedSSFree( ssblockSeuil );
        return;
    }
    
    // Sélectionner les polylignes contours des batiments
    ads_name ssPoly3d;
    print( "Sélectionner les polylignes 3D: " );
    long lnpoly3d = getSsPoly3D( ssPoly3d, "TOPO_Bâtiments_Contours,TOPO_Bâtiments_Annexes" );
    
    if( lnpoly3d == 0 )
    {
        print( "Sélection vide." );
        acedSSFree( ssblockPTopo );
        acedSSFree( ssblockSeuil );
        acedSSFree( ssPoly3d );
        return;
    }
    
    // Créer le calque erreur
    if( eOk != createLayer( "SEUIL_ERREUR" ) )
        print( "Impossible de créer le calque : SEUIL_ERREUR" );
        
    long nbErreur = 0;
    
    if( eOk != sytralSeuil( nbErreur, ssblockSeuil, ssblockPTopo, ssPoly3d ) )
    {
    
        print( "Commande annulée." );
        acedSSFree( ssblockPTopo );
        acedSSFree( ssblockSeuil );
        acedSSFree( ssPoly3d );
        return;
    }
    
    
    print( "Nombre d'erreurs trouvées: %d ", nbErreur );
    acedSSFree( ssblockPTopo );
    acedSSFree( ssblockSeuil );
    acedSSFree( ssPoly3d );
}


void cmdSytralSeuilControl()
{
    // Selectionner les blocs seuils
    ads_name ssblockSeuil, ssblockPTopo;
    long lnblockSeuil( 0 ), lnBlockPt( 0 );
    print( "Selectionner les blocs seuils: " );
    lnblockSeuil = getSsBlock( ssblockSeuil, "", "ENTREE" );
    
    if( lnblockSeuil == 0 )
    {
        print( "Sélection vide." );
        acedSSFree( ssblockSeuil );
        return;
    }
    
    // Selectionner les blocs points Topos
    print( "Sélectionner les points topos des blocs Seuils:" );
    lnBlockPt = getSsBlock( ssblockPTopo, "TOPO_Points_Seuil_Piqués" );
    
    if( lnBlockPt == 0 )
    {
        print( "Sélection vide." );
        acedSSFree( ssblockPTopo );
        acedSSFree( ssblockSeuil );
        return;
    }
    
    // Sélectionner les polylignes contours des batiments
    ads_name ssPoly3d;
    print( "Sélectionner les polylignes 3D: " );
    long lnpoly3d = getSsPoly3D( ssPoly3d, "TOPO_Bâtiments_Contours,TOPO_Bâtiments_Annexes" );
    
    if( lnpoly3d == 0 )
    {
        print( "Sélection vide." );
        acedSSFree( ssblockPTopo );
        acedSSFree( ssblockSeuil );
        acedSSFree( ssPoly3d );
        return;
    }
    
    // Créer le calque erreur
    if( eOk != createLayer( "SEUIL_ERREUR_CONTROL" ) )
        print( "Impossible de créer le calque : SEUIL_ERREUR" );
        
    long nbErreur = 0;
    
    if( eOk != sytralSeuilControl( nbErreur, ssblockSeuil, ssblockPTopo, ssPoly3d ) )
    {
    
        print( "Commande annulée." );
        acedSSFree( ssblockPTopo );
        acedSSFree( ssblockSeuil );
        acedSSFree( ssPoly3d );
        return;
    }
    
    
    print( "Nombre d'erreurs trouvées: %d ", nbErreur );
    acedSSFree( ssblockPTopo );
    acedSSFree( ssblockSeuil );
    acedSSFree( ssPoly3d );
}

void cmdSytralTopo()
{
    // Sélectionner les polylignes fil d'eau
    ads_name ssPoly3D;
    print( "Sélectionner les polylignes 3D: " );
    long lnPoly3d = getSsPoly3D( ssPoly3D, "TOPO_Voirie_Caniveaux,TOPO_Voirie_Bordures" );
    
    if( lnPoly3d == 0 )
    {
        print( "Sélection vide." );
        acedSSFree( ssPoly3D );
        return;
    }
    
    // Sélectionner les blocs points topo
    ads_name ssblock;
    print( "Sélectionner les blocs points Topo:" );
    long lnblock = getSsBlock( ssblock, "TOPO_Points_Piqués,TOPO_Points_Surplus_Piqués" );
    
    if( lnblock == 0 )
    {
        print( "Sélection vide." );
        acedSSFree( ssblock );
        acedSSFree( ssPoly3D );
        return;
    }
    
    
    
    long nbSommets = 0;
    
    if( eOk != sytralTopo( nbSommets, ssPoly3D, ssblock ) )
    {
        print( "Commande annulée." );
        acedSSFree( ssblock );
        acedSSFree( ssPoly3D );
        return;
    }
    
    
    
    
    print( "Nombre de sommets ajoutés: %d", nbSommets );
    acedSSFree( ssblock );
    acedSSFree( ssPoly3D );
    
}



void cmdSytralGrille()
{
    // Selectionner les blocs grille
    print( "Selectionner les blocs : " );
    ads_name ssblock;
    long lnblock = getSsBlock( ssblock, "", "E_P07" );
    
    if( lnblock == 0 )
    {
        acedSSFree( ssblock );
        return;
    }
    
    // REcuperer les points topo
    ads_name ssptTopo;
    print( "Sélectionner les points topo:" );
    long lnPtopo = getSsBlock( ssptTopo, "TOPO_Points_Surplus_Piqués,TOPO_Points_Piqués" );
    
    if( lnPtopo == 0 )
    {
        acedSSFree( ssptTopo );
        acedSSFree( ssblock );
        return;
    }
    
    
    createLayer( "GRILLE_ERREUR" );
    
    long nbErreur = 0;
    
    if( eOk != sytralGrille( nbErreur, ssblock, ssptTopo ) )
    {
        print( "Commande annulée." );
        acedSSFree( ssptTopo );
        acedSSFree( ssblock );
        return;
    }
    
    
    print( "Nombre d'erreurs trouvées: %d", nbErreur );
    acedSSFree( ssptTopo );
    acedSSFree( ssblock );
}


void cmdSytralAvaloir()
{
    // Selectionner les blocs avaloirs
    print( "Sélectionner les blocs avaloir:" );
    ads_name ssblock, sstopo;
    long lnblock = getSsBlock( ssblock, "", "E_P04" );
    
    if( lnblock == 0 )
    {
        acedSSFree( ssblock );
        return;
    }
    
    
    // Selectionner les blocs points topo
    print( "Sélectionner les points topo:" );
    long lntopo = getSsBlock( sstopo, "TOPO_Points_Surplus_Piqués,TOPO_Points_Piqués" );
    
    if( lntopo == 0 )
    {
        acedSSFree( sstopo );
        acedSSFree( ssblock );
        return;
    }
    
    // Création de calque
    if( eOk != createLayer( "AVALOIR_ERREUR" ) )
        print( "Erreur lors de la création du calque AVALOIR_ERREUR." );
        
    // Controler les blocs avaloir
    long nbErreur = 0;
    
    if( eOk != sytralAvaloir( nbErreur, ssblock, sstopo ) )
    {
        print( "Commande annulée." );
        acedSSFree( sstopo );
        acedSSFree( ssblock );
        return;
    }
    
    
    print( "Nombre d'erreurs trouvées: %d", nbErreur );
    
    // afficher le message d'erreur
    acedSSFree( sstopo );
    acedSSFree( ssblock );
}


void cmdSytralFileDeauZ()
{
    //Selection sur les polylignes de bordure
    ads_name ssPolyBordure;
    
    //Selection sur les polylignes de fil d'eau
    ads_name ssPolyFileEau;
    
    //Selection sur les polylignes de caniveau
    ads_name ssPolyCaniveau;
    
    //Recuperer les polylignes de bordure
    long bordureLength = getSsAllPoly3D( ssPolyBordure, "TOPO_Voirie_Habillages" );
    
    //Recuperer les polylignes de fil d'eau
    long fileDeauLength = getSsAllPoly3D( ssPolyFileEau, "TOPO_Voirie_Bordures" );
    
    //Recuperer les polylignes de caniveau
    long caniveauLength = getSsAllPoly3D( ssPolyCaniveau, "TOPO_Voirie_Caniveaux" );
    
    //Verifier
    if( bordureLength == 0 ||
        fileDeauLength == 0 ||
        caniveauLength == 0 )
    {
        //Afficher message
        print( "Impossible de récuperer les polylignes à verifier." );
        
        //Liberer les selections
        acedSSFree( ssPolyBordure );
        acedSSFree( ssPolyFileEau );
        acedSSFree( ssPolyCaniveau );
        
        //Sortir
        return;
    }
    
    //Creer le calque de patate
    createLayer( _T( "PATATE_SYTRALFILEDEAUZ" ) );
    
    //Compteur de patate
    long comp = 0;
    
    //Tableau qui va contenir les points où on va ajouter des patates
    AcGePoint3dArray patateArray;
    vector<double> xVecPatate;
    
    //Tableau qui va contenir tous les vertexs des fil d'eau
    vector<AcGePoint3d> fileDeauVec;
    
    //Polyligne 3d
    AcDb3dPolyline* polyFileDeau;
    AcDb3dPolyline* polyBordure;
    AcDb3dPolyline* polyCaniveau;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Préparation" ), fileDeauLength + bordureLength + caniveauLength );
    
    //Boucle sur les polylignes de fil d'eau et recuperer ses vertexs
    for( long f = 0; f < fileDeauLength; f++ )
    {
        //Recuperer la polyligne
        polyFileDeau = getPoly3DFromSs( ssPolyFileEau, f, AcDb::kForRead );
        
        //Verifier
        if( !polyFileDeau )
            continue;
            
        AcString lineType = polyFileDeau->linetype();
        
        //Si le type de ligne n'est pas celui du calque on continue
        if( lineType != _T( "ByLayer" ) )
        {
            polyFileDeau->close();
            continue;
        }
        
        //Recuperer les sommets de la polyligne
        AcGePoint3dArray fileDeauVtx;
        vector<double> xVec;
        getVertexesPoly( polyFileDeau, fileDeauVtx, xVec );
        
        //Fermer la polyligne
        polyFileDeau->close();
        
        long sizeTemp = fileDeauVtx.size();
        
        //Ajouter les sommets dans le tableau
        for( int t = 0; t < sizeTemp; t++ )
            fileDeauVec.emplace_back( fileDeauVtx[t] );
            
        prog.moveUp( f );
    }
    
    //Vecteur de polyligne de bordure
    vector<AcDb3dPolyline*> bordurePolyVec;
    
    //Vecteur de polyligne de caniveau
    vector<AcDb3dPolyline*> caniveauPolyVec;
    
    //Boucle sur la selection de polyligne de bordure
    for( long b = 0; b < bordureLength; b++ )
    {
        //Recuperer la polyligne
        polyBordure = getPoly3DFromSs( ssPolyBordure, b, AcDb::kForRead );
        
        //Verifier
        if( !polyBordure )
            continue;
            
        bordurePolyVec.emplace_back( polyBordure );
        
        prog.moveUp( fileDeauLength + b );
    }
    
    //Boucle sur la selection de polyligne de caniveau
    for( long c = 0; c < caniveauLength; c++ )
    {
        //Recuperer la polyligne
        polyCaniveau = getPoly3DFromSs( ssPolyCaniveau, c, AcDb::kForRead );
        
        //Verifier
        if( !polyCaniveau )
            continue;
            
        caniveauPolyVec.emplace_back( polyCaniveau );
        
        prog.moveUp( fileDeauLength + bordureLength + c );
    }
    
    //Recuperer la taille des vertex des file d'eau
    long lenFileDeau = fileDeauVec.size();
    
    //Tableau de patate
    AcGePoint3dArray bordurePatate0;
    AcGePoint3dArray bordurePatate1;
    AcGePoint3dArray bordurePatate2;
    AcGePoint3dArray caniveauPatate0;
    AcGePoint3dArray caniveauPatate1;
    AcGePoint3dArray caniveauPatate2;
    
    //Barre de progression
    ProgressBar progFile = ProgressBar( _T( "Contrôle" ), lenFileDeau );
    
    //Boucle sur les points
    for( int f = 0; f < lenFileDeau; f++ )
    {
        //Recuperer le vertex
        AcGePoint3d pt3d = fileDeauVec[f];
        
        //Boucle sur les polylignes de bordure
        for( long b = 0; b < bordureLength; b++ )
        {
            //Recuperer la polyligne
            polyBordure = bordurePolyVec[b];
            
            //Verifier
            if( !polyBordure )
                continue;
                
            //Verifier que le point est dans le boundingbox de la polyligne
            if( !isPointInsidePolyBox( polyBordure, pt3d ) )
                continue;
                
            //Point Projeter
            AcGePoint3d ptProj;
            
            //Projeter le point
            polyBordure->getClosestPointTo( pt3d, ptProj );
            
            //Verifier la distance en 2d
            if( getDistance2d( pt3d, ptProj ) > 2.0 )
                continue;
                
            //Sinon
            else
            {
                //Verifier le z
                if( ptProj.z < pt3d.z )
                {
                    //Ajouter le point dans le tableau
                    patateArray.push_back( pt3d );
                    xVecPatate.push_back( pt3d.x );
                }
            }
        }
        
        //Boucle sur les polylignes de caniveau
        for( long c = 0; c < caniveauLength; c++ )
        {
            //Recuperer la polyligne
            polyCaniveau = caniveauPolyVec[c];
            
            //Verifier
            if( !polyCaniveau )
                continue;
                
            //Verifier que le point est dans le boundingbox de la polyligne
            if( !isPointInsidePolyBox( polyCaniveau, pt3d ) )
                continue;
                
            //Point Projeter
            AcGePoint3d ptProj;
            
            //Projeter le point
            polyCaniveau->getClosestPointTo( pt3d, ptProj );
            
            //Verifier la distance en 2d
            if( getDistance2d( pt3d, ptProj ) > 2 )
                continue;
                
            //Sinon
            else
            {
                //Verifier le z
                if( ptProj.z < pt3d.z )
                {
                    //Ajouter le point dans le tableau
                    patateArray.push_back( pt3d );
                    xVecPatate.push_back( pt3d.x );
                }
            }
            
            //Fermer la polyligne
            polyCaniveau->close();
        }
        
        progFile.moveUp( f );
    }
    
    //Fermer les polylignes
    for( long b = 0; b < bordureLength; b++ )
        bordurePolyVec[b]->close();
        
    //Fermer les polylignes
    for( long c = 0; c < caniveauLength; c++ )
        caniveauPolyVec[c]->close();
        
    //Supprimer les doublons de points
    sortList( patateArray, xVecPatate );
    eraseDoublons( patateArray );
    
    //Recuperer la taille des patates
    long patateSz = patateArray.size();
    
    //Boucle pour tracer les patates
    for( long i = 0; i < patateSz; i++ )
        drawCircle( patateArray[i], 1, _T( "PATATE_SYTRALFILEDEAUZ" ) );
        
    //Afficher message
    print( "Nombre de patate ajoutés : %d", patateSz );
}


void cmdSytralAddBlocTopo()
{
    //Selection sur les polylignes 3d
    ads_name ssPoly;
    
    //Selection sur les blocs topos
    ads_name ssBlock;
    
    //Recuperer tous les polylignes 3d
    long lenPoly3d = getSsAllPoly3D( ssPoly );
    
    //Verifier
    if( lenPoly3d == 0 )
    {
        //Afficher message
        print( "Impossible de récuperer les polylignes." );
        
        return;
    }
    
    //Recuperer tous les blocs topos
    long lenBlockTopo = getSsAllBlock( ssBlock, _T( "" ), _T( "PTOPO" ) );
    
    //Verifier
    if( lenBlockTopo == 0 )
    {
        //Afficher message
        print( "Impossible de récuperer les blocks topos." );
        acedSSFree( ssPoly );
        
        return;
    }
    
    //Vecteur contenant les sommets des polylignes
    vector<AcGePoint3d> polyVtxVec;
    vector<double> polyVecX;
    
    //Vecteur contenant les sommets des blocs topos
    vector<AcGePoint3d> blocTopoVec;
    vector<double> blocVecX;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Préparation des données" ), lenPoly3d + lenBlockTopo );
    
    //Polyligne 3d
    AcDb3dPolyline* poly3d;
    
    //Boucle sur les polylignes
    for( long p = 0; p < lenPoly3d; p++ )
    {
        //Recuperer le p-eme polyligne
        poly3d = getPoly3DFromSs( ssPoly, p, AcDb::kForRead );
        
        //Verifier
        if( !poly3d )
            continue;
            
        //Tableau contenant les sommets de la polyligne
        AcGePoint3dArray ptArray;
        vector<double> xVec;
        
        //Recuperer les sommets de la polyligne
        getVertexesPoly( poly3d, ptArray, xVec );
        
        //Fermer la polyligne
        poly3d->close();
        
        //Taille des points
        long ptSize = ptArray.size();
        
        //Ajouter les points dans le vecteur
        for( long i = 0; i < ptSize; i++ )
        {
            polyVtxVec.emplace_back( ptArray[i] );
            polyVecX.emplace_back( ptArray[i].x );
        }
        
        //Progresser
        prog.moveUp( p );
    }
    
    //Reference de bloc
    AcDbBlockReference* block;
    
    //Boucle sur les blocs
    for( long b = 0; b < lenBlockTopo; b++ )
    {
        //Recuperer le b-eme bloc
        block = getBlockFromSs( ssBlock, b );
        
        //Verifier
        if( !block )
            continue;
            
        //Ajouter le point d'insertion dans le vecteur
        blocTopoVec.emplace_back( block->position() );
        blocVecX.emplace_back( block->position().x );
        
        //Fermer le bloc
        block->close();
        
        //Progresser
        prog.moveUp( lenPoly3d + b );
    }
    
    //Vider les selections
    acedSSFree( ssPoly );
    acedSSFree( ssBlock );
    
    //Trier les deux vecteurs
    sortList( polyVtxVec, polyVecX );
    sortList( blocTopoVec, blocVecX );
    
    //Recuperer la taille des vecteurs
    long polyVtxSize = polyVtxVec.size();
    long topoVecSize = blocTopoVec.size();
    
    //Compteur
    long comp = 0;
    
    //Barre de progression
    ProgressBar blockAdd = ProgressBar( _T( "Insertion des blocs topos." ), polyVtxSize );
    
    //Boucle sur les sommets de la polylignes
    for( long p = 0; p < polyVtxSize; p++ )
    {
        //Recuperer le p-eme point
        AcGePoint3d ptPoly = polyVtxVec[p];
        
        int idx = 0;
        
        //Tester si le point est sur une polyligne
        if( !isInList( ptPoly,
                blocVecX,
                blocTopoVec,
                idx ) )
        {
            //Ajouter le bloc
            AcDbBlockReference* blockTopoRef = insertBlockReference( _T( "PTOPO" ),
                    ptPoly );
                    
            //Verifier
            if( !blockTopoRef )
                continue;
                
            //Setter l'attribut du bloc
            setAttributValue( blockTopoRef, _T( "NUMERO" ), 1 );
            setAttributValue( blockTopoRef, _T( "ALTITUDE" ), ptPoly.z );
            
            //Setter le calque du block
            blockTopoRef->setLayer( _T( "TOPO_Points_Surplus_Piqués" ) );
            
            //Fermer le block
            blockTopoRef->close();
            
            //Compter
            comp++;
        }
        
        //Progresser
        blockAdd.moveUp( p );
    }
    
    //Afficher message
    print( "Nombre de bloc topos ajoutés : %d", comp );
}


void cmdSytralCheckAloneBlocTopo()
{
    //Selection sur les blocs topos
    ads_name ssBloc;
    
    //Selections sur les polylignes 3d
    ads_name ssPoly3d;
    
    //Selections sur les autres block
    ads_name ssAnotherBlock;
    
    //Vecteur contenant les noms des calques de bloc
    vector<AcString> layersVector;
    layersVector.emplace_back( _T( "TOPO_Points_Piqués" ) );
    layersVector.emplace_back( _T( "TOPO_Points_Surplus_Piqués" ) );
    layersVector.emplace_back( _T( "TOPO_Points_Seuil Soupiraux_Piqués" ) );
    layersVector.emplace_back( _T( "TOPO_Points_Seuil vitrine_Piqués" ) );
    layersVector.emplace_back( _T( "TOPO_Points_Seuil_Piqués" ) );
    layersVector.emplace_back( _T( "TOPO_Points_Sous-sol_Piqués" ) );
    
    //Recuperer les calques des blocs topos
    AcString blocTopoLayer = getLayer( layersVector, true );
    
    //Recuperer les calques des blocs qui ne sont pas des blocs topos
    AcString anotherBlockLayer = getLayer( layersVector, false );
    
    //Selection des blocs topo
    long lenBlocTopo = getSsAllBlock( ssBloc, blocTopoLayer, _T( "PTOPO" ) );
    
    //Selection des autres blocs
    long lenOtherBloc = getSsAllBlock( ssAnotherBlock, anotherBlockLayer );
    
    //Selection des polylignes
    long lenPoly3d = getSsAllPoly3D( ssPoly3d );
    
    //Calque de patate
    createLayer( _T( "PATATES_SYTRALTOPOSEUL" ) );
    
    //Vecteur de sommet de polyligne et de point d'insertion de bloc
    vector<AcGePoint3d> vecSommetPB;
    vector<double> vecxSommetPB;
    vector<AcDbBlockReference*>blockVec;
    
    //Vecteur de sommet des blocs topos
    vector<AcGePoint3d> vecSommetTopo;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Préparation" ), lenBlocTopo + lenOtherBloc + lenPoly3d );
    
    //Remplir le vecteur de point d'insertion des blocs topo
    AcDbBlockReference* block = NULL;
    
    for( long i = 0; i < lenBlocTopo; i++ )
    {
        block = getBlockFromSs( ssBloc, i );
        
        if( !block )
            continue;
            
        vecSommetTopo.emplace_back( block->position() );
        
        block->close();
        
        prog.moveUp( i );
    }
    
    //Recuperer les sommets des autres blocs
    for( long i = 0; i < lenOtherBloc; i++ )
    {
        block = getBlockFromSs( ssAnotherBlock, i );
        
        if( !block )
            continue;
            
        vecSommetPB.emplace_back( block->position() );
        vecxSommetPB.emplace_back( block->position().x );
        
        blockVec.emplace_back( block );
        
        prog.moveUp( lenBlocTopo + i );
    }
    
    //Recuperer les sommets des polylignes
    AcDb3dPolyline* poly = NULL;
    
    for( long i = 0; i < lenPoly3d; i++ )
    {
        poly = getPoly3DFromSs( ssPoly3d, i );
        
        if( !poly )
            continue;
            
        AcGePoint3dArray arrVtx;
        vector<double> xVtx;
        getVertexesPoly( poly, arrVtx, xVtx );
        
        for( long p = 0; p < arrVtx.size(); p++ )
            vecSommetPB.emplace_back( arrVtx[p] );
            
        for( long p = 0; p < xVtx.size(); p++ )
            vecxSommetPB.emplace_back( xVtx[p] );
            
        prog.moveUp( lenBlocTopo + lenOtherBloc + i );
        
        poly->close();
    }
    
    //Trier le vecteur
    sortList( vecSommetPB, vecxSommetPB );
    
    //Recuperer la taille du vecteur
    long sz = vecSommetPB.size();
    
    //Barre de progression
    long iterP = vecSommetTopo.size();
    ProgressBar prog1 = ProgressBar( _T( "Test Accrochage Polyligne" ), iterP );
    long it = 0;
    
    //Boucle sur les points topos
    for( long i = 0; i < vecSommetTopo.size(); i++ )
    {
        //Recuperer le point
        AcGePoint3d pt = vecSommetTopo[i];
        
        int idx = 0;
        
        //Tester si le point est dans la liste
        if( isInList( pt, vecxSommetPB, vecSommetPB, idx ) )
        {
            vecSommetTopo.erase( vecSommetTopo.begin() + i );
            i--;
        }
        
        prog1.moveUp( it++ );
    }
    
    ProgressBar prog2 = ProgressBar( _T( "Test Accrochage Bloc" ), vecSommetTopo.size() );
    it = 0;
    AcDbPoint* point = new AcDbPoint( AcGePoint3d::kOrigin );
    
    //Tester sur le reste des blocs sur l'intersection entre les
    for( long i = 0; i < vecSommetTopo.size(); i++ )
    {
        //Recuperer le point
        AcGePoint3d pt = vecSommetTopo[i];
        
        //Setter le point
        point->setPosition( pt );
        
        //Boucle sur les blocs
        for( long b = 0; b < lenOtherBloc; b++ )
        {
            //Si le point n"est pas dans le box du blox on le passe
            AcDbExtents blockExt;
            blockVec[b]->getGeomExtents( blockExt );
            
            if( !isPointInsideRect( pt, blockExt.minPoint(), blockExt.maxPoint() ) )
                continue;
                
            // On cherche si cette point intersecte avec la polyligne
            AcGePlane projectionPlane = AcGePlane( AcGePoint3d::kOrigin, AcGeVector3d::kZAxis );
            
            AcGePoint3dArray intersectArray;
            
            blockVec[b]->intersectWith( point, AcDb::kOnBothOperands,
                projectionPlane,
                intersectArray );
                
            //Si on a une intersection
            if( intersectArray.size() > 0 )
            {
                vecSommetTopo.erase( vecSommetTopo.begin() + i );
                i--;
                break;
            }
        }
        
        prog2.moveUp( it++ );
    }
    
    //Ajouter les patates
    iterP = vecSommetTopo.size();
    ProgressBar prog3 = ProgressBar( _T( "Ajout des patates" ), iterP );
    
    for( long i = 0; i < iterP; i++ )
    {
        drawCircle( vecSommetTopo[i], 1, _T( "PATATES_SYTRALTOPOSEUL" ) );
        prog3.moveUp( i );
    }
    
    //Fermer les blocks
    for( long i = 0; i < lenOtherBloc; i++ )
        blockVec[i]->close();
        
    //Afficher message
    print( "Nombre de patate ajoutés : %d", iterP );
    
    //Liberer les selections
    acedSSFree( ssBloc );
    acedSSFree( ssPoly3d );
    acedSSFree( ssAnotherBlock );
}



void cmdSytralUnrotate()
{
    //Declaration du pas de la matrice
    int pas = 1;
    
    //Declaration de la matrice
    AcDbExtents matrice = AcDbExtents( AcGePoint3d( DBL_MAX, DBL_MAX, .0 ), AcGePoint3d( DBL_MIN, DBL_MIN, .0 ) );
    
    struct infosBlock
    {
        int i;
        vector<int> iTopo;
        inline bool operator == ( infosBlock p )
        {
            if( i == p.i )
                return true;
                
            else return false;
        }
        infosBlock& operator = ( const infosBlock& p )
        {
            i = p.i;
            iTopo = p.iTopo;
            return *this;
        }
    };
    struct bInfos
    {
        AcGePoint3d position;
        AcGeScale3d scale;
        double rotation;
        inline bool operator == ( bInfos p )
        {
            if( isEqual2d( position, p.position ) )
                return true;
                
            else return false;
        }
    };
    
    //Declarer les selections
    ads_name ssBlock;
    ads_name ssselection, sstemp;
    
    //Creer le calqlue si n'existes pas
    if( !isLayerExisting( _T( "TOPO_Points_Surplus_Piqués" ) ) )
        createLayer( _T( "TOPO_Points_Surplus_Piqués" ) );
        
    if( !isLayerExisting( _T( "TOPO_Points_Surplus_Matricules" ) ) )
        createLayer( _T( "TOPO_Points_Surplus_Matricules" ) );
        
    if( !isLayerExisting( _T( "TOPO_Points_Surplus_Altitudes" ) ) )
        createLayer( _T( "TOPO_Points_Surplus_Altitudes" ) );
        
    //Selectionner les points topos
    print( "Selectionner le block a modifier" );
    int blockNumber = getSsBlock( ssBlock, "", "PTT04,PTT05,PTT06,PTT14,E_U01,E_U02,E_U07,E_P01,E_P02,E_P07,ASS01,ASS02,ASS07,CAB01,CAB02,CAB03,EDF08,EDF14,ECL02,ECL10,GAZ03,PIP03,CH03,SIL06,SIL07,SIL09,SIL10,TCL02,AFF03,AFF04,AFF07,AFF09,AFF10,PTOPO" );
    
    //Listes des blocks cibles
    vector<AcString> vecblockTarget{ "PTT04", "PTT05", "PTT06", "PTT14", "E_U01", "E_U02", "E_U07", "E_P01", "E_P02", "E_P07", "ASS01", "ASS02", "ASS07", "CAB01", "CAB02"
        , "CAB03", "EDF08", "EDF14", "ECL02", "ECL10", "GAZ03", "PIP03", "CH03", "SIL06", "SIL07", "SIL09", "SIL10", "TCL02", "AFF03", "AFF04", "AFF07", "AFF09", "AFF10" };
        
        
    //Verifions que la selection n'est pas vde
    if( blockNumber < 1 )
    {
        print( "Commande annul" );
        acedSSFree( ssBlock );
        return;
    }
    
    //Recuperer le fichier de parametre
    AcString filePath = askForFilePath( true,
            _T( "dwg" ),
            _T( "Selectionner L'ancien fichier" ) );
            
    //Verifions que filepath n'est pas vide
    if( !isFileExisting( filePath ) )
    {
        print( "Commande annule" );
        acedSSFree( ssBlock );
        return;
    }
    
    // Base de donnes du fichier slectionn par le dessinateur
    AcDbDatabase* pDb = new AcDbDatabase( Adesk::kFalse );
    
    // On rcupre la base de donnes
    Acad::ErrorStatus es;
    
    if( es = pDb->readDwgFile( filePath ) )
        print( es );
        
    // Liste des object id de chaque entit
    AcDbObjectIdArray idEnt;
    
    //Importer les IDs de l' entit
    if( es = importEntity( idEnt, filePath, pDb ) )
    {
        print( es );
        return;
    }
    
    int length = idEnt.length();
    
    //Recuperer des informations des blocks
    map<AcString, vector<bInfos>> blockMap;
    
    //Recuperer la listes des poimts topos a supprimes
    vector<int> iptopoTodelete;
    
    //Boucler sur les entites de l'ancien fichiers
    ProgressBar prog = ProgressBar( _T( "Lecture de l'ancien fichier" ), length );
    
    for( int i = 0; i < length; i++ )
    {
        // Incrmentation de la progress bar
        prog.moveUp( i );
        AcDbEntity* entity;
        
        // On ouvre l'entit
        if( es = acdbOpenAcDbEntity( entity, idEnt[i], AcDb::kForRead ) )
        {
            print( es );
            continue;
        }
        
        //erifions que l'entite soit un block
        if( _T( "AcDbBlockReference" ) == entityType( entity ) )
        {
            double scale;
            AcDbBlockReference* blockref = AcDbBlockReference::cast( entity );
            AcString name;
            getBlockName( name, blockref );
            
            
            if( find( vecblockTarget.begin(), vecblockTarget.end(), name ) != vecblockTarget.end() )
            {
                bInfos b;
                b.position = blockref->position();
                b.scale = blockref->scaleFactors();
                b.rotation = blockref->rotation();
                
                if( blockMap.find( name ) != blockMap.end() )
                {
                    vector<bInfos>mapTemp = blockMap[name];
                    mapTemp.push_back( b );
                    blockMap[name] = mapTemp;
                }
                
                else
                {
                    vector<bInfos>mapTemp{ b };
                    blockMap.insert( { name, mapTemp } );
                }
                
            }
        };
        
        //Fermer les entites
        entity->close();
    }
    
    //Recherche de la matrice
    for( int i = 0; i < blockNumber; i++ )
    {
        AcDbExtents ext;
        AcDbBlockReference* bref = getBlockFromSs( ssBlock, i, AcDb::kForRead );
        bref->getGeomExtents( ext );
        
        if( ext.minPoint().x < matrice.minPoint().x )
            matrice = AcDbExtents( AcGePoint3d( ext.minPoint().x, matrice.minPoint().y, .0 ), matrice.maxPoint() );
            
        if( ext.minPoint().y < matrice.minPoint().y )
            matrice = AcDbExtents( AcGePoint3d( matrice.minPoint().x, ext.minPoint().y, .0 ), matrice.maxPoint() );
            
        if( ext.maxPoint().x > matrice.maxPoint().x )
            matrice = AcDbExtents( matrice.minPoint(), AcGePoint3d( ext.maxPoint().x, matrice.maxPoint().y, .0 ) );
            
        if( ext.maxPoint().y > matrice.maxPoint().y )
            matrice = AcDbExtents( matrice.minPoint(), AcGePoint3d( matrice.maxPoint().x, ext.maxPoint().y, .0 ) );
            
        bref->close();
    }
    
    //Mettre les bord de la matrice au multiple de 5
    AcGePoint3d ptExt1 = AcGePoint3d( int( matrice.minPoint().x ) - int( matrice.minPoint().x ) % pas,
            int( matrice.minPoint().y ) - int( matrice.minPoint().y ) % pas, .0 );
    AcGePoint3d ptExt2 = AcGePoint3d( int( matrice.maxPoint().x + 1 ) + pas - int( matrice.maxPoint().x + pas ) % pas,
            int( matrice.maxPoint().y ) + pas - int( matrice.maxPoint().y + pas ) % pas, .0 );
    matrice = AcDbExtents( ptExt1, ptExt2 );
    
    //Bien definir la taille de la matrice
    int taill1 = ( matrice.maxPoint().x - matrice.minPoint().x ) / pas;
    int taill2 = ( matrice.maxPoint().y - matrice.minPoint().y ) / pas;
    
    //Mettres les polylignes dans la matrice
    vector<vector<vector<AcDbObjectId>>> Matrice;
    Matrice.resize( taill1 );
    vector<vector<AcDbObjectId>> polyidList( taill2 );
    
    for( int iMt = 0; iMt < taill1; iMt++ )
        Matrice[iMt] = polyidList;
        
    //Remplir la matrice
    for( int i = 0; i < blockNumber; i++ )
    {
        AcDbExtents extnts;
        AcDbBlockReference* bref = getBlockFromSs( ssBlock, i, AcDb::kForRead );
        AcString name;
        getBlockName( name, bref );
        
        //verifier le nom
        if( _T( "PTOPO" ) != name )
        {
            bref->getGeomExtents( extnts );
            vector<AcDbObjectId> vecPpolyid;
            vecPpolyid.resize( 0 );
            vecPpolyid.shrink_to_fit();
            //Recuperer les case que se trouves le polyligne
            //pair<pair<int, int>, pair<int, int>> mcoord;
            int iMinX = int( ( extnts.minPoint().x - .01 - matrice.minPoint().x ) / pas );
            int iMinY = int( ( extnts.minPoint().y - .01 - matrice.minPoint().y ) / pas );
            int iMaxX = int( ( extnts.maxPoint().x + .01 - matrice.minPoint().x ) / pas );
            int iMaxY = int( ( extnts.maxPoint().y + .01 - matrice.minPoint().y ) / pas );
            
            //Si le point est exatement equal au box du dessin on recalcul le bounding
            if( iMinX < 0 ) iMinX = 0;
            
            if( iMinY < 0 ) iMinY = 0;
            
            if( iMaxX >= taill1 ) iMaxX = taill1 - 1;
            
            if( iMaxY >= taill2 ) iMaxY = taill2 - 1;
            
            //Ajouter le polylignes parmis les polignes de la case
            for( int iminx = iMinX; iminx <= iMaxX; iminx++ )
            {
                for( int iminy = iMinY; iminy <= iMaxY; iminy++ )
                {
                    if( Matrice[iminx][iminy].size() == 0 )
                    {
                        vecPpolyid.push_back( bref->id() );
                        Matrice[iminx][iminy] = vecPpolyid;
                        vecPpolyid.resize( 0 );
                        vecPpolyid.shrink_to_fit();
                    }
                    
                    else
                    {
                        vecPpolyid = Matrice[iminx][iminy];
                        vecPpolyid.push_back( bref->id() );
                        Matrice[iminx][iminy] = vecPpolyid;
                        vecPpolyid.resize( 0 );
                        vecPpolyid.shrink_to_fit();
                    }
                }
            }
        }
        
        bref->close();
    }
    
    //Nombre de block modifier
    int nbBlock = 0;
    vector<infosBlock> vecblockInfos;
    vector<int> iedited;
    
    //Boucler sur la selection
    ProgressBar prog1 = ProgressBar( _T( "Modification des blocks" ), blockNumber );
    
    for( int i = 0; i < blockNumber; i++ )
    {
        prog1.moveUp( i );
        AcDbBlockReference* bref = getBlockFromSs( ssBlock, i, AcDb::kForWrite );
        AcString name;
        getBlockName( name, bref );
        
        //verifier le nom
        if( _T( "PTOPO" ) == name )
        {
            //Reboucler sur les blocks point topo
            for( int j = 0; j < blockNumber; j++ )
            {
                if( j != i )
                {
                    AcDbBlockReference* bref1 = getBlockFromSs( ssBlock, j, AcDb::kForRead );
                    
                    if( bref1 )
                    {
                        AcString name1;
                        getBlockName( name1, bref1 );
                        
                        if( name1 != _T( "PTOPO" ) )
                        {
                            AcGePoint3dArray pA;
                            AcGePoint3d pinsert1 = bref->position();
                            
                            //Retrouvons le cellule du point
                            int iCageX = int( pinsert1.x - matrice.minPoint().x ) / pas;
                            int iCageY = int( pinsert1.y - matrice.minPoint().y ) / pas;
                            vector<AcDbObjectId> vcObjId = Matrice[iCageX][iCageY];
                            
                            if( find( vcObjId.begin(), vcObjId.end(), bref1->objectId() ) != vcObjId.end() )
                            {
                                AcGePoint3d pinsert2 = bref1->position();
                                
                                if( name1 != _T( "PTOPO" ) )
                                    bref1->intersectWith( bref, AcDb::kExtendBoth, AcGePlane::kXYPlane, pA );
                                    
                                if( pA.size() > 0 && !isEqual3d( pinsert1, pinsert2, .00 ) )
                                {
                                    infosBlock b;
                                    b.i = j;
                                    vector<infosBlock>::iterator it = find( vecblockInfos.begin(), vecblockInfos.end(), b );
                                    
                                    if( it == vecblockInfos.end() )
                                    {
                                        b.iTopo.push_back( i );
                                        vecblockInfos.push_back( b );
                                    }
                                    
                                    else
                                    {
                                        vector<int> itvec = it->iTopo;
                                        itvec.push_back( i );
                                        it->iTopo = itvec;
                                    }
                                    
                                    iptopoTodelete.push_back( i );
                                    bref1->close();
                                    break;
                                }
                            }
                        }
                        
                        bref1->close();
                    }
                }
            }
        }
        
        else if( find( vecblockTarget.begin(), vecblockTarget.end(), name ) != vecblockTarget.end() )
        {
            vector <bInfos> mapTemp = blockMap[name];
            bInfos b;
            b.position = bref->position();
            
            //Verifier le point d'insertion
            vector <bInfos>::iterator iter = find( mapTemp.begin(), mapTemp.end(), b );
            
            if( iter != mapTemp.end() )
            {
                bref->setRotation( iter->rotation );
                bref->setScaleFactors( iter->scale );
                bref->setNormal( AcGeVector3d( .0, .0, 1.0 ) );
                bref->setPosition( iter->position );
                
                //Dessiner et ajouter les valeurs de l'attribut du point topo
                AcDbBlockReference* pTopo = insertBlockReference( _T( "PTOPO" ), b.position );
                setAttributValue( pTopo, _T( "NUMERO" ), _T( "1" ) );
                setAttributValue( pTopo, _T( "ALTITUDE" ), numberToAcString( pTopo->position().z, 2 ) );
                pTopo->setLayer( _T( "TOPO_Points_Surplus_Piqués" ) );
                
                //Modifier le calques des attributs
                AcDbAttribute* attribute1 = getAttributObject( pTopo, _T( "NUMERO" ), AcDb::kForWrite );
                AcDbAttribute* attribute2 = getAttributObject( pTopo, _T( "ALTITUDE" ), AcDb::kForWrite );
                attribute1->setLayer( _T( "TOPO_Points_Surplus_Matricules" ) );
                attribute2->setLayer( _T( "TOPO_Points_Surplus_Altitudes" ) );
                
                acdbGetAdsName( sstemp, bref->objectId() );
                acedSSAdd( sstemp, ssselection, ssselection );
                acedSSFree( sstemp );
                attribute1->close();
                attribute2->close();
                pTopo->close();
                nbBlock++;
                iedited.push_back( i );
            }
        }
        
        bref->close();
    }
    
    //Boucler sur les blocks qui n'ont pas de corespondance
    vector<int> notTodelete, todelete;
    prog1 = ProgressBar( _T( "Calcul des blocks qui n'ont pas de reference" ), vecblockInfos.size() );
    int iB = 0;
    
    for( infosBlock b : vecblockInfos )
    {
        prog.moveUp( iB++ );
        vector<infosBlock>::iterator iterate = find( vecblockInfos.begin(), vecblockInfos.end(), b );
        vector<int>::iterator iiterator = find( iedited.begin(), iedited.end(), b.i );
        
        if( iterate != vecblockInfos.end() && iiterator == iedited.end() )
        {
            //b = *iterate;
            //Verifions que les points topos sont 3 ou 4
            if( iterate->iTopo.size() == 3 )
            {
                int itest = 0;
                //Verifions dans quelle axe la modification est elle fait
                AcGePoint3d pt = AcGePoint3d::kOrigin;
                pair<int, AcGePoint3d> solo, duo1, duo2;
                AcGePoint3d p1 = AcGePoint3d::kOrigin, p2 = AcGePoint3d::kOrigin, p3 = AcGePoint3d::kOrigin;
                
                AcDbBlockReference* bref1 = getBlockFromSs( ssBlock, iterate->iTopo[0], AcDb::kForWrite );
                AcDbBlockReference* bref2 = getBlockFromSs( ssBlock, iterate->iTopo[1], AcDb::kForWrite );
                AcDbBlockReference* bref3 = getBlockFromSs( ssBlock, iterate->iTopo[2], AcDb::kForWrite );
                p1 = bref1->position();
                p2 = bref2->position();
                p3 = bref3->position();
                
                if( p1.z < p2.z + 0.01 && p1.z > p2.z - .01 )
                {
                    solo = {iterate->iTopo[2], p3};
                    duo1 = {iterate->iTopo[0], p1};
                    duo2 = {iterate->iTopo[1], p2};
                }
                
                else if( p1.z < p3.z + 0.01 && p1.z > p3.z - .01 )
                {
                    solo = {iterate->iTopo[1], p2};
                    duo1 = {iterate->iTopo[2], p3};
                    duo2 = {iterate->iTopo[0], p1};
                }
                
                else
                {
                    solo = {iterate->iTopo[0], p1};
                    duo1 = {iterate->iTopo[1], p2};
                    duo2 = {iterate->iTopo[2], p3};
                    
                }
                
                bref1->close();
                bref2->close();
                bref3->close();
                
                //Chercher l'origine du plan
                AcGePoint3d orig, udir, vdir;
                
                if( abs( duo1.second.distanceTo( solo.second ) ) > abs( duo2.second.distanceTo( solo.second ) ) )
                {
                    orig = duo2.second;
                    udir = duo1.second;
                    vdir = solo.second;
                }
                
                else
                {
                    orig = duo1.second;
                    udir = duo2.second;
                    vdir = solo.second;
                }
                
                
                AcDbBlockReference* bref = getBlockFromSs( ssBlock, iterate->i, AcDb::kForWrite );
                double rt = bref->rotation();
                acdbGetAdsName( sstemp, bref->objectId() );
                acedSSAdd( sstemp, ssselection, ssselection );
                acedSSFree( sstemp );
                nbBlock++;
                
                //Dessiner et ajouter les valeurs de l'attribut du point topo
                AcGePoint3d positionP = bref->position();
                AcDbBlockReference* pTopo = insertBlockReference( _T( "PTOPO" ), bref->position() );
                setAttributValue( pTopo, _T( "NUMERO" ), _T( "1" ) );
                setAttributValue( pTopo, _T( "ALTITUDE" ), numberToAcString( pTopo->position().z, 2 ) );
                pTopo->setLayer( _T( "TOPO_Points_Surplus_Piqués" ) );
                
                //Modifier le calques des attributs
                AcDbAttribute* attribute1 = getAttributObject( pTopo, _T( "NUMERO" ), AcDb::kForWrite );
                AcDbAttribute* attribute2 = getAttributObject( pTopo, _T( "ALTITUDE" ), AcDb::kForWrite );
                attribute1->setLayer( _T( "TOPO_Points_Surplus_Matricules" ) );
                attribute2->setLayer( _T( "TOPO_Points_Surplus_Altitudes" ) );
                
                acdbGetAdsName( sstemp, bref->objectId() );
                acedSSAdd( sstemp, ssselection, ssselection );
                acedSSFree( sstemp );
                attribute1->close();
                attribute2->close();
                pTopo->close();
                
                
                
                AcGeScale3d scale = bref->scaleFactors();
                
                if( scale.sx < abs( orig.distanceTo( vdir ) ) + .01 && scale.sx > abs( orig.distanceTo( vdir ) ) - .01 )
                {
                    scale.sx = scale.sx * getDistance2d( orig, vdir ) / orig.distanceTo( vdir );
                    rt = getVector2d( vdir, orig ).angleTo( AcGeVector2d::kXAxis );
                }
                
                else if( scale.sy < abs( orig.distanceTo( vdir ) ) + .01 && scale.sy > abs( orig.distanceTo( vdir ) ) - .01 )
                {
                    scale.sy = scale.sy * getDistance2d( orig, vdir ) / orig.distanceTo( vdir );
                    rt = getVector2d( vdir, orig ).angleTo( AcGeVector2d::kYAxis );
                }
                
                bref->setScaleFactors( scale );
                bref->setNormal( AcGeVector3d( .0, .0, 1 ) );
                bref->setPosition( positionP );
                bref->setRotation( rt );
                bref->close();
                todelete.insert( todelete.end(), b.iTopo.begin(), b.iTopo.end() );
            }
            
            else
                notTodelete.insert( notTodelete.end(), b.iTopo.begin(), b.iTopo.end() );
        }
        
    }
    
    //Supressions des blocks su point toposde l'ancien dessin
    prog = ProgressBar( _T( "Supprimer les points topos accroche au block" ), iedited.size() );
    int ierasedPtopo = 0;
    vector<int> ideleded;
    
    for( int i : iedited )
    {
        ierasedPtopo++;
        infosBlock b;
        b.i = i;
        vector<infosBlock>::iterator it = find( vecblockInfos.begin(), vecblockInfos.end(), b );
        
        if( it != vecblockInfos.end() )
        {
            vector<int> itodel = it->iTopo;
            todelete.insert( todelete.end(), itodel.begin(), itodel.end() );
        }
    }
    
    for( int i : todelete )
    {
        if( find( ideleded.begin(), ideleded.end(), i ) == ideleded.end() )
        {
            AcDbBlockReference* ptopo = getBlockFromSs( ssBlock, i, AcDb::kForWrite );
            ptopo->erase();
            ptopo->close();
            ideleded.push_back( i );
        };
    }
    
    //Modification des blocks ajoutes
    acedSSSetFirst( ssselection, ssselection );
    //Nombre de block modifie
    print( "%i : block modifie", nbBlock );
    
}

void cmdSytralTopoAlt()
{
    //Selection sur les blocs topos
    ads_name ssBlock;
    
    //Selection sur les autres blocs
    ads_name ssOtherBlock;
    
    //Recuperer les blocs topo
    long lenBlocTopo = getSsAllBlock( ssBlock, _T( "" ), _T( "PTOPO" ) );
    
    //Verifier
    if( lenBlocTopo == 0 )
    {
        //Afficher message
        print( "Aucun bloc topo dans le dessin" );
        
        return;
    }
    
    //Recuperer tous les noms des blocs
    vector<AcString> vecBlocName = getBlockList();
    
    //Nom du bloc
    AcString blockNameSs;
    
    //Boucle sur le vecteur de nom
    for( int i = 0; i < vecBlocName.size(); i++ )
    {
        //Verifier
        if( vecBlocName[i] == _T( "PTOPO" ) ||
            vecBlocName[i] == _T( "PTT99" ) ||
            vecBlocName[i] == _T( "ECL99" ) ||
            vecBlocName[i] == _T( "EDF99" ) ||
            vecBlocName[i] == _T( "GAZ99" ) ||
            vecBlocName[i] == _T( "PIP99" ) ||
            vecBlocName[i] == _T( "SIL99" ) ||
            vecBlocName[i] == _T( "CH99" ) ||
            vecBlocName[i] == _T( "CAB99" ) ||
            vecBlocName[i] == _T( "E_P97" ) ||
            vecBlocName[i] == _T( "EAU99" ) ||
            vecBlocName[i] == _T( "EAU98" ) ||
            vecBlocName[i] == _T( "E_U96" ) ||
            vecBlocName[i] == _T( "ASS99" ) ||
            vecBlocName[i] == _T( "TCL99" ) ||
            vecBlocName[i] == _T( "ESC01" ) )
        {
            vecBlocName.erase( vecBlocName.begin() + i );
            i--;
            continue;
        }
        
        blockNameSs.append( vecBlocName[i] );
        
        if( i != vecBlocName.size() - 1 )
            blockNameSs.append( _T( "," ) );
    }
    
    //Recuperer les autres blocs
    long lenOtherBloc = getSsAllBlock( ssOtherBlock, _T( "" ), blockNameSs );
    
    //Vecteur qui va contenir les points
    vector<AcGePoint3d> vecTopo;
    vector<double> xVecTopo;
    vector<AcGePoint3d> vecOtherBlock;
    vector<double> xVecOtherBlock;
    
    //Reference de block
    AcDbBlockReference* block = NULL;
    
    //Barre de progression
    ProgressBar progPrepa = ProgressBar( _T( "Préparation" ), lenBlocTopo + lenOtherBloc );
    
    //Remplir les vecteurs
    for( long i = 0; i < lenBlocTopo; i++ )
    {
        //Recuperer le ieme block
        block = getBlockFromSs( ssBlock, i );
        
        //Verifier
        if( !block )
            continue;
            
        //Ajouter le point d'insertion du blocs dans le vecteur
        vecTopo.emplace_back( block->position() );
        xVecTopo.emplace_back( block->position().x );
        
        //Fermer le bloc
        block->close();
        
        //Progresser
        progPrepa.moveUp( i );
    }
    
    AcDbBlockReference* otherBlock = NULL;
    
    for( long i = 0; i < lenOtherBloc; i++ )
    {
        //Recuperer le ieme block
        otherBlock = getBlockFromSs( ssOtherBlock, i );
        
        //Verifier
        if( !otherBlock )
            continue;
            
        //Ajouter le point d'insertion du bloc dans le vecteur
        vecOtherBlock.emplace_back( otherBlock->position() );
        xVecOtherBlock.emplace_back( otherBlock->position().x );
        
        //Fermer le bloc
        otherBlock->close();
        
        //Progresser
        progPrepa.moveUp( lenBlocTopo + i );
    }
    
    //Liberer la selection
    acedSSFree( ssBlock );
    acedSSFree( ssOtherBlock );
    
    //Creer le calque de patate
    createLayer( _T( "PATATE_SYTRALTOPOALT" ) );
    
    //Trier les vecteurs
    sortList( vecTopo, xVecTopo );
    sortList( vecOtherBlock, xVecOtherBlock );
    
    //Recuperer la taille des vecteurs
    long tSz = vecTopo.size();
    long tOSz = vecOtherBlock.size();
    
    //Compteur
    long compt = 0;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), tOSz );
    
    //Boucle sur le vecteur des autres blocs
    for( long i = 0; i < tOSz; i++ )
    {
        //Recuperer les points
        AcGePoint3d pt = vecOtherBlock[i];
        
        int idx = 0;
        
        //Verifier si le point est dans la liste
        if( !isInList( pt, xVecTopo, vecTopo, idx ) )
        {
            //Ajouter une patate
            drawCircle( pt, 1, _T( "PATATE_SYTRALTOPOALT" ) );
            
            //Compter
            compt++;
        }
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Afficher message
    print( "Nombre de patate ajoutés : %d", compt );
}

void cmdLevelUpBlockText()
{
    //Declarer les selections
    ads_name ssblock, ssblockText;
    
    //Demander a l'utilisateur de selectionner les blocks
    int blockNumber = getSsBlock( ssblock, "", "ASS01,ASS02,ASS03,E_UNIT05,ASS04,ASS07,ASS09,ASS10,CAB01,CAB02,CAB03,CH01,CH02,CH03,E_AR01,E_AR02,E_P01,E_P02,E_P03,E_P04,E_P05,E_U01,E_U02,E_U03,E_U04,E_U07,E_U09,E_U10,EAU04,EAU01,EAU05,EAU08,EAU09,ECL02,ECL10,ECL11,ECL12,EDF08,EDF10,EDF11,EDF13,EDF14,EDF15,GAZ01,GAZ03,GAZ04,GAZ06,GAZ07,PIP01,PIP02,PIP03,PTT04,PTT05,PTT06,PTT07,PTT09,PTT14,SIL05,SIL06,SIL07,SIL09,SIL10,TCL02,CAB04,CAB05,E_P06,E_P07,E_P09,E_P10,ECL01,ECL03,ECL04,ECL05,ECL07,ECL13,ECL01,ECL03,ECL04,ECL05,ECL07,ECL13,EDF01,EDF02,EDF03,EDF04,EDF12,PTT01,PTT04,PTT12,PTT13,SIL01,SIL02,SIL03,SIL04,SIL11,TCL02,TCL03,TCL04,TCL05,TCL06,TCL07,TCL08" );
    int blockTextNumer = getSsBlock( ssblockText, "", "ASS99,CAB99,CH99,E_AR99,E_P97,E_U96,EAU98,EAU99,ECL99,EDF99,GAZ99,PIP99,PTT99,SIL99,TCL99" );
    map<AcString, vector<AcString>> mapRef
    {
        {
            _T( "ASS99" ),
            {
                _T( "ASS01" ),
                _T( "ASS02" ),
                _T( "ASS03" ),
                _T( "E_UNIT05" ),
                _T( "ASS04" ),
                _T( "ASS07" ),
                _T( "ASS09" ),
                _T( "ASS10" )
            }
        },
        
        {
            _T( "CAB99" ),
            {
                _T( "CAB01" ),
                _T( "CAB02" ),
                _T( "CAB03" ),
                _T( "CAB04" ),
                _T( "CAB05" )
            }
        },
        
        {
            _T( "CH99" ),
            {
                _T( "CH01" ),
                _T( "CH02" ),
                _T( "CH03" )
            }
        },
        
        {
            _T( "E_AR99" ),
            {
                _T( "E_AR01" ),
                _T( "E_AR02" )
            }
        },
        
        {
            _T( "E_P97" ),
            {
                _T( "E_P01" ),
                _T( "E_P02" ),
                _T( "E_P03" ),
                _T( "E_P04" ),
                _T( "E_P05" ),
                _T( "E_P06" ),
                _T( "E_P07" ),
                _T( "E_P08" ),
                _T( "E_P09" ),
                _T( "E_P10" )
            }
        },
        
        {
            _T( "E_U96" ),
            {
                _T( "E_U01" ),
                _T( "E_U02" ),
                _T( "E_U03" ),
                _T( "E_U04" ),
                _T( "E_U07" ),
                _T( "E_U09" ),
                _T( "E_U10" )
            }
        },
        
        {_T( "EAU98" ), {_T( "EAU04" )}},
        
        {
            _T( "EAU99" ),
            {
                _T( "EAU01" ),
                _T( "EAU05" ),
                _T( "EAU08" ),
                _T( "EAU09" )
            }
        },
        {
            _T( "ECL99" ),
            {
                _T( "ECL01" ),
                _T( "ECL02" ),
                _T( "ECL03" ),
                _T( "ECL04" ),
                _T( "ECL05" ),
                _T( "ECL07" ),
                _T( "ECL10" ),
                _T( "ECL11" ),
                _T( "ECL12" ),
                _T( "ECL13" )
            }
        },
        {
            _T( "EDF99" ),
            {
                _T( "EDF01" ),
                _T( "EDF02" ),
                _T( "EDF03" ),
                _T( "EDF04" ),
                _T( "EDF08" ),
                _T( "EDF10" ),
                _T( "EDF11" ),
                _T( "EDF12" ),
                _T( "EDF13" ),
                _T( "EDF14" ),
                _T( "EDF15" )
            }
        },
        {
            _T( "GAZ99" ),
            {
                _T( "GAZ01" ),
                _T( "GAZ03" ),
                _T( "GAZ04" ),
                _T( "GAZ06" ),
                _T( "GAZ07" )
            }
        },
        
        {
            _T( "PIP99" ),
            {
                _T( "PIP01" ),
                _T( "PIP02" ),
                _T( "PIP03" )
            }
        },
        
        {
            _T( "PTT99" ),
            {
                _T( "PTT01" ),
                _T( "PTT04" ),
                _T( "PTT05" ),
                _T( "PTT06" ),
                _T( "PTT07" ),
                _T( "PTT09" ),
                _T( "PTT12" ),
                _T( "PTT13" ),
                _T( "PTT14" )
            }
        },
        
        {
            _T( "SIL99" ),
            {
                _T( "SIL01" ),
                _T( "SIL02" ),
                _T( "SIL03" ),
                _T( "SIL04" ),
                _T( "SIL05" ),
                _T( "SIL06" ),
                _T( "SIL07" ),
                _T( "SIL09" ),
                _T( "SIL10" ),
                _T( "SIL11" )
            }
        },
        
        {_T( "TCL99" ), {_T( "TCL01" ), _T( "TCL02" ), _T( "TCL03" ), _T( "TCL04" ), _T( "TCL05" ), _T( "TCL06" ), _T( "TCL07" ), _T( "TCL08" ), }}
    };
    int nbErBlockTxt = 0;
    
    if( blockNumber < 1 )
    {
        print( "Commande annulé" );
        return;
    }
    
    if( blockTextNumer < 1 )
    {
        print( "Commande annulé" );
        return;
    }
    
    //boucler sur les blocks texts
    for( int i = 0; i < blockTextNumer; i++ )
    {
        //Creer un block reference
        AcDbBlockReference* bTexte = getBlockFromSs( ssblockText, i, AcDb::kForWrite );
        double dist = DBL_MAX;
        double z = 0;
        AcString btName;
        getBlockName( btName, bTexte );
        
        AcGePoint3d btxtPoint = bTexte->position();
        
        //Verifions que le nom du block texte est dans la listes
        if( mapRef.find( btName ) != mapRef.end() )
        {
            //Recuperer la listes des nom du block a verifiers
            vector<AcString> vecRef = mapRef[btName];
            
            for( int j = 0; j < blockNumber; j++ )
            {
                //Recuperer les blocks qui ont de z
                AcDbBlockReference* bref = getBlockFromSs( ssblock, j, AcDb::kForRead );
                
                //Recuperer le nom du block
                AcString bName;
                getBlockName( bName, bref );
                
                //Si le block est dans la listse
                if( find( vecRef.begin(), vecRef.end(), bName ) != vecRef.end() )
                {
                    AcGePoint3d bPosition = bref->position();
                    //Recuperer la distance entre les points d'insertions
                    double dist2d = getDistance2d( btxtPoint, bPosition );
                    
                    if( dist2d < dist && dist2d < 1.0 )
                    {
                        dist = dist2d;
                        z = bPosition.z;
                    }
                }
                
                //Fermer les blocks
                bref->close();
            }
        }
        
        if( dist < 1.0 )
        {
            btxtPoint.z = z;
            bTexte->setPosition( btxtPoint );
        }
        
        else
        {
            drawCircle( btxtPoint, .05 );
            nbErBlockTxt++;
        }
        
        //Fermer les blocks
        bTexte->close();
    }
    
    print( "%i Blocktext modifié", blockTextNumer - nbErBlockTxt );
    print( "%i Blocktext a pas de correspondance", nbErBlockTxt );
    acedSSFree( ssblock );
    acedSSFree( ssblockText );
}


void cmdSytralAccrocheTopo()
{
    //Selection sur les polylignes 3d
    ads_name ssPoly;
    
    //Selection sur les blocs topos
    ads_name ssBlockTopo;
    
    //Demander de selectionner les polylignes
    print( "Selectionner les polylignes 3d." );
    long lenPoly3d = getSsPoly3D( ssPoly );
    
    //Verifier
    if( lenPoly3d == 0 )
    {
        //Afficher message
        print( "Selection vide." );
        
        return;
    }
    
    //Demander de selectionner les blocs
    print( "Selectionner les blocs Topo." );
    long lenBlockTopo = getSsBlock( ssBlockTopo,
            _T( "" ),
            _T( "PTOPO" ) );
            
    //Verifier
    if( lenBlockTopo == 0 )
    {
        //Afficher message
        print( "Selection de block vide." );
        
        //Liberer la selection
        acedSSFree( ssPoly );
        
        return;
    }
    
    //Demander la tolérance de recherche
    double tol = 0.001;
    
    if( RTCAN == askForDouble( _T( "Entrer la tolérance de recherche : " ), tol, tol ) )
    {
        //LIberer les selections
        acedSSFree( ssPoly );
        acedSSFree( ssBlockTopo );
        
        //Afficher message
        print( "Commande annulée" );
        
        //Sortir
        return;
    }
    
    //Compteur de bloc
    long comp = 0;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), lenBlockTopo );
    
    //Reference de block
    AcDbBlockReference* block = NULL;
    
    //Polyligne
    AcDb3dPolyline* poly = NULL;
    
    //Extents de polyligne
    AcDbExtents extPoly;
    
    //Boucle sur les blocs topos
    for( long i = 0; i < lenBlockTopo; i++ )
    {
        //Recuperer le bloc
        block = getBlockFromSs( ssBlockTopo, i );
        
        //Verifier
        if( !block )
            continue;
            
        //Boucle sur les polylignes
        for( long p = 0; p < lenPoly3d; p++ )
        {
            //Recuperer la polyligne
            poly = getPoly3DFromSs( ssPoly, p );
            
            //Verifier
            if( !poly )
                continue;
                
            //Recuperer le box de la polyligne
            poly->getGeomExtents( extPoly );
            
            //Verifier si le point est dans le contour de la polyligne
            if( !isInBoundingBox( extPoly, block->position() ) )
            {
                //Fermer la polyligne
                poly->close();
                
                continue;
            }
            
            //Projeté le point sur la polyligne
            AcGePoint3d ptProj;
            poly->getClosestPointTo( block->position(),
                ptProj );
                
            //Fermer la polyligne
            poly->close();
            
            //Verifier si on a le meme point en X Y
            if( isTheSame( block->position().x, ptProj.x, tol ) &&
                isTheSame( block->position().y, ptProj.y, tol ) )
            {
                //On met en ecriture le bloc
                block->upgradeOpen();
                
                //Setter la position du bloc
                block->setPosition( ptProj );
                
                //Compter
                comp++;
                
                //Sortir de la boucle
                break;
            }
        }
        
        //Fermer le bloc
        block->close();
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Liberer les selections
    acedSSFree( ssPoly );
    acedSSFree( ssBlockTopo );
    
    //Afficher message
    print( "Nombre de bloc repositionné : %d", comp );
}


void cmdArroundPolys()
{
    map<AcDbObjectId, vector<AcGePoint3d>> mapPoly;
    
    ads_name sspoly2d;
    ads_name sspoly3d;
    
    int poly2dsize = getSsAllPoly2D( sspoly2d );
    int poly3dsize = getSsAllPoly3D( sspoly3d );
    
    
    ProgressBar prog = ProgressBar( _T( "Informations polylines 2d:" ), poly2dsize );
    
    //boucler sur les polylines
    for( auto i = 0; i < poly2dsize; i++ )
    {
        prog.moveUp( i++ );
        
        //progresser
        //prog.moveUp( i++ );
        
        vector<AcGePoint3d> vecPoints;
        
        //caster l'entité
        AcDbPolyline* poly = getPoly2DFromSs( sspoly2d, i );
        
        //verifier poly
        if( !poly )
            continue;
            
        //Boucler sur les sommets
        int nbVert = poly->numVerts();
        
        for( int k = 0; k < nbVert; k++ )
        {
            AcGePoint3d p;
            poly->getPointAt( k, p );
            AcGePoint3d temp;
            temp.x = std::round( p.x );
            temp.y = std::round( p.y );
            temp.z = 0.0;
            
            //Inserer les sommets dans pointList
            vecPoints.push_back( temp );
            //poly->removeVertexAt( k );
            
        }
        
        //recuperer les informations
        mapPoly.insert( make_pair( poly->id(), vecPoints ) );
        
        vecPoints.clear();
        
        
        //liberer la mémoire
        poly->close();
    }
    
    
    prog = ProgressBar( _T( "Informations polylines 3d:" ), poly3dsize );
    
    for( auto i = 0; i < poly3dsize; i++ )
    {
        prog.moveUp( i++ );
        
        vector<AcGePoint3d> vecPoints;
        
        //caster l'entité
        AcDb3dPolyline* poly = getPoly3DFromSs( sspoly3d, i, AcDb::kForWrite );
        
        //verifier poly
        if( !poly )
            continue;
            
        //Creer un itérateur pour les sommets de la polyligne 3D
        AcDb3dPolylineVertex* vertex = NULL;
        AcDbObjectIterator* iterPoly3D = poly->vertexIterator();
        
        //Recuperer les sommets du polylignes
        for( iterPoly3D->start(); !iterPoly3D->done(); iterPoly3D->step() )
        {
            //Recuperer le vertex puis le sommet
            if( Acad::eOk == poly->openVertex( vertex, iterPoly3D->objectId(), AcDb::kForRead ) )
            {
                AcGePoint3d temp;
                temp.x = std::round( vertex->position().x );
                temp.y = std::round( vertex->position().y );
                temp.z = std::round( vertex->position().z );
                //Stocker le sommet dans vecPt
                vecPoints.push_back( temp );
                
                //Liberer la mémoire
                vertex->close();
            }
        }
        
        
        //recuperer les informations
        mapPoly.insert( make_pair( poly->id(), vecPoints ) );
        
        vecPoints.clear();
        
        //liberer la mémoire
        poly->close();
    }
    
    prog = ProgressBar( _T( "Rearanger les polylines : " ), mapPoly.size() );
    int count = 0;
    
    //Boucle sur les polylignes
    for( auto& ptPoly : mapPoly )
    {
        //progresser
        prog.moveUp( count );
        count++;
        
        AcDbEntity* ent;
        acdbOpenAcDbEntity( ent, ptPoly.first, AcDb::kForWrite );
        
        //Verifier ent
        if( !ent )
            continue;
            
            
        if( ent->isKindOf( AcDb2dPolyline::desc() ) ||
            ent->isKindOf( AcDbPolyline::desc() ) )
        {
            //Caster l'entité en polyline
            AcDbPolyline* poly = AcDbPolyline::cast( ent );
            
            //Verifier poly3d
            if( !poly )
                continue;
                
            int nbverts = poly->numVerts();
            
            AcDbPolyline* newPoly = new AcDbPolyline( nbverts );
            newPoly->setPropertiesFrom( poly );
            
            bool setclose = false;
            
            if( isClosed( poly ) )
                setclose = true;
                
            poly->upgradeOpen();
            poly->erase();
            poly->close();
            
            int vert = 0;
            
            //Reajuster les sommets
            for( AcGePoint3d pt : ptPoly.second )
            {
                newPoly->addVertexAt( vert, AcGePoint2d( pt.x, pt.y ) );
                vert++;
            }
            
            if( setclose )
                newPoly->setClosed( true );
                
            addToModelSpace( newPoly );
            newPoly->close();
            
        }
        
        else
        {
            //Caster l'entité en polyline
            AcDb3dPolyline* poly3d = AcDb3dPolyline::cast( ent );
            
            //Verifier poly3d
            if( !poly3d )
                continue;
                
            //Creer un itérateur pour les sommets de la polyligne 3D
            AcDb3dPolylineVertex* vertex = NULL;
            AcDbObjectIterator* iterPoly3D = poly3d->vertexIterator();
            
            //Supprimer les sommets
            for( iterPoly3D->start(); !iterPoly3D->done(); iterPoly3D->step() )
            {
                //Recuperer le vertex puis le sommet
                if( Acad::eOk == poly3d->openVertex( vertex, iterPoly3D->objectId(), AcDb::kForWrite ) )
                {
                    vertex->erase();
                    vertex->close();
                }
            }
            
            //Reajuster les sommets
            for( AcGePoint3d pt : ptPoly.second )
            {
                AcDb3dPolylineVertex* vert = new AcDb3dPolylineVertex( pt );
                poly3d->appendVertex( vert );
                vert->close();
            }
            
            if( vertex )vertex->close();
            
            poly3d->close();
            
            delete iterPoly3D;
        }
        
        
    }
    
    //LIberer les selections
    acedSSFree( sspoly2d );
    acedSSFree( sspoly3d );
}


void cmdIGNExportCenter()
{
    // Recuperer tous les cercles dans le dessin
    ads_name sscircle;
    long lncircle = getSsAllCircle( sscircle );
    
    if( lncircle == 0 )
    {
        acedSSFree( sscircle );
        print( "Commande annulée." );
        return;
    }
    
    // AcString file = "";
    AcString file = askForFilePath( false, _T( "txt" ), _T( "Enregistrer sous" ), getCurrentFileFolder() );
    
    if( file.compare( "" ) == 0 )
        file = getCurrentFileFolder() + "//IGNExportCenter.txt";
        
    ofstream fileExport( acStrToStr( file ) );
    
    for( int i = 0; i < lncircle; i++ )
    {
        AcDbCircle* circle = getCircleFromSs( sscircle, i );
        
        if( !circle )
            continue;
            
        fileExport << circle->center().x << "\t" << circle->center().y << "\t" << circle->center().z << endl;
        
        
        circle->close();
    }
    
    print( "Nombre de cercle(s) exporté(s) %d", lncircle );
    
    fileExport.close();
    acedSSFree( sscircle );
}

void cmdBimGenererFb()
{
    //Creer le calque du patate si n'existe pas
    if( !isLayerExisting( _T( "Erreur_Z" ) ) ) createLayer( _T( "Erreur_Z" ) );
    
    //Declarer les selections
    ads_name ssblock, ssblockText;
    
    //Demander a l'utilisateur de selectionner les blocks
    int blockNumber = getSsBlock( ssblock, "", "ASS01,ASS02,ASS03,E_UNIT05,ASS04,ASS07,ASS09,ASS10,CAB01,CAB02,CAB03,CH01,CH02,CH03,E_AR01,E_AR02,E_P01,E_P02,E_P03,E_P04,E_P05,E_U01,E_U02,E_U03,E_U04,E_U07,E_U09,E_U10,EAU04,EAU01,EAU05,EAU08,EAU09,ECL02,ECL10,ECL11,ECL12,EDF08,EDF10,EDF11,EDF13,EDF14,EDF15,GAZ01,GAZ03,GAZ04,GAZ06,GAZ07,PIP01,PIP02,PIP03,PTT04,PTT05,PTT06,PTT07,PTT09,PTT14,SIL05,SIL06,SIL07,SIL09,SIL10,TCL02,CAB04,CAB05,E_P06,E_P07,E_P09,E_P10,ECL01,ECL03,ECL04,ECL05,ECL07,ECL13,ECL01,ECL03,ECL04,ECL05,ECL07,ECL13,EDF01,EDF02,EDF03,EDF04,EDF12,PTT01,PTT04,PTT12,PTT13,SIL01,SIL02,SIL03,SIL04,SIL11,TCL02,TCL03,TCL04,TCL05,TCL06,TCL07,TCL08" );
    int blockTextNumer = getSsBlock( ssblockText, "", "ASS99,CAB99,CH99,E_AR99,E_P97,E_U96,EAU98,EAU99,ECL99,EDF99,GAZ99,PIP99,PTT99,SIL99,TCL99" );
    map<AcString, vector<AcString>> mapRef
    {
        {
            _T( "ASS99" ),
            {
                _T( "ASS01" ),
                _T( "ASS02" ),
                _T( "ASS03" ),
                _T( "E_UNIT05" ),
                _T( "ASS04" ),
                _T( "ASS07" ),
                _T( "ASS09" ),
                _T( "ASS10" )
            }
        },
        
        {
            _T( "CAB99" ),
            {
                _T( "CAB01" ),
                _T( "CAB02" ),
                _T( "CAB03" ),
                _T( "CAB04" ),
                _T( "CAB05" )
            }
        },
        
        {
            _T( "CH99" ),
            {
                _T( "CH01" ),
                _T( "CH02" ),
                _T( "CH03" )
            }
        },
        
        {
            _T( "E_AR99" ),
            {
                _T( "E_AR01" ),
                _T( "E_AR02" )
            }
        },
        
        {
            _T( "E_P97" ),
            {
                _T( "E_P01" ),
                _T( "E_P02" ),
                _T( "E_P03" ),
                _T( "E_P04" ),
                _T( "E_P05" ),
                _T( "E_P06" ),
                _T( "E_P07" ),
                _T( "E_P08" ),
                _T( "E_P09" ),
                _T( "E_P10" )
            }
        },
        
        {
            _T( "E_U96" ),
            {
                _T( "E_U01" ),
                _T( "E_U02" ),
                _T( "E_U03" ),
                _T( "E_U04" ),
                _T( "E_U07" ),
                _T( "E_U09" ),
                _T( "E_U10" )
            }
        },
        
        {_T( "EAU98" ), {_T( "EAU04" )}},
        
        {
            _T( "EAU99" ),
            {
                _T( "EAU01" ),
                _T( "EAU05" ),
                _T( "EAU08" ),
                _T( "EAU09" )
            }
        },
        {
            _T( "ECL99" ),
            {
                _T( "ECL01" ),
                _T( "ECL02" ),
                _T( "ECL03" ),
                _T( "ECL04" ),
                _T( "ECL05" ),
                _T( "ECL07" ),
                _T( "ECL10" ),
                _T( "ECL11" ),
                _T( "ECL12" ),
                _T( "ECL13" )
            }
        },
        {
            _T( "EDF99" ),
            {
                _T( "EDF01" ),
                _T( "EDF02" ),
                _T( "EDF03" ),
                _T( "EDF04" ),
                _T( "EDF08" ),
                _T( "EDF10" ),
                _T( "EDF11" ),
                _T( "EDF12" ),
                _T( "EDF13" ),
                _T( "EDF14" ),
                _T( "EDF15" )
            }
        },
        {
            _T( "GAZ99" ),
            {
                _T( "GAZ01" ),
                _T( "GAZ03" ),
                _T( "GAZ04" ),
                _T( "GAZ06" ),
                _T( "GAZ07" )
            }
        },
        
        {
            _T( "PIP99" ),
            {
                _T( "PIP01" ),
                _T( "PIP02" ),
                _T( "PIP03" )
            }
        },
        
        {
            _T( "PTT99" ),
            {
                _T( "PTT01" ),
                _T( "PTT04" ),
                _T( "PTT05" ),
                _T( "PTT06" ),
                _T( "PTT07" ),
                _T( "PTT09" ),
                _T( "PTT12" ),
                _T( "PTT13" ),
                _T( "PTT14" )
            }
        },
        
        {
            _T( "SIL99" ),
            {
                _T( "SIL01" ),
                _T( "SIL02" ),
                _T( "SIL03" ),
                _T( "SIL04" ),
                _T( "SIL05" ),
                _T( "SIL06" ),
                _T( "SIL07" ),
                _T( "SIL09" ),
                _T( "SIL10" ),
                _T( "SIL11" )
            }
        },
        
        {_T( "TCL99" ), {_T( "TCL01" ), _T( "TCL02" ), _T( "TCL03" ), _T( "TCL04" ), _T( "TCL05" ), _T( "TCL06" ), _T( "TCL07" ), _T( "TCL08" ), }}
    };
    int nbErBlockTxt = 0;
    
    if( blockNumber < 1 )
    {
        print( "Commande annulé" );
        return;
    }
    
    if( blockTextNumer < 1 )
    {
        print( "Commande annulé" );
        return;
    }
    
    //boucler sur les blocks texts
    for( int i = 0; i < blockTextNumer; i++ )
    {
        //Creer un block reference
        AcDbBlockReference* bTexte = getBlockFromSs( ssblockText, i, AcDb::kForWrite );
        double dist = DBL_MAX;
        double z = 0;
        AcString btName;
        getBlockName( btName, bTexte );
        
        AcGePoint3d btxtPoint = bTexte->position();
        
        //Verifions que le nom du block texte est dans la listes
        if( mapRef.find( btName ) != mapRef.end() )
        {
            //Recuperer la listes des nom du block a verifiers
            vector<AcString> vecRef = mapRef[btName];
            
            for( int j = 0; j < blockNumber; j++ )
            {
                //Recuperer les blocks qui ont de z
                AcDbBlockReference* bref = getBlockFromSs( ssblock, j, AcDb::kForRead );
                
                //Recuperer le nom du block
                AcString bName;
                getBlockName( bName, bref );
                
                //Si le block est dans la listse
                if( find( vecRef.begin(), vecRef.end(), bName ) != vecRef.end() )
                {
                    AcGePoint3d bPosition = bref->position();
                    //Recuperer la distance entre les points d'insertions
                    double dist2d = getDistance2d( btxtPoint, bPosition );
                    
                    if( dist2d < dist && dist2d < 1.5 )
                    {
                        dist = dist2d;
                        z = bPosition.z;
                    }
                }
                
                //Fermer les blocks
                bref->close();
            }
        }
        
        if( dist < 1.5 )
        {
            btxtPoint.z = z;
            bTexte->setPosition( btxtPoint );
        }
        
        else
        {
            drawCircle( btxtPoint, .05, _T( "Erreur_Z" ) );
            nbErBlockTxt++;
        }
        
        //Fermer les blocks
        bTexte->close();
    }
    
    print( "%i Blocktext modifié", blockTextNumer - nbErBlockTxt );
    print( "%i Blocktext a pas de correspondance", nbErBlockTxt );
    acedSSFree( ssblock );
    acedSSFree( ssblockText );
}


void cmdArroundVertex()
{
    //declarer la selection
    ads_name sspoly;
    
    int poly2dnb = 0;
    int poly3dnb = 0;
    
    //lancer le traitement
    arroundVertex( sspoly, poly2dnb, poly3dnb );
    
    //message
    print( to_string( poly2dnb ) + " Polylines 2d traites, " + to_string( poly3dnb ) + " Polylines 3d traites." );
    
    //liberer la selection
    acedSSFree( sspoly );
}

void cmdGetDwgCenter()
{
    //Listes des ACDBENTITES a recuperer sa bounding obx
    vector<AcString> DRAWINGENT{ "AcDbBlockReference", "AcDb2dPolyline", "AcDb3dPolyline",
        "AcDbArc", "AcDbCircle", "AcDbMText", "AcDbEllipse", "AcDbLeader", "AcDbLine",
        "AcDbText", "AcDbVertex", "AcDbPolyline", "AcDbMLeader", "AcDbFace", "AcDbPoint"};
    int iOkEnt = 0;
    
    //Recuperer tout les entites dans le dessins
    ads_name ssAlldrawing;
    int iobj = getSsAllObject( ssAlldrawing );
    
    ProgressBar progres = ProgressBar( _T( "CENTREDWG :" ), iobj );
    AcDbExtents ext( AcGePoint3d( DBL_MAX, DBL_MAX, 0 ),
        AcGePoint3d( DBL_MIN, DBL_MIN, 0 ) );
        
    //Boucler sur tout les entites
    for( int i = 0; i < iobj; i++ )
    {
        progres.moveUp( i );
        //Recuperer les entites
        AcDbEntity* ent = getEntityFromSs( ssAlldrawing, i, AcDb::kForRead );
        
        if( ent )
        {
            AcString type = entityType( ent );
            
            if( find( DRAWINGENT.begin(),
                    DRAWINGENT.end(), type ) != DRAWINGENT.end() )
            {
                AcDbExtents entext;
                
                //Recuperer la bondingbox
                ent->getGeomExtents( entext );
                
                AcGePoint3d minTot = ext.minPoint();
                AcGePoint3d maxTot = ext.maxPoint();
                AcGePoint3d minpot = entext.minPoint();
                AcGePoint3d maxpot = entext.maxPoint();
                
                if( minpot.x < minTot.x )
                    minTot.x = minpot.x;
                    
                if( minpot.y < minTot.y )
                    minTot.y = minpot.y;
                    
                if( maxpot.x > maxTot.x )
                    maxTot.x = maxpot.x;
                    
                if( maxpot.y > maxTot.y )
                    maxTot.y = maxpot.y;
                    
                ext.set( minTot, maxTot );
                iOkEnt++;
            }
            
            //Fermer l'entites
            ent->close();
        }
    }
    
    //Nom du fichier
    AcString filepath = getCurrentFileName();
    AcString fileDir = getCurrentFileFolder();
    AcString filename = getFileName( filepath );
    
    if( iOkEnt > 0 )
    {
        //Ecrire le json
        string strjsonpath = acStrToStr( fileDir ) + "\\" + acStrToStr( filename ) + ".json";
        ofstream jsonfile( strjsonpath.c_str() );
        writeCentreDwg( jsonfile, ext, filename );
        jsonfile.close();
        
        if( isFileExisting( strjsonpath ) )
            print( "CENTREDWG exporter dans : \"" + strjsonpath + "\"" );
        else
        {
            string newpath = "C:";
            newpath += getenv( "HOMEPATH" );
            newpath += "\\Documents\\" + acStrToStr( filename ) + ".json";
            ofstream defaultfile( newpath.c_str() );
            writeCentreDwg( defaultfile, ext, filename );
            defaultfile.close();
            print( "CENTREDWG exporter dans : \"" + newpath  + "\"" );
        }
        
    }
    
    else print( "Dessin vide" );
    
    //Liberer la selection
    acedSSFree( ssAlldrawing );
}


void cmdQrtListSurf()
{
    //Selection sur les blocs
    ads_name ssBloc;
    
    //Afficher message
    print( "Selectionner les blocs : " );
    
    //Selection des blocks
    long lenBloc = getSsBlock( ssBloc, _T( "" ), _T( "IDLOCAL_BIS" ) );
    
    //Verifier
    if( lenBloc == 0 )
    {
        //Afficher message
        print( "Selection vide." );
        return;
    }
    
    //Recuperer le chemin de sortie
    AcString filePath = getCurrentFileName() + _T( ".txt" );
    
    //Creer un ofstream
    ofstream out( acStrToStr( filePath ) );
    
    //Vecteur de string qui va contenir les informations sur le bloc
    vector<string> infoToExport;
    
    //Reference de bloc
    AcDbBlockReference* blockRef = NULL;
    
    //Compteur
    long comp = 0;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), lenBloc );
    
    //Boucle pour recuperer la selection de bloc
    for( long i = 0; i < lenBloc; i++ )
    {
        //Recuperer le ieme bloc
        blockRef = getBlockFromSs( ssBloc, i );
        
        //Verifier
        if( !blockRef )
            continue;
            
        //Recuperer les attributs du bloc
        AcString att_Batiment = getAttributValue( blockRef, _T( "BATIMENT" ) );
        AcString att_Etage = getAttributValue( blockRef, _T( "ETAGE" ) );
        AcString att_Utilisation = getAttributValue( blockRef, _T( "UTILISATION" ) );
        AcString att_Numero = getAttributValue( blockRef, _T( "NUMERO" ) );
        AcString att_CodeUtil = getAttributValue( blockRef, _T( "CODE_UTIL" ) );
        AcString att_Surface = getAttributValue( blockRef, _T( "SURFACE" ) );
        AcString att_sunsub = getAttributValue( blockRef, _T( "SUN/SUB" ) );
        
        //Ajouter les informations dans le fichier
        out << att_Batiment << "\t" << att_Etage << "\t" << att_Utilisation << "\t" << att_Numero << "\t" << att_CodeUtil;
        
        if( att_sunsub == _T( "SUN" ) )
            out << "\t" << att_Surface;
        else if( att_sunsub == _T( "SUB" ) )
            out << "\t\t" << att_Surface;
        else if( att_sunsub == _T( "HORS" ) )
            out << "\t\t\t" << att_Surface;
            
        //Mettre le endl
        if( i != lenBloc - 1 )
            out << "\n";
            
        //Fermer le blocs
        blockRef->close();
        
        //Compter
        comp++;
        
        //Progresser
        prog.moveUp( i );
    }
    
    //Fermer le fichier
    out.close();
    
    //Liberer la selection
    acedSSFree( ssBloc );
    
    //Afficher message
    print( "Nombre de bloc exporté : %d.", comp );
    print( "Txt exporté vers : %s", filePath );
}


void cmdDelTab()
{
    // Calque utilisé
    AcString layerlist = _T( "PROFIL_10,PROFIL_HM,PROFIL_KM,PROFIL_SC" );
    
    // Demander à l'utilisateur la distance minimale entre les tabulations
    double value = g_distMinTab;
    int es = askForDouble( _T( "Quelle est la distance minimale entre deux tabulations (m) ?" ), g_distMinTab, value );
    
    // Vérifier la valeur entrée
    if( es == RTCAN )
    {
        print( "Commande annulée." );
        return;
    }
    
    else if( es == RTNONE )
        value = g_distMinTab;
    else if( es == RTNORM )
        g_distMinTab = value;
        
    // Sélectionner l'axe de tabulation
    ads_name ssaxe, sstab;
    print( "Sélectionner l'axe des tabulations: " );
    long nbAxe = getSsPoly2D( ssaxe, _T( "FMAP_AXE" ) );
    print( "Sélectionner les tabulations: " );
    long nbTab = getSsLine( sstab, layerlist );
    
    if( nbAxe == 0 || nbTab == 0 )
    {
        print( "Problème de sélection." );
        acedSSFree( ssaxe );
        acedSSFree( sstab );
        return;
    }
    
    map<AcString, long> tabDel;
    tabDel.insert( { _T( "PROFIL_10" ), 0 } );
    tabDel.insert( { _T( "PROFIL_HM" ), 0 } );
    tabDel.insert( { _T( "PROFIL_KM" ), 0 } );
    tabDel.insert( { _T( "PROFIL_SC" ), 0 } );
    
    Acad::ErrorStatus er =  delTab( tabDel, ssaxe, sstab, value );
    map<AcString, long>::iterator itmap;
    AcString message = _T( "" );
    
    for( itmap = tabDel.begin(); itmap != tabDel.end(); ++itmap )
    {
        message = numberToAcString( itmap->second ) + _T( " tabulation(s) supprimée(s) dans le calque " ) + itmap->first;
        print( message );
    }
    
    acedSSFree( ssaxe );
    acedSSFree( ssaxe );
    
}

void cmdSiemlRegard()
{
    //  -----CREATION DES COULEURS DES PATATES------
    
    AcCmColor regard_ass_color, regard_div_color, regard_goutt_color;
    regard_ass_color.setRGB( 255, 0, 255 );
    regard_div_color.setRGB( 255, 255, 0 );
    regard_goutt_color.setRGB( 0, 255, 255 );
    
    //  ----------CREATION DES CALQUES----------
    
    //  Calque des + de 40 cm de côté
    if( !isLayerExisting( _T( "Vérifier_regard_ass" ) ) )
        createLayer( _T( "Vérifier_regard_ass" ), regard_ass_color );
        
    //  Calque des - de 40 cm de coté
    if( !isLayerExisting( _T( "Vérifier_regard_div" ) ) )
        createLayer( _T( "Vérifier_regard_div" ), regard_div_color );
        
    //  ----------SELECTION DES POLYLIGNES----------
    
    //Sélectionner les polylignes 3D "4_AffleurantPCRS_Regard_Ass"
    ads_name sspolylignes, ename, ss;
    int nbPolyligne = getSsPoly3D( sspolylignes, _T( "4_AffleurantPCRS_Regard_Ass" ) );
    
    //  Vérification
    if( nbPolyligne == 0 )
    {
        //  Afficher dans la console
        print( "\nAucune polyligne séléctionnée." );
        print( "\nCommande annulée." );
        return;
    }
    
    //  Tolérance des côtés
    double  tol = .40;
    
    //  Nombre de polyligne supprimer/controllée
    int     deletedPoly = 0,
            nbPolyControlled = 0;
            
    //  Barre de progression
    ProgressBar pBar = ProgressBar( _T( "Vérification regards" ), nbPolyligne );
    
    //Boucle sur le polyligne
    for( int i = 0; i < nbPolyligne; i++ )
    {
        //  Chargement
        pBar.moveUp( i );
        
        //Recuperer le polyligne
        AcDb3dPolyline* poly = getPoly3DFromSs( sspolylignes, i, AcDb::kForWrite );
        
        //  Vérification
        if( !poly )
            continue;
            
        //Recuperer la listes des points
        AcGePoint3dArray pointlist = getPoint3dArray( poly );
        
        //  Point d'insertion des patates
        AcGePoint3d insPoint;
        getPointAt( poly, 0, insPoint );
        
        //Recuperer la longueur, largeur
        double longueur = 0, largeur = 0;
        
        //  Si polyligne à quatre sommets
        if( pointlist.size() == 4 && poly->isClosed() )
        {
            //Recuperer la longueur et la largeur
            double cote_1    = pointlist[0].distanceTo( pointlist[1] );
            double cote_2    = pointlist[1].distanceTo( pointlist[2] );
            
            //Recuperer le rapport entre les deux distances
            double rapport;
            
            //  Vérification du longueur et largeur
            if( cote_1 > cote_2 )
            {
                //  Assigner les valeurs
                longueur = cote_1;
                largeur = cote_2;
                
                //  Calculer le rapport
                rapport = longueur / largeur;
            }
            
            else if( cote_1 < cote_2 )
            {
                //  Assigner les valeurs
                longueur = cote_2;
                largeur = cote_1;
                
                //  Calculer le rapport
                rapport = longueur / largeur;
            }
            
            else rapport = longueur / largeur;
            
            
            //  Supprimer le rectangle avec un rapport sup à 1.3
            if( rapport > 1.3 )
            {
                deletedPoly++;
                poly->erase();
            }
            
            else
            {
                //  Vérifier les côtés + de 40 cm
                if( longueur >= tol || largeur >= tol )
                {
                    //  Ajouter le patate sur le regard
                    gcedCommand(
                        RTSTR,
                        _T( "_MLEADER" ),
                        RT3DPOINT,
                        insPoint,
                        RT3DPOINT,
                        insPoint + AcGeVector3d( 0.5, 0.5, 0 ),
                        RTSTR,
                        _T( "Vérifier_regard_ass" ),
                        RTNONE );
                        
                    //  Entité patate de regard
                    AcDbEntity* entity = NULL;
                    
                    //  Prendre le dernier entité insérée
                    if( eOk != getLastEntityInserted( entity, AcDb::kForWrite ) )
                        entity->close();
                        
                    else
                    {
                        //  Vérifier si c'est un MLEAD
                        if( !entity->isKindOf( AcDbMLeader::desc() ) )
                            entity->close();
                        else
                        {
                            // Caster l'entité en mlead
                            AcDbMLeader* mleader = AcDbMLeader::cast( entity );
                            
                            //  Ajouter le patate dans le calque correspondant
                            mleader->setLayer( _T( "Vérifier_regard_ass" ) );
                            
                            mleader->setTextHeight( 0.18 );
                            mleader->setLandingGap( 0.0 );
                            
                            mleader->setColor( regard_ass_color );
                            
                            mleader->setLineWeight( AcDb::LineWeight::kLnWt035 );
                            
                            mleader->close();
                            acdbGetAdsName( ename, mleader->objectId() );
                            acedSSAdd( ename, ss, ss );
                            acedSSFree( ename );
                        }
                    }
                }
                
                //  Vérifier les côtés - de 40 cm
                else
                {
                    //  Ajouter le patate sur le regard
                    gcedCommand(
                        RTSTR,
                        _T( "_MLEADER" ),
                        RT3DPOINT,
                        insPoint,
                        RT3DPOINT,
                        insPoint + AcGeVector3d( 0.5, 0.5, 0 ),
                        RTSTR,
                        _T( "Vérifier_regard_div" ),
                        RTNONE );
                        
                    //  Entité patate de regard
                    AcDbEntity* entity = NULL;
                    
                    //  Prendre le dernier entité insérée
                    if( eOk != getLastEntityInserted( entity, AcDb::kForWrite ) )
                        entity->close();
                        
                    else
                    {
                        //  Vérifier si c'est un MLEAD
                        if( !entity->isKindOf( AcDbMLeader::desc() ) )
                            entity->close();
                        else
                        {
                            // Caster l'entité en mlead
                            AcDbMLeader* mleader = AcDbMLeader::cast( entity );
                            
                            //  Mettre dans le calque correspondant
                            mleader->setLayer( _T( "Vérifier_regard_div" ) );
                            
                            //  Appliquer le style
                            mleader->setTextHeight( 0.18 );
                            mleader->setLandingGap( 0.0 );
                            mleader->setLineWeight( AcDb::LineWeight::kLnWt035 );
                            
                            //  Définir la couleur
                            mleader->setColor( regard_div_color );
                            
                            //  Fermer
                            mleader->close();
                            acdbGetAdsName( ename, mleader->objectId() );
                            acedSSAdd( ename, ss, ss );
                            acedSSFree( ename );
                        }
                    }
                }
            }
        }
        
        //Fermer
        poly->close();
        
        //  Incrémentation du nombre de polyligne prises en compte
        nbPolyControlled++;
    }
    
    //  Affichage final dans la console
    print( "\n" + to_string( nbPolyControlled ) + " polylignes contrôlée(s)." );
    print( "\n" + to_string( deletedPoly ) + " polylignes supprimée(s)." );
}

void cmdSiemlGoutt()
{
    //  -----CREATION DES COULEURS DES PATATES------
    
    AcCmColor regard_goutt_color;
    
    regard_goutt_color.setRGB( 0, 255, 255 );
    
    
    //  Sélection sur les polylignes
    ads_name ssPoly;
    
    //  Nombre de polyligne modifiées
    int nbPolyControlled = 0;
    
    //  Créer le nouveau calque si il n'est pas encore créé
    if( !isLayerExisting( _T( "Vérifier_regard_goutt" ) ) )
        createLayer( _T( "Vérifier_regard_goutt" ) );
        
    if( !isLayerExisting( _T( "4_AffleurantPCRS_Regard_Goutt" ) ) )
        createLayer( _T( "4_AffleurantPCRS_Regard_Goutt" ) );
        
    //  Faire une sélection sur les polylignes 3D
    int nbSs = getSsPoly3D( ssPoly, _T( "4_AffleurantPCRS_Regard_Ass" ) );
    
    if( nbSs == 0 )
    {
        //  Afficher dans la console
        print( "\nAucune polyligne sélectionnée.\nCommande annulée." );
        return;
    }
    
    //  Sélection sur les batis et proéminence
    ads_name ssFacadePCRS, ssFacadePCRS_fig, ssProemBatiPCRS;
    int nbFacadePCRS = getSsAllPoly3D( ssFacadePCRS, _T( "2_FacadePCRS" ) );
    int nbFacadePCRS_fig = getSsAllPoly3D( ssFacadePCRS_fig, _T( "2_FacadePCRS_figuratif" ) );
    int nbproemBatPCRS = getSsAllPoly3D( ssProemBatiPCRS, _T( "2_ProeminenceBatiPCRS " ) );
    
    //  Barre de progression
    ProgressBar pBar = ProgressBar( _T( "Vérification regard_goutt" ), nbSs );
    
    //  Itération sur la sélection
    for( int s = 0; s < nbSs; s++ )
    {
        //  Chargement
        pBar.moveUp( s );
        
        //  Prendre la polyligne
        AcDb3dPolyline* poly = getPoly3DFromSs( ssPoly, s, AcDb::kForWrite );
        
        //  Vérification
        if( !poly )
            continue;
            
        //Recuperer la listes des points
        AcGePoint3dArray pointlist = getPoint3dArray( poly );
        
        //  Vérification regard
        if( pointlist.size() != 4 && !poly->isClosed() )
        {
            poly->close();
            continue;
        }
        
        //  Point d'insertion des patates
        AcGePoint3d insPoint;
        getPointAt( poly, 0, insPoint );
        
        //  Tableau des points d'intersection
        AcGePoint3dArray intersections;
        
        //  Itération sur les sommets du rectangle
        for( int pt = 0; pt < pointlist.size(); pt++ )
        {
            if( drawCircle( pointlist[pt], 0.05 ) != ErrorStatus::eOk )
                continue;
                
            //  Prendre le dernier entité inséré dans le dessin
            AcDbEntity* lastEnt;
            ErrorStatus es = getLastEntityInserted( lastEnt, AcDb::kForWrite );
            
            //  Vérification
            if( es == ErrorStatus::eOk && lastEnt->isKindOf( AcDbCircle::desc() ) )
            {
                //  Caster en cercle
                AcDbCircle* cercle = AcDbCircle::cast( lastEnt );
                
                if( !cercle )
                    continue;
                    
                //  Itération sur les facades et proeminences
                for( int fac1 = 0; fac1 < nbFacadePCRS; fac1++ )
                {
                    //  Prendre la facade
                    AcDb3dPolyline* facadePCRS = getPoly3DFromSs( ssFacadePCRS, fac1 );
                    
                    //  Prendre les intersections
                    if( computeIntersection( cercle, facadePCRS, intersections ) != ErrorStatus::eOk )
                    {
                        //  Fermer
                        facadePCRS->close();
                        continue;
                    }
                    
                    //  Fermer la facade
                    facadePCRS->close();
                }
                
                for( int fac2 = 0; fac2 < nbFacadePCRS_fig; fac2++ )
                {
                    //  Prendre la facade
                    AcDb3dPolyline* facadePCRS_fig = getPoly3DFromSs( ssFacadePCRS_fig, fac2 );
                    
                    //  Prendre les intersections
                    if( computeIntersection( cercle, facadePCRS_fig, intersections ) != ErrorStatus::eOk )
                    {
                        //  Fermer
                        facadePCRS_fig->close();
                        continue;
                    }
                    
                    //  Fermer la facade
                    facadePCRS_fig->close();
                }
                
                for( int fac3 = 0; fac3 < nbproemBatPCRS; fac3++ )
                {
                    //  Prendre la facade
                    AcDb3dPolyline* proemBatPCRS = getPoly3DFromSs( ssProemBatiPCRS, fac3 );
                    
                    //  Prendre les intersections
                    if( computeIntersection( cercle, proemBatPCRS, intersections ) != ErrorStatus::eOk )
                    {
                        //  Fermer
                        proemBatPCRS->close();
                        continue;
                    }
                    
                    //  Fermer la facade
                    proemBatPCRS->close();
                }
                
                //  Supprimer & Fermer
                cercle->erase();
                cercle->close();
            }
            
            //  Fermer
            lastEnt->close();
        }
        
        //  Vérifier si il y a intersection
        if( intersections.size() != 0 )
        {
            //  Ajouter le patate sur le regard
            gcedCommand(
                RTSTR,
                _T( "_MLEADER" ),
                RT3DPOINT,
                insPoint,
                RT3DPOINT,
                insPoint + AcGeVector3d( 0.5, 0, 0 ),
                RTSTR,
                _T( "Vérifier_regard_goutt" ),
                RTNONE );
                
            //  Entité patate de regard
            AcDbEntity* entity = NULL;
            
            //  Prendre le dernier entité insérée
            if( eOk != getLastEntityInserted( entity, AcDb::kForWrite ) )
                entity->close();
                
            else
            {
                //  Vérifier si c'est un MLEAD
                if( !entity->isKindOf( AcDbMLeader::desc() ) )
                    entity->close();
                else
                {
                    // Caster l'entité en mlead
                    AcDbMLeader* mleader = AcDbMLeader::cast( entity );
                    
                    //  Mettre dans le calque correspondant
                    mleader->setLayer( _T( "Vérifier_regard_goutt" ) );
                    
                    //  Modification du style
                    mleader->setTextHeight( 0.18 );
                    mleader->setLandingGap( 0.0 );
                    mleader->setLineWeight( AcDb::LineWeight::kLnWt035 );
                    
                    //  Définir la couleur
                    mleader->setColor( regard_goutt_color );
                    
                    //  Fermer le patate
                    mleader->close();
                    
                    //  Mettre dans le calque correspondant
                    poly->setLayer( _T( "4_AffleurantPCRS_Regard_Goutt" ) );
                    
                    //  Incrémentation du nombre de polyligne prises en compte
                    nbPolyControlled++;
                }
            }
        }
        
        //Fermer
        poly->close();
    }
    
    //  Affichage final dans la console
    print( "\n" + to_string( nbPolyControlled ) + " polyligne(s) mise(s) dans le calque '4_AffleurantPCRS_Regard_Goutt'." );
}

void cmdSMDiv()
{
    //  Vérifier les regards div
    AcString layer_regard_div = _T( "Vérifier_regard_div" );
    int nbPoly = siemlVerification( layer_regard_div );
    
    //  Vérification
    if( nbPoly == 0 )
    {
        //  Afficher dans la console
        print( "\nAucune polyligne.\nCommande annulée." );
        return;
    }
    
    //  Affichage dans la console
    print( "\n" + to_string( nbPoly ) + " polyligne(s) mise(s) dans le calque '4_AffleurantPCRS_Regard_Div'." );
}

void cmdMajngfAlt()
{
    //Nombre de block modifier
    int iChanged = 0;
    
    //Selectionner les blocks
    ads_name ssblock, ssblockref;
    print( "Sélectionner le bloc référence." );
    int ibref = getSsOneBlock( ssblockref );
    
    if( ibref > 0 )
    {
        //Selection des blocks a modifiers
        print( "Sélectionner les blocs NIVEAU_PLANCHER." );
        int nbBlocks = getSsBlock( ssblock, "", "NIVEAU_PLANCHER" );
        
        if( nbBlocks > 0 )
        {
            //Recuperation de l'objet block
            AcDbBlockReference* blockref = getBlockFromSs( ssblockref, 0, AcDb::kForRead );
            
            if( blockref )
            {
                //Recuperer l'id de la block reference
                AcDbObjectId refID = blockref->id();
                
                //Recuperations des attributs du blocks
                map <AcString, AcString> attvalue = getBlockAttWithValuesList( blockref );
                blockref->close();
                
                if( isAcstringNumber( attvalue[_T( "NIVEAUP" )] ) )
                {
                    double altRef = stod( acStrToStr( attvalue[_T( "NIVEAUP" )] ) );
                    
                    //Boucle sur les blocks
                    for( int i = 0; i < nbBlocks; i++ )
                    {
                        //Recuperer le block
                        AcDbBlockReference* block = getBlockFromSs( ssblock, i, AcDb::kForWrite );
                        
                        if( block->id() != refID )
                        {
                            //Recuperer la listes des attributs
                            map <AcString, AcString> attval = getBlockAttWithValuesList( block );
                            
                            if( attval.find( "NIVEAU" ) != attval.end() )
                            {
                                //Recuperer les niveau
                                string strNiveau = acStrToStr( attval["NIVEAU"] );
                                char sign = strNiveau.at( 0 );
                                strNiveau.erase( strNiveau.begin(), strNiveau.begin() + 1 );
                                
                                //Nettoyer le string
                                while( !isAcstringNumber( strToAcStr( strNiveau ) ) && strNiveau.length() > 0 )
                                {
                                    sign = strNiveau.at( 0 );
                                    strNiveau.erase( strNiveau.begin(), strNiveau.begin() + 1 );
                                }
                                
                                if( isAcstringNumber( strToAcStr( strNiveau ) ) )
                                {
                                    //Recuperer le niveau par rapport a la
                                    double niveau = stod( strNiveau );
                                    
                                    if( sign == '-' ) niveau = -niveau;
                                    
                                    double alt = altRef + niveau;
                                    
                                    //Modifier l'attribut
                                    if( setAttributValue( block, _T( "NIVEAUP" ), numberToAcString( alt, 2 ) ) )
                                        iChanged++;
                                }
                                
                                else
                                    ;
                            }
                        }
                        
                        //Fermer le block
                        block->close();
                    }
                }
                
                else
                {
                    //
                    print( "Altitude de référence incorrecte." );
                    acedSSFree( ssblock );
                    return;
                }
            }
            
            else
            {
                print( "Référence incorrecte." );
                acedSSFree( ssblock );
                return;
            }
        }
        
        else
        {
            print( "Commande annulée." );
            acedSSFree( ssblock );
            return;
        }
    }
    
    else print( "Commande annulée." );
    
    //Liberer la selection
    acedSSFree( ssblock );
    
    print( to_string( iChanged ), " blocs modifiés." );
}

void cmdMajngfRef()
{
    //Nombre de block modifier
    int iChanged = 0;
    
    //Demander la valeur a rajouter
    double dblRef = .0;
    int ans = askForDouble( _T( "Entrer la valeur à ajouter aux attributs NIVEAU." ), dblRef, dblRef );
    
    if( ans == RTCAN )
    {
        print( "Commande annulée." );
        return;
    }
    
    //Selectionner les blocks
    ads_name ssblock;
    print( "Sélectionner les blocs NIVEAU_PLANCHER." );
    int nbBlock = getSsBlock( ssblock, "", "NIVEAU_PLANCHER" );
    
    if( nbBlock > 0 )
    {
        for( int i = 0; i < nbBlock; i++ )
        {
            //Recupererles blocks
            AcDbBlockReference* blockref = getBlockFromSs( ssblock, i, AcDb::kForRead );
            
            //Valeures des attributs
            map <AcString, AcString> attval = getBlockAttWithValuesList( blockref );
            
            if( attval.find( "NIVEAU" ) != attval.end() )
            {
                //Recuperer les niveau
                string strNiveau = acStrToStr( attval["NIVEAU"] );
                char sign = strNiveau.at( 0 );
                
                //Nettoyer le string
                while( !isAcstringNumber( strToAcStr( strNiveau ) ) && strNiveau.length() > 0 )
                {
                    sign = strNiveau.at( 0 );
                    strNiveau.erase( strNiveau.begin(), strNiveau.begin() + 1 );
                }
                
                if( isAcstringNumber( strToAcStr( strNiveau ) ) )
                {
                    //Recuperer le niveau par rapport a la
                    double niveau = stod( strNiveau );
                    
                    if( sign == '-' ) niveau = -niveau;
                    
                    double alt = niveau + dblRef;
                    
                    if( alt < 0 ) sign = ' ';
                    else sign = '+';
                    
                    //Mettre le signe a jour
                    AcString acsign = sign;
                    
                    if( int( alt * 100 ) == 0 )acsign = _T( "%%p" );
                    
                    //Modifier l'attribut
                    if( setAttributValue( blockref, _T( "NIVEAU" ), acsign + numberToAcString( alt, 2 ) ) )
                        iChanged++;
                }
            }
            
            blockref->close();
        };
    }
    
    else
    {
        print( "Commande annnulée." );
        return;
    }
    
    //Liberer la selection
    acedSSFree( ssblock );
    
    print( to_string( iChanged ), " blocs modifiés." );
}