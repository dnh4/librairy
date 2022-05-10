#pragma once

#include <acedads.h>
#include "fmap.h"
#include "boundingBox.h"
#include "rasterImageEntity.h"
#include "mleaderEntity.h"

struct Dalle
{
    int zoneDalle;
    AcString nomDalle;
    int nombreDePatate;
};

/**
  * \fn void cmdCorrectScale();
  * \cmdName CORRECTSCALE
  * \desc Mettre l'echelle du bloc à la valeur la plus grande entre l'échelle X et l'échelle Y.
  * \bullet Sélectionner les blocs.
  * \section Blocs
  * \end
  */
void cmdCorrectScale();

/**
  * \fn void cmdCGName();
  * \cmdName CGNAME
  * \desc Ajoute un préfixe ALTAMETRIS_719_ et un suffixe _LL01 dans les attibuts des block qui ont un attribut DEBUT et FIN.
  * \bullet Etape 1 : Sélectionner les blocs.
  * \section Blocs
  * \end
  */
void cmdCGName();

/**
  * \fn void cmdRobinet();
  * \cmdName ROBINET
  * \desc Mise à jour des attributs des blocs.
  * \bullet Etape 1 : Sélection des blocs.
  * \section Blocs
  * \end
  */
void cmdRobinet();

/**
  * \fn void cmdDiscretizePoly();
  * \cmdName ADDVERTEX3D
  * \desc Discrétise des polylignes 3D.
  * \bullet Etape 1 : Sélection des polylignes à discrétiser.
  * \section Polylignes
  * \end
  */
void cmdDiscretizePoly();

/**
  * \fn void cmdCleanBlockName();
  * \cmdName CLEANBLOCNAME
  * \desc Lance la procédure de nettoyage du nom des blocs.
  * \section Blocs
  * \end
  */
void cmdCleanBlockName();

/**
  * \fn void cmdGMTCheckError();
  * \cmdName GMTCHECKERROR
  * \desc Lance la procédure de vérification des erreurs.
  * \section Autres
  * \end
  */
void cmdGMTCheckError();

/**
  * \fn void cmdReplaceBlockDyn();
  * \cmdName REPLACEBLOCKDYN
  * \desc Réinsère le bloc dont le nom est entré par l'utilisateur.
  * \bullet Etape 1 : Entrer le nom du bloc.
  * \section Blocs
  * \end
  */
void cmdReplaceBlockDyn();

/**
  * \fn void cmdExportPoint();
  * \cmdName EXPORTRTE
  * \desc Permet d'exporter dans un fichier TXT le code, le X, le Y, le Z des blocs dont le code est entré par l'utilisateur (exemple : 120, 121).
  * \free Remarque : Pour entrer plusieurs valeurs, appuyer sur entrée et pour valider les entrées appuyer sur echap.
  * \bullet Etape 1 : Récupérer les blocs de nom RTE.
  * \bullet Etape 2 : Entrer le code des blocs à exporter.
  * \section RTE
  * \end
  */
void cmdExportPoint();

/**
  * \fn void cmdExportDalle();
  * \cmdName EXPORTNUMPATATEINDALLE
  * \desc Permet d'exporter dans un fichier csv la zone, le nom de la dalle, et le nombre de patate dans la dalle.
  * \free Le nom de la zone correspond aux trois premières lettres du calque du texte comme Z01... Z02...
  * \bullet Etape 1 : Sélectionner les polylignes.
  * \bullet Etape 2 : Sélectionner les textes.
  * \bullet Etape 3 : Sélectionner les cercles (patates).
  * \section Export
  * \end
  */
void cmdExportDalle();

/**
  * \fn void cmdAddBlocOnAllVertex();
  * \cmdName INSERTOPOALLVERTEX
  * \desc Insère des points topo sur tous les sommets d'une sélection de polylignes 3D.
  * \bullet Etape 1 : Sélectionner les polylignes.
  * \bullet Etape 2 : Entrer le nom du bloc à insérer.
  * \section Points topo
  * \end
  */
void cmdAddBlocOnAllVertex();

/**
  * \fn void cmdAddPointXYZPoly3d();
  * \cmdName POLY3DADDPTXYZ
  * \desc Insère un sommet sur une polyligne 3D avec la possibilité de choisir les valeurs XY et Z séparément.
  * \bullet Etape 1 : Cliquer sur la polyligne 3D.
  * \section Polylignes
  * \end
  */
void cmdAddPointXYZPoly3d();

/**
  * \fn void cmdCopyBlockLayer();
  * \cmdName FMAPCOPYBLOCKLAYER
  * \desc A clarifier.
  * \section Blocs
  * \end
  */
void cmdCopyBlockLayer();


/**
  * \fn void cmdFilterPCJ();
  * \cmdName FMAPFILTERPCJ
  * \desc Filtre le PCJ dans un dossier de PCD.
  * \bullet Etape 1 : Sélectionner le fichier PCJ dans le dossier.
  * \section Nuage
  * \end
  */
void cmdFilterPcj();

/**
  * \fn void cmdChangeLayerOfText();
  * \cmdName FMAPCHANGELAYOFTXT
  * \desc PModifie le calque des textes suivant un gabarit.
  * \bullet Etape 1 : Sélectionner les textes.
  * \section Textes
  * \end
  */
void cmdChangeLayerOfText();

/**
  * \fn void cmdArrondirH();
  * \cmdName ARRONDIRH
  * \desc Arrondit la hauteur des textes à 5 cm près.
  * \bullet Etape 1 : Sélectionner les textes pour lesquels il faut arrondir la hauteur.
  * \section KDN
  * \end
  */
void cmdArrondirH();

/**
  * \fn void cmdRoadName();
  * \cmdName ROADNAME
  * \desc Met le contenu des textes multiples en majuscule.
  * \bullet Etape 1 : Sélectionner les noms de rue MTexte.
  * \section Textes
  * \end
  */
void cmdRoadName();

/**
  * \fn void cmdAddPolyPonctText();
  * \cmdName KDNPOLYPONCTTEXT
  * \desc A clarifier.
  * \section KDN
  * \end
  */
void cmdAddPolyPonctText();

/**
  * \fn void cmdOrientTxtWithPoly()
  * \cmdName TEXTWITHPOLY
  * \desc Réoriente les textes selon les sommets des polylignes.
  * \bullet Etape 1 : Sélectionner un texte multiple à aligner.
  * \bullet Etape 2 : Sélectionner la polyligne.
  * \section Textes
  * \end
  */
void cmdOrientTxtWithPoly();

/**
  * \fn void cmdControlBlocTxt();
  * \cmdName CONTROLBLOCTXT
  * \desc cmdControlBlocTxt
  * \desc Lance la procédure de contrôle de blocs et de textes avec fichier de configuration.
  * \free Remarque : insère des patates sur les erreurs.
  * \section Textes
  * \end
  */
void cmdControlBlocTxt();

/**
  * \fn void cmdSetBlockScale();
  * \cmdName SETBLOCKSCALE
  * \desc Arrondit l'échelle des blocs à un multiple de 10.
  * \bullet Etape 1 : Sélectionner les blocs.
  * \section Blocs
  * \end
  */
void cmdSetBlockScale();

/**
  * \fn void cmdSetPTOPOPosition();
  * \cmdName SETPTOPOPOSITION
  * \desc Change l'élévation des blocs à la même altitude que les points topo qui leur sont superposés.
  * \bullet Etape 1 : Sélectionner les points topo.
  * \bullet Etape 2 : Sélectionner les blocs.
  * \section Points topo
  * \end
  */
void cmdSetPTOPOPosition();

/**
  * \fn void cmdIsolerVertex();
  * \cmdName CHECKALONEVERTEX
  * \desc Met des patates sur les sommets isolés des polylignes.
  * \bullet Etape 1 : Sélectionner les polylignes 3D.
  * \bullet Etape 2 : Entrer la tolérance.
  * \section Polylignes
  * \end
  */
void cmdIsolerVertex();

/**
  * \fn void cmdIntersectVertex();
  * \cmdName INTERSECTVERTEX
  * \desc Insère des patates et des sommets aux intersections des polylignes 3D.
  * \bullet Etape 1 : Sélectionner les polylignes 3D.
  * \bullet Etape 2 : Entrer la valeur de la tolerance.
  * \section Polylignes
  * \end
  */
void cmdIntersectVertex();


/**
  * \fn void cmdDrawContourBlock();
  * \cmdName DRAWCONTOURBLOCK
  * \desc Dessine le contour d'un bloc.
  * \bullet Etape 1 : Sélectionner les blocs.
  * \section Blocs
  * \end
  */
void cmdDrawContourBlock();

/**
  * \fn void cmdMergePhotoCsv();
  * \cmdName PRMERGEPHOTOCSV
  * \desc Exporte les données d'une photo vers un fichier CSV.
  * \bullet Etape 1 : Sélectionner le fichier de sortie.
  * \section Export
  * \end
  */
void cmdMergePhotoCsv();

/**
  * \fn void cmdExportCopyPcd();
  * \cmdName COPYPCD
  * \desc Copie des PCD vers le bloc le plus proche.
  * \free Remarque 1 : Si un PCD a son centre à moins de 20 m d'un linéaire, il est copié dans ce bloc.
  * \free Remarque 2 : Le PCD est toujours copié au moins une fois. Si aucun linéaire n'est à moins de 20 m, alors il est copié dans le bloc du linéaire le plus proche.
  * \section Nuage
  * \end
  */
void cmdExportCopyPcd();

/**
  * \fn void cmdDivPCJ();
  * \cmdName DIVPCJ
  * \desc A clarifier.
  * \section Nuage
  * \end
  */
void cmdDivPCJ();

/**
  * \fn void cmdCleanImgGtcFolder();
  * \cmdName CLEANGTCIMAGEFOLDER
  * \desc Nettoie les balises image dans le fichier GTC.
  * \bullet Etape 1 : Sélectionner le fichier GTC.
  * \section Palette
  * \end
  */
void cmdCleanImgGtcFolder();

/**
  * \fn void cmdProjectJoi();
  * \cmdName PROJOI
  * \desc Projète les deux points des blocs de jointure sur les polylignes.
  * \bullet Etape 1 : Sélectionner les blocs.
  * \bullet Etape 2 : Sélectionner les polylignes 3D.
  * \section Blocs
  * \end
  */
void cmdProjectJoi();

/**
  * \fn void cmdDrawPcj();
  * \cmdName DRAWPCJ
  * \desc Commandes pour dessiner les contours du PCJ.
  * \bullet Etape 1 : Sélectionner le fichier PCJ.
  * \section Nuage
  * \end
  */
void cmdDrawPcj();

/**
  * \fn void cmdKdnCheckSeuil();
  * \cmdName KDNCHECKSEUIL
  * \desc Insère une patate sur les erreurs de seuil pour le projet KDN.
  * \bullet Etape 1 : Sélection des polylignes 2_SeuilPCRS.
  * \bullet Etape 2 : Sélection des autres polylignes.
  * \section KDN
  * \end
  */
void cmdKdnCheckSeuil();

/**
  * \fn void cmdKdnCorrectSeuil();
  * \cmdName KDNCORRECTSEUIL
  * \desc Corrige les erreurs de seuil pour le projet KDN.
  * \bullet Etape 1 : Sélection des polylignes 2_SeuilPCRS.
  * \section KDN
  * \end
  */
void cmdKdnCorrectSeuil();

/**
  * \fn void cmdEscaFleche();
  * \cmdName ESCAFLECHE
  * \desc A clarifier.
  * \section Autres
  * \end
  */
void cmdEscaFleche();

/**
  * \fn void cmdCheckTopoAttr();
  * \cmdName CHECKTOPOATTR
  * \desc Vérifie les attributs des points topo.
  * \bullet Etape 1 : Sélectionner les points topo.
  * \bullet Etape 2 : Sélectionner les polylignes 3D.
  * \bullet Résultat : Met les patates dans la calque "PATATES_TOPO_ATTR".
  * \section Points topo
  * \end
  */
void cmdCheckTopoAttr();

/**
  * \fn void cmdSiemlRotate();
  * \cmdName SIEMLROTATE
  * \desc Etire des blocs selon la distance entre des points topo.
  * \bullet Etape 1 : Sélectionner les points topos.
  * \bullet Etape 2 : Sélectionner les blocs à réorienter.
  * \section SIEML
  * \end
  */
void cmdSiemlRotate();

/**
  * \fn void cmdSetViewPortEp();
  * \cmdName SETVIEWPORTEP
  * \desc Affiche une coupe dans une épaisseur donnée.
  * \bullet Etape 1 : Entrer l'epaisseur.
  * \section Nuage
  * \end
  */
void cmdSetViewPortEp();

/**
  * \fn void cmdBimEtiqueter();
  * \cmdName BIMETIQUETER
  * \desc Un excel rassemble des informations sur des objets BIM (coordonnées X et Y). Chaque objet doit avoir un identifiant unique appelé FB01 (colonne FB01_ID unique dans l'excel), mais cette information est vide. Un fichier DWG regroupe des blocs situés aux mêmes coordonnées que l'objet dans excel avec l'information à rentrer pour l'identifiant FB01. Pour chaque objet excel, il va falloir trouver le bon bloc DWG qui lui correspond, récupérer son attribut FB01 et remplir le tableau avec cette information.
  * \bullet Etape 1 : Entrer une tolérance en mètres.
  * \bullet Etape 2 : Sélectionner le fichier excel à remplir.
  * \section BIM
  * \end
  */
void cmdBimEtiqueter();

/**
  * \fn void cmdOrientBrestBlock();
  * \cmdName ORIENTEBLOCKBREST
  * \desc Met à jour l'orientation des blocs BREST qui ont une rotation de 100 grades.
  * \bullet Etape 1 : Sélectionner les blocs.
  * \section Brest
  * \end
  */
void cmdOrientBrestBlock();

/**
  * \fn void cmdMmsTab();
  * \cmdName MMSTAB
  * \desc Pour la préparation des projets MMS : déplace les polylignes du calque "FMAP_Tabulation" dans le calque du linéaire qu'elles traversent.
  * \free Remarque : Le calque du linéaire doit contenir "Bloc".
  * \bullet Etape 1 : Sélectionner les tabulations du projet.
  * \bullet Etape 2 : Sélectionner les linéaires du projet.
  * \section Nuage
  * \end
  */
void cmdTabLineaire();

//------------------------------------------------------------------------------------------------------------<SYTRAL>

/**
  * \fn void cmdSytralSeuil();
  * \cmdName SYTRALSEUIL
  * \desc Vérifie si le bloc a un point topo et insère une patate si ce n'est pas le cas.
  * \bullet Etape 1 : Sélectionner les blocs seuil du calque "ENTREE".
  * \bullet Etape 2 : Sélectionner les points topo des blocs seuil du calque "TOPO_Points_Seuil_Piqués".
  * \bullet Etape 3 : Sélectionner les polylignes 3D du calque : "TOPO_Bâtiments_Contours,TOPO_Bâtiments_Annexes".
  * \section Sytral
  * \end
  */
void cmdSytralSeuil();

/**
  * \fn void cmdSytralSeuil();
  * \cmdName SYTRALSEUILCONTROL
  * \desc Vérifie si le bloc a un point topo et insère une patate si ce n'est pas le cas.
  * \bullet Etape 1 : Sélectionner les blocs seuil du calque "ENTREE".
  * \bullet Etape 2 : Sélectionner les points topo des blocs seuil du calque "TOPO_Points_Seuil_Piqués".
  * \bullet Etape 3 : Sélectionner les polylignes 3D du calque : "TOPO_Bâtiments_Contours,TOPO_Bâtiments_Annexes".
  * \section Sytral
  * \end
  */
void cmdSytralSeuilControl();

/**
  * \fn void cmdSytralTopo();
  * \cmdName SYTRALTOPO
  * \desc Rajoute des sommets sur les polylignes si un point topo est proche d'un des ses segments selon une tolérance donnée.
  * \bullet Etape 1 : Sélectionner les polylignes 3D du calque "TOPO_Voirie_Caniveaux,TOPO_Voirie_Bordures".
  * \bullet Etape 2 : Sélectionner les blocs points Topo du calque "TOPO_Points_Piqués,TOPO_Points_Surplus_Piqués".
  * \section Sytral
  * \end
  */
void cmdSytralTopo();

/**
  * \fn void cmdSytralGrille();
  * \cmdName SYTRALGRILLE
  * \desc Vérifie si les grilles ont un point topo sur chaque sommet.
  * \bullet Etape 1 : Sélectionner les blocs du calque "E_P07".
  * \bullet Etape 2 : Sélectionner les blocs du calque "TOPO_Points_Piqués,TOPO_Points_Surplus_Piqués".
  * \section Sytral
  * \end
  */
void cmdSytralGrille();

/**
  * \fn void cmdSytralAvaloir();
  * \cmdName SYTRALAVALOIR
  * \desc Vérifie que les avaloirs ont un point topo sur les deux sommets côté fil d'eau.
  * \bullet Etape 1 : Sélectionner les blocs avaloir.
  * \bullet Etape 2 : Sélectionner les points topo.
  * \section Sytral
  * \end
  */
void cmdSytralAvaloir();

/**
  * \fn void cmdSytralFileDeauZ();
  * \cmdName SYTRALFILEDEAUZ
  * \desc Vérifie que le fil d'eau a toujours une altitude plus basse que le caniveau et la bordure.
  * \bullet Etape 1 : Récuperer les polylignes de bordure.
  * \bullet Etape 2 : Récuperer les polylignes de fil d'eau.
  * \bullet Etape 2 : Récuperer les polylignes de caniveau.
  * \section Sytral
  * \end
  */
void cmdSytralFileDeauZ();

/**
  * \fn void cmdSytralAddBlocTopo();
  * \cmdName SYTRALADDBLOCTOPO
  * \desc A clarifier.
  * \section Sytral
  * \end
  */
void cmdSytralAddBlocTopo();

/**
  * \fn void cmdSytralCheckAloneBlocTopo();
  * \cmdName SYTRALCHECKALONEBLOCKTOPO
  * \desc Récupère les points topo isolés.
  * \section Sytral
  * \end
  */
void cmdSytralCheckAloneBlocTopo();

/**
  * \fn void cmdSytralUnrotate();
  * \cmdName SYTRALUNROTATE
  * \desc Annule les effets de la commande SYTRALROTATE.
  * \bullet Etape 1 : Sélectionner les blocs à modifier.
  * \bullet Etape 2 : Récupère l'ancien fichier qui n'est pas encore orienté.
  * \section Sytral
  * \end
  */
void cmdSytralUnrotate();

/**
  * \fn void cmdSytralTopoAlt();
  * \cmdName SYTRALTOPOALT
  * \desc Met des patates sur les blocs dont le point topo n'est pas le même que le z.
  * \bullet Etape 1 : Sélectionner les blocs topos.
  * \bullet Etape 2 : Sélectionner les autres blocs.
  * \bullet Etape 3 : Récuperer les blocs topo.
  * \section Sytral
  * \end
  */
void cmdSytralTopoAlt();

/**
  * \fn void cmdLevelUpBlockText();
  * \cmdName LEVELUPBLOCKTEXT
  * \desc A clarifier.
  * \section Blocs
  * \end
  */
void cmdLevelUpBlockText();

/**
  * \fn void cmdSytralAccrocheTopo();
  * \cmdName LEVELUPBLOCKTEXT
  * \desc Si un point topo se superpose à un sommet de polyligne en XY (avec une tolérance), le point topo est déplacé sur le sommet.
  * \bullet Etape 1 : Sélectionner les polylignes 3D.
  * \bullet Etape 2 : Sélectionner les blocs topo.
  * \section Sytral
  * \end
  */
void cmdSytralAccrocheTopo();

/**
  * \fn void cmdArroundPolys();
  * \cmdName ARROUNDPOLYS
  * \desc RA clarifier.
  * \section Polylignes
  * \end
  */
void cmdArroundPolys();

/**
  * \fn void cmdIGNExportCenter();
  * \cmdName IGNEXPORTCENTER
  * \desc Exporte les centres des cercles.
  * \section IGN
  */
void cmdIGNExportCenter();

/**
  * \fn void cmdBimGenererFb();
  * \cmdName BIMGENERERFB
  * \desc Dessine des cercles à partir d'un fichier excel.
  * \section BIM
  * \end
  */
void cmdBimGenererFb();

/**
  * \fn void cmdArroundVertex();
  * \cmdName ARROUNDVERTEX
  * \desc Arrondit la positions des sommets d'une sélection de polylignes à une tolérance près.+
  * \bullet Etape 1 : Sélection des polylignes.
  * \section Polylignes
  * \end
  */
void cmdArroundVertex();

/**
  * \fn void cmdGetCentroid();
  * \cmdName CENTREDWG
  * \desc Exporte les coordonnées du dessin dans un fichier JSON.
  * \free Cette commande est utilisée pour géoréférencer les fichiers de livraison sur la plateforme.
  * \section Plateforme
  * \end
  */
void cmdGetDwgCenter();


void cmdQrtListSurf();
/**
* \fn void cmdDelTab()
* \cmdName DELTAB
* \desc Supprime les tabulations superflues dans une certaine tolérance selon un ordre de priorité sur les calques.
* \free La commande supprime d'abord les tabulations avec la plus petite importance (PROFIL_SC > PROFIL_KM > PROFIL_HM > PROFIL_10).
* \bullet Etape 1 : Entrer la distance minimale entre les tabulations.
* \bullet Etape 2 : Sélectionner l'axe des tabulations.
* \bullet Etape 3 : Sélectionner les tabulations (sélection sur les calques : PROFIL_10, PROFIL_HM, PROFIL_KM, PROFIL_SC).
* \bullet Sortie : Le nombre de tabulations supprimées pour chaque calque est affiché.
* \section Tabulations
* \end
*/
void cmdDelTab();


/**
* \fn void CmdSiemlRegard()
* \cmdName SIEMLREGARDS
* \desc Supprime ou met des patates sur les polylignes 3D du calque "4_AffleurantPCRS_Regard_Ass" qui ne doivent pas être dessinées.
* \free Supprime les polylignes 3D dans le calque "4_AffleurantPCRS_Regard_Ass" qui ont une forme rectangulaire (rapport longueur/largeur < 1.3).
* \free Met des patates dans le calque "Vérifier_regard_ass" sur les polylignes dont un des côtés mesure PLUS de 40 cm.
* \free Met des patates dans le calque "Vérifier_regard_div" sur les polylignes dont un des côtés mesure MOINS de 40 cm.
* \free Met des patates dans le calque "Vérifier_regard_goutt" sur les polylignes à proximité des murs et batîs dont le calque est "2_FacadePCRS" ou "2_FacadePCRS_figuratif".
* \bullet Sélectionner les polylignes 3D dans le calque "4_AffleurantPCRS_Regard_Ass".
* \section SIEML
* \end
*/
void cmdSiemlRegard();

/**
* \fn void cmdSiemlGoutt()
* \cmdName SIEMLGOUTT
* \bullet Sélectionner les polylignes 3D dans le calque "4_AffleurantPCRS_Regard_Ass".
* \free Met des patates dans le calque "Vérifier_regard_goutt" sur les polylignes qui sont à proximité des bâtis et proéminences.
* \bullet Remarque : Vérification sur les bâtis et proéminences qui ont les calques "2_ProeminenceBatiPCRS", "2_FacadePCRS" et "2_FacadePCRS_figuratif"
* \section SIEML
* \end
*/
void cmdSiemlGoutt();

/**
* \fn void cmdSMDiv()
* \cmdName SMDIV
* \bullet Sélectionner le(s) polyligne(s) 3D dans le calque "4_AffleurantPCRS_Regard_Ass"
* \free Supprime les patates dans le calque "Vérifier_regard_div" qui sont attachés sur le(s) polyligne(s) sélectionnée(s)
* \bullet Met le(s) polyligne(s) sélectionnée(s) dans le calque "4_AffleurantPCRS_Regard_Div"
* \section SIEML
* \end
*/
void cmdSMDiv();

/**
* \fn void cmdMajngf()
* \cmdName MAJNGFALT
* \desc Modifie l'attribut NIVEAUP des blocs NIVEAU_PLANCHER en additionnant la valeur de leur attribut NIVEAU à l'attribut NIVEAUP d'un bloc NIVEAU_PLANCHER de référence.
* \bullet Etape 1 : Sélectionner le bloc NIVEAU_PLANCHER de référence.
* \bullet Etape 2 : Sélectionner les blocs à modifier.
* \section Statique
* \end
*/
void cmdMajngfAlt();

/**
* \fn void cmdMajngf()
* \cmdName MAJNGFREF
* \desc Modifie l'attribut NIVEAU des blocs NIVEAU_PLANCHER en ajoutant une valeur saisie par l'utilisateur
* \bullet Etape 1 : Entrer la valeur à ajouter.
* \bullet Etape 2 : Sélectionner les blocs NIVEAU_PLANCHER.
* \section Statique
* \end
*/
void cmdMajngfRef();