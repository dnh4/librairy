#pragma once

#include <xlnt/xlnt.hpp>
#include "fmap.h"
#include <adslib.h>
#include <oaidl.h>
#include <aced.h>
#include "tchar.h"
#include "dbmain.h"
#include "fmapMath.h"
#include "boundingBox.h"
#include <cstdio>

std::mutex lockThread;

AcDbBlockReference* insertFmapDynamicBlockReference( const AcString& blockName,
    const AcGePoint3d& position,
    const AcStringArray& propName,
    const AcStringArray& propValue,
    const AcString& blockLayer,
    const double& rotation,
    const double& scaleX,
    const double& scaleY,
    const double& scaleZ )
{
    // On insère la référence de bloc
    AcDbBlockReference* pBlock = insertBlockReference( blockName,
            blockLayer,
            position,
            rotation,
            scaleX,
            scaleY,
            scaleZ );
            
    if( !pBlock )
        return NULL;
        
    // On vérfie que le bloc soit bien dynamique
    if( !isDynamicBlock( pBlock ) )
    {
        acutPrintf( _T( "\nBlock <%s> n'est pas un bloc dynamique" ), blockName );
        pBlock->erase();
        pBlock->close();
        return NULL;
    }
    
    // On crée le bloc dynamique
    AcDbDynBlockReference pDynRef( pBlock );
    
    // On récupère les propriétés du bloc dynamique
    AcDbDynBlockReferencePropertyArray propArr;
    pDynRef.getBlockProperties( propArr );
    
    // On boucle sur les propriétés du bloc
    int length = propName.size();
    
    //Boucle sur les proprietes
    for( int i = length - 1; i >= 0; i-- )
    {
        //Vrifier les proprietes
        if( propValue[i].find( _T( "FC" ), 0 ) == -1 && propName[i].find( _T( "Rail-Insertion" ), 0 ) == -1 )
        {
            double d = 0;
            
            try
            {
                d = std::stod( acStrToStr( propValue[i] ), 0 );
            }
            
            catch( ... )
            {
                setPropertyFromBlock( pBlock, propName[i], propValue[i] );
                
                continue;
            }
            
            //Setter le propriete pour un double
            setPropertyFromBlock( pBlock, propName[i], d );
        }
        
        //Setter le propriete pour un acstring
        else
            setPropertyFromBlock( pBlock, propName[i], propValue[i] );
    }
    
    //Retourner le block
    return pBlock;
}


Acad::ErrorStatus addTopoOnAllVertex( const ads_name& ssPoly,
    const AcString& blockName )
{
    //Resultat par défaut
    Acad::ErrorStatus er = Acad::eOk;
    
    //Selection sur les blocs topos
    ads_name ssBlock;
    
    //Tableau pour les blocs
    AcGePoint3dArray arBlock;
    vector<double> xBlock;
    
    //Recuperer tous les blocs
    long lenBlock = getSsAllBlock( ssBlock, "", blockName );
    
    //Verifier
    if( lenBlock == 0 )
    {
        print( "Aucun bloc trouvé dans le dessin, sortie" );
        acedSSFree( ssBlock );
        return Acad::eNotApplicable;
    }
    
    //Block
    AcDbBlockReference* block3D = NULL;
    
    AcGePoint3d pos;
    
    //Ajouter les positions des blocs dans le vecteurs
    for( int i = 0; i < lenBlock; i++ )
    {
        //Recuperer le ieme bloc
        block3D = getBlockFromSs( ssBlock, i, AcDb::kForRead );
        
        if( !block3D )
            continue;
            
        //Recuperer la position du bloc
        pos = block3D->position();
        
        //Ajouter le point dans le vecteur
        arBlock.push_back( pos );
        xBlock.push_back( pos.x );
        
        //Fermer le bloc
        block3D->close();
    }
    
    //Tableau qui va contenir tous les vertexs de la polyligne
    AcGePoint3dArray arVtx;
    
    //Vecteur de x des vertexs de la polyligne
    vector< double > xPos;
    
    //Recuperer la taille des polylignes dans la selection
    long lengthPoly = 0;
    
    if( acedSSLength( ssPoly, &lengthPoly ) != RTNORM || lengthPoly == 0 )
        return Acad::eNotApplicable;
        
    //Polyligne 3d
    AcDb3dPolyline* poly3D;
    
    //Boucle sur les polylignes 3d
    for( int i = 0; i < lengthPoly; i++ )
    {
        //Récupérer le i-eme polyligne
        poly3D = getPoly3DFromSs( ssPoly, i, AcDb::kForRead );
        
        //Déclarer les vertex et creer un itérateur sur les sommets de la polyligne 3D
        AcDb3dPolylineVertex* vtx = NULL;
        AcDbObjectIterator* iterPoly3D = poly3D->vertexIterator();
        
        //Boucle sur les sommets de la  polyligne
        for( iterPoly3D->start(); !iterPoly3D->done(); iterPoly3D->step() )
        {
            //Ouvrir le premier sommet de la polyligne
            if( er != poly3D->openVertex( vtx, iterPoly3D->objectId(), AcDb::kForWrite ) ) return er;
            
            //Récupérer et sauvegarder les coordonnées du vertex en cours
            AcGePoint3d vertexPos = vtx->position();
            arVtx.append( vertexPos );
            xPos.push_back( vertexPos.x );
            
            //Fermer le vertex
            vtx->close();
        }
        
        //On ferme la poly
        poly3D->close();
        
    }
    
    //Verification que le tri est paraite
    er = sortList( arBlock, xBlock );
    
    if( Acad::eOk != er )
    {
        print( _T( "Impossible de trier le vecteur" ) );
        return er;
    }
    
    //Trier le vecteur de sommet
    er = sortList( arVtx, xPos );
    
    if( Acad::eOk != er )
    {
        print( _T( "Impossible de trier le vecteur" ) );
        return er;
    }
    
    //Supprimer les doublons dans le vecteur
    er = eraseDuplicate( arVtx );
    
    if( Acad::eOk != er )
    {
        print( _T( "Impossible de supprimer les doublons" ) );
        return er;
    }
    
    //On recuperer la taille du tableau de vertex
    long size = arVtx.size();
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Progression" ), size );
    
    //Compteur
    long count = 0;
    
    //Boucle sur les sommets des polylignes
    for( long counter = 0; counter < size; counter++ )
    {
        //Recuperer le premier vertex
        AcGePoint3d pt = arVtx[counter];
        
        //Index
        int index = 0;
        
        //Vérification si il n'y a pas bloc topo sur le point
        if( !isInList( pt,
                xBlock,
                arBlock,
                index ) )
        {
            //Dessiner le bloc sur le vertex
            drawBlock( blockName, "", pt );
            
            //Iterer les compteurs
            count++;
        }
        
        //Progresser la barre
        prog.moveUp( counter );
    }
    
    //Afficher une message
    print( "Nombre de points topos insérés: %d", count );
    
    //Retourner le resultat
    return Acad::eOk;
}


vector<string> cleanVectorOfName( vector<string>& vecName )
{
    //Resultat
    vector<string> resString;
    
    //Recuperer la taille du vecteur
    int taille = vecName.size();
    
    if( taille == 0 )
        return resString;
        
    //Boucle sur le vecteur
    for( int i = 0; i < taille ; i++ )
    {
        if( vecName[i] == "" || vecName[i] == "." || vecName[i] == ".." )
            continue;
            
        if( std::find( resString.begin(), resString.end(), vecName[i] ) == resString.end() )
            resString.push_back( vecName[i] );
    }
    
    //Retourner le resultat
    return resString;
}


AcString getLayerOfThisText( const AcString& text, const AcString& layerOfText )
{
    //Resultat par défaut
    AcString res = _T( "" );
    
    //Ouvrir le fichier .csv
    std::ifstream csvFile( acStrToStr( "C:\\Futurmap\\Outils\\GStarCAD2020\\BDX\\LAYERTEXT.txt" ) );
    
    //Verifier qu'on a bien le fichier
    if( !csvFile )
    {
        //Afficher message
        acutPrintf( _T( "Impossible de recuperer le fichier de gabarit, Sortie" ) );
        
        return res;
    }
    
    //Changer les entrée en espace
    AcString cleanText = crToSpace( text );
    
    //Spliter le string nom du fichier
    vector<string> textInfos = splitString( acStrToStr( cleanText ), " " );
    
    //Recuperer la taille du vecteur
    int tailleT = textInfos.size();
    
    if( tailleT == 0 )
    {
        csvFile.close();
        return res;
    }
    
    //Creer un vecteur de layText
    vector<LayText> layTextVector;
    
    //Ligne et colonne
    std::string line;
    
    //Lire le fichier ligne par ligne
    while( std::getline( csvFile, line ) )
    {
        //Splitter le string du fichier
        vector<string> lineOfFile = splitString( line, "\t" );
        
        //Creer un layText
        LayText l;
        l.text = strToAcStr( lineOfFile[0] );
        l.oldLayer = strToAcStr( lineOfFile[1] );
        l.newLayer = strToAcStr( lineOfFile[2] );
        
        //Ajouter dans le vecteur
        layTextVector.push_back( l );
    }
    
    //Fermer le fichier
    csvFile.close();
    
    //Recuperer la taille du vecteur
    int taille = layTextVector.size();
    
    //Verifier
    if( taille == 0 )
        return res;
        
    //Vecteur de filtration
    vector<LayText> tempFil;
    
    //Premier filtre de calque
    for( int i = 0; i < taille ; i++ )
    {
        //Recuperer le premier lay
        LayText l = layTextVector[i];
        
        //Verifier le nom de l'ancien calque
        if( l.oldLayer == layerOfText )
            tempFil.push_back( l );
    }
    
    //Recuperer la taille
    taille = tempFil.size();
    
    //Boucle sur le texte
    for( int i = 0 ; i < tailleT ; i++ )
    {
        //Recuperer le premier texte
        AcString t = strToAcStr( textInfos[i] );
        
        //Boucle sur le vecteur de nom
        for( int j = 0 ; j < tempFil.size(); j++ )
        {
            //Recuperer le jeme lay
            LayText l = tempFil[j];
            
            //Verifier
            if( l.text.find( t, 0 ) == -1 )
            {
                //Enlever l'element
                tempFil.erase( tempFil.begin() + j );
                
                j--;
            }
        }
        
        if( tempFil.size() == 1 )
            return tempFil[0].newLayer;
    }
    
    if( tempFil.size() == 0 )
        return layerOfText;
    else if( tempFil.size() == 2 )
    {
        //Recuperer la longueur du texte
        int lenOrig = cleanText.length();
        
        //Recuperer la longueur du premier
        int len1 = tempFil[0].text.length();
        
        //Recuperer la longueur du second
        int len2 = tempFil[1].text.length();
        
        //Verifier
        if( abs( len1 - lenOrig ) < abs( len2 - lenOrig ) )
            return tempFil[0].newLayer;
        else
            return tempFil[1].newLayer;
    }
    
    //Recuperer le calque
    return tempFil[0].newLayer;
}


AcString crToSpace( const AcString& text )
{
    //Changer l'acstring en string
    string str = acStrToStr( text );
    
    //Boucle pour remplacer les entrée en espace
    for( string::iterator it = str.begin(); it != str.end(); ++it )
    {
        if( *it == '\n' || *it == '\r' )
            *it = ' ';
    }
    
    //Caster en AcString
    AcString res = strToAcStr( str );
    
    //Cherche h=
    int index = res.find( "h=", 0 );
    
    //Tester
    if( index != -1 )
        res = res.substr( 0, index - 1 );
        
    return res;
}


void interpolThisPointArray( AcGePoint3dArray& ptArray,
    Slope& startSlope,
    Slope& endSlope )
{
    //Recuperer la taille du tableau
    int taille = ptArray.size();
    
    //Verifier
    if( taille < 2 )
        return;
        
    bool someSlope = false;
    
    AcGePoint3d pt1 = AcGePoint3d::kOrigin;
    AcGePoint3d pt2 = AcGePoint3d::kOrigin;
    
    //Boucle sur les points
    for( int i = 0; i < taille; i++ )
    {
        //Recuperer le point
        if( pt2 == AcGePoint3d::kOrigin )
            pt1 = ptArray[i];
            
        //Verifier le z
        if( pt1.z == 0 )
            continue;
            
        //Creer un second point
        pt2 = AcGePoint3d::kOrigin;
        
        //Index Max
        int index = 0;
        
        //Si on a trouvé un second point
        bool findPt2 = false;
        
        //Boucle sur les points suivants
        for( int j = i + 1; j < taille; j++ )
        {
            //Recuper le j-eme point
            pt2 = ptArray[j];
            
            //Verifier le z
            if( pt2.z == 0 )
                continue;
                
            //Recuperer le premier slope
            if( startSlope.pt1 == AcGePoint3d::kOrigin )
                startSlope.pt1 = pt1;
                
            //Stocker le dernier slope
            else
                endSlope.pt1 = pt1;
                
            //Recuperer le premier slope
            if( startSlope.pt2 == AcGePoint3d::kOrigin )
                startSlope.pt2 = pt2;
                
            //Stocker le dernier slope
            else
                endSlope.pt2 = pt2;
                
            //Changer l'index de i
            index = j;
            
            //Changer le booleen
            findPt2 = true;
            
            //Sortir
            break;
        }
        
        //Si on n'a pas trouvé de pt2
        if( !findPt2 && pt2 == AcGePoint3d::kOrigin )
        {
            //Setter le z de tous les points avec le z du pt1
            for( int j = i; j < taille; j++ )
                ptArray[j].z = pt1.z;
                
            //Sortir
            return;
        }
        
        //Creer une ligne
        AcDbXline* line = new AcDbXline();
        
        //Setter le base point de la ligne
        line->setBasePoint( pt1 );
        
        //Setter l'unit dir
        line->setUnitDir( getVector3d( pt1, pt2 ) );
        
        //Setter le z sur les points entre les deux points
        for( int k = i + 1; k < index; k++ )
        {
            //Point projeté sur la ligne
            AcGePoint3d ptOnCurve;
            
            //Recuperer le point projété sur la ligne
            line->getClosestPointTo( ptArray[k], AcGeVector3d::kZAxis, ptOnCurve );
            
            //Recuperer le z du point projeté
            ptArray[k].z = ptOnCurve.z;
        }
        
        pt1 = pt2;
        
        //Recuperer le dernier index
        i = index - 1;
        
        //Fermer la ligne
        line->close();
    }
    
    //Si on n'a qu'une seule droite de pente on recupere la premiere pente
    if( endSlope.pt1 == AcGePoint3d::kOrigin && endSlope.pt2 == AcGePoint3d::kOrigin )
    {
        endSlope.pt1 = startSlope.pt1;
        endSlope.pt2 = startSlope.pt2;
    }
}

void extrapolThisPointArray( AcGePoint3dArray& ptArray,
    const Slope& startSlope,
    const Slope& endSlope )
{
    //Recuperer la taille du tableau
    int taille = ptArray.size();
    
    //Verifier
    if( taille < 2 )
        return;
        
    //Boucle sur les points
    for( int i = 0; i < taille ; i++ )
    {
        //Recuperer le ieme point
        AcGePoint3d pt1 = ptArray[i];
        
        //Verifier le z
        if( pt1.z == 0 )
            continue;
            
        //Creer une ligne
        AcDbXline* line = new AcDbXline();
        
        //Setter le base point
        line->setBasePoint( startSlope.pt1 );
        
        //Setter l'unit dir
        line->setUnitDir( getVector3d( startSlope.pt1, startSlope.pt2 ) );
        
        //Boucle pour recuperer les z des points d'avant
        for( int j = i - 1; j >= 0; j-- )
        {
            //Point projété
            AcGePoint3d ptOnCurve = AcGePoint3d::kOrigin;
            
            //Projeter le point
            line->getClosestPointTo( ptArray[j], AcGeVector3d::kZAxis, ptOnCurve );
            
            //Setter le z du point
            ptArray[j].z = ptOnCurve.z;
        }
        
        //Sortir
        break;
    }
    
    //Boucle vers la fin
    for( int i = taille - 1; i >= 0; i-- )
    {
        //Recuperer le ieme points
        AcGePoint3d pt1 = ptArray[i];
        
        //Verifier le z
        if( pt1.z == 0 )
            continue;
            
        //Creer une ligne
        AcDbXline* line = new AcDbXline();
        
        //Setter le base point
        line->setBasePoint( endSlope.pt1 );
        
        //Setter l'unit dir
        line->setUnitDir( getVector3d( endSlope.pt1, endSlope.pt2 ) );
        
        //Boucle pour recuperer les z des points d'apres
        for( int j = i + 1; j < taille; j++ )
        {
            //Point projété
            AcGePoint3d ptOnCurve = AcGePoint3d::kOrigin;
            
            //Projeter le point
            line->getClosestPointTo( ptArray[j], AcGeVector3d::kZAxis, ptOnCurve );
            
            //Setter le z du point
            ptArray[j].z = ptOnCurve.z;
        }
        
        //Sortir
        break;
    }
}




bool getDistCurvOfArray( vector<double>& res,
    AcGePoint3dArray& ptArray,
    const AcDb3dPolyline* poly3D,
    const bool& sort )
{
    //Recuperer la taille du ptArray
    int taille = ptArray.size();
    
    //Verifier
    if( taille == 0 )
        return false;
        
    double dist = 0;
    
    //Boucle sur les points
    for( int i = 0 ; i < taille; i++ )
    {
        //Recuperer la distance curviligne
        poly3D->getDistAtPoint( ptArray[i], dist );
        
        //Ajouter le point dans le vecteur
        res.push_back( dist );
    }
    
    //Si on veut trier
    if( sort )
        std::sort( res.begin(), res.end() );
        
    //Sortir
    return true;
}


bool getIndexOfPoint( vector<int>& res,
    const vector<double>& curvDistPtArray,
    const vector<double>& curvDistPtIntersectArray,
    const double& secondDistCurv )
{
    //Recuperer la taille des vecteur
    int taille = curvDistPtArray.size();
    int tailleDist = curvDistPtIntersectArray.size();
    
    //Verifier
    if( taille == 0 || tailleDist == 0 )
        return false;
        
    //Dernier index
    int maxInd = tailleDist - 1;
    
    double min, max;
    
    min = curvDistPtIntersectArray[0];
    max = curvDistPtIntersectArray[maxInd];
    
    //Boucle sur les points
    for( int i = 0; i < taille; i++ )
    {
        //Tester
        if( curvDistPtArray[i] >= min
            && curvDistPtArray[i] <= max )
            res.push_back( i );
    }
    
    return true;
}


void projectIndexedPointToPlane( AcGePoint3dArray& ptArray,
    const vector<int>& indexes,
    const AcGePlane& plane )
{
    //Recuperer la taille des vecteurs
    int taille = ptArray.size();
    int tailleInd = indexes.size();
    
    //Verifier
    if( taille == 0 || tailleInd == 0 )
        return;
        
    //Boucle sur les index
    for( int i = 0; i < tailleInd; i++ )
    {
        //Point projeté sur le plan
        AcGePoint3d ptOnPlane = AcGePoint3d::kOrigin;
        
        //Projeter le point
        ptOnPlane = plane.closestPointTo( ptArray[indexes[i]] );
        
        //Changer le z
        ptArray[ indexes[i] ].z = ptOnPlane.z;
    }
}


AcGePoint3d getFaceCenter( const AcDbFace* face )
{
    //Variables
    AcGePoint3d res, pt1, pt2, pt3, pt4;
    
    //Recuperer les points de la face
    face->getVertexAt( 0, pt1 );
    face->getVertexAt( 1, pt2 );
    face->getVertexAt( 2, pt3 );
    face->getVertexAt( 3, pt4 );
    
    double x = pt1.x + pt2.x + pt3.x;
    double y = pt1.y + pt2.y + pt3.y;
    double z = pt1.z + pt2.z + pt3.z;
    
    //Verifier si on a une face de 3 point
    if( !isEqual3d( pt1, pt4 ) )
    {
        x += pt4.x;
        y += pt4.y;
        z += pt4.z;
        
        //Retourner le resultat
        return AcGePoint3d( x / 4, y / 4, z / 4 );
    }
    
    //Retourner le resultat
    return AcGePoint3d( x / 3, y / 3, z / 3 );
}


void getFaceCenterUVVector( AcGePoint3d& center,
    AcGeVector3d& u,
    AcGeVector3d& v,
    const AcDbFace* face )
{
    //Variables
    AcGePoint3d  pt1, pt2, pt3, pt4;
    
    //Recuperer les points de la face
    face->getVertexAt( 0, pt1 );
    face->getVertexAt( 1, pt2 );
    face->getVertexAt( 2, pt3 );
    face->getVertexAt( 3, pt4 );
    
    double x = pt1.x + pt2.x + pt3.x;
    double y = pt1.y + pt2.y + pt3.y;
    double z = pt1.z + pt2.z + pt3.z;
    
    //Recuperer le u
    u = getVector3d( pt1, pt2 );
    
    //Verifier si on a une face de 3 point
    if( !isEqual3d( pt1, pt4 ) )
    {
        x += pt4.x;
        y += pt4.y;
        z += pt4.z;
        
        //Retourner le resultat
        center = AcGePoint3d( x / 4, y / 4, z / 4 );
        
        //Recuperer le vecteur v
        v = getVector3d( pt1, pt4 );
        
        //Sortir
        return;
    }
    
    //Retourner le resultat
    center = AcGePoint3d( x / 3, y / 3, z / 3 );
    
    //Creer une ligne
    AcDbXline* line = new AcDbXline();
    
    //Setter la ligne
    line->setBasePoint( pt1 );
    line->setUnitDir( u );
    
    //Point projété sur la ligne
    AcGePoint3d ptOnLine = AcGePoint3d::kOrigin;
    
    //Recuperer le point projété
    line->getClosestPointTo( pt3, ptOnLine );
    
    //Fermer la ligne
    line->close();
    
    //Recuperer le vecteur v avant de sortir
    v = getVector3d( pt1, pt1 + getVector3d( ptOnLine, pt3 ) );
}


void projectInPlaneIfPointInsideFace( AcGePoint3d& pt,
    const AcDbFace* face )
{
    //Recuperer les points de la face
    AcGePoint3d pt1, pt2, pt3;
    
    face->getVertexAt( 0, pt1 );
    face->getVertexAt( 1, pt2 );
    face->getVertexAt( 2, pt3 );
    
    //Tester si le point est dans le triangle
    if( isInsideTriangle( pt, pt1, pt2, pt3 ) )
    {
        //Centre de la face
        AcGePoint3d faceCenter = AcGePoint3d::kOrigin;
        
        //Recuperer les vecteur u et v
        AcGeVector3d u, v;
        getFaceCenterUVVector( faceCenter, u, v, face );
        
        //Creer le plan de la face
        AcGePlane planeOfFace = AcGePlane( faceCenter, u, v );
        
        //Projeter le point pt sur le plan
        AcGePoint3d ptOnFace;
        ptOnFace = planeOfFace.closestPointTo( pt );
        
        //Changer le z
        pt.z = ptOnFace.z;
    }
}


bool isInsideTriangle( const AcGePoint3d& pt,
    const AcGePoint3d& pt0,
    const AcGePoint3d& pt1,
    const AcGePoint3d& pt2 )
{
    //Calculer s
    double s = pt0.y * pt2.x - pt0.x * pt2.y + ( pt2.y - pt0.y ) * pt.x + ( pt0.x - pt2.x ) * pt.y;
    
    //Calculer t
    double t = pt0.x * pt1.y - pt0.y * pt1.x + ( pt0.y - pt1.y ) * pt.x + ( pt1.x - pt0.x ) * pt.y;
    
    //Verifier si on peut déja valider
    if( ( s < 0 ) != ( t < 0 ) )
        return false;
        
    //Calculer l'aire
    double a = -pt1.y * pt2.x + pt0.y * ( pt2.x - pt1.x ) + pt0.x * ( pt1.y - pt2.y ) + pt1.x * pt2.y;
    
    //Verifier
    return a < 0 ?
        ( s <= 0 && s + t >= a ) :
        ( s >= 0 && s + t <= a );
}

std::vector<AcString> fileLineToUTF8StringVector( const AcString& filePath )
{
    //Vecteur resultat
    std::vector<AcString> vecRes;
    
    if( !isFileExisting( filePath ) )
        return vecRes;
        
    //Ouvrir le fichier txt
    ifstream fTxtFile( acStrToStr( filePath ) );
    
    std::string line;
    
    while( std::getline( fTxtFile, line ) )
    {
        std::istringstream iss( line );
        vecRes.push_back( strToAcStr( line ) );
    }
    
    //Fermer le fichier
    fTxtFile.close();
    
    //Retourner le resutlat
    return vecRes;
}


int findAcStringInVector( std::vector<AcString>& vecOfElements,
    const AcString& element )
{
    int res = -1;
    
    // Si on a un vecteur vide en retour toujours 0
    if( vecOfElements.size() == 0 )
        return 0;
        
    int index = 0;
    
    
    for( vector<AcString>::iterator it = vecOfElements.begin(); it != vecOfElements.end(); it ++ )
    {
        AcString oneElt = ( AcString ) * it;
        
        if( oneElt.compare( element ) == 0 )
        {
            res = index;
            break;
        }
        
        else
        {
            if( oneElt.compare( element ) == 0 )
            {
                res = index;
                break;
            }
        }
        
        index ++;
    }
    
    return res;
}

bool checkIfOneBoxIsTotallyInsideAnother( AcGePoint3dArray& oneBox,
    AcGePoint3dArray& anothereBox )
{
    if( oneBox[0].x < anothereBox[0].x )
        return false;
        
    if( oneBox[0].y < anothereBox[0].y )
        return false;
        
    if( oneBox[1].x > anothereBox[1].x )
        return false;
        
    if( oneBox[1].y < anothereBox[1].y )
        return false;
        
    if( oneBox[2].x > anothereBox[2].x )
        return false;
        
    if( oneBox[2].y > anothereBox[2].y )
        return false;
        
    if( oneBox[3].x < anothereBox[3].x )
        return false;
        
    if( oneBox[2].y > anothereBox[2].y )
        return false;
        
    return true;
}

double getMaxSeg( AcDb3dPolyline*& poly3d, AcGePoint3d& pt1, AcGePoint3d& pt2, double& maxDist, AcGePoint3d& centerPoly )
{
    if( !poly3d )
        return -1;
        
    AcDb3dPolylineVertex* vertex = NULL;
    AcDb3dPolylineVertex* nextVertex = NULL;
    AcDbObjectIterator* iterPoly3D = poly3d->vertexIterator();
    AcDbObjectIterator* iterPoly3DNext = poly3d->vertexIterator();
    
    //Initialiser l'itérateur
    iterPoly3D->start();
    iterPoly3DNext->start();
    iterPoly3DNext->step();
    
    double curDist = -1;
    double buDist = -1;
    
    vector<pair<AcGePoint3dArray, double>> vecData;
    
    AcGePoint3d ptFirst, ptLast;
    iterPoly3D->start();
    
    if( Acad::eOk != poly3d->openVertex( vertex, iterPoly3D->objectId(), AcDb::kForWrite ) ) return -1;
    
    ptFirst = vertex->position();
    //Fermer le vertex
    vertex->close();
    vertex = NULL;
    
    iterPoly3D->start( true );
    
    if( Acad::eOk != poly3d->openVertex( vertex, iterPoly3D->objectId(), AcDb::kForWrite ) ) return -1;
    
    ptLast = vertex->position();
    
    //Fermer le vertex
    vertex->close();
    vertex = NULL;
    
    for( iterPoly3D->start(); !iterPoly3D->done(); iterPoly3D->step() )
    {
        //Ouvrir le premier sommet de la polyligne
        if( Acad::eOk != poly3d->openVertex( vertex, iterPoly3D->objectId(), AcDb::kForWrite ) ) return -1;
        
        if( iterPoly3DNext->done() )
            break;
            
        //Récupérer les coordonnées du vertex en cours
        AcGePoint3d curPt = vertex->position();
        
        //Fermer le vertex
        vertex->close();
        vertex = NULL;
        
        //Ouvrir le suivant sommet de la polyligne
        if( Acad::eOk != poly3d->openVertex( nextVertex, iterPoly3DNext->objectId(), AcDb::kForWrite ) ) return -1;
        
        //Récupérer les coordonnées du vertex suivant
        AcGePoint3d nextPt = nextVertex->position();
        //Fermer le vertex
        nextVertex->close();
        nextVertex = NULL;
        
        // Calcul de distance
        curDist = getDistance2d( curPt, nextPt );
        AcGePoint3dArray arCurSeg;
        
        if( curPt.x < nextPt.x )
        {
            arCurSeg.append( curPt );
            arCurSeg.append( nextPt );
        }
        
        else
        {
            arCurSeg.append( nextPt );
            arCurSeg.append( curPt );
        }
        
        pair<AcGePoint3dArray, double> oneData;
        oneData.first = arCurSeg;
        oneData.second = curDist;
        vecData.push_back( oneData );
        
        iterPoly3DNext->step();
        continue;
    }
    
    if( poly3d->isClosed() )
    {
        // Calcul de distance
        curDist = getDistance2d( ptFirst, ptLast );
        AcGePoint3dArray arCurSeg;
        
        if( ptFirst.x < ptLast.x )
        {
            arCurSeg.append( ptFirst );
            arCurSeg.append( ptLast );
        }
        
        else
        {
            arCurSeg.append( ptLast );
            arCurSeg.append( ptFirst );
        }
        
        pair<AcGePoint3dArray, double> oneData;
        oneData.first = arCurSeg;
        oneData.second = curDist;
        vecData.push_back( oneData );
    }
    
    delete iterPoly3D;
    
    if( vecData.size() > 2 )
    {
        sort( vecData.begin(), vecData.end(), sortBySecond );
        pair<AcGePoint3dArray, double> fisrtPair = vecData[0];
        pair<AcGePoint3dArray, double> secondPair = vecData[1];
        
        AcGePoint3dArray firstAr = fisrtPair.first;
        AcGePoint3dArray secondtAr = secondPair.first;
        AcGePoint3d midFirst = AcGePoint3d( ( firstAr.at( 0 ).x + firstAr.at( 1 ).x ) / 2, ( firstAr.at( 0 ).y + firstAr.at( 1 ).y ) / 2, ( firstAr.at( 0 ).z + firstAr.at( 1 ).z ) / 2 );
        AcGePoint3d midSecond = AcGePoint3d( ( secondtAr.at( 0 ).x + secondtAr.at( 1 ).x ) / 2, ( secondtAr.at( 0 ).y + secondtAr.at( 1 ).y ) / 2, ( secondtAr.at( 0 ).z + secondtAr.at( 1 ).z ) / 2 );
        
        if( midFirst.y > midSecond.y )
        {
            pt1 = firstAr.at( 0 );
            pt2 = firstAr.at( 1 );
            maxDist = fisrtPair.second;
        }
        
        else
        {
            pt1 = secondtAr.at( 0 );
            pt2 = secondtAr.at( 1 );
            maxDist = secondPair.second;
        }
        
        if( vecData.size() > 3 )
        {
            pair<AcGePoint3dArray, double> thirdPair = vecData[2];
            AcGePoint3dArray thirdAr = thirdPair.first;
            
            centerPoly = AcGePoint3d( ( firstAr.at( 0 ).x + secondtAr.at( 1 ).x ) / 2, ( firstAr.at( 0 ).y + secondtAr.at( 1 ).y ) / 2, 0 );
        }
        
        else
            centerPoly = AcGePoint3d::kOrigin;
            
        return maxDist;
    }
    
    return -1;
}

bool sortBySecond( const pair<AcGePoint3dArray, double>& onePair,
    const pair<AcGePoint3dArray, double>& othPair )
{
    return ( onePair.second > othPair.second );
}

bool checkIfCanFit( const double& textWidth,
    AcDb3dPolyline*& poly3d )
{
    return false;
}


int verifyLayer( const ads_name& ssBlock,
    const ads_name& ssMText,
    const ads_name& ssText,
    const long& lenBloc,
    const long& lenMText,
    const long& lenText )
{
    //Reference de bloc
    AcDbBlockReference* block = NULL;
    
    //MTexte
    AcDbMText* mText = NULL;
    
    //Texte
    AcDbText* text = NULL;
    
    //Compteur
    int count = 0;
    
    //Vecteur de point 3d
    AcGePoint3dArray bArray, tArray;
    
    //Boucle sur la selection de block
    for( long i = 0; i < lenBloc; i++ )
    {
        //Recuperer le ieme bloc
        block = getBlockFromSs( ssBlock, i );
        
        //Verifier
        if( !block )
            continue;
            
        //Recuperer sa position
        bArray.push_back( block->position() );
        
        //Fermer le bloc
        block->close();
    }
    
    //Boucle sur la selection de Mtexte
    for( long i = 0; i < lenMText; i++ )
    {
        //Recuperer le ieme Mtexte
        mText = getMTextFromSs( ssMText, i, AcDb::kForRead );
        
        //Verifier
        if( !mText )
            continue;
            
        //Recuperer le bounding box du texte
        AcDbExtents bbMText;
        mText->getGeomExtents( bbMText );
        
        //Recuperer le centre du bounding box
        AcGePoint3d position = AcGePoint3d( bbMText.minPoint().x, bbMText.maxPoint().y, 0.0 );
        
        //Recuperer sa position
        tArray.push_back( position );
        
        //Fermer le texte
        mText->close();
    }
    
    //Boucle sur la selection de texte
    for( long i = 0; i < lenText; i++ )
    {
        //Recuperer le ieme texte
        text = getTextFromSs( ssText, i, AcDb::kForRead );
        
        //Verifier
        if( !text )
            continue;
            
        //Recuperer sa position
        tArray.push_back( text->position() );
        
        //Fermer le texte
        text->close();
    }
    
    //Recuperer la taille des tableaux
    long bTaille = bArray.size();
    long tTaille = tArray.size();
    
    //Boucle sur les points d'insertions des bloc
    for( long b = 0; b < bTaille; b++ )
    {
        //Recuperer le point d'insertion du bloc
        AcGePoint3d bInsert = bArray[b];
        
        //Index
        int index;
        
        //Booleen si on a trouve
        bool find = false;
        
        //Distance de recherche
        double distTemp = 5.0;
        
        //Boucle sur les points d'insertions des textes
        for( long t = 0; t < tTaille; t++ )
        {
            //Recuperer le point d'insertion du texte
            AcGePoint3d tInsert = tArray[t];
            
            //Recuperer la distance
            double dist = getDistance2d( tInsert, bInsert );
            
            //Verifier
            if( dist < distTemp )
            {
                //Recuperer l'index
                index = t;
                
                //Recuperer la derniere distance
                distTemp = dist;
                
                //Changer le booleen
                find = true;
            }
        }
        
        //Verifier déja si on a recuperer des textes
        if( !find )
        {
            //Ajouter une patate
            drawCircle( bInsert,
                1,
                _T( "Manque_Texte" ) );
                
            //Compter
            count++;
            
            //Continuer
            continue;
        }
        
        //Supprimer l'index dans le vecteur de texte
        tArray.erase( tArray.begin() + index );
    }
    
    //Recuperer la taille du tableau de texte
    tTaille = tArray.size();
    
    //Boucle sur les textes restants
    for( int i = 0; i < tTaille ; i++ )
    {
        //Ajouter patate
        drawCircle( tArray[i],
            1,
            _T( "Manque_Bloc" ) );
            
        //Compter
        count++;
    }
    
    //Retourner les compteurs
    return count;
}


double roundUp( const double& value, const int& numberDigit, const  int& multiple )
{
    int number = 10;
    
    if( numberDigit == 2 )
        number = 100;
        
    int nber = floor( value * number );
    
    int modulo = nber % multiple;
    
    while( modulo != 0 )
    {
        nber++;
        modulo = nber % multiple;
    }
    
    double result = ( ( double )nber ) / number;
    return result;
}


Acad::ErrorStatus eraseDoublons( AcGePoint3dArray& pointArray,
    const double& tol )
{
    //Resultat par défaut
    Acad::ErrorStatus er = Acad::eOk;
    
    //Recuperer la taille
    int taille = pointArray.size();
    
    //Verifier
    if( taille == 0 )
        return Acad::eNotApplicable;
        
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Scan des points" ), taille );
    
    //Boucle sur les elements du tableau
    for( int i = 0; i < pointArray.size() - 1; i++ )
    {
        bool autoErase = false;
        
        for( int j = i + 1; j < pointArray.size(); j++ )
        {
            if( isEqual3d( pointArray[i], pointArray[j], tol ) )
            {
                autoErase = true;
                
                //Supprimer l'element
                pointArray.erase( pointArray.begin() + j );
                
                j--;
            }
        }
        
        if( autoErase )
        {
            pointArray.erase( pointArray.begin() + i );
            
            i--;
        }
        
        prog.moveUp( i );
    }
    
    //Retourner eOk
    return er;
}


Acad::ErrorStatus insertPatate( const AcGePoint3dArray& ptArr,
    const AcString& layer )
{
    //Recuperer la taille du tableau
    long taille = ptArr.size();
    
    //Boucle sur le tableau
    for( int i = 0; i < taille; i++ )
        drawCircle( ptArr[i], 1, layer );
        
    //Retourner eOk
    return Acad::eOk;
}


Acad::ErrorStatus insertNewVertToPoly3D( const AcDbObjectId& idPoly3D,
    AcGePoint3dArray& pointsToInsertArray,
    const AcString& layer,
    const double& tol )
{
    Acad::ErrorStatus errorStatus = Acad::eOk;
    
    // Vérification de la polyline
    if( idPoly3D.isNull() || !idPoly3D.isValid() )
        return Acad::eNullHandle;
        
    //On vérifie qu'il y a au moin un point à insérer
    long pointsCount = pointsToInsertArray.length();
    
    if( pointsCount == 0 )
        return errorStatus;
        
    // On ouvre la polyligne pour écriture
    AcDb3dPolyline* poly3D = NULL;
    AcDbEntity* entity = NULL;
    
    if( Acad::eOk != acdbOpenAcDbEntity( entity, idPoly3D, AcDb::kForWrite ) )
        return Acad::eNullHandle;
        
    poly3D = AcDb3dPolyline::cast( entity );
    
    //On recupere les sommets de la polyligne
    AcGePoint3dArray ptArr;
    vector<double> vecX;
    getVertexesPoly( poly3D, ptArr, vecX );
    long lenArr = ptArr.size();
    
    for( long counter = 0; counter < pointsCount; counter++ )
    {
        AcGePoint3d onePoint = pointsToInsertArray[counter];
        
        bool isVert = false;
        
        //Verifier le point
        for( long l = 0; l < lenArr; l++ )
        {
            AcGePoint3d pt = ptArr[l];
            
            if( isEqual3d( onePoint, pt, tol ) )
            {
                isVert = true;
                break;
            }
        }
        
        if( isVert )
            continue;
            
        //Inserer les patates
        drawCircle( onePoint, 1, layer );
        
        errorStatus = insertVertexToPoly3D( poly3D, onePoint, tol );
    }
    
    poly3D->close();
    
    //On ferme la polyline
    return errorStatus;
}




Acad::ErrorStatus getPolyBlock( const AcString& path,
    const AcString& blockName,
    AcGePoint3dArray& res )
{
    //Ouvrir le fichier
    std::ifstream csvFile( acStrToStr( path ) );
    
    //Verifier qu'on a bien le fichier
    if( !csvFile )
    {
        //Afficher message
        print( "Impossible de recuperer le fichier de gabarit, Sortie" );
        
        return Acad::eNotApplicable;
    }
    
    //Ligne et colomne
    std::string line;
    
    //Lire le fichier ligne par ligne
    while( std::getline( csvFile, line ) )
    {
        //Spliter le string
        vector<string> lineOfFile = splitString( line, ";" );
        
        //Verifier le nom
        if( blockName != strToAcStr( lineOfFile[0] ) )
            continue;
        else
        {
            //Boucle pour recuperer les données
            for( int b = 1; b < 5; b++ )
            {
                //Recherche du premier (
                int ind1 = lineOfFile[b].find( "(" );
                
                //Recherche du premier ,
                int ind2 = lineOfFile[b].find( "," );
                
                //Recherche du premier )
                int ind3 = lineOfFile[b].find( ")" );
                
                //Tester si on a un donnée
                if( ind1 != -1 && ind2 != -1 && ind3 != -1 )
                {
                    //String
                    string resX;
                    
                    //Boucle sur le string
                    for( std::string::size_type i = 0; i < ind2; i++ )
                    {
                        if( i >= ind1 + 1 && i < ind2 )
                        {
                            //Recuperer lettre par lettre
                            char c = lineOfFile[b][i];
                            resX.push_back( c );
                        }
                    }
                    
                    //Changer en double
                    double ptX = stod( resX );
                    
                    //String
                    string resY;
                    
                    //Boucle sur le string
                    for( std::string::size_type i = ind2; i < ind3; i++ )
                    {
                        if( i >= ind2 + 1 && i < ind3 )
                        {
                            //Recuperer lettre par lettre
                            char c = lineOfFile[b][i];
                            resY.push_back( c );
                        }
                    }
                    
                    //Changer en double
                    double ptY = stod( resY );
                    
                    //Recuperer les points
                    if( b == 1 )
                        res.push_back( AcGePoint3d( ptX, ptY, 0 ) );
                        
                    //Recuperer les points
                    if( b == 2 )
                        res.push_back( AcGePoint3d( ptX, ptY, 0 ) );
                        
                    //Recuperer les points
                    if( b == 3 )
                        res.push_back( AcGePoint3d( ptX, ptY, 0 ) );
                        
                    //Recuperer les points
                    if( b == 4 )
                        res.push_back( AcGePoint3d( ptX, ptY, 0 ) );
                }
                
                //Sinon on met des valeurs par défauts ( ici -2 car ce n'est pas dans le bloc )
                else
                {
                    //Recuperer les points
                    if( b == 1 )
                        res.push_back( AcGePoint3d( -2, -2, 0 ) );
                        
                    //Recuperer les points
                    if( b == 2 )
                        res.push_back( AcGePoint3d( -2, -2, 0 ) );
                        
                    //Recuperer les points
                    if( b == 3 )
                        res.push_back( AcGePoint3d( -2, -2, 0 ) );
                        
                    //Recuperer les points
                    if( b == 4 )
                        res.push_back( AcGePoint3d( -2, -2, 0 ) );
                }
            }
            
            //Fermer le fichier
            csvFile.close();
            
            //Retourner eok
            return Acad::eOk;
        }
    }
    
    //Fermer le fichier
    csvFile.close();
    
    //Mettre les valeur par défaut
    res.push_back( AcGePoint3d( -2, -2, 0 ) );
    res.push_back( AcGePoint3d( -2, -2, 0 ) );
    res.push_back( AcGePoint3d( -2, -2, 0 ) );
    res.push_back( AcGePoint3d( -2, -2, 0 ) );
    
    //Retourner ok
    return Acad::eNotApplicable;
}


Acad::ErrorStatus drawPolyBlock( const AcDbBlockReference* block3D,
    const AcGePoint3dArray& vecPt )
{
    //Recuperer le calque du bloc
    AcString blockLayer = block3D->layer();
    
    //Tableau contenant des points
    AcGePoint3dArray ptArr;
    
    //Recuperer la matrice de transformation du bloc
    AcGeMatrix3d blockMatrix = block3D->blockTransform();
    
    //Recuperer les quatres points
    AcGePoint3d pt1 = vecPt[0];
    AcGePoint3d pt2 = vecPt[1];
    AcGePoint3d pt3 = vecPt[2];
    AcGePoint3d pt4 = vecPt[3];
    
    //Recuperer la matrice de tranformation du bloc
    blockMatrix = block3D->blockTransform();
    
    //On recupere le premier point
    if( pt1.x != -2 && pt1.y != -2
        && pt2.x != -2 && pt2.y != -2
        && pt3.x != -2 && pt3.y != -2
        && pt4.x != -2 && pt4.y != -2 )
    {
        //Ajouter le point dans le vecteur
        ptArr.push_back( pt1.transformBy( blockMatrix ) );
        
        //Ajouter le points dans le vecteur
        ptArr.push_back( pt2.transformBy( blockMatrix ) );
        
        //Ajouter le point dans le vecteur
        ptArr.push_back( pt3.transformBy( blockMatrix ) );
        
        //Ajouter le point dans le vecteur
        ptArr.push_back( pt4.transformBy( blockMatrix ) );
        
        //Créer la polyligne 3d
        AcDb3dPolyline* poly3D = new AcDb3dPolyline( AcDb::k3dSimplePoly, ptArr );
        
        //Fermer la polyligne
        poly3D->makeClosed();
        
        //Ajouter la polyligne dans le modelspace
        addToModelSpace( poly3D );
        
        //Setter la calque de la polyligne
        poly3D->setLayer( blockLayer );
        
        //Fermer la polyligne
        poly3D->close();
    }
    
    //Sinon on explose le block et on trace les cercles
    else
    {
        //Pointeur sur les entités
        AcDbVoidPtrArray ptrEntityArray;
        
        //Exploser le bloc
        block3D->explode( ptrEntityArray );
        
        //Taille
        int lenEnt = ptrEntityArray.length();
        
        //On parcourt les entités après explosion
        for( int j = 0; j < lenEnt; j++ )
        {
            //Cast de l'entité courante
            AcDbEntity* ent = AcDbEntity::cast( ( AcRxObject* )ptrEntityArray[j] );
            
            //Si l'entité est une polyligne 2d
            if( ent->isKindOf( AcDbCircle::desc() ) )
            {
                //On cast en polyligne
                AcDbCircle* circ = AcDbCircle::cast( ent );
                
                //Verifier
                if( !circ )
                    continue;
                    
                //Ajouter le cercle dans le modelspace
                addToModelSpace( circ );
                
                //Setter le calque du cercle
                circ->setLayer( blockLayer );
                
                //Fermer le cercle
                circ->close();
            }
        }
    }
    
    //Retourner eOk
    return Acad::eOk;
}


vector<string> getSortedFileNamesInFolder( const AcString& folderFiles )
{
    //Resultat
    vector<string> res;
    
    struct dirent** namelist;
    int n, i;
    
    n = scandir( acStrToStr( folderFiles ).c_str(), &namelist, 0, versionsort );
    
    if( n < 0 )
        perror( "scandir" );
    else
    {
        for( i = 0; i < n; ++i )
        {
            res.push_back( namelist[i]->d_name );
            delete namelist[i];
        }
        
        delete namelist;
    }
    
    return res;
}


AcString getDalleName( AcDbPolyline* poly )
{
    //Selection sur les textes
    ads_name ssText;
    
    //Recuperer tous les textes du dessin
    long lenTxt = getSsAllText( ssText );
    
    //Verifier
    if( lenTxt == 0 )
    {
        //Afficher message
        print( "Aucun texte dans le dessin" );
        return _T( "Introuvable" );
    }
    
    //Texte
    AcDbText* txt = NULL;
    
    //Boucle sur les textes
    for( int i = 0; i < lenTxt; i++ )
    {
        //Recuperer le ieme texte
        txt = getTextFromSs( ssText, i, AcDb::kForRead );
        
        //Tester si le texte est dans la polyligne
        if( isPointInPoly( poly, getPoint2d( txt->position() ) ) )
        {
            acedSSFree( ssText );
            return txt->textString();
        }
    }
    
    acedSSFree( ssText );
    return _T( "Introuvable" );
}


bool copyFileTo( const AcString& sourceFile, const AcString& destinationFile )
{
    char buf[BUFSIZ];
    size_t size;
    
    FILE* source = fopen( acStrToStr( sourceFile ).c_str(), "rb" );
    FILE* dest = fopen( acStrToStr( destinationFile ).c_str(), "wb" );
    
    while( size = fread( buf, 1, BUFSIZ, source ) )
        fwrite( buf, 1, size, dest );
        
    fclose( source );
    fclose( dest );
    
    return true;
}


bool getDalleInfos( vector<DallePoly>& dallePolyVec )
{
    //Selection sur les polylignes
    ads_name ss_Poly;
    
    //Recuperer tous les polylignes 2d fermées
    long lenPoly2d = getSsAllPoly2D( ss_Poly, _T( "" ) );
    
    //Verifier
    if( lenPoly2d == 0 )
        return false;
        
    //Polyligne
    AcDbPolyline* poly = NULL;
    
    //Boucler sur les polylignes
    for( int i = 0; i < lenPoly2d; i++ )
    {
        //Recuperer le i-eme polyligne
        poly = getPoly2DFromSs( ss_Poly, i );
        
        if( !poly )
            continue;
            
        //Recuperer le calque de la polyligne
        AcDbLayerTableRecord* layerPoly = NULL;
        getLayer( layerPoly, poly->layer() );
        
        //Ne prendre que les calques non gelés
        if( layerPoly->isOff() )
        {
            layerPoly->close();
            continue;
        }
        
        layerPoly->close();
        
        //Recuperer la polyligne
        DallePoly dalle;
        dalle.dalleName = poly->layer();
        dalle.idPoly = poly->id();
        
        dallePolyVec.push_back( dalle );
    }
    
    return true;
}


void divPcj()
{
    // Sélectionner le pcj
    AcString pcjPath = askForFilePath( true, _T( "pcj" ), _T( "Sélectionner le pcj" ) );
    
    //Verifier
    if( pcjPath == _T( "" ) )
    {
        //Afficher message
        print( "Entrée Vide" );
        
        return;
    }
    
    AcString dirPath = getFileDir( pcjPath );
    
    // Récupérer toutes les polylignes du dessin
    ads_name ssPoly;
    int size = getSsAllPoly2D( ssPoly );
    
    if( size == 0 )
    {
        print( "Le linéaire ne contient aucune polyligne, fin de la commande." );
        return;
    }
    
    double value = 20.0;
    
    if( askForDouble( _T( "Distance minimale entre pcd et linéaire:" ), value, value ) != RTNORM )
    {
    
        print( "Commande annulée." );
        return;
    }
    
    // Nombre de lignes du pcj pour la progress bar
    int nbOfLines = getNumberOfLines( acStrToStr( pcjPath ) );
    
    //Variable pour le pcj
    std::ifstream input( acStrToStr( pcjPath ) );
    std::string line;
    
    //Entete du fichier
    vector<string> headers;
    
    //Vecteur qui va contenir les information sur le pcd
    vector<PcdInfos> pcdInfosVec;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Préparation des pcd" ), nbOfLines );
    int ppp = 0;
    
    //Boucle pour lire le fichier pcj
    while( getline( input, line ) )
    {
        //Si on n'est pas sur un fichier pcd
        if( line.find( ".pcd" ) == std::string::npos )
        {
            //Ajouter la ligne dans l'entete
            headers.push_back( line );
        }
        
        //Sinon
        else
        {
            //Splitter la ligne
            vector<string> splittedLine;
            splittedLine = splitString( line, "\t" );
            
            //Recuperer les informations sur le pcd
            PcdInfos inf;
            inf.pcdName = strToAcStr( splittedLine[0] );
            inf.pcdCenter = AcGePoint3d( ( stof( splittedLine[1] ) + stof( splittedLine[2] ) ) / 2,
                    ( stof( splittedLine[3] ) + stof( splittedLine[4] ) ) / 2,
                    0 );
            inf.pcdLine = strToAcStr( line );
            
            //Ajouter l'information dans le vecteur
            pcdInfosVec.push_back( inf );
        }
        
        //Progresser
        prog.moveUp( ppp++ );
    }
    
    //Fermer le fichier pcj
    input.close();
    
    //Recuperer la taille des pcdInfos
    long lenPcdInfos = pcdInfosVec.size();
    
    //Vecteur de structure DivPcd
    vector<DivPcd> vecResult;
    
    //Polyligne
    AcDbPolyline* poly = NULL;
    
    //Boucle sur les polylignes pour remplir le vecteur de nom du DivPcd
    for( int p = 0; p < size; p++ )
    {
        //Recuperer le p-eme polyligne
        poly = getPoly2DFromSs( ssPoly, p );
        
        //Verifier la polyligne
        if( !poly )
            continue;
            
        //Recuperer le calque de la polyligne
        AcString polyLayer = poly->layer();
        
        if( vecResult.size() == 0 )
        {
            DivPcd temp;
            temp.blocName = polyLayer;
            
            //Creer le dossier
            string folderName = acStrToStr( dirPath ) + "\\" + acStrToStr( polyLayer );
            createFolder( folderName );
            
            //Setter le chemin vers le pcj du divPcd
            temp.pcjPath = strToAcStr( folderName ) + _T( "\\" ) + polyLayer + _T( ".pcj" );
            
            //Ajouter le calque
            vecResult.push_back( temp );
            
            //Fermer la polyligne
            poly->close();
            
            continue;
        }
        
        //Bool
        bool find = false;
        
        //Boucle de verification
        for( int v = 0; v < vecResult.size(); v++ )
        {
            //Creer un DivPcd
            DivPcd temp;
            
            //Verifier le nom du bloc et le calque de la polyligne
            if( vecResult[v].blocName == polyLayer )
            {
                find = true;
                break;
            }
        }
        
        //On ajout l'information
        if( !find )
        {
            DivPcd temp;
            
            //Setter le calque du divPcd
            temp.blocName = polyLayer;
            
            //Creer le dossier
            string folderName = acStrToStr( dirPath ) + "\\" + acStrToStr( polyLayer );
            createFolder( folderName );
            
            //Setter le chemin vers le pcj du divPcd
            temp.pcjPath = strToAcStr( folderName ) + _T( "\\" ) + polyLayer + _T( ".pcj" );
            
            //Ajouter le calque
            vecResult.push_back( temp );
            
            //Fermer la polyligne
            poly->close();
            
        }
    }
    
    //Recuperer la taille du DivPcd
    long lenDivPcd = vecResult.size();
    
    //Barre de progression
    ProgressBar prog2 = ProgressBar( _T( "Préparation des pcd" ), lenDivPcd );
    
    //Boucle sur le vecteur de DivPcd
    for( int i = 0; i < lenDivPcd; i++ )
    {
        //Boucle sur les polylignes
        for( int p = 0; p < size; p++ )
        {
            //Recuperer le p-eme polyligne
            poly = getPoly2DFromSs( ssPoly, p );
            
            //Verifier la polyligne
            if( !poly )
                continue;
                
            //Verifier le calque de la polyligne
            if( vecResult[i].blocName != poly->layer() )
            {
                //Fermer la polyligne
                poly->close();
                
                //continuer
                continue;
            }
            
            //Boucle sur les lignes du fichier pcj
            for( int j = 0; j < lenPcdInfos; j++ )
            {
                //Point projeté
                AcGePoint3d ptProj;
                
                //Recuperer le j-eme pcdinfos
                PcdInfos pcdInfosTemp = pcdInfosVec[j];
                
                //Projeter le point central du pcd sur le vecteur
                if( Acad::eOk != poly->getClosestPointTo( pcdInfosTemp.pcdCenter,
                        ptProj ) )
                {
                    //Afficher message
                    print( "Impossible de Calculer la distance" );
                    
                    continue;
                }
                
                //Recupere la distance entre le point projete sur la polyligne et le point
                double distance = getDistance2d( pcdInfosTemp.pcdCenter, ptProj );
                
                //Si le pcd est dans la bonne distance
                if( distance <= value )
                {
                    //Nom du pcd
                    AcString pcdname = pcdInfosTemp.pcdName;
                    
                    bool find = false;
                    
                    //Verifier si on n'a pas déja ce pcd
                    for( int mm = 0; mm < vecResult[i].pcdVectorName.size(); mm++ )
                    {
                        if( vecResult[i].pcdVectorName[mm] == pcdname )
                        {
                            find = true;
                            break;
                        }
                    }
                    
                    if( !find )
                    {
                        //Ajouter le nom du pcd dans le vecteur
                        vecResult[i].pcdVectorName.push_back( pcdname );
                        
                        //Recuperer le nom du pcd en rar
                        pcdname.replace( _T( ".pcd" ), _T( ".rar" ) );
                        
                        //Ajouter le nom du pcd dans le vecteur
                        vecResult[i].rarVectorName.push_back( pcdname );
                        
                        //Ajouter la ligne du fichier
                        vecResult[i].pcjLine.push_back( pcdInfosTemp.pcdLine );
                    }
                }
            }
            
            //Fermer la polyligne
            poly->close();
        }
        
        //Progresser
        prog2.moveUp( i );
    }
    
    //Recuperer la taille des headers
    long lenHead = headers.size();
    
    //Barre de progression
    ProgressBar copyProg = ProgressBar( _T( "Copie des pcd" ), lenDivPcd );
    
    //Boucle pour copier les pcds
    for( int i = 0; i < lenDivPcd; i++ )
    {
        //Recuperer le ieme divpcd
        DivPcd temp = vecResult[i];
        
        //Creer le fichier pcj
        std::ofstream outfile( acStrToStr( temp.pcjPath ) );
        
        //Boucle pour ajouter les entetes
        for( int e = 0; e < lenHead; e++ )
        {
            //Ecrire les entetes
            outfile << headers[e];
            
            //Ajouter les à la ligne
            if( e != lenHead - 1 )
                outfile << "\n";
        }
        
        //Recuperer le nombre de pcj
        int pcjLineSize = temp.pcjLine.size();
        
        //Ajouter les pcd dans le fichier pcj
        for( int k = 0; k < pcjLineSize; k++ )
        {
            //Ecrire dans le pcj
            outfile << "\n" << acStrToStr( temp.pcjLine[k] );
        }
        
        //Recuperer la taille des pcd du bloc
        long lenPcd = temp.pcdVectorName.size();
        
        //Boucle pour copier les fichiers
        for( int p = 0; p < lenPcd; p++ )
        {
            // Verifier si le fichier pcd existe on le copie
            if( isFileExisting( dirPath + _T( "\\" ) + temp.pcdVectorName[p] ) )
            {
                //Copie du PCD dans le nouveau dossier
                copyFileTo( dirPath + _T( "\\" ) + temp.pcdVectorName[p],
                    dirPath + _T( "\\" ) + temp.blocName + _T( "\\" ) + temp.pcdVectorName[p] );
            }
            
            // Sinon on cherche le fichier rar
            else if( isFileExisting( dirPath + _T( "\\" ) + temp.rarVectorName[p] ) )
            {
                //Copie du rar dans le nouveau dossier
                copyFileTo( dirPath + _T( "\\" ) + temp.rarVectorName[p],
                    dirPath + _T( "\\" ) + temp.blocName + _T( "\\" ) + temp.rarVectorName[p] );
            }
        }
        
        //Fermer le fichier
        outfile.close();
        
        //Progresser
        copyProg.moveUp( i );
    }
}

void divPcjXlsx()
{
    // Sélectionner le pcj
    AcString pcjPath = askForFilePath( true, _T( "xlsx" ), _T( "Sélectionner le pcj" ) );
    
    //Verifieri
    if( pcjPath == _T( "" ) )
    {
        //Afficher message
        print( "Entrée vide" );
        
        return;
    }
    
    AcString dirPath = getFileDir( pcjPath );
    
    // Récupérer toutes les polylignes du dessin
    ads_name ssPoly;
    int size = getSsAllPoly2D( ssPoly );
    
    if( size == 0 )
        print( "Le linéaire ne contient aucune polyligne, fin de la commande." );
        
    // Nombre de lignes du pcj pour la progress bar
    int nbOfLines = getNumberOfLines( acStrToStr( pcjPath ) );
    
    xlnt::workbook excel;
    xlnt::workbook excelTarget;
    excel.load( acStrToStr( pcjPath ) );
    std::vector<std::string> headers;
    
    // Stockage des nouveaux noms de pcj
    std::vector<AcString> pcjNames;
    std::vector<AcString>::iterator it;
    
    // Barre de progression
    ProgressBar bar( _T( "Découpage du PCJ: " ), nbOfLines );
    
    //Boucler sur les feuille
    for( xlnt::worksheet sheet : excel )
    {
        string sheetTitle = sheet.title();
        
        if( sheetTitle == "PCD" )
        {
            for( auto row : sheet.rows() )
            {
                int col = 0;
                string name, point, xmin, ymin, zmin,
                       xmax, ymax, zmax;
                       
                for( auto cell : row )
                {
                    if( cell.to_string().find( ".pcd" ) > 0 && col == 0 )
                        name = cell.to_string();
                    else if( col = 1 )
                        point = cell.to_string();
                    else if( col = 2 )
                        xmin = cell.to_string();
                    else if( col = 3 )
                        ymin = cell.to_string();
                    else if( col = 4 )
                        zmin = cell.to_string();
                    else if( col = 5 )
                        xmax = cell.to_string();
                    else if( col = 6 )
                        ymax = cell.to_string();
                    else if( col = 1 )
                        zmax = cell.to_string();
                        
                    col++;
                }
                
                if( !name.empty() )
                {
                    // On cherche le bloc le plus proche
                    double distance = 1000000000000000;
                    AcString layerPoly;
                    
                    for( int id = 0; id < size; id++ )
                    {
                        // Caste l'entité en polyligne
                        AcDbPolyline* poly = getPoly2DFromSs( ssPoly, id );
                        
                        if( !poly )
                        {
                            poly->close();
                            continue;
                        }
                        
                        // Verifier le calque de la polyligne
                        AcString layer = poly->layer();
                        
                        // Recuperer le point le plus proche de la pointe de la flèche
                        AcGePoint3d ptOnCurve;
                        AcGePoint3d boxCenter = AcGePoint3d( ( stof( xmin ) + stof( xmax ) ) / 2, ( stof( ymin ) + stof( ymax ) ) / 2, ( ( ( stof( zmin ) ) + stof( zmax ) ) / 2 ) );
                        poly->getClosestPointTo( boxCenter, ptOnCurve );
                        
                        // Recuperer la distance entre ce point et le point du mlead
                        double dist = getDistance2d( boxCenter, ptOnCurve );
                        
                        // Comparer la distance à la distance precedente
                        if( distance >= dist )
                        {
                            distance = dist;
                            layerPoly = poly->layer();
                        }
                        
                        poly->close();
                    }
                }
            }
        }
    }
}


AcDbText* getTopoAttr( const AcDbVoidPtrArray& entityArray )
{
    //Recuperer la taille
    int size = entityArray.size();
    
    //Resultat
    AcDbText* text = NULL;
    
    //Boucle sur les entités du tableau
    for( int iEnt = 0; iEnt < size; iEnt++ )
    {
        //Recuperer l'entité
        AcDbEntity* ent = AcDbEntity::cast( ( AcRxObject* )entityArray[iEnt] );
        
        //Verifier
        if( !ent )
            continue;
            
        //Verifier si l'entité est un texte
        if( ent->isKindOf( AcDbText::desc() ) )
        {
            text = AcDbText::cast( ent );
            
            if( !text )
            {
                //ent->close();
                continue;
            }
        }
        
        //else
        //ent->close();
    }
    
    //Sinon retourner un pointeur NULL
    if( text )
        return text;
    else
        return NULL;
}

/*
    Sous fonction pour la Com: BIMETIQUETER
*/

void askFileInfos( string& _filename, string& _config_filename, double& _tolerance )
{

    //Déclaration des variables
    AcString file;
    AcString config;
    
    //Démander à l'utilisateur la tolérance
    if( RTCAN == askForDouble( _T( "Veuillez entrer la tolérance: " ), _tolerance, _tolerance ) )
    {
        print( "Tolerance invalide" );
        print( "commande annulée" );
        _tolerance = 0;
        return;
    }
    
    //Démander à l'utilisateur le fichier excel
    file = askForFilePath( false,
            "xlsx",
            "Sélectionner le fichier Excel à remplir (Veuillez noter que le chemin et le fichier excel ne doivent pas contenir des espaces ni accents)",
            "D:\\" );
            
    //Verifier la validité du fichier
    if( file == _T( "" ) )
    {
        print( "Impossible de lire le fichier" );
        print( "Commande annulée" );
        _config_filename = "";
        return;
    }
    
    //Changer filename en string
    _filename = acStrToStr( file );
    
    //Nettoyer le nom du fichier "\" --> "/"
    int n_occurence = _filename.length();
    
    for( int i = 0; i < n_occurence; i++ )
    {
        if( _filename[i] == '\\' )
            _filename[i] = '/';
    }
    
    //Démander à l'utilisateur le fichier de correspondance (Configuration)
    config = askForFilePath( true,
            "xlsx",
            "Selectionner le fichier Excel de configuration (Veuillez noter que le chemin et le fichier excel ne doivent pas contenir des espaces ni accents)",
            "D:\\" );
            
    //Verifier la validité du fichier
    if( config == _T( "" ) )
    {
        print( "Impossible de lire le fichier" );
        print( "Commande annulée" );
        return;
    }
    
    //Changer filename en string
    _config_filename = acStrToStr( config );
    
    //Nettoyer le nom du fichier "\" --> "/"
    int n_occ = _config_filename.length();
    
    for( int i = 0; i < n_occ; i++ )
    {
        if( _config_filename[i] == '\\' )
            _config_filename[i] = '/';
    }
    
}

/*
    Informations sur le fichier de configuration Com: BIMETIQUETER
*/
void getConfigInfos(
    string _config_path,
    map<string, vector<string>>& _vec_config
)
{
    //Déclaration des variables pour le traitement du fichier de configuration
    map<string, vector<string>>::iterator it = _vec_config.begin();
    string c_famille;
    string c_temp_famille;
    string c_blockName;
    string c_temp_blockName;
    
    vector<string> temp_vec;
    
    xlnt::workbook wb_config;
    
    try
    {
        wb_config.load( _config_path );
    }
    
    catch( const std::exception& e )
    {
        print( "Impossible de lire le fichier de configuration" );
        return;
    }
    
    
    //Acquierir les données de configurations famille -> Nom du block
    xlnt::worksheet ws_config = wb_config.active_sheet();
    
    string conf = ws_config.cell( "A2" ).to_string();
    int j = 2;
    
    while( conf.compare( "" ) != 0 )
    {
        c_famille = ws_config.cell( "A", j ).to_string();
        c_blockName = ws_config.cell( "B", j ).to_string();
        
        it = _vec_config.find( c_famille );
        
        if( it != _vec_config.end() )
            it->second.push_back( c_blockName );
        else
        {
            temp_vec.push_back( c_blockName );
            _vec_config.insert( make_pair( c_famille, temp_vec ) );
        }
        
        //nettoyage du vecteur temp_vec;
        temp_vec.clear();
        
        //incrementer la colonne
        j++;
        
        //Avancer la boucle
        conf = ws_config.cell( "A", j ).to_string();
    }
    
    
    try
    {
        wb_config.save( _config_path );
    }
    
    catch( const std::exception& e )
    {
        print( "Impossible de sauvegarder les modifications." );
        return;
    }
}



Acad::ErrorStatus sytralSeuil( long& nbErreur,
    const ads_name& ssblockSeuil,
    const ads_name& ssblockPTopo,
    const ads_name& sspoly3d )
{
    long lnblockSeuil( 0 ), lnblockPtopo( 0 ), lnpoly3d( 0 );
    
    // Recuperer la selection
    if( acedSSLength( ssblockSeuil, &lnblockSeuil ) != RTNORM
        || lnblockSeuil == 0
        ||  acedSSLength( ssblockPTopo, &lnblockPtopo ) != RTNORM
        || lnblockPtopo == 0 ||
        acedSSLength( sspoly3d, &lnpoly3d ) != RTNORM
        || lnpoly3d == 0 )
    {
        print( "Sélection vide." );
        return eNotApplicable;
    }
    
    vector<AcGePoint3d> ptArraySeuil, ptArrayPtopo;
    
    // Parcourir les blocs ptopos et recuperer les points d'insertion
    for( int iSeuil = 0; iSeuil < lnblockSeuil; iSeuil++ )
    {
        AcDbBlockReference* blockSeuil = getBlockFromSs( ssblockSeuil, iSeuil );
        
        // Verifier le bloc seuil
        if( !blockSeuil )
            continue;
            
        // Ajouter son point d'insertion dans le tableau
        ptArraySeuil.emplace_back( blockSeuil->position() );
        
        // Fermer le bloc seuil
        blockSeuil->close();
    }
    
    // Parcourir les blocs pttopo des blocs seuils
    for( int iPTopo = 0; iPTopo < lnblockPtopo; iPTopo++ )
    {
        AcDbBlockReference* blockPTopo = getBlockFromSs( ssblockPTopo, iPTopo );
        
        // Verifier le bloc
        if( !blockPTopo )
            continue;
            
        // Recuperer la position du bloc et l'ajouter dans le tableau
        ptArrayPtopo.emplace_back( blockPTopo->position() );
        
        // Fermer le bloc pTopo
        blockPTopo->close();
    }
    
    // Parcourir le point d'insertion du bloc seuil
    int szPosSeuil = ptArraySeuil.size();
    int szPosPtTopo = ptArrayPtopo.size();
    ProgressBar prog = ProgressBar( _T( "Contrôle des seuils:" ), lnblockSeuil );
    
    
    
    for( int i = 0; i < szPosSeuil; i++ )
    {
        prog.moveUp( i );
        AcGePoint3d posSeuil = ptArraySeuil[i];
        bool isOk = false;
        
        for( int j = 0; j < szPosPtTopo; j++ )
        {
            AcGePoint3d posPtTopo = ptArrayPtopo[j];
            
            if( isEqual3d( posPtTopo, posSeuil ) )
            {
                isOk = true;
                break;
            }
        }
        
        // S'il n'y a pas de points topo sur le bloc on met de patates
        if( !isOk )
        {
            nbErreur++;
            drawCircle( posSeuil, 2, "SEUIL_ERREUR" );
            ///continue;
            
        }
        
        double distance = 100000;
        AcGePoint3d ptProj = AcGePoint3d::kOrigin;
        
        // On parcours les polylignes contours de batiments
        for( int iPoly = 0; iPoly < lnpoly3d; iPoly++ )
        {
            // Recuperer la polyligne la plus proche du bloc seuil
            AcDb3dPolyline* poly3d = getPoly3DFromSs( sspoly3d, iPoly );
            
            if( !poly3d )
                continue;
                
            AcGePoint3d ptClosest = AcGePoint3d::kOrigin;
            
            Acad::ErrorStatus es = poly3d->getClosestPointTo( posSeuil, ptClosest );
            
            // On arrive pas à recuperer le point la plus proche
            if( es != eOk )
            {
                poly3d->close();
                continue;
            }
            
            double dist = getDistance2d( posSeuil, ptClosest );
            
            if( distance > dist )
            {
                distance = dist;
                ptProj = ptClosest;
            }
            
            // Fermer la polyligne
            poly3d->close();
        }
        
        
        // Verifier l'altitude des deux blocs
        if( ptProj.z > posSeuil.z )
        {
            drawCircle( posSeuil, 4, "SEUIL_ERREUR" );
            nbErreur++;
        }
    }
    
    return eOk;
}


Acad::ErrorStatus sytralSeuilControl( long& nbErreur,
    const ads_name& ssblockSeuil,
    const ads_name& ssblockPTopo,
    const ads_name& sspoly3d )
{
    long lnblockSeuil( 0 ), lnblockPtopo( 0 ), lnpoly3d( 0 );
    
    // Recuperer la selection
    if( acedSSLength( ssblockSeuil, &lnblockSeuil ) != RTNORM
        || lnblockSeuil == 0
        || acedSSLength( ssblockPTopo, &lnblockPtopo ) != RTNORM
        || lnblockPtopo == 0 ||
        acedSSLength( sspoly3d, &lnpoly3d ) != RTNORM
        || lnpoly3d == 0 )
    {
        print( "Sélection vide." );
        return eNotApplicable;
    }
    
    vector<AcGePoint3d> ptArraySeuil, ptArrayPtopo;
    
    // Parcourir les blocs ptopos et recuperer les points d'insertion
    for( int iSeuil = 0; iSeuil < lnblockSeuil; iSeuil++ )
    {
        AcDbBlockReference* blockSeuil = getBlockFromSs( ssblockSeuil, iSeuil );
        
        // Verifier le bloc seuil
        if( !blockSeuil )
            continue;
            
        // Ajouter son point d'insertion dans le tableau
        ptArraySeuil.emplace_back( blockSeuil->position() );
        
        // Fermer le bloc seuil
        blockSeuil->close();
    }
    
    // Parcourir les blocs pttopo des blocs seuils
    for( int iPTopo = 0; iPTopo < lnblockPtopo; iPTopo++ )
    {
        AcDbBlockReference* blockPTopo = getBlockFromSs( ssblockPTopo, iPTopo );
        
        // Verifier le bloc
        if( !blockPTopo )
            continue;
            
        // Recuperer la position du bloc et l'ajouter dans le tableau
        ptArrayPtopo.emplace_back( blockPTopo->position() );
        
        // Fermer le bloc pTopo
        blockPTopo->close();
    }
    
    // Parcourir le point d'insertion du bloc seuil
    int szPosSeuil = ptArraySeuil.size();
    int szPosPtTopo = ptArrayPtopo.size();
    ProgressBar prog = ProgressBar( _T( "Contrôle des seuils:" ), lnblockSeuil );
    
    
    
    for( int i = 0; i < szPosSeuil; i++ )
    {
        prog.moveUp( i );
        AcGePoint3d posSeuil = ptArraySeuil[i];
        bool isOk = false;
        
        for( int j = 0; j < szPosPtTopo; j++ )
        {
            AcGePoint3d posPtTopo = ptArrayPtopo[j];
            
            if( isEqual3d( posPtTopo, posSeuil ) )
            {
                isOk = true;
                break;
            }
        }
        
        // S'il n'y a pas de points topo sur le bloc on met de patates
        if( !isOk )
        {
            nbErreur++;
            drawCircle( posSeuil, 2, "SEUIL_ERREUR_CONTROL" );
            ///continue;
            
        }
        
        double distance = 100000;
        AcGePoint3d ptProj = AcGePoint3d::kOrigin;
        
        // On parcours les polylignes contours de batiments
        for( int iPoly = 0; iPoly < lnpoly3d; iPoly++ )
        {
            // Recuperer la polyligne la plus proche du bloc seuil
            AcDb3dPolyline* poly3d = getPoly3DFromSs( sspoly3d, iPoly );
            
            if( !poly3d )
                continue;
                
            AcGePoint3d ptClosest = AcGePoint3d::kOrigin;
            
            Acad::ErrorStatus es = poly3d->getClosestPointTo( posSeuil, ptClosest );
            
            // On arrive pas à recuperer le point la plus proche
            if( es != eOk )
            {
                poly3d->close();
                continue;
            }
            
            double dist = getDistance2d( posSeuil, ptClosest );
            
            if( distance > dist )
            {
                distance = dist;
                ptProj = ptClosest;
            }
            
            // Fermer la polyligne
            poly3d->close();
        }
        
        
        // Verifier l'altitude des deux blocs
        if( abs( ptProj.z - posSeuil.z ) > 0.05 )
        {
            if( ptProj.z > posSeuil.z )
                drawCircle( posSeuil, 4, "SEUIL_ERREUR_CONTROL", 1 );
            else
                drawCircle( posSeuil, 4, "SEUIL_ERREUR_CONTROL", 6 );
                
            nbErreur++;
        }
    }
    
    return eOk;
}


Acad::ErrorStatus sytralTopo2( long& nbSommets,
    const ads_name& sspoly3d,
    const ads_name& ssblockPTopo )
{
    long lnPoly3d( 0 ), lnBlock( 0 );
    
    // Recuperer les sélections
    if( acedSSLength( sspoly3d, &lnPoly3d ) != RTNORM || lnPoly3d == 0
        || acedSSLength( ssblockPTopo, &lnBlock ) != RTNORM || lnBlock == 0 )
        return eNotApplicable;
        
    AcGePoint3dArray ptsTopo;
    vector<double> vecX;
    
    // Parcourir les blocs pt et recuperer leur positions
    if( eOk != getInsertPointArrayOfSSBlock( ssblockPTopo, ptsTopo, vecX ) )
        return eNotApplicable;
        
        
    //Tester qu'on a des points dans le vecteur
    if( !ptsTopo.length() )
    {
        //Afficher une message
        print( "Aucun point d'insertion trouvé pour les blocs topos" );
        return eNotApplicable;
    }
    
    // Recupere le bounding box tous les polylignes
    
    // Parcourir les points topos
    long szBlock = ptsTopo.size();
    
    // Declaration des variables utilisé pour isPointOnPoly
    bool isAVertex = false;
    int index = 0;
    
    // ProgressBarr
    ProgressBar prog = ProgressBar( _T( "Contrôle points Topo: " ), lnPoly3d );
    
    for( int i = 0; i < szBlock; i++ )
    {
        prog.moveUp( i );
        
        for( int j = 0; j < lnPoly3d; j++ )
        {
            // Recuperer la ième selection
            AcDb3dPolyline* poly3d = getPoly3DFromSs( sspoly3d, j, AcDb::kForWrite );
            
            if( !poly3d )
                continue;
                
            // Recuperer les sommets de la polylignes
            AcGePoint3dArray ptsVtxs;
            vector<double> xVtxs;
            
            if( eOk != getVertexesPoly( poly3d, ptsVtxs, xVtxs ) )
            {
                poly3d->close();
                continue;
            }
            
            
            
            poly3d->close();
        }
        
    }
    
    // Parcourir les points d'insertions de bloc
    //for( int j = 0; j < lnPoly3d; j++ )
    //{
    //    // Recuperer la ième selection
    //    AcDb3dPolyline* poly3d = getPoly3DFromSs( sspoly3d, j, AcDb::kForWrite );
    //
    //    if( !poly3d )
    //        continue;
    //
    //    // Recuperer les sommets de la polylignes
    //    AcGePoint3dArray ptsVtxs;
    //    vector<double> xVtxs;
    //
    //    if( eOk != getVertexesPoly( poly3d, ptsVtxs, xVtxs ) )
    //    {
    //        poly3d->close();
    //        continue;
    //    }
    //
    //  poly3d->close();
    //
    
    
    
    // for( int j = 0; j < szBlock; j++ )
    // {
    //     AcGePoint3d pt = ptsTopo[j];
    //
    //     // Verifier si le point topo est sur le sommet de la polyligne
    //     bool isOnPoly = true;
    //
    //     if( eOk != isPointOnPoly( poly3d, pt, isOnPoly, 0.001 ) || isOnPoly )
    //         continue;
    //
    //
    //     // Verifier la distance entre le point topo et la polyligne
    //     AcGePoint3d ptClosest;
    //
    //     if( eOk != poly3d->getClosestPointTo( pt, ptClosest ) )
    //         continue;
    //
    //     double dist = getDistance2d( pt, ptClosest );
    //
    //     if( dist > 0.02 )
    //         continue;
    //
    //
    //
    //     double distance = 10000;
    //
    //     // Recuperer la distance curviligne du point projeté du point topo
    //     //if( eOk != poly3d->getDistAtPoint( ptClosest, distance ) )
    //     //  continue;
    //
    //     // Parcourir chaque sommet de la polyligne
    //     int index = -1;
    //     int szVtx = ptsVtxs.size();
    //
    //     if( szVtx == 0 )
    //     {
    //         poly3d->close();
    //         continue;
    //     }
    //
    //     double diff = 1000;
    //     bool isPos = false;
    //     AcGePoint3d ptCur;
    //
    //     for( int idx = 0; idx < szVtx; idx++ )
    //     {
    //         double distCurr =  getDistance2d( ptsVtxs[idx], ptClosest );
    //
    //
    //         // poly3d->getDistAtPoint( ptsVtxs[idx], distCurr );
    //         //double diffCurr = abs( distance - distCurr );
    //
    //         if( distance > distCurr )
    //         {
    //             distance = distCurr;
    //             index = idx;
    //             ptCur = ptsVtxs[idx];
    //         }
    //     }
    //
    //     if( index !=  -1 )
    //     {
    //         double distPoly( 0.0 ), distTopo( 0.0 );
    //         poly3d->getDistAtPoint( ptCur, distPoly );
    //         poly3d->getDistAtPoint( ptClosest, distTopo );
    //
    //         if( distTopo > distPoly )
    //             index = index + 1;
    //
    //
    //         ptsVtxs.insertAt( index, pt );
    //
    //         // Incrementer le nombre de sommets ajoutés
    //         nbSommets++;
    //
    //         ptsTopo.removeAt( j );
    //     }
    //
    //
    // }
    //
    // // Retracer la polyligne
    // AcDb3dPolyline* newpolyline = new AcDb3dPolyline( poly3d->polyType(), ptsVtxs, poly3d->isClosed() );
    //
    // addToModelSpace( newpolyline );
    //
    // if( newpolyline->id() == AcDbObjectId::kNull )
    // {
    //     poly3d->close();
    //     continue;
    // }
    //
    // newpolyline->setPropertiesFrom( poly3d );
    //
    // newpolyline->close();
    //
    //
    // // Fermer la polyligne
    // poly3d->erase();
    //poly3d->close();
    
    //}
    
    
    // Renvoyer le résultat
    return eOk;
}


Acad::ErrorStatus sytralGrille( long& nbErreur,
    const ads_name& ssblock,
    const ads_name& sstopo )
{
    // Recuperer les selections
    
    long lnblock( 0 ), lnPtopo( 0 );
    
    // Recuperer les sélections
    if( acedSSLength( ssblock, &lnblock ) != RTNORM || lnblock == 0
        || acedSSLength( sstopo, &lnPtopo ) != RTNORM || lnPtopo == 0 )
        return eNotApplicable;
        
    // Création de calque
    AcGePoint3dArray ptarray;
    vector<double> vecX;
    
    if( eOk != getInsertPointArrayOfSSBlock( sstopo, ptarray, vecX ) )
        return eNotApplicable;
        
    if( ptarray.size() == 0 )
        return eNotApplicable;
        
        
    // Parcourir les blocs grilles
    for( int iblock = 0; iblock < lnblock; iblock++ )
    {
        AcDbBlockReference* block = getBlockFromSs( ssblock, iblock );
        
        if( !block )
            continue;
            
        AcGePoint3dArray pt3Darray;
        AcDb3dPolyline* poly = NULL;
        
        if( eOk != getClosedPolyBlock( poly, pt3Darray, block ) )
        {
            block->close();
            continue;
        }
        
        int szvtx = pt3Darray.size();
        bool found = false;
        
        for( int ivtx = 0; ivtx < szvtx; ivtx++ )
        {
        
            found = false;
            AcGePoint3d pt = pt3Darray[ivtx];
            
            int sztopo = ptarray.size();
            
            for( int j = 0; j < sztopo; j++ )
            {
                AcGePoint3d temp = ptarray[j];
                
                if( isEqual2d( pt, temp, 0.001 ) )
                {
                    found = true;
                    break;
                }
            }
            
            if( !found )
                break;
        }
        
        if( !found )
        {
            drawCircle( block->position(), 2, "GRILLE_ERREUR" );
            nbErreur++;
            
        }
        
        delete poly;
        poly = NULL;
        block->close();
    }
    
    // Renvoyer le résultat
    return eOk;
}


Acad::ErrorStatus sytralAvaloir( long& nbErreur,
    const ads_name& ssblock,
    const ads_name& sstopo )
{
    // Recuperer les selections
    
    long lnblock( 0 ), lnPtopo( 0 );
    
    // Recuperer les sélections
    if( acedSSLength( ssblock, &lnblock ) != RTNORM || lnblock == 0
        || acedSSLength( sstopo, &lnPtopo ) != RTNORM || lnPtopo == 0 )
        return eNotApplicable;
        
    // Création de calque
    AcGePoint3dArray ptarray;
    vector<double> vecX;
    
    if( eOk != getInsertPointArrayOfSSBlock( sstopo, ptarray, vecX ) )
        return eNotApplicable;
        
    if( ptarray.size() == 0 )
        return eNotApplicable;
        
        
    // Parcourir les blocs grilles
    for( int iblock = 0; iblock < lnblock; iblock++ )
    {
        AcDbBlockReference* block = getBlockFromSs( ssblock, iblock );
        
        if( !block )
            continue;
            
        AcGePoint3dArray pt3Darray;
        AcDb3dPolyline* poly = NULL;
        
        // Récuperer la polyligne fermée dans la définition du bloc
        if( eOk != getClosedPolyBlock( poly, pt3Darray, block ) )
        {
            block->close();
            continue;
        }
        
        int szvtx = pt3Darray.size();
        
        bool found = false;
        
        // Parcourir les deux premiers sommets de la polyligne
        for( int ivtx = 0; ivtx < 2; ivtx++ )
        {
            found = false;
            AcGePoint3d pt = pt3Darray[ivtx];
            int index = -1;
            
            int sztopo = ptarray.size();
            
            
            for( int j = 0; j < sztopo; j++ )
            {
                AcGePoint3d temp = ptarray[j];
                
                if( isEqual2d( pt, temp, 0.001 ) )
                {
                    found = true;
                    break;
                }
            }
            
            if( !found )
                break;
        }
        
        if( !found )
        {
            drawCircle( block->position(), 2, "AVALOIR_ERREUR" );
            nbErreur++;
            
        }
        
        delete poly;
        poly = NULL;
        block->close();
    }
    
    // Renvoyer le résultat
    return eOk;
}


Acad::ErrorStatus sytralTopo( long& nbSommets,
    const ads_name& sspoly3d,
    const ads_name& ssblockPTopo )
{

    // Calcul de bounding box
    // Initialisation de la matrice
    // Recuperer les sélections blocs points topo et polylignes
    long lnpoly( 0 ),  lnblock( 0 );
    
    if( acedSSLength( sspoly3d, &lnpoly ) != RTNORM
        || lnpoly == 0
        || acedSSLength( ssblockPTopo, &lnblock ) != RTNORM
        || lnblock == 0 )
        return eNotApplicable;
        
        
    // Recuperer les coordonnées des points topo
    AcGePoint3dArray ptsblock;
    vector<double> vecX;
    
    if( eOk != getInsertPointArrayOfSSBlock( ssblockPTopo, ptsblock, vecX ) )
        return eNotApplicable;
        
        
    vector< Poly3dSytral> polyList;
    vector<vector<vector<SytralCell>>> matrice;
    double xmin( 10000000 ), ymin( 10000000 ), zmin( 10000000 ), xmax( 0.0 ), ymax( 0.0 ), zmax( 0.0 );
    
    for( int ipoly = 0; ipoly < lnpoly; ipoly++ )
    {
        AcDb3dPolyline* poly = getPoly3DFromSs( sspoly3d, ipoly );
        
        if( !poly )
            continue;
            
        // Recuperer l'id de la polyligne
        Poly3dSytral polySytral;
        polySytral.idPoly = poly->objectId();
        AcDbExtents box;
        
        // Recuperer la bounding box
        if( eOk != poly->getGeomExtents( box ) )
        {
            poly->close();
            continue;
        }
        
        polySytral.xmin = box.minPoint().x;
        polySytral.ymin = box.minPoint().y;
        polySytral.zmin = box.minPoint().z;
        polySytral.xmax = box.maxPoint().x;
        polySytral.ymax = box.maxPoint().y;
        polySytral.zmax = box.maxPoint().z;
        
        // Recuperer les sommets de la polylignes
        vector<double> xpos;
        
        if( eOk != getVertexesPoly( poly, polySytral.ptList, xpos ) )
        {
            poly->close();
            continue;
        }
        
        double xminTemp = polySytral.xmin;
        double yminTemp = polySytral.ymin;
        double zminTemp = polySytral.zmin;
        double xmaxTemp = polySytral.xmax;
        double ymaxTemp = polySytral.ymax;
        double zmaxTemp = polySytral.zmax;
        
        if( xminTemp < xmin )
            xmin = xminTemp;
            
        if( yminTemp < ymin )
            ymin = yminTemp;
            
        if( zminTemp < zmin )
            zmin = zminTemp;
            
        if( xmaxTemp > xmax )
            xmax = xmaxTemp;
            
        if( ymaxTemp > ymax )
            ymax = ymaxTemp;
            
        if( zmaxTemp > zmax )
            zmax = zmaxTemp;
            
        polyList.emplace_back( polySytral );
        
        // Fermer la polyligne
        poly->close();
    }
    
    int szPoly = polyList.size();
    
    if( szPoly == 0 )
        return eNotApplicable;
        
    // Initalisation de la matrice
    int pas = 25;
    int Xmind = multipleMin( xmin, pas );
    int Xmaxd = multipleMax( xmax, pas );
    int Ymind = multipleMin( ymin, pas );
    int Ymaxd = multipleMax( ymax, pas );
    int Zmind = multipleMin( zmin, pas );
    int Zmaxd = multipleMax( zmax, pas );
    
    AcDbExtents boxMatrice = AcDbExtents( AcGePoint3d( Xmind, Ymind, Zmind ),
            AcGePoint3d( Xmaxd, Ymaxd, Zmaxd ) );
            
    int szX = ( Xmaxd - Xmind ) / pas;
    int szY = ( Ymaxd - Ymind ) / pas;
    int szZ = ( Zmaxd - Zmind ) / pas;
    
    double xminK( Xmind ), yminK( Ymind ), zminK( Zmind ), xmaxK( 0 ), ymaxK( 0 ), zmaxK( 0 );
    
    ProgressBar prog = ProgressBar( _T( "Initialisation des matrices: ", szX ) );
    
    // Initialisation de la polyligne
    for( int i = 0; i < szX; i++ )
    {
        prog.moveUp( i );
        vector<vector<SytralCell>> PolyXlist;
        
        for( int j = 0; j < szY; j++ )
        {
            vector<SytralCell> PolyYlist;
            
            for( int k = 0; k < szZ; k++ )
            {
                xminK = Xmind + ( pas * i );
                xmaxK = xminK + pas;
                yminK = Ymind + ( pas * j );
                ymaxK = yminK + pas;
                zminK = Zmind + ( pas * k );
                zmaxK = zminK + pas;
                
                SytralCell cellI;
                // Calcul de la bounding box
                cellI.xmin = xminK;
                cellI.ymin = yminK;
                cellI.zmin = zminK;
                cellI.xmax = xmaxK;
                cellI.ymax = ymaxK;
                cellI.zmax = zmaxK;
                
                AcDbExtents boxMat = AcDbExtents( AcGePoint3d( xminK, yminK, zminK ),
                        AcGePoint3d( xmaxK, ymaxK, zmaxK ) );
                        
                        
                        
                        
                for( int ipoly = 0; ipoly < szPoly; ipoly++ )
                {
                    Poly3dSytral polyI = polyList[ipoly];
                    AcDbExtents boxPolyI = AcDbExtents( AcGePoint3d( polyI.xmin, polyI.ymin, polyI.zmin ),
                            AcGePoint3d( polyI.xmax, polyI.ymax, polyI.zmax ) );
                            
                    AcDbExtents::IntersectionStatus status = boxPolyI.intersectWith( boxMat );
                    
                    if( status == AcDbExtents::kIntersectNot ||
                        status == AcDbExtents::kIntersectUnknown )
                        continue;
                        
                    cellI.polyList.emplace_back( polyI );
                }
                
                PolyYlist.emplace_back( cellI );
            }
            
            PolyXlist.emplace_back( PolyYlist );
        }
        
        matrice.emplace_back( PolyXlist );
        
    }
    
    
    // Boucle sur les points topo et insertions de vertex
    int sz = ptsblock.size();
    ProgressBar  progBlock = ProgressBar( _T( "Insertion des sommets: " ), sz );
    
    for( int iblock = 0; iblock < sz; iblock++ )
    {
        progBlock.moveUp( iblock );
        AcGePoint3d ptblock = ptsblock[iblock];
        
        // VErifier si le point n'est pas dans la bbox de la matrice
        bool isInsideBox = false;
        
        //Recuperer les deux points limites du box
        AcGePoint3d ptBottomLeft = boxMatrice.minPoint();
        AcGePoint3d ptTopRight = boxMatrice.maxPoint();
        
        //Tester si le point est dans le box
        if( ptblock.x >= ptBottomLeft.x && ptblock.x <= ptTopRight.x
            && ptblock.y >= ptBottomLeft.y && ptblock.y <= ptTopRight.y
            && ptblock.z >= ptBottomLeft.z && ptblock.z <= ptTopRight.z )
            isInsideBox = true;
            
            
        if( !isInsideBox )
            continue;
            
        //  Calculer l'indice en x , y, z la case de la polyligne la plus proche
        int idx( -1 ), idy( -1 ), idz( -1 );
        idx = ( multipleMin( ptblock.x, pas ) - Xmind ) / pas;
        idy = ( multipleMin( ptblock.y, pas ) - Ymind ) / pas;
        idz = ( multipleMin( ptblock.z, pas ) - Zmind ) / pas;
        
        SytralCell cell = matrice[idx][idy][idz];
        int szPolyInCell = cell.polyList.size();
        
        for( int icell = 0; icell < szPolyInCell; icell++ )
        {
            Poly3dSytral polysytral = cell.polyList[icell];
            
            if( polysytral.idPoly == AcDbObjectId::kNull )
                continue;
                
            // Ouvrir la polyligne
            AcDbEntity* ent = NULL;
            
            if( eOk != acdbOpenAcDbEntity( ent, polysytral.idPoly, AcDb::kForWrite ) )
                continue;
                
            AcDb3dPolyline* poly = AcDb3dPolyline::cast( ent );
            
            if( !poly )
            {
                ent->close();
                continue;
            }
            
            // Verifier si le point topo est sur la polylgine
            bool isOnPoly = false;
            
            if( eOk != isPointOnPoly( poly, ptblock, isOnPoly, 0.001 ) || isOnPoly )
            {
                poly->close();
                continue;
            }
            
            // Projeter le point sur la polyligne
            AcGePoint3d ptClosest = AcGePoint3d::kOrigin;
            
            if( eOk != poly->getClosestPointTo( ptblock, ptClosest ) )
            {
                poly->close();
                continue;
            }
            
            // Verifier la distance entre le point et le projet sur la polyligne
            double distOk = getDistance2d( ptblock, ptClosest );
            
            if( distOk > 0.02 )
            {
                poly->close();
                continue;
            }
            
            // Distance curviligne
            double distCurvtopo = 0.0;
            
            if( eOk != poly->getDistAtPoint( ptClosest, distCurvtopo ) )
            {
                poly->close();
                continue;
            }
            
            // Parcourir les sommets de la polyligne
            AcDbObjectIterator* iterpoly = poly->vertexIterator();
            
            if( !iterpoly )
            {
                poly->close();
                continue;
            }
            
            AcDbObjectId idVtx = AcDbObjectId::kNull;
            AcDbObjectId idVtxLast = AcDbObjectId::kNull;
            double diff = 100000;
            
            AcDb3dPolylineVertex* vertex = NULL;
            iterpoly->start();
            
            if( eOk != poly->openVertex( vertex, iterpoly->objectId(), AcDb::kForRead ) )
            {
                poly->close();
                continue;
                
            }
            
            idVtxLast = iterpoly->objectId();
            
            vertex->close();
            iterpoly->step();
            
            for( ; !iterpoly->done(); iterpoly->step() )
            {
            
                if( eOk != poly->openVertex( vertex, iterpoly->objectId(), AcDb::kForRead ) )
                    continue;
                    
                AcGePoint3d pt3d = vertex->position();
                vertex->close();
                
                // Calculer la distance curviligne
                double distCurvtxI = 0.0;
                
                if( eOk != poly->getDistAtPoint( pt3d, distCurvtxI ) )
                    continue;
                    
                if( distCurvtxI < distCurvtopo )
                {
                    idVtxLast = iterpoly->objectId();
                    continue;
                }
                
                break;
            }
            
            
            //Ajouter le nouveau sommet sur la polyligne
            AcDb3dPolylineVertex* newVertex = new AcDb3dPolylineVertex( ptblock );
            AcDbObjectId idnewvtx;
            
            if( eOk != poly->insertVertexAt( idnewvtx, idVtxLast, newVertex ) )
            {
            
                poly->close();
                continue;
            }
            
            newVertex->close();
            nbSommets++;
            
            poly->close();
        }
        
    }
    
    return eOk;
}
void thread_ControlSytralFileDeau( const vector<AcGePoint3d>& ptVec,
    AcGePoint3dArray& vecPatate,
    const ads_name& ssPoly3d,
    const long& polySz,
    const long& vecSize,
    const int& threadNumber,
    const int& threadIterator )
{
    //Polyligne 3d
    AcDb3dPolyline* poly3d = NULL;
    
    //Boucle sur les points
    for( long i = threadNumber; i < vecSize; i += threadIterator )
    {
        //Recuperer le vertex
        AcGePoint3d pt3d = ptVec[i];
        
        //Boucle sur les polylignes
        for( long b = 0; b < polySz; b++ )
        {
            lockThread.lock();
            
            //Recuperer la polyligne
            poly3d = getPoly3DFromSs( ssPoly3d, b, AcDb::kForRead );
            
            //Verifier
            if( !poly3d )
                continue;
                
            //Point Projeter
            AcGePoint3d ptProj;
            
            //Projeter le point
            Acad::ErrorStatus er = poly3d->getClosestPointTo( pt3d, ptProj );
            
            if( er != Acad::eOk )
            {
                lockThread.unlock();
                poly3d->close();
                continue;
            }
            
            //Fermer la polyligne
            poly3d->close();
            
            lockThread.unlock();
            
            //Verifier la distance en 2d
            if( getDistance2d( pt3d, ptProj ) > 2.0 )
            {
                poly3d->close();
                continue;
            }
            
            //Sinon
            else
            {
                //Verifier le z
                if( ptProj.z < pt3d.z )
                {
                    //Ajouter le point dans le tableau
                    vecPatate.push_back( pt3d );
                }
            }
            
        }
    }
}


bool findInVector3D( vector<AcGePoint3d>& vecOfElements,
    const AcGePoint3d& element,
    const double& tol )
{
    bool result = false;
    pair<bool, AcGePoint3d> res;
    AcGePoint3d ptRes = AcGePoint3d( 0, 0, 0 );
    
    vector<AcGePoint3d>::iterator  it = vecOfElements.begin();
    
    for( it = vecOfElements.begin(); it != vecOfElements.end(); it++ )
    {
        AcGePoint3d temp = ( AcGePoint3d ) * it;
        
        //Comparaison
        if( isEqual3d( temp, element, tol ) )
        {
            result = true;
            ptRes = element;
            ptRes.z = temp.z;
            break;
        }
        
        continue;
    }
    
    return result;
}


vector<SytralAloneTopoCell> fillSytralAloneTopoCell( const vector<AcGePoint3d>& vecTopoPoint,
    const vector<AcDb3dPolyline*>& vecPoly,
    const vector<AcDbBlockReference*>& vecBlock,
    const AcGePoint3d& ptMin,
    const AcGePoint3d& ptMax,
    const int& cellSize )
{
    //Taille de polyligne
    long polySz = vecPoly.size();
    long blockSz = vecBlock.size();
    long topoSz = vecTopoPoint.size();
    
    //Calculer la division des cellules
    int divX = ( ptMax.x - ptMin.x ) / cellSize;
    int divY = ( ptMax.y - ptMin.y ) / cellSize;
    
    //Resultat
    vector<SytralAloneTopoCell> vecTopoCell;
    
    AcGePoint3d minCell = ptMin;
    AcGePoint3d maxCell = AcGePoint3d( minCell.x + divX, minCell.y + divY, 0 );
    
    //Creer la matrice
    for( int y = 0; y < cellSize; y++ )
    {
        for( int x = 0; x < cellSize; x++ )
        {
            AcDbExtents ext;
            
            ext.set( minCell, maxCell );
            
            SytralAloneTopoCell cell;
            cell.extents = ext;
            vecTopoCell.emplace_back( cell );
            
            minCell.x += divX;
            maxCell.x += divX;
        }
        
        minCell.x = ptMin.x;
        maxCell.x = ptMin.x + divX;
        minCell.y += divY;
        maxCell.y += divY;
    }
    
    //Recuperer la taille du vecteur
    int vecSz = vecTopoCell.size();
    
    AcDbExtents result;
    
    //Barre de progression
    ProgressBar prog = ProgressBar( _T( "Préparation" ), vecSz );
    
    //Boucle sur le vecteur pour recuperer les polylignes et les blocs qui y passent
    for( int i = 0; i < vecSz; i++ )
    {
        //Recuperer le box du cellule
        AcDbExtents ext = vecTopoCell[i].extents;
        
        //drawBoundingBox3d( ext );
        
        //Boucle sur les points topos
        for( long t = 0; t < topoSz; t++ )
        {
            //Tester
            if( isPointInsideRect( vecTopoPoint[t],
                    ext.minPoint(),
                    ext.maxPoint(),
                    false ) )
                vecTopoCell[i].posTopo.emplace_back( vecTopoPoint[t] );
        }
        
        //Boucle sur les polylignes
        for( long p = 0; p < polySz; p++ )
        {
            //Recuperer le bounding box de la polyligne
            AcDbExtents extPoly;
            vecPoly[p]->getGeomExtents( extPoly );
            
            //drawBoundingBox3d( extPoly );
            
            //Tester si la polyligne s'intersecte ou non avec une cellule de matrice
            if( isThisFirstBoundIntersectWithSecond( extPoly, ext ) )
                vecTopoCell[i].polyIds.emplace_back( vecPoly[p] );
        }
        
        //Boucle sur les blocs
        for( long b = 0; b < blockSz; b++ )
        {
            //Recuperer le bounding box de la polyligne
            AcDbExtents extBlock;
            vecBlock[b]->getGeomExtents( extBlock );
            
            //Tester si la polyligne s'intersecte ou non avec une cellule de matrice
            if( isThisFirstBoundIntersectWithSecond( extBlock, ext ) )
                vecTopoCell[i].blockIds.emplace_back( vecBlock[b] );
        }
        
        prog.moveUp( i );
    }
    
    //Retourner le resultat
    return vecTopoCell;
}


bool isThisFirstBoundIntersectWithSecond( const AcDbExtents& ext1,
    const AcDbExtents& ext2 )
{
    //Recuperer le min et max points du premier
    AcGePoint3d min1 = ext1.minPoint();
    AcGePoint3d max1 = ext1.maxPoint();
    AcGePoint3d m2 = AcGePoint3d( min1.x, max1.y, 0 );
    AcGePoint3d m4 = AcGePoint3d( max1.x, min1.y, 0 );
    
    
    AcGePoint3d min2 = ext2.minPoint();
    AcGePoint3d max2 = ext2.maxPoint();
    
    //Verifier d'abord que le premier soit contenu dans le second
    if( isPointInsideRect( min1,
            min2,
            max2,
            false ) &&
        isPointInsideRect( max1,
            min2,
            max2,
            false
        ) )
        return true;
        
    //Ca s'intersectent si un point de l'autre est dans l'autre
    if( isPointInsideRect( min1,
            min2,
            max2,
            false ) ||
        isPointInsideRect( max1,
            min2,
            max2,
            false ) ||
        isPointInsideRect( m2,
            min2,
            max2,
            false ) ||
        isPointInsideRect( m4,
            min2,
            max2,
            false ) )
        return true;
        
    return false;
}

//Encode excel path into utf8 encodage
std::string latin1_to_utf8( const std::string& latin1 )
{
    std::string utf8;
    
    for( auto character : latin1 )
    {
        if( character >= 0 )
            utf8.push_back( character );
        else
        {
            utf8.push_back( 0xc0 | static_cast<std::uint8_t>( character ) >> 6 );
            utf8.push_back( 0x80 | ( static_cast<std::uint8_t>( character ) & 0x3f ) );
        }
    }
    
    return utf8;
}

void arroundVertex( ads_name& sspoly, int& polyc, int& poly3dc )
{
    //declarer une map qui va contenir les coordonnées des points
    map<AcDbObjectId, vector<AcGePoint3d>> mapPoly;
    
    //remplir la selection
    long sizePoly = getSelectionSet( sspoly, "", "LWPOLYLINE,POLYLINE" );
    
    //définir une barre de progression: progressbar
    ProgressBar prog = ProgressBar( _T( "Informations polylines:" ), sizePoly );
    
    AcDbObjectId idPoly = AcDbObjectId::kNull;
    AcDbEntity* entity = NULL;
    polyc = 0;
    poly3dc = 0;
    
    //boucler sur les polylines
    for( long i = 0; i < sizePoly; i++ )
    {
        prog.moveUp( i );
        
        //déclarer un vecteur temporaire
        vector<AcGePoint3d> vecPoints;
        
        //recuperer le i-eme idPoly
        idPoly = getObIdFromSs( sspoly, i );
        
        //verifier poly
        if( !idPoly.isValid() )
            continue;
            
        //recuperer l'entité
        acdbOpenAcDbEntity( entity, idPoly, AcDb::kForWrite );
        
        //verifier l'entité
        if( !entity )
            continue;
            
            
        //caster l'entité en polyline
        if( entity->isKindOf( AcDbPolyline::desc() ) || entity->isKindOf( AcDb2dPolyline::desc() ) )
        {
        
            polyc++;
            
            //caster l'entité
            AcDbPolyline* poly = AcDbPolyline::cast( entity );
            
            //verifier poly
            if( !poly )
            {
                entity->close();
                continue;
            }
            
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
            }
            
            //stocker les informations
            mapPoly.insert( make_pair( poly->id(), vecPoints ) );
            
            poly->close();
        }
        
        else if( entity->isKindOf( AcDb3dPolyline::desc() ) )
        {
        
            poly3dc++;
            
            //caster l'entité
            AcDb3dPolyline* poly = AcDb3dPolyline::cast( entity );
            
            //Verifier poly3d
            if( !poly )
            {
                entity->close();
                continue;
            }
            
            //Creer un itérateur pour les sommets de la polyligne 3D
            AcDb3dPolylineVertex* vertex = NULL;
            AcDbObjectIterator* iterPoly3D = poly->vertexIterator();
            
            //Supprimer les sommets
            for( iterPoly3D->start(); !iterPoly3D->done(); iterPoly3D->step() )
            {
                //Recuperer le vertex puis le sommet
                if( Acad::eOk == poly->openVertex( vertex, iterPoly3D->objectId(), AcDb::kForWrite ) )
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
            
            //stocker les informations
            mapPoly.insert( make_pair( poly->id(), vecPoints ) );
            
            if( vertex )vertex->close();
            
            delete iterPoly3D;
            
            poly->close();
        }
        
        //vider le vecteur temporaire
        vecPoints.clear();
        
        //liberer la selection
        if( entity )
            entity->close();
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
            
            if( isClosed( poly ) )
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
}

void writeCentreDwg( std::ofstream& file, AcDbExtents ext, AcString filename )
{
    file << "{\n\t\"Filename\" : \""
        << acStrToStr( filename ) <<
        "\",\n\t\"Center\" : [" << to_string( ext.center().x ) <<
        ", " << to_string( ext.center().y ) << "],\n" << "\t\"LowerLeft\" : [" <<
        to_string( ext.minPoint().x ) << ", " << to_string( ext.minPoint().y ) << "],\n"
        << "\t\"LowerRight\" : [" <<
        to_string( ext.maxPoint().x ) << ", " << to_string( ext.minPoint().y ) << "],\n"
        << "\t\"UpperRight\" : [" <<
        to_string( ext.maxPoint().x ) << ", " << to_string( ext.maxPoint().y ) << "],\n"
        << "\t\"UpperLeft\" : [" <<
        to_string( ext.minPoint().x ) << ", " << to_string( ext.maxPoint().y ) << "]\n}" << endl;
}


Acad::ErrorStatus delTab( map<AcString, long>& tabDel, const ads_name& ssaxe, const ads_name& sstab, const double& distmin )
{
    long nbTabDel = 0;
    
    // trier les tabulations
    AcDbPolyline* axe = readPoly2DFromSs( ssaxe );
    AcGePoint3d ptStart;
    axe->getPointAt( 0, ptStart );
    
    Acad::ErrorStatus er = eNotApplicable;
    
    //Recuperer le nombre de la tabulation
    long length;
    acedSSLength( sstab, &length );
    
    vector<Tabulation> lineTabVec;
    
    //Boucler sur la selection de tabulation
    for( int i = 0; i < length; i++ )
    {
        //Recuperer les lignes de la selection
        AcDbLine* line = readLineFromSs( sstab, i );
        
        //Structure
        Tabulation lineTab;
        lineTab.layer = line->layer();
        
        //Recuperer l'objectId de la ligne
        lineTab.idLine = line->objectId();
        
        //Recuperer le point milieu de la ligne projeté sur la polyligne
        AcGePoint3d ptOnCurve;
        er = axe->getClosestPointTo( getPoint3d( midPoint2d( line->startPoint(), line->endPoint() ) ),
                ptOnCurve );
                
        //Verifier
        if( er != Acad::eOk )
        {
            line->close();
            axe->close();
            
            print( "Erreur pendant la recuperation des tabulations, Sortie" );
            
            return er;
        }
        
        //Recuperer la distance curviligne de la ligne sur la polyligne
        double cDist = 0.0;
        er = axe->getDistAtPoint( ptOnCurve, cDist );
        
        //Verifier
        if( er != Acad::eOk )
        {
            line->close();
            axe->close();
            
            print( "Erreur pendant la recuperation des tabulations, Sortie" );
            
            return er;
        }
        
        //Ajouter la distance curviligne dans le structure
        lineTab.distCurv = cDist;
        
        //Inserer le structure dans le vecteur
        lineTabVec.push_back( lineTab );
        
        //Fermer la ligne
        line->close();
    }
    
    ///Trier le vecteur de ligne selon leur distance curviligne
    // Longueur de la liste de points
    int taille = lineTabVec.size();
    
    //Initialisation de la barre de progression
    ProgressBar prog = ProgressBar( _T( "Tri" ), taille );
    
    for( int i = 0; i < taille - 1; i++ )
    {
        //Recuperer l'index du min
        int min = i;
        
        //Boucle de test
        for( int j = i + 1; j < taille; j++ )
        {
            if( lineTabVec[j].distCurv < lineTabVec[min].distCurv )
                min = j;
        }
        
        //Swaper le vecteur et les AcArrays
        if( min != i )
            std::swap( lineTabVec[i], lineTabVec[min] );
            
        //Progresser
        prog.moveUp( i );
    }
    
    vector<Tabulation> lineTabToDel;
    vector< int > indexesToDel;
    vector<int>::iterator it;
    
    int sztabs = lineTabVec.size();
    
    Tabulation tab0 = lineTabVec[0];
    int index = 0;
    prog = ProgressBar( _T( "Suppression" ), sztabs );
    
    for( int i = 1; i < sztabs; i++ )
    {
        prog.moveUp( i );
        // Verifier si l'indice est dans la tabulation à supprimer
        it = std::find( indexesToDel.begin(), indexesToDel.end(), i );
        
        if( it != indexesToDel.end() )
            continue;
            
        Tabulation tabI =  lineTabVec[i];
        double distance =  abs( tabI.distCurv - tab0.distCurv ) ;
        
        if( distance < distmin )
        {
            if( tab0.layer.compare( _T( "PROFIL_10" ) ) == 0 )
            {
                indexesToDel.emplace_back( index );
                tab0 = tabI;
                index = i;
                continue;
            }
            
            else if( tab0.layer.compare( _T( "PROFIL_HM" ) ) == 0 )
            {
                if( tabI.layer.compare( _T( "PROFIL_HM" ) ) == 0 || tabI.layer.compare( _T( "PROFIL_KM" ) ) == 0 || tabI.layer.compare( _T( "PROFIL_SC" ) ) == 0 )
                {
                    indexesToDel.emplace_back( index );
                    tab0 = tabI;
                    index = i;
                    continue;
                }
                
                else if( tabI.layer.compare( _T( "PROFIL_10" ) ) == 0 )
                {
                    indexesToDel.emplace_back( i );
                    continue;
                }
            }
            
            else if( tab0.layer.compare( _T( "PROFIL_KM" ) ) == 0 )
            {
                if( tabI.layer.compare( _T( "PROFIL_KM" ) ) == 0 || tabI.layer.compare( _T( "PROFIL_SC" ) ) == 0 )
                {
                    indexesToDel.emplace_back( index );
                    tab0 = tabI;
                    index = i;
                    continue;
                }
                
                else if( tabI.layer.compare( _T( "PROFIL_HM" ) ) == 0 || tabI.layer.compare( _T( "PROFIL_10" ) ) == 0 )
                {
                    indexesToDel.emplace_back( i );
                    continue;
                }
            }
            
            else if( tab0.layer.compare( _T( "PROFIL_SC" ) ) == 0 )
            {
            
                if( tabI.layer.compare( _T( "PROFIL_KM" ) ) == 0 || tabI.layer.compare( _T( "PROFIL_HM" ) ) == 0 || tabI.layer.compare( _T( "PROFIL_10" ) ) == 0 )
                {
                    indexesToDel.emplace_back( i );
                    continue;
                }
            }
            
        }
        
        else
        {
            tab0 = tabI;
            index = i;
            continue;
        }
        
    }
    
    // Suppression des tabulations inutiles
    int sztabTodel = indexesToDel.size();
    map<AcString, long>::iterator itmap;
    
    prog = ProgressBar( _T( "Suppression" ), sztabTodel );
    
    for( int k = 0; k < sztabTodel; k++ )
    {
        prog.moveUp( k );
        // Recuperer l'indice de la tabulation
        int i = indexesToDel[k];
        AcDbObjectId idI = lineTabVec[i].idLine;
        AcString layer = lineTabVec[i].layer;
        
        // Recuperer la ligne à partir de la selection
        AcDbLine* lineToDel = NULL;
        AcDbEntity* ent = NULL;
        acdbOpenAcDbEntity( ent, idI, AcDb::kForWrite );
        
        if( !ent )
            continue;
            
            
        if( !ent->isKindOf( AcDbLine::desc() ) )
        {
            ent->close();
            continue;
        }
        
        ent->erase();
        ent->close();
        
        
        itmap = tabDel.find( layer );
        
        if( itmap == tabDel.end() )
        {
            tabDel.insert( { layer, 1 } );
        }
        
        else
            itmap->second++;
            
            
    }
    
    
    // Renvoyer le resultat
    return eOk;
}

int siemlVerification( AcString& mleadLayer )
{
    //  Nombre de polyligne
    int nbPolyModif = 0;
    
    //  Calque final
    AcString outputLayer = _T( "" );
    
    //  Création du calque final
    if( mleadLayer == _T( "Vérifier_regard_div" ) )
    {
        outputLayer = _T( "4_AffleurantPCRS_Regard_Div" );
        
        //  Créer le nouveau calque si il n'est pas encore créé
        if( !isLayerExisting( outputLayer ) )
            createLayer( outputLayer );
    }
    
    else if( mleadLayer == _T( "Vérifier_regard_goutt" ) )
    {
        outputLayer = _T( "4_AffleurantPCRS_Regard_Goutt" );
        
        //  Créer le nouveau calque si il n'est pas encore créé
        if( !isLayerExisting( outputLayer ) )
            createLayer( outputLayer );
    }
    
    else
        return 0;
        
    //  La sélection sur les polylignes 3D
    ads_name ssPoly, ssMleader;
    
    //  Faire une sélection sur les polylignes 3D
    int nbSs = getSsPoly3D( ssPoly, _T( "4_AffleurantPCRS_Regard_Ass" ) );
    
    //  Faire une sélection de tout les multileader
    int nbMlead = getSsAllMLeader( ssMleader, mleadLayer );
    
    //  Vérification de la sélection
    if( nbSs == 0 )
    {
        //  Afficher dans la console
        print( "\nAucune polyligne sélectionnée.\nCommande annulée." );
        return 0;
    }
    
    //  Vérification
    if( nbMlead == 0 )
    {
        //  Afficher dans la console
        print( _T( "\nAucune patate dans le calque '" + mleadLayer + "'." ) );
    }
    
    //  Itération sur la sélection
    for( int s = 0; s < nbSs; s++ )
    {
        //  Prendre la polyligne
        AcDb3dPolyline* poly = getPoly3DFromSs( ssPoly, s, AcDb::kForWrite );
        
        //  Vérification
        if( !poly )
            continue;
            
        //  Premier point de la polyligne
        AcGePoint3d pt0;
        
        //  Prendre le premier point de la polyligne
        if( getPointAt( poly, 0, pt0 ) != ErrorStatus::eOk )
        {
            poly->close();
            continue;
        }
        
        //  Itération sur les patates
        for( int p = 0; p < nbMlead; p++ )
        {
            //  Prendre la patate
            AcDbMLeader* mlead = getMLeaderFromSs( ssMleader, p, AcDb::kForWrite );
            
            //  Vérification
            if( !mlead )
                continue;
                
            //  Premier point de la patate
            AcGePoint3d mLpos;
            
            //  Prendre le premier point de la patate
            if( mlead->getFirstVertex( 0, mLpos ) == Acad::eOk )
            {
                //  Vérification l'appartenance à la polyligne
                if( !isEqual2d( mLpos, pt0 ) )
                {
                    mlead->close();
                    continue;
                }
                
                //  Changer le calque de la polyligne
                poly->setLayer( outputLayer );
                
                //  Supprimer la patate
                mlead->erase();
            }
            
            //  Fermer la patate
            mlead->close();
        }
        
        //  Fermer la polyligne
        poly->close();
        
        //  Incrémenter le nombre de polyligne
        nbPolyModif++;
    }
    
    //  Renvoyer le nombre de polyligne
    return nbPolyModif;
}
