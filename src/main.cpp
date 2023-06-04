#include "application_ui.h"
#include "SDL2_gfxPrimitives.h"
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <algorithm>
#include <iostream>
using namespace std;

#define EPSILON 0.0001f

static const SDL_Color jaune = {240, 240, 23, 255};
static const SDL_Color vert = {0, 240, 160, 255};
static const SDL_Color bleu = {40, 30, 200, 255};
static const SDL_Color rouge = {200, 30, 100, 255};


struct Coords
{
    int x, y;

    bool operator==(const Coords& other)
    {
        return x == other.x and y == other.y;
    }
};

struct Segment
{
    Coords p1, p2;
};

struct Triangle
{
    Coords p1, p2, p3;
    bool complet=false;
};

struct Polygone {
    Coords centre;
    std::vector<Coords> sommets;  
};


// ...
struct Application
{
    int width, height;
    Coords focus{100, 100};

    std::vector<Coords> Dpoints;
    std::vector<Triangle> triangles;
    std::vector<Polygone> polygones;
};

bool compareCoords(Coords point1, Coords point2)
{
    if (point1.y == point2.y)
        return point1.x < point2.x;
    return point1.y < point2.y;
}

void drawPoints(SDL_Renderer *renderer, const std::vector<Coords> &points, SDL_Color color)
{
    for (std::size_t i = 0; i < points.size(); i++)
    {
        filledCircleRGBA(renderer, points[i].x, points[i].y, 3, color.r, color.g, color.b , SDL_ALPHA_OPAQUE);
    }
}

void drawSegments(SDL_Renderer *renderer, const std::vector<Segment> &segments, SDL_Color color)
{
    for (std::size_t i = 0; i < segments.size(); i++)
    {
        lineRGBA(
            renderer,
            segments[i].p1.x, segments[i].p1.y,
            segments[i].p2.x, segments[i].p2.y,
            color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
    }
}

void drawTriangles(SDL_Renderer *renderer, const std::vector<Triangle> &triangles, SDL_Color color)
{
    for (std::size_t i = 0; i < triangles.size(); i++)
    {
        const Triangle& t = triangles[i];
        trigonRGBA(
            renderer,
            t.p1.x, t.p1.y,
            t.p2.x, t.p2.y,
            t.p3.x, t.p3.y,
            color.r, color.g, color.b, SDL_ALPHA_OPAQUE
        );
    }
}

void drawPolygones(SDL_Renderer *renderer, const std::vector<Polygone> &polygones, SDL_Color color)
{
    for (std::size_t i = 0; i < polygones.size(); i++)
    {
        const vector<Coords> sommets = polygones[i].sommets;
        Sint16 vx[sommets.size()], vy[sommets.size()];
        for (int j=0; j<sommets.size(); j++) {
            vx[j] = sommets[j].x;
            vy[j] = sommets[j].y;
        }
        const Polygone& p = polygones[i];
        polygonRGBA(
            renderer,
            vx, vy,
            sommets.size(),
            color.r, color.g, color.b, SDL_ALPHA_OPAQUE
        );
    }
}

void draw(SDL_Renderer *renderer, const Application &app)
{
    /* Remplissez cette fonction pour faire l'affichage du jeu */
    int width, height;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    drawPoints(renderer, app.Dpoints, jaune);
    drawTriangles(renderer, app.triangles, vert);
    drawPolygones(renderer, app.polygones, bleu);
    // drawPoints(renderer, app.Vpoints, rouge);
    // drawSegments(renderer, app.Vsegments, bleu);

}

/*
   Détermine si un point se trouve dans un cercle définit par trois points
   Retourne, par les paramètres, le centre et le rayon
*/
bool CircumCircle(
    float pX, float pY,
    float x1, float y1, float x2, float y2, float x3, float y3,
    float *xc, float *yc, float *rsqr
)
{
    float m1, m2, mx1, mx2, my1, my2;
    float dx, dy, drsqr;
    float fabsy1y2 = fabs(y1 - y2);
    float fabsy2y3 = fabs(y2 - y3);

    /* Check for coincident points */
    if (fabsy1y2 < EPSILON && fabsy2y3 < EPSILON)
        return (false);

    if (fabsy1y2 < EPSILON)
    {
        m2 = -(x3 - x2) / (y3 - y2);
        mx2 = (x2 + x3) / 2.0;
        my2 = (y2 + y3) / 2.0;
        *xc = (x2 + x1) / 2.0;
        *yc = m2 * (*xc - mx2) + my2;
    }
    else if (fabsy2y3 < EPSILON)
    {
        m1 = -(x2 - x1) / (y2 - y1);
        mx1 = (x1 + x2) / 2.0;
        my1 = (y1 + y2) / 2.0;
        *xc = (x3 + x2) / 2.0;
        *yc = m1 * (*xc - mx1) + my1;
    }
    else
    {
        m1 = -(x2 - x1) / (y2 - y1);
        m2 = -(x3 - x2) / (y3 - y2);
        mx1 = (x1 + x2) / 2.0;
        mx2 = (x2 + x3) / 2.0;
        my1 = (y1 + y2) / 2.0;
        my2 = (y2 + y3) / 2.0;
        *xc = (m1 * mx1 - m2 * mx2 + my2 - my1) / (m1 - m2);
        if (fabsy1y2 > fabsy2y3)
        {
            *yc = m1 * (*xc - mx1) + my1;
        }
        else
        {
            *yc = m2 * (*xc - mx2) + my2;
        }
    }

    dx = x2 - *xc;
    dy = y2 - *yc;
    *rsqr = dx * dx + dy * dy;

    dx = pX - *xc;
    dy = pY - *yc;
    drsqr = dx * dx + dy * dy;

    return ((drsqr - *rsqr) <= EPSILON ? true : false);
}

/*
    Détermine si un point fait partie des sommets d'un triangle
*/
bool hasVertex(Coords point, Triangle triangle){
    return (point == triangle.p1 || point == triangle.p2 || point == triangle.p3);

}

void construitVoronoi(Application &app)
{
// Milestone 0 : on dessine des triangles
    // int size = app.points.size();
    // if (size>=3){
    //     for(int i=0; i<size-2; i++){
    //         app.triangles.push_back(Triangle{app.points.at(i), app.points.at(i+1), app.points.at(i+2)});
    //     }
    // }

//Milestone 1 Delaunay
    //on trie les points selon x
    sort(app.Dpoints.begin(), app.Dpoints.end(), compareCoords); 

    //on vide la liste existante des triangles
    app.triangles.clear();

    //on créé un très grand triangle et on l'ajoute à la liste de triangles
    Triangle bigTriangle{Coords{-1000, -1000}, Coords{500, 3000}, Coords{1500, -1000}};
    app.triangles.push_back(bigTriangle);

    for(size_t i=0; i<app.Dpoints.size(); i++){
        
        //on créé la liste de segments
        vector<Segment> LS;
        
        //on boucle sur les triangles déjà créés
        for (size_t j=0; j < app.triangles.size(); j++){
            Triangle T = app.triangles.at(j);

            // Si le cercle circonscrit contient le point P...
            float xc, yc, rayon;
            if (CircumCircle(app.Dpoints.at(i).x, app.Dpoints.at(i).y, T.p1.x, T.p1.y, T.p2.x, T.p2.y, T.p3.x, T.p3.y, &xc, &yc, &rayon)){
                //on récupère les segments de ce triangle dans LS
                LS.push_back(Segment{T.p1, T.p2});
                LS.push_back(Segment{T.p2, T.p3});
                LS.push_back(Segment{T.p3, T.p1});

                //on retire le triangle T de la liste
                app.triangles.erase(app.triangles.begin() + j);
                j--;
            }
        }

        //on boucle sur les segments de liste LS
        for (size_t k=0; k<LS.size(); k++){
            Segment Sk = LS.at(k);
            
            for (size_t l=0; l < LS.size(); l++){
                Segment Sl = LS.at(l);

                //on ne compare pas un segment à lui-même
                if (k == l) break;

                //s'il y a doublon on supprime les 2 segments
                if ((Sk.p1 == Sl.p2) && (Sk.p2 == Sl.p1)){
                    LS.erase(LS.begin() + k);
                    LS.erase(LS.begin() + l);
                    k--;
                    l--;
                }
            }
        }

        //on créé des triangles à partir des segments de LS et du point sur lequel on boucle actuellement 
        for(size_t m=0; m<LS.size(); m++){
            app.triangles.push_back(Triangle{LS.at(m).p1, LS.at(m).p2, app.Dpoints.at(i)});
        }
        
    }

//Milestone 2 Voronoi

    app.polygones.clear();
    // for (size_t i=0; i < app.triangles.size(); i++){
    //     float xc, yc, rayon;
    //     Triangle T = app.triangles.at(i);
    //     CircumCircle(0,0, T.p1.x, T.p1.y, T.p2.x, T.p2.y, T.p3.x, T.p3.y, &xc, &yc, &rayon);
    //     app.Vpoints.push_back(Coords{(int)xc, (int)yc});
    // }

    Polygone bigPolygone;
    bigPolygone.sommets.push_back(Coords{-1000, -1000});
    bigPolygone.sommets.push_back(Coords{500, 3000});
    bigPolygone.sommets.push_back(Coords{1500, -1000});

    app.polygones.push_back(bigPolygone);
    // Parcourir chaque point de la triangulation de Delaunay
    for (size_t i = 0; i < app.Dpoints.size(); i++) {

        // Récupérer les triangles adjacents partageant le sommet
        vector<Triangle> adjacentTriangles;
        for (size_t j = 0; j < app.triangles.size(); j++) {
            if (hasVertex(app.Dpoints.at(i), app.triangles.at(j))) {
                adjacentTriangles.push_back(app.triangles.at(j));
            }
        }

        // Récupérer les centres des cercles circonscrits
        std::vector<Coords> pointsVoronoi;
        for (const auto& triangle : adjacentTriangles) {
            float xc, yc, rayon;
            CircumCircle(0,0, triangle.p1.x, triangle.p1.y, triangle.p2.x, triangle.p2.y, triangle.p3.x, triangle.p3.y, &xc, &yc, &rayon);
            pointsVoronoi.push_back(Coords{(int)xc, (int)yc});
        }

        // Créer une nouvelle cellule de Voronoi
        Polygone polygone;
        polygone.centre = app.Dpoints.at(i);
        polygone.sommets = pointsVoronoi;

        // Ajouter la cellule à la liste
        app.polygones.push_back(polygone);
    }

    
}


bool handleEvent(Application &app)
{
    /* Remplissez cette fonction pour gérer les inputs utilisateurs */
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            return false;
        else if (e.type == SDL_WINDOWEVENT_RESIZED)
        {
            app.width = e.window.data1;
            app.height = e.window.data1;
        }
        else if (e.type == SDL_MOUSEWHEEL)
        {
        }
        else if (e.type == SDL_MOUSEBUTTONUP)
        {
            if (e.button.button == SDL_BUTTON_RIGHT)
            {
                app.focus.x = e.button.x;
                app.focus.y = e.button.y;
                app.Dpoints.clear();
                // app.Vpoints.clear();
            }
            else if (e.button.button == SDL_BUTTON_LEFT)
            {
                app.focus.y = 0;
                app.Dpoints.push_back(Coords{e.button.x, e.button.y});
                construitVoronoi(app);
            }
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    SDL_Window *gWindow;
    SDL_Renderer *renderer;
    Application app{720, 720, Coords{0, 0}};
    bool is_running = true;

    // Creation de la fenetre
    gWindow = init("Awesome Voronoi", 720, 720);

    if (!gWindow)
    {
        SDL_Log("Failed to initialize!\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(gWindow, -1, 0); // SDL_RENDERER_PRESENTVSYNC

    /*  GAME LOOP  */
    while (true)
    {
        // INPUTS
        is_running = handleEvent(app);
        if (!is_running)
            break;

        // EFFACAGE FRAME
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // DESSIN
        draw(renderer, app);

        // VALIDATION FRAME
        SDL_RenderPresent(renderer);

        // PAUSE en ms
        SDL_Delay(1000 / 30);
    }

    // Free resources and close SDL
    close(gWindow, renderer);

    return 0;
}
