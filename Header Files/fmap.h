#pragma once
#include <string>
#include "fmapHeaders.h"
#include "blockEntity.h"
#include "mleaderEntity.h"
#include "print.h"
#include "layer.h"
#include <iomanip>
#include <thread>
#include <mutex>

#define MAKEDLL

#ifdef MAKEDLL
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __declspec(dllimport)
#endif


struct LayText
{
    //Le texte
    AcString text;
    
    //Le calque d'avant
    AcString oldLayer;
    
    //Le calque après
    AcString newLayer;
};

struct vtxPt
{
    AcGePoint2d ptPoly = AcGePoint2d::kOrigin;
    AcDbObjectId idPoly = AcDbObjectId::kNull;
    bool isVisited = false;
    AcGePoint2d ptMean = AcGePoint2d::kOrigin;
};

struct bimGenererInfos
{
    int id;
    double x;
    double y;
    string calque;
};


struct Slope
{
    //Le premier point du slope
    AcGePoint3d pt1;
    
    //Le second point du slope
    AcGePoint3d pt2;
};


struct DallePoly
{
    //La polyligne qui delimite le dalle
    AcDbObjectId idPoly;
    
    //Le nom de la dalle
    AcString dalleName;
};

struct DivPcd
{
    //Nom du bloc
    AcString blocName;
    
    //Vecteur qui va contenir les noms des pcd pour le bloc
    vector<AcString> pcdVectorName;
    
    //Vecteur qui va contenir les noms des pc pour le bloc mais en rar
    vector<AcString> rarVectorName;
    
    //Chemin du fichier pcj
    AcString pcjPath;
    
    //Ligne du fichier pcj
    vector<AcString> pcjLine;
    
    //Vecteur contenant l'objectId des polylignes
    vector<AcDbObjectId> vecPolyId;
};


struct PcdInfos
{
    //Nom du pcd
    AcString pcdName;
    
    //Point min du pcd
    AcGePoint3d pcdCenter;
    
    //Ligne du fichier
    AcString pcdLine;
};

struct Poly3dSytral
{
    AcDbObjectId idPoly;
    AcGePoint3dArray ptList;
    double xmin;
    double ymin;
    double zmin;
    double xmax;
    double ymax;
    double zmax;
};

struct SytralCell
{
    vector<Poly3dSytral> polyList;
    double xmin;
    double ymin;
    double zmin;
    double xmax;
    double ymax;
    double zmax;
};

struct SytralAloneTopoCell
{
    vector<AcGePoint3d> posTopo;
    vector<AcDb3dPolyline*> polyIds;
    vector<AcDbBlockReference*> blockIds;
    AcDbExtents extents;
};

struct Tabulation
{
    AcDbObjectId idLine;
    double distCurv;
    AcString layer;
};

/**
  * \brief Inserer un bloc dynamique dans le dessin, en passant une liste de proprietes et une liste de valeurs, avec la valuer i qui correspond a la valeur de la propriete i
  * \param blockName    Nom de la definition du bloc
  * \param position     Point ou on veut inserer le bloc
  * \param propNames    Liste des noms des proprietes à modifer
  * \param propValue    Liste des valeurs de proprietes à modifier, ATTENTION on a besoin de caster les valeurs dans ce vecteur à celui du type demandé par le bloc
  * \param blockLayer   Nom du calque du bloc
  * \param rotation     Rotation du bloc
  * \param scaleX       Valeur de l'echelle X (1 par defaut)
  * \param scaleY       Valeur de l'echelle Y (= scaleX par defaut)
  * \param scaleZ       Valeur de l'echelle Z (= scaleX par defaut)
  * \return Pointeur sur la reference de bloc, ATTENTION vous etes responsable de sa fermetureS
  */
AcDbBlockReference* insertFmapDynamicBlockReference( const AcString& blockName,
    const AcGePoint3d& position,
    const AcStringArray& propName,
    const AcStringArray& propValue,
    const AcString& blockLayer,
    const double& rotation = 0,
    const double& scaleX = 1,
    const double& scaleY = 1,
    const double& scaleZ = 1 );


/**
  * \brief Permet d'inserer des blocs topos sur les vertexs d'une selection de polyligne 3d
  * \param ssPoly Selection sur des polylignes 3d
  * \param blockName Nom du bloc à inserer sur les vertexs d'une selection de polyligne 3d
  * \return ErrorStatus
  */
Acad::ErrorStatus addTopoOnAllVertex( const ads_name& ssPoly,
    const AcString& blockName );


/**
  * \brief Permet de nettoyer le vecteur de nom apres recuperation des noms des fichiers dans un dossier, i.e supprimer l'extension, supprimer les doublons des noms de fichier, et les "." ".."
  * \param vecName Vecteur de string contenant les noms des fichiers dans un dossier
  * \param blockName Nom du bloc à inserer sur les vertexs d'une selection de polyligne 3d
  * \return ErrorStatus
  */
vector<string> cleanVectorOfName( vector<string>& vecName );


AcString getLayerOfThisText( const AcString& text,
    const AcString& layerOfText );

AcString crToSpace( const AcString& text );


void interpolThisPointArray( AcGePoint3dArray& ptArray,
    Slope& startSlope,
    Slope& endSlope );

void extrapolThisPointArray( AcGePoint3dArray& ptArray,
    const Slope& startSlope,
    const Slope& endSlope );



bool getDistCurvOfArray( vector<double>& res,
    AcGePoint3dArray& ptArray,
    const AcDb3dPolyline* poly3D,
    const bool& sort = false );

bool getIndexOfPoint( vector<int>& res,
    const vector<double>& curvDistPtArray,
    const vector<double>& curvDistPtIntersectArray,
    const double& secondDistCurv );

void projectIndexedPointToPlane( AcGePoint3dArray& ptArray,
    const vector<int>& indexes,
    const AcGePlane& plane );


void getFaceCenterUVVector( AcGePoint3d& center,
    AcGeVector3d& u,
    AcGeVector3d& v,
    const AcDbFace* face );


void projectInPlaneIfPointInsideFace( AcGePoint3d& pt,
    const AcDbFace* face );


bool isInsideTriangle( const AcGePoint3d& pt,
    const AcGePoint3d& pt0,
    const AcGePoint3d& pt1,
    const AcGePoint3d& pt2 );

/**
  * \brief Récupérer les textes d'un fichier texte ligne par ligne
  * \param filePath Le chemin vers le fichier texte
  * \return Vecteur contenant chaque ligne du fichier texte
  */
std::vector<AcString> fileLineToUTF8StringVector( const AcString& filePath );

/**
  * \brief recherche un Actring dans un vecteur de string
  * \param vecOfElements vecteur de string
  * \param element élément à chercher
  * \return int , -1 si pas trouve sinon >= 0
  */
int findAcStringInVector( std::vector<AcString>& vecOfElements,
    const AcString& element );

/**
  * \brief verifie si un rectancgle est totalement incluse dans un autre
  * \param oneBox Rectangle à vérifier
  * \param anothereBox Rectangle de verification
  * \return bool
  */
bool checkIfOneBoxIsTotallyInsideAnother( AcGePoint3dArray& oneBox,
    AcGePoint3dArray& anothereBox );

bool checkIfCanFit( const double& textWidth,
    AcDb3dPolyline*& poly3d );

double getMaxSeg( AcDb3dPolyline*& poly3d, AcGePoint3d& pt1, AcGePoint3d& pt2, double& maxDist, AcGePoint3d& centerPoly );

bool sortBySecond( const pair<AcGePoint3dArray, double>& onePair,
    const pair<AcGePoint3dArray, double>& othPair );


int verifyLayer( const ads_name& ssBlock,
    const ads_name& ssMText,
    const ads_name& ssText,
    const long& lenBloc,
    const long& lenMText,
    const long& lenText );

double roundUp( const double& value, const int& numberDigit, const int& multiple );


Acad::ErrorStatus eraseDoublons( AcGePoint3dArray& pointArray,
    const double& tol = 0.1 );


Acad::ErrorStatus insertPatate( const AcGePoint3dArray& ptArr,
    const AcString& layer = _T( "0" ) );

Acad::ErrorStatus insertNewVertToPoly3D( const AcDbObjectId& idPoly3D,
    AcGePoint3dArray& pointsToInsertArray,
    const AcString& layer,
    const double& tol = 0.01 );


Acad::ErrorStatus getPolyBlock( const AcString& path,
    const AcString& blockName,
    AcGePoint3dArray& res );


Acad::ErrorStatus drawPolyBlock( const AcDbBlockReference* block3D,
    const AcGePoint3dArray& vecPt );

vector<string> getSortedFileNamesInFolder( const AcString& folderFiles );


AcString getDalleName( AcDbPolyline* poly );


bool copyFileTo( const AcString& sourceFile, const AcString& destinationFile );


bool getDalleInfos( vector < DallePoly>& dallePolyVec );


void divPcj();


void divPcjXlsx();


AcDbText* getTopoAttr( const AcDbVoidPtrArray& entityArray );

/*
    Sous fonctions pour la commande BIMETIQUETER
*/
void askFileInfos(
    string& _filename,
    string& _config_filename,
    double& _tolerance
);

/*
    Informations sur le fichier de configuration
*/

void getConfigInfos(
    string _config_path,
    map<string, vector<string>>& _vec_config
);


//-----------------------------------------------------------------------------------------------------------<SYTRAL>
Acad::ErrorStatus sytralSeuil( long& nbErreur,
    const ads_name& ssblockSeuil,
    const ads_name& ssblockPTopo,
    const ads_name& sspoly3d );


Acad::ErrorStatus sytralSeuilControl( long& nbErreur,
    const ads_name& ssblockSeuil,
    const ads_name& ssblockPTopo,
    const ads_name& sspoly3d );


Acad::ErrorStatus sytralTopo2( long& nbSommets,
    const ads_name& sspoly3d,
    const ads_name& ssblockPTopo );


Acad::ErrorStatus sytralGrille( long& nbErreur,
    const ads_name& ssblock,
    const ads_name& sstopo );

Acad::ErrorStatus sytralAvaloir( long& nbErreur,
    const ads_name& ssblock,
    const ads_name& sstopo );


Acad::ErrorStatus sytralTopo( long& nbSommets,
    const ads_name& sspoly3d,
    const ads_name& ssblockPTopo );

void thread_ControlSytralFileDeau( const vector<AcGePoint3d>& ptVec,
    AcGePoint3dArray& vecPatate,
    const ads_name& ssPoly3d,
    const long& polySz,
    const long& vecSize,
    const int& threadNumber,
    const int& threadIterator );

bool findInVector3D( vector<AcGePoint3d>& vecOfElements,
    const AcGePoint3d& element,
    const double& tol );


vector<SytralAloneTopoCell> fillSytralAloneTopoCell( const vector<AcGePoint3d>& vecTopoPoint,
    const vector<AcDb3dPolyline*>& vecPoly,
    const vector<AcDbBlockReference*>& vecBlock,
    const AcGePoint3d& ptMin,
    const AcGePoint3d& ptMax,
    const int& cellSize );


bool isThisFirstBoundIntersectWithSecond( const AcDbExtents& ext1,
    const AcDbExtents& ext2 );

std::string latin1_to_utf8( const std::string& latin1 );

/*
    brief : fonction qui arrondit les coordonnées des points
    param : aucun paramètres
    return : void
*/
void arroundVertex( ads_name& sspoly, int& polyc, int& poly3dc );

void writeCentreDwg( std::ofstream&, AcDbExtents, AcString );


/**
  * \brief Supprime les tabulations inutiles
  * \param tabDEl               : Nombre de tabulations supprimées par calque
  * \param ssaxe                : Sélection de l'axe de tabulation
  * \param sstab                : Sélection de tabulations
  * \param distmin              : Distance minimale entre les tabulations
*/
Acad::ErrorStatus delTab( map<AcString, long>& tabDel, const ads_name& ssaxe, const ads_name& sstab, const double& distmin );


//-----------------------------------------------------------------------------------------------------------<SIEML>

/**
  * \brief
  * \param
  * \param
*/
int siemlVerification( AcString& mleadLayer );