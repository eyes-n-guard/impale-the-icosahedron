#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL.h>
#include <math.h>
#include <time.h>

static int WIDTH = 1920; //640
static int HEIGHT = 1080; //360
static float SENSITIVITY = 2.5;

#define CROSSHAIR_SIZE 10 //distance from edge of crosshair to center
#define CROSSHAIR_R 10
#define CROSSHAIR_G 255
#define CROSSHAIR_B 50

#define SETTINGS_FILE "settings.txt"
static char MAP_FILE[30];

#define BACKFACE_CULL_WIREFRAME true
#define BACKFACE_CULL_FILL true
#define GENERATE_FACE_NORMALS true

static float FRUSTUM_WIDTH = 1.0; // = 1/tan(fov/2)
static float FRUSTUM_NEAR_LENGTH = 0.01;
static int DRAW_EDGES = false;



int sign(float a);
float min(float a, float b);
float max(float a, float b);
float clamp(float a, float bot, float top);



typedef struct //vec3
{
    float x,y,z;
} vec3;

typedef struct //vec2
{
    float x,y;
} vec2;

typedef struct //camera
{
    vec3 pos, vel;
    float pitch, yaw, speed, accel, decel; //angle and yaw in radians
} camera;

typedef struct //face //each face is a triangle, with 3 indexes in the vertex array
{
    int p1, p2, p3, texture, type, flags;
    vec3 norm, mid;
    vec2 uv1, uv2, uv3;
} face;

typedef struct //colour
{
    int r,g,b;
} colour;

float length(vec3 a);
float dot(vec3 a, vec3 b);

vec3 add(vec3 a, vec3 b);
vec3 sub(vec3 a, vec3 b);
vec3 mul(vec3 a, float b);
vec3 unit(vec3 a);
vec3 cross(vec3 a, vec3 b);
vec3 rotate(vec3 a, vec3 b, float theta);
vec3 rotateZ(vec3 a, float theta);
vec3 rotateX(vec3 a, float theta);
vec3 dropX(vec3 a);
vec3 dropY(vec3 a);
vec3 dropZ(vec3 a);

vec2 add2(vec2 a, vec2 b);
vec2 sub2(vec2 a, vec2 b);
vec2 mul2(vec2 a, float b);
float length2(vec2 a);
float dot2(vec2 a, vec2 b);
vec2 unit2(vec2 a);
vec3 toVec3(vec2 a, float z);
vec2 perspective2d(vec3 a);

bool intersection(vec2 a1, vec2 a2, vec2 b1, vec2 b2, vec2 *result);

void loadConstants(int *w, int *h, float *sens, char *map, float *frustumW, float *frustumN, int *edges, char* fileName);
void quicksort(int list[], float ref[], int l, int r);
void drawFilledFaces(face *faces, int nFaces, camera player, vec3 *points, SDL_Renderer *render, colour *colours);
void drawLine(vec3 p1, vec3 p2, SDL_Renderer *render);
void drawWireframeFace(face f, camera player, vec3 *points, SDL_Renderer *render);
void loadMap(int *nVectors, int *nFaces, int *nColours, vec3 **vectors, face **faces, colour **colours, char *fileName, camera *player, SDL_Surface ***textures, int *nTextures);
void transformFace(face f, vec3 *points, camera player, SDL_Renderer *renderer);
void drawWireframePolygon(vec2 *polygon, int nPoints, SDL_Renderer *renderer);
void fillTriangle(vec2 p1, vec2 p2, vec2 p3, SDL_Renderer *renderer);
void swapVec2Ptr(vec2 **p1, vec2 **p2);
int getClipCode(vec2 a);
void drawClippedLine(vec2 a, vec2 b, SDL_Renderer *renderer);
bool clipVelocity(camera *player, face triangle, vec3 *points);
void buildClipVectors(int nVectors, int nFaces, vec3 *mapVectors, face *mapFaces, vec3 *clipVectors);

int main(int argc, char **argv)
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if(SDL_Init(SDL_INIT_VIDEO) != 0)
        return 1;

    loadConstants(&WIDTH, &HEIGHT, &SENSITIVITY, MAP_FILE, &FRUSTUM_WIDTH, &FRUSTUM_NEAR_LENGTH, &DRAW_EDGES, SETTINGS_FILE);

    window = SDL_CreateWindow("Dank meme", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window,-1, SDL_RENDERER_SOFTWARE);

    SDL_CaptureMouse(true);
    SDL_SetRelativeMouseMode(true);

    //set up map
    camera player;
    int mapVectorsNum = 0;
    int mapFacesNum = 0;
    int mapColoursNum = 0;
    int mapTexturesNum = 0;
    vec3 *mapVectors = NULL;
    face *mapFaces = NULL;
    colour *mapColours = NULL;
    SDL_Surface **mapTextures = NULL;
    loadMap(&mapVectorsNum, &mapFacesNum, &mapColoursNum, &mapVectors, &mapFaces, &mapColours, MAP_FILE, &player, &mapTextures, &mapTexturesNum);
    bool *clippedFaces = malloc(mapFacesNum * sizeof(bool));
    vec3 *mapClipVectors = malloc(mapVectorsNum * sizeof(vec3));
    buildClipVectors(mapVectorsNum, mapFacesNum, mapVectors, mapFaces, mapClipVectors);

    int lastTime;
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    printf("%s\n", info.name);

    bool quit = false;
    SDL_Event e;
    int arrows = 0; //up: 1, left: 2, down: 4, right: 8
    int wasd = 0; //w: 1, a: 2, s: 4, d: 8, space: 16
    while(!quit)
    {
        lastTime = SDL_GetTicks();
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
                quit = true;
            else if(e.type == SDL_KEYDOWN) //wasd keycode thing
            {
                switch(e.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        quit = true;
                    break;

                    case SDLK_w:
                        wasd |= 1;
                    break;

                    case SDLK_a:
                        wasd |= 2;
                    break;

                    case SDLK_s:
                        wasd |= 4;
                    break;

                    case SDLK_d:
                        wasd |= 8;
                    break;

                    case SDLK_SPACE:
                        wasd |= 16;
                    break;

                    case SDLK_UP:
                        arrows |= 1;
                    break;

                    case SDLK_LEFT:
                        arrows |= 2;
                    break;

                    case SDLK_DOWN:
                        arrows |= 4;
                    break;

                    case SDLK_RIGHT:
                        arrows |= 8;
                    break;
                }
            }
            else if(e.type == SDL_KEYUP)
            {
                switch(e.key.keysym.sym)
                {
                    case SDLK_w:
                        wasd &= ~1;
                    break;

                    case SDLK_a:
                        wasd &= ~2;
                    break;

                    case SDLK_s:
                        wasd &= ~4;
                    break;

                    case SDLK_d:
                        wasd &= ~8;
                    break;

                    case SDLK_SPACE:
                        wasd &= ~16;
                    break;


                    case SDLK_UP:
                        arrows &= 14;
                    break;

                    case SDLK_LEFT:
                        arrows &= 13;
                    break;

                    case SDLK_DOWN:
                        arrows &= 11;
                    break;

                    case SDLK_RIGHT:
                        arrows &= 7;
                    break;
                }
            }
            else if(e.type == SDL_MOUSEMOTION)
            {
                player.yaw -= e.motion.xrel*0.0004*SENSITIVITY; //change player's yaw and pitch angles based on the change in mouse position
                player.pitch = clamp(player.pitch + e.motion.yrel*0.0004*SENSITIVITY, -M_PI/2, M_PI/2); //keep pitch within -2*pi and 2*pi
            }
        }

        //FRUSTUM_WIDTH += (float)((arrows & 1) - ((arrows & 4) >> 2)) * 0.01;

        if(player.yaw > 2*M_PI) //keep yaw within the bounds of 0 and 2*pi
            player.yaw -= 2*M_PI;
        else if(player.yaw < 0)
            player.yaw += 2*M_PI;

        //player movement & linear interpolation
        vec3 dv = {.x = (((wasd & 8) >> 3) - ((wasd & 2) >> 1)), .y = ((wasd & 1) - ((wasd & 4) >> 2)), .z = 0};
        dv = mul(unit(dv),player.accel);

        //player.vel.z = (((arrows & 4) >> 2) - (arrows & 1)) * player.speed;

        //janky temporary jumping code
        player.vel.z += 0.5; //gravity
        if(player.pos.z >= -400 && false) //move player out of ground
        {
            player.pos.z = -400;
            player.vel.z = 0;
        }

        //if(player.pos.z == -400 && ((wasd & 16) == 16)) //if jump is pressed and at z == 0 then jump
        if((wasd & 16) == 16)
            player.vel.z = -5;

        float zHold = player.vel.z; //zero z value of velocity vector so movement is only applied to x and y axes
        player.vel.z = 0;

        //FRUSTUM_WIDTH = clamp(length(player.vel) * 2 / player.speed, 0.01, 2);

        player.vel = rotateZ(player.vel, -player.yaw);
        if(dv.x == 0)//decelerate player based on released keys and camera yaw
        {
            if(player.vel.x > 0)
                player.vel.x = max(player.vel.x - player.decel, 0);
            else
                player.vel.x = min(player.vel.x + player.decel, 0);
        }

        if(dv.y == 0)
        {
            if(player.vel.y > 0)
                player.vel.y = max(player.vel.y - player.decel, 0);
            else
                player.vel.y = min(player.vel.y + player.decel, 0);
        }

        player.vel = add(player.vel, dv);
        player.vel = rotateZ(player.vel, player.yaw);

        if(length(player.vel) > player.speed) //limit speed
            player.vel = mul(unit(player.vel),max(length(player.vel) - player.decel, player.speed));
            //player.vel = mul(unit(player.vel), player.speed);
        player.vel.z = zHold;

        //int i;
        //for(i=0;i < mapFacesNum;i++)
        //    clipVelocity(&player, mapFaces[i], mapVectors);

        int clipTimeStart = SDL_GetTicks();
        int i;
        bool velNotClipped = true;
        for(i=0;i < mapFacesNum;i++)
            clippedFaces[i] = true;
        while(velNotClipped)
        {
            velNotClipped = false;
            for(i=0;i < mapFacesNum;i++)
                if(clippedFaces[i])
                {
                    vec3 clipPosHold = player.pos;
                    if(player.vel.z > 0)
                        player.pos.z += 400;
                    else if(player.vel.z < 0)
                        player.pos.z -= 50;
                    player.pos = add(player.pos, mul(unit(dropZ(mapFaces[i].norm)), -100));
                    if(clipVelocity(&player, mapFaces[i], mapVectors))
                    {
                        velNotClipped = true;
                        clippedFaces[i] = false;
                        player.pos = clipPosHold;
                        break;
                    }
                    player.pos = clipPosHold;
                }
            if(SDL_GetTicks() - clipTimeStart > 50) //prevent infinite loop in case something breaks badly
            {
                printf("rekt\n");
                velNotClipped = false;
            }

        }

        player.pos = add(player.pos, player.vel); //move player based on velocity



        //printf("x: %0.2f y: %0.2f z: %0.2f  speed: %0.2f\n", player.pos.x, player.pos.y, player.pos.z, length(player.vel));

        //SDL_SetRenderDrawColor(renderer, 128,128,128, SDL_ALPHA_OPAQUE);
        //SDL_RenderClear(renderer);

        drawFilledFaces(mapFaces, mapFacesNum, player, mapVectors, renderer, mapColours);
        //drawFilledFaces(mapFaces, mapFacesNum, player, mapClipVectors, renderer, mapColours);

        //draw crosshair
        SDL_SetRenderDrawColor(renderer, CROSSHAIR_R, CROSSHAIR_G, CROSSHAIR_B, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(renderer, WIDTH/2 + CROSSHAIR_SIZE, HEIGHT/2, WIDTH/2 - CROSSHAIR_SIZE, HEIGHT/2);
        SDL_RenderDrawLine(renderer, WIDTH/2, HEIGHT/2 + CROSSHAIR_SIZE, WIDTH/2, HEIGHT/2 - CROSSHAIR_SIZE);


        if((arrows & 1) == 1)
        {
            SDL_Rect rekt = {0,0,800,600};
            SDL_BlitSurface(mapTextures[0],NULL,SDL_GetWindowSurface(window),&rekt);
        }


        SDL_RenderPresent(renderer);

        SDL_Delay(max(0,16 - SDL_GetTicks() + lastTime)); //make sure the time interval is always the same
        //SDL_Delay(500);
        //printf("FPS: %d\n", (int)(1000.0f/(float)(SDL_GetTicks() - lastTime))); //print fps
    }

    if(renderer)
        SDL_DestroyRenderer(renderer);

    if(window)
        SDL_DestroyWindow(window);

    int i;
    for(i=0;i < mapTexturesNum;i++)
        SDL_FreeSurface(mapTextures[i]);

    free(mapVectors);
    free(mapFaces);
    free(mapColours);
    free(clippedFaces);
    free(mapClipVectors);
    free(mapTextures);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

/*
map file format:
player.pos.x,player.pos.y,player.pos.z,player.vel.x,player.vel.y,player.vel.z,player.pitch,player.yaw,player.speed,player.accel,player.decel
num_of_vertices,num_of_faces,num_of_colours
vec0.x,vec0.y,vec0.z
vec1.x,vec1.y,vec1.z
vec2.x,vec2.y,vec2.z&(*player)
...
face0.p1,face0.p2,face0.p3,face0.texture,face0.norm.x,face0.norm.y,face0.norm.z,face0.type,face0.flags
face1.p1,face1.p2,face1.p3,face1.texture,face1.norm.x,face1.norm.y,face1.norm.z,face1.type,face1.flags
...
*/

/*
since calculating the surface normals for every face manually can be tedious, it is calculated by taking the cross product of the vectors p0-p1 and p0-p2
the type is used to determine whether the face normal points towards the origin or away (0 towards, 1 away)
*/

void loadMap(int *nVectors, int *nFaces, int *nColors, vec3 **vectors, face **faces, colour **colours, char *fileName, camera *player, SDL_Surface ***textures, int *nTextures)
{
    FILE *mapFile = fopen(fileName, "r");
    fscanf(mapFile,"%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", &(*player).pos.x, &(*player).pos.y, &(*player).pos.z, &(*player).vel.x, &(*player).vel.y, &(*player).vel.z, &(*player).pitch, &(*player).yaw, &(*player).speed, &(*player).accel, &(*player).decel);
    fscanf(mapFile,"%d,%d,%d,%d\n",nVectors,nFaces,nColors,nTextures);
    *vectors = (vec3 *)malloc(*nVectors * sizeof(vec3));
    *faces = (face *)malloc(*nFaces * sizeof(face));
    *colours = (colour *)malloc(*nColors * sizeof(colour));
    *textures = (SDL_Surface **)malloc(*nTextures * sizeof(SDL_Surface *));

    int i;
    for(i=0;i < *nVectors;i++)
        fscanf(mapFile,"%f,%f,%f\n",&(*vectors)[i].x, &(*vectors)[i].y, &(*vectors)[i].z);
    for(i=0;i < *nFaces;i++)
    {
        fscanf(mapFile,"%d,%d,%d,%d,%f,%f,%f,%d,%d,%f,%f,%f,%f,%f,%f\n", &(*faces)[i].p1, &(*faces)[i].p2, &(*faces)[i].p3, &(*faces)[i].texture, &(*faces)[i].norm.x, &(*faces)[i].norm.y, &(*faces)[i].norm.z, &(*faces)[i].type, &(*faces)[i].flags, &(*faces)[i].uv1.x, &(*faces)[i].uv1.y, &(*faces)[i].uv2.x, &(*faces)[i].uv2.y, &(*faces)[i].uv3.x, &(*faces)[i].uv3.y);
        (*faces)[i].mid = mul(add((*vectors)[(*faces)[i].p1],add((*vectors)[(*faces)[i].p2],(*vectors)[(*faces)[i].p3])), 1.0/3.0);
        if(GENERATE_FACE_NORMALS)
        {
            (*faces)[i].norm = unit(cross(sub((*vectors)[(*faces)[i].p2], (*vectors)[(*faces)[i].p1]), sub((*vectors)[(*faces)[i].p3], (*vectors)[(*faces)[i].p1])));
            //(*faces)[i].norm = unit(mul((*faces)[i].norm, dot((*faces)[i].norm,(*faces)[i].mid) * ((*faces)[i].type * 2 - 1)));
            if(((dot((*faces)[i].norm, (*faces)[i].mid) <= 0) == (*faces)[i].type) || (dot((*faces)[i].norm, (*faces)[i].mid) == 0 && (*faces)[i].norm.z > 0))
                (*faces)[i].norm = mul((*faces)[i].norm, -1);
        }

    }
    for(i=0;i < *nColors;i++)
        fscanf(mapFile,"%d,%d,%d\n", &(*colours)[i].r, &(*colours)[i].g, &(*colours)[i].b);
    static char tempString[30];
    for(i=0;i < *nTextures;i++)
    {
        fscanf(mapFile,"%[^\n]\n",tempString);
        (*textures)[i] = SDL_LoadBMP(tempString);
    }


    fclose(mapFile);
}


#define OFFSET 100.0 //temporary until the offset value is added to the face struct

void buildClipVectors(int nVectors, int nFaces, vec3 *mapVectors, face *mapFaces, vec3 *clipVectors) //sorry, clip machine broke
{
    int i;
    for(i=0;i < nVectors;i++)
    {
        int j;
        int nLinkedFaces = 0;
        for(j=0;j < nFaces;j++)
            if(mapFaces[j].p1 == i || mapFaces[j].p2 == i || mapFaces[j].p3 == i)
                nLinkedFaces++;

        if(nLinkedFaces == 0) //gg
            continue;

        face *linkedFaces = malloc(sizeof(face) * nLinkedFaces);

        int k = 0;
        for(j=0;j < nFaces && k < nLinkedFaces;j++)
            if(mapFaces[j].p1 == i || mapFaces[j].p2 == i || mapFaces[j].p3 == i)
                linkedFaces[k++] = mapFaces[j];

        int nFinalFaces = nLinkedFaces;

        for(j=0;j < nLinkedFaces;j++) //type is not being used here so use it as a null flag
            for(k=j+1; linkedFaces[j].type != -1 && k < nLinkedFaces;k++)
                if(linkedFaces[k].type != -1 && fabs(linkedFaces[k].norm.x) == fabs(linkedFaces[j].norm.x) && fabs(linkedFaces[k].norm.y) == fabs(linkedFaces[j].norm.y) && fabs(linkedFaces[k].norm.z) == fabs(linkedFaces[j].norm.z))
                {
                    //printf("wew");
                    linkedFaces[k].type = -1;
                    nFinalFaces--;
                }

        int faceNum;
        if(nFinalFaces == 1)
        {
            for(faceNum=0;linkedFaces[faceNum].type == -1 && faceNum < nLinkedFaces;faceNum++);
            clipVectors[i] = add(mapVectors[i], mul(linkedFaces[faceNum].norm, OFFSET));
        }
        else
        {
            vec3 norm1, norm2, norm3;
            vec3 origin1, origin2, origin3;
            float d1, d2, d3;

            for(faceNum=0;linkedFaces[faceNum].type == -1 && faceNum < nLinkedFaces;faceNum++);
            norm1 = linkedFaces[faceNum].norm;
            if(i == 15)
            {
                int rip;
                for(rip=0;rip < nFaces;rip++)
                    if(mapFaces[rip].p1 == linkedFaces[faceNum].p1 && mapFaces[rip].p2 == linkedFaces[faceNum].p2 && mapFaces[rip].p3 == linkedFaces[faceNum].p3)
                    {
                        mapFaces[rip].texture = 4;
                        break;
                    }
            }

            //origin1 = add(mapVectors[i], mul(norm1, OFFSET));
            origin1 = add(mapVectors[linkedFaces[faceNum].p1], mul(norm1, OFFSET));
            linkedFaces[faceNum].type = -1;

            for(faceNum=0;linkedFaces[faceNum].type == -1 && faceNum < nLinkedFaces;faceNum++);
            norm2 = linkedFaces[faceNum].norm;
            if(i == 15)
            {
                int rip;
                for(rip=0;rip < nFaces;rip++)
                    if(mapFaces[rip].p1 == linkedFaces[faceNum].p1 && mapFaces[rip].p2 == linkedFaces[faceNum].p2 && mapFaces[rip].p3 == linkedFaces[faceNum].p3)
                    {
                        mapFaces[rip].texture = 4;
                        break;
                    }
            }
            //origin2 = add(mapVectors[i], mul(norm2, OFFSET));
            origin2 = add(mapVectors[linkedFaces[faceNum].p1], mul(norm2, OFFSET));
            linkedFaces[faceNum].type = -1;

            nFinalFaces -= 2;

            do
            {
                if(nFinalFaces <= 0 && false)
                {
                    norm3 = unit(cross(norm1, norm2));
                    origin3 = mapVectors[i];
                    break;
                }
                else
                {
                    for(faceNum=0;faceNum < nLinkedFaces && linkedFaces[faceNum].type == -1;faceNum++);
                    norm3 = linkedFaces[faceNum].norm;
                    if(i == 15 && nFinalFaces  > 1)
                    {
                        int rip;
                        for(rip=0;rip < nFaces;rip++)
                            if(mapFaces[rip].p1 == linkedFaces[faceNum].p1 && mapFaces[rip].p2 == linkedFaces[faceNum].p2 && mapFaces[rip].p3 == linkedFaces[faceNum].p3)
                            {
                                mapFaces[rip].texture = 3;
                                break;
                            }
                    }
                    //origin3 = add(mapVectors[i], mul(norm3, OFFSET));
                    origin3 = add(mapVectors[linkedFaces[faceNum].p1], mul(norm3, OFFSET));
                    linkedFaces[faceNum].type = -1;
                    nFinalFaces--;
                }
            }
            while(dot(norm1, cross(norm2, norm3)) == 0);


            d1 = dot(origin1, norm1);
            d2 = dot(origin2, norm2);
            d3 = dot(origin3, norm3);

            /*
            vec3 lStart, lNorm;
            lNorm = unit(cross(norm1, norm2));

            float lineX = fabs(lNorm.x);
            float lineY = fabs(lNorm.y);
            float lineZ = fabs(lNorm.z);

            if(lineX > lineY && lineX > lineZ)
            {
                lStart.x = 0;
                float det = norm1.y * norm2.z - norm1.z * norm2.y;
                lStart.y = (norm2.z * d1 - norm1.z * d2) / det;
                lStart.z = (norm1.y * d2 - norm2.y * d1) / det;
            }
            else if(lineY > lineZ)
            {
                lStart.y = 0;
                float det = norm1.x * norm2.z - norm1.z * norm2.x;
                lStart.x = (norm2.z * d1 - norm1.z * d2) / det;
                lStart.z = (norm1.x * d2 - norm2.x * d1) / det;
            }
            else
            {
                lStart.z = 0;
                float det = norm1.x * norm2.y - norm1.y * norm2.x;
                lStart.x = (norm2.z * d1 - norm1.z * d2) / det;
                lStart.y = (norm1.x * d2 - norm2.x * d1) / det;
            }

            printf("%d %d %d %0.4f %0.4f %0.4f\n", i, nLinkedFaces, nFinalFaces, d1, d2, dot(lNorm, norm3));

            if(i == 14 || i == 15)
            {
                printf("%0.3f %0.3f %0.3f   ", lNorm.x, lNorm.y, lNorm.z);
                printf("%0.3f %0.3f %0.3f   ", norm1.x, norm1.y, norm1.z);
                printf("%0.3f %0.3f %0.3f\n", norm2.x, norm2.y, norm2.z);
            }

            if(dot(lNorm, norm3) != 0)
                clipVectors[i] = add(lStart, mul(lNorm, (d3 - dot(lStart, norm3)) / dot(lNorm, norm3)));
            else
                clipVectors[i] = mapVectors[i];

            */


            float det = dot(norm1, cross(norm2, norm3));

            printf("%d %f %d %d %0.4f %0.4f %0.4f\n", i, det, nLinkedFaces, nFinalFaces, length(norm1), length(norm2), length(norm3));

            if(i == 14 && i == 15)
            {
                printf("%0.3f %0.3f %0.3f   ", norm1.x, norm1.y, norm1.z);
                printf("%0.3f %0.3f %0.3f   ", norm2.x, norm2.y, norm2.z);
                printf("%0.3f %0.3f %0.3f\n", norm3.x, norm3.y, norm3.z);
            }


            if(det != 0) //if two of the planes being used were too similar in orientation, det would be too small and cause division overflow
                clipVectors[i] = mul(add(mul(cross(norm2, norm3),d1), add(mul(cross(norm3, norm1),d2), mul(cross(norm1, norm2),d3))), 1.0/det);
            //else
                //clipVectors[i] = mapVectors[i];

        }

        free(linkedFaces);
    }

}


void loadConstants(int *w, int *h, float *sens, char *map, float *frustumW, float *frustumN, int *edges, char *fileName)
{
    FILE *settingsFile = fopen(fileName, "r");
    fscanf(settingsFile, "map = %[^\n]\n", map);
    fscanf(settingsFile, "width = %d\n", w);
    fscanf(settingsFile, "height = %d\n", h);
    fscanf(settingsFile, "sensitivity = %f\n", sens);
    fscanf(settingsFile, "frustum width = %f\n", frustumW);
    fscanf(settingsFile, "frustum near length = %f\n", frustumN);
    fscanf(settingsFile, "draw edges = %d", edges);
    fclose(settingsFile);
}

void drawFilledFaces(face *faces, int nFaces, camera player, vec3 *points, SDL_Renderer *renderer, colour *colours)
{
    int facesIndex[nFaces]; //index of each visible face
    int i, nVisible = 0;
    for(i=0;i < nFaces;i++) //cull faces which are facing away from the player, or are behind the player
        if((!BACKFACE_CULL_FILL || dot(sub(player.pos, points[faces[i].p1]), faces[i].norm) >= 0)
            && (rotateX(rotateZ(sub(points[faces[i].p1], player.pos), -player.yaw), -player.pitch).y >= FRUSTUM_NEAR_LENGTH
            || rotateX(rotateZ(sub(points[faces[i].p2], player.pos), -player.yaw), -player.pitch).y >= FRUSTUM_NEAR_LENGTH
            || rotateX(rotateZ(sub(points[faces[i].p3], player.pos), -player.yaw), -player.pitch).y >= FRUSTUM_NEAR_LENGTH))
            facesIndex[nVisible++] = i;

    if(nVisible > 0)
    {
        float dist[nFaces];//sort facesIndex based on median depth value of each corner of the triangle after translation and rotation relative to player
        for(i=0;i < nVisible;i++)
        {
            float L1 = length(rotateX(rotateZ(sub(points[faces[facesIndex[i]].p1], player.pos), -player.yaw), -player.pitch));
            float L2 = length(rotateX(rotateZ(sub(points[faces[facesIndex[i]].p2], player.pos), -player.yaw), -player.pitch));
            float L3 = length(rotateX(rotateZ(sub(points[faces[facesIndex[i]].p3], player.pos), -player.yaw), -player.pitch));
            dist[facesIndex[i]] = (L1 + L2 + L3);//min((L1 + L2 + L3 - min(min(L1, L2), L3) - max(max(L1, L2), L3)), (L1 + L2 + L3)/3.0);
        }

        quicksort(facesIndex, dist, 0, nVisible-1);

        //fill in order from furthest to closest (painter's algorithm) or front to back with some spicy clipping (reverse painter's algorithm)
        for(i=nVisible-1;i >= 0;i--)
        {
            SDL_SetRenderDrawColor(renderer, colours[faces[facesIndex[i]].texture].r, colours[faces[facesIndex[i]].texture].g, colours[faces[facesIndex[i]].texture].b, SDL_ALPHA_OPAQUE);
            transformFace(faces[facesIndex[i]], points, player, renderer);
        }

    }

}


void quicksort(int list[], float ref[], int l, int r)
{
    int a=l,b=r;
    int p = l;
    int hold;
    while(p != a || p != b)
    {
        while(ref[list[p]] <= ref[list[b]] && p != b)
            b--;
        hold = list[b];
        list[b] = list[p];
        list[p] = hold;
        p = b;
        while(ref[list[p]] >= ref[list[a]] && p != a)
            a++;
        hold = list[a];
        list[a] = list[p];
        list[p] = hold;
        p = a;
    }
    if(p > l)
        quicksort(list,ref,l,p-1);
    if(p < r)
        quicksort(list,ref,p+1,r);

}

void transformFace(face f, vec3 *points, camera player, SDL_Renderer *renderer) //also fills face
{
    vec3 pointsR[3] = {rotateX(rotateZ(sub(points[f.p1], player.pos), -player.yaw), -player.pitch), //rotate and translate points relative to player
    rotateX(rotateZ(sub(points[f.p2], player.pos), -player.yaw), -player.pitch),
    rotateX(rotateZ(sub(points[f.p3], player.pos), -player.yaw), -player.pitch)};
    vec3 forward = {0,1,0};

    int i;
    int nPoints = 0;
    vec2 pointsOut[4];
    for(i=0;i < 3;i++)
    {
        if(pointsR[i].y >= FRUSTUM_NEAR_LENGTH)
            pointsOut[nPoints++] = perspective2d(pointsR[i]);
        if((pointsR[i].y >= FRUSTUM_NEAR_LENGTH) != (pointsR[(i+1)%3].y >= FRUSTUM_NEAR_LENGTH))
            pointsOut[nPoints++] = perspective2d(add(pointsR[i], mul(sub(pointsR[(i+1)%3],pointsR[i]), dot(sub(mul(forward,FRUSTUM_NEAR_LENGTH),pointsR[i]),forward)/dot(sub(pointsR[(i+1)%3],pointsR[i]),forward))));
    }

    int codes[nPoints];
    for(i=0;i < nPoints;i++)
    {
        codes[i] = 0;
        if(pointsOut[i].y < 0)
            codes[i] |= 1;
        else if(pointsOut[i].y > HEIGHT)
            codes[i] |= 2;
        if(pointsOut[i].x > WIDTH)
            codes[i] |= 4;
        if(pointsOut[i].x < 0)
            codes[i] |= 8;
    }

    int visible = codes[0];
    for(i=1;i < nPoints;i++) //check if any segment of the polygon is in the view frustum
        visible &= codes[i];


    if(visible == 0)
    {
        fillTriangle(pointsOut[0], pointsOut[1], pointsOut[2], renderer);
        if(nPoints == 4)
        {
            fillTriangle(pointsOut[0], pointsOut[2], pointsOut[3], renderer);
            if(DRAW_EDGES == 2)
                SDL_SetRenderDrawColor(renderer, 50,50,50,SDL_ALPHA_OPAQUE);
            drawClippedLine(pointsOut[0], pointsOut[2], renderer);
            //SDL_RenderDrawLine(renderer, pointsOut[0].x + (float)WIDTH/2, pointsOut[0].y + (float)HEIGHT/2, pointsOut[2].x + (float)WIDTH/2, pointsOut[2].y + (float)HEIGHT/2);
        }

        if(DRAW_EDGES)
        {
            SDL_SetRenderDrawColor(renderer, 0,0,0,SDL_ALPHA_OPAQUE);
            drawWireframePolygon(pointsOut, nPoints, renderer);
        }
    }



}

void swapVec2Ptr(vec2 **p1, vec2 **p2)
{
    vec2 *hold = *p1;
    *p1 = *p2;
    *p2 = hold;
}

void fillTriangle(vec2 p1, vec2 p2, vec2 p3, SDL_Renderer *renderer)
{
    vec2 *top = &p1;
    vec2 *mid = &p2;
    vec2 *bot = &p3;

    //sort by y value

    if(top->y > bot->y)
        swapVec2Ptr(&top,&bot);
    if(top->y > mid->y)
        swapVec2Ptr(&top,&mid);
    if(mid->y > bot->y)
        swapVec2Ptr(&mid,&bot);

    vec2 mid2 = {.x = (bot->x - top->x) * (mid->y - top->y) / (bot->y - top->y) + top->x, .y = mid->y};

    if(mid->y != top->y) //draw flat bottom triangle
    {
        float starty = max(top->y, 0); //top
        float endy = min(mid->y, HEIGHT); //bottom

        float slope1 = (mid->x - top->x) / (mid->y - top->y);
        float slope2 = (mid2.x - top->x) / (mid2.y - top->y);

        float x1 = top->x + (slope1 * (starty - top->y));
        float x2 = top->x + (slope2 * (starty - top->y));

        float y;
        for(y = starty;y <= endy;y++)
        {
            if((x1 >= 0 || x2 >= 0) && (x1 <= WIDTH || x2 <= WIDTH))
                SDL_RenderDrawLine(renderer, clamp(min(x1,x2),0,WIDTH) - 1, y + 0.5f, clamp(max(x1,x2),0,WIDTH) + 1, y + 0.5f);
            x1 += slope1;
            x2 += slope2;
            //SDL_RenderPresent(renderer);
        }
    }

    if(mid->y != bot->y) //draw flat top triangle
    {
        float starty = max(mid->y, 0);
        float endy = min(bot->y, HEIGHT);

        float slope1 = (bot->x - mid->x) / (bot->y - mid->y);
        float slope2 = (bot->x - mid2.x) / (bot->y - mid2.y);

        float x1 = mid->x + (slope1 * (starty - mid->y));
        float x2 = mid2.x + (slope2 * (starty - mid->y));

        float y;
        for(y = starty;y <= endy;y++)
        {
            if((x1 >= 0 || x2 >= 0) && (x1 <= WIDTH || x2 <= WIDTH))
                SDL_RenderDrawLine(renderer, clamp(min(x1,x2),0,WIDTH), y + 0.5f, clamp(max(x1,x2),0,WIDTH) + 1, y + 0.5f);
            x1 += slope1;
            x2 += slope2;
            //SDL_RenderPresent(renderer);
        }
    }

    //SDL_RenderDrawLine(renderer, mid->x, mid->y, mid2.x, mid2.y);
    //SDL_RenderDrawLine(renderer, top->x , top->y + 0.5f, mid->x, mid->y + 0.5f);
    //SDL_RenderDrawLine(renderer, top->x, top->y + 0.5f, bot->x, bot->y + 0.5f);
    //SDL_RenderDrawLine(renderer, bot->x, bot->y + 0.5f, mid->x, mid->y + 0.5f);
    drawClippedLine(p1, p2, renderer);
    drawClippedLine(p1, p3, renderer);
    drawClippedLine(p3, p2, renderer);
}

void drawWireframePolygon(vec2 *polygon, int nPoints, SDL_Renderer *renderer)
{
    int i;
    for(i=0;i < nPoints;i++)
        drawClippedLine(polygon[i], polygon[(i+1)%nPoints], renderer);
        //SDL_RenderDrawLine(renderer, polygon[i].x + WIDTH/2, polygon[i].y + HEIGHT/2, polygon[(i+1)%nPoints].x + WIDTH/2, polygon[(i+1)%nPoints].y + HEIGHT/2);
}

int getClipCode(vec2 a) //up 1, down 2, right 4, left 8
{
    int out = 0;
    if(a.y < 0)
        out |= 1;
    else if(a.y > HEIGHT)
        out |= 2;
    if(a.x > WIDTH)
        out |= 4;
    else if(a.x < 0)
        out |= 8;
    return out;
}

void drawClippedLine(vec2 a, vec2 b, SDL_Renderer *renderer) //up 1, down 2, right 4, left 8
{
    bool found = false, valid = false;
    int codeA = getClipCode(a);
    int codeB = getClipCode(b);

    while(!found)
    {
        if(!(codeA | codeB))
            found = valid = true;
        else if((codeA & codeB))
            found = true;
        else
        {
            float x, y;
            int code = (codeA ? codeA : codeB);

            if(code & 1)
            {
                x = a.x + (b.x - a.x) * -a.y / (b.y - a.y);
                y = 0;
            }
            else if(code & 2)
            {
                x = a.x + (b.x - a.x) * (HEIGHT - a.y) / (b.y - a.y);
                y = HEIGHT;
            }
            else if(code & 4)
            {
                y = a.y + (b.y - a.y) * (WIDTH - a.x) / (b.x - a.x);
                x = WIDTH;
            }
            else if(code & 8)
            {
                y = a.y + (b.y - a.y) * -a.x / (b.x - a.x);
                x = 0;
            }

            if(code == codeA)
            {
                a.x = x;
                a.y = y;
                codeA = getClipCode(a);
            }
            else
            {
                b.x = x;
                b.y = y;
                codeB = getClipCode(b);
            }
        }
    }

    if(valid)
        SDL_RenderDrawLine(renderer, a.x, a.y + 0.5f, b.x, b.y + 0.5f);

}

bool clipVelocity(camera *player, face triangle, vec3 *points)
{
    float e = 0.001;
    if(dot(triangle.norm, player->vel) >= 0)
        return false;
    float d = dot(sub(points[triangle.p1], player->pos), triangle.norm) / dot(player->vel, triangle.norm);

    if(d >= 1 || d < -e)
        return false;

    vec3 intersect = add(player->pos, mul(player->vel, d));
    vec3 t2 = sub(points[triangle.p2], points[triangle.p1]);
    vec3 t3 = sub(points[triangle.p3], points[triangle.p1]);
    vec3 w = sub(intersect, points[triangle.p1]);

    float bot = dot(t2, t3);
    bot *= bot;
    bot -= dot(t2, t2) * dot(t3, t3);

    float u = ((dot(t2, t3) * dot(w, t2)) - (dot(t2, t2) * dot(w, t3))) / bot;
    float v = ((dot(t2, t3) * dot(w, t3)) - (dot(t3, t3) * dot(w, t2))) / bot;

    if(v < -e || v > 1.0 + e || u < -e || u > 1.0 + e || u + v > 1.0 + e)
        return false;

    player->vel = sub(player->vel, mul(triangle.norm, dot(triangle.norm, mul(player->vel, 1.0 - d)) -e));
    return true;
}

void drawWireframeFace(face f, camera player, vec3 *points, SDL_Renderer *render)
{
    if(!BACKFACE_CULL_WIREFRAME || dot(sub(player.pos, points[f.p1]), f.norm) >= 0) //if backface culling is enabled, only draw if the face is facing towards the player
    {
        vec3 p1R = rotateX(rotateZ(sub(points[f.p1], player.pos), -player.yaw), -player.pitch); //rotate and translate points relative to player
        vec3 p2R = rotateX(rotateZ(sub(points[f.p2], player.pos), -player.yaw), -player.pitch);
        vec3 p3R = rotateX(rotateZ(sub(points[f.p3], player.pos), -player.yaw), -player.pitch);

        drawLine(p1R,p2R,render); //draw the 3 lines that make up the triangle face
        drawLine(p2R,p3R,render);
        drawLine(p3R,p1R,render);
    }
}

void drawLine(vec3 p1, vec3 p2, SDL_Renderer *render)
{
    vec3 start = {p1.x, p1.y, p1.z};
    vec3 end = {p2.x, p2.y, p2.z};
    vec3 disp = sub(end, start);

    if(start.y >= FRUSTUM_NEAR_LENGTH || end.y >= FRUSTUM_NEAR_LENGTH)
    {
        vec3 left = {-FRUSTUM_WIDTH, 1, 0}; //normals of the planes which make up the view frustum (point inwards)
        left = unit(left);
        vec3 right = {FRUSTUM_WIDTH, 1, 0};
        right = unit(right);
        vec3 up = {0, 1, FRUSTUM_WIDTH};
        up = unit(up);
        vec3 down = {0, 1, -FRUSTUM_WIDTH};
        down = unit(down);
        vec3 forward = {0,1,0};


        if(start.y <= FRUSTUM_NEAR_LENGTH)
            start = add(start, mul(disp, dot(sub(mul(forward,FRUSTUM_NEAR_LENGTH),start),forward)/dot(disp,forward)));
            //start = add(start, mul(disp, dot(mul(start, -1), forward)/dot(disp,forward)));
        if(end.y <= FRUSTUM_NEAR_LENGTH)
            end = add(start, mul(disp, dot(sub(mul(forward,FRUSTUM_NEAR_LENGTH),start),forward)/dot(disp,forward)));
            //end = add(start, mul(disp, dot(mul(start, -1), forward)/dot(disp,forward)));

        int code1 = 0, code2 = 0;
        if(dot(start, up) < 0)
            code1 |= 1;
        if(dot(start, down) < 0)
            code1 |= 2;
        if(dot(start, right) < 0)
            code1 |= 4;
        if(dot(start, left) < 0)
            code1 |= 8;

        if(dot(end, up) < 0)
            code2 |= 1;
        if(dot(end, down) < 0)
            code2 |= 2;
        if(dot(end, right) < 0)
            code2 |= 4;
        if(dot(end, left) < 0)
            code2 |= 8;

        if((code1 & code2) == 0)
        {
            /*
            if((code1 & 1) == 1) //start should be clipped to top plane
                start = add(start, mul(disp, dot(mul(start,-1),up)/dot(disp,up)));
            else if((code1 & 2) == 2) //start should be clipped to bottom plane
                start = add(start, mul(disp, dot(mul(start,-1),down)/dot(disp,down)));
            if((code1 & 4) == 4) //start should be clipped to right plane
                start = add(start, mul(disp, dot(mul(start,-1),right)/dot(disp,right)));
            else if((code1 & 8) == 8) //start should be clipped to left plane
                start = add(start, mul(disp, dot(mul(start,-1),left)/dot(disp,left)));


            if((code2 & 1) == 1) //end should be clipped to top plane
                end = add(start, mul(disp, dot(mul(start,-1),up)/dot(disp,up)));
            else if((code2 & 2) == 2) //end should be clipped to bottom plane
                end = add(start, mul(disp, dot(mul(start,-1),down)/dot(disp,down)));
            if((code2 & 4) == 4) //end should be clipped to right plane
                end = add(start, mul(disp, dot(mul(start,-1),right)/dot(disp,right)));
            else if((code2 & 8) == 8) //end should be clipped to left plane
                end = add(start, mul(disp, dot(mul(start,-1),left)/dot(disp,left)));
            */


            //3d projection transformation
            start.z *= FRUSTUM_WIDTH * (WIDTH/2) / start.y;
            start.x *= FRUSTUM_WIDTH * (WIDTH/2)  / start.y;

            end.z *= FRUSTUM_WIDTH * (WIDTH/2) / end.y;
            end.x *= FRUSTUM_WIDTH * (WIDTH/2)  / end.y;

            //printf("%f %f   %f %f", start.x, start.z, end.x, end.z);
            SDL_RenderDrawLine(render,start.x + WIDTH/2, start.z + HEIGHT/2, end.x + WIDTH/2, end.z + HEIGHT/2);
        }
    }

}

//more useful stuff

int sign(float a)
{
    if(a > 0)
        return 1;
    if(a < 0)
        return -1;
    return 0;
}

float min(float a, float b)
{
    return a < b ? a : b;
}

float max(float a, float b)
{
    return a > b ? a : b;
}

float clamp(float a, float bot, float top)
{
    return min(max(bot, a), top);
}

float length(vec3 a)
{
    return sqrt((double)((a.x * a.x) + (a.y * a.y) + (a.z * a.z)));
}

float dot(vec3 a, vec3 b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

vec3 add(vec3 a, vec3 b)
{
    vec3 r = {.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
    return r;
}

vec3 sub(vec3 a, vec3 b)
{
    vec3 r = {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
    return r;
}

vec3 mul(vec3 a, float b)
{
    vec3 r = {.x = a.x * b, .y = a.y * b, .z = a.z * b};
    return r;
}

vec3 unit(vec3 a)
{
    float len = length(a);
    vec3 r = mul(a,(len != 0 ? 1/len : 0));//check for vector length of 0 to prevent divide by zero error
    return r;
}

vec3 cross(vec3 a, vec3 b)
{
    vec3 r = {.x = (a.y * b.z) - (a.z * b.y), .y = (a.z * b.x) - (a.x * b.z), .z = (a.x * b.y) - (a.y * b.x)};
    return r;
}

vec3 rotate(vec3 a, vec3 b, float theta) //rotates vector a theta radians around axis b //TODO: implement the more efficient way of doing this
{
    vec3 parallel = mul(unit(a),dot(a,unit(b)));
    vec3 perpendicular = sub(a,parallel);
    vec3 r = add(mul(add(mul(unit(perpendicular),cos(theta)),mul(unit(cross(b,perpendicular)),sin(theta))),length(perpendicular)),parallel);
    return r;
}

vec3 rotateZ(vec3 a, float theta) //faster version of rotate with z as the axis
{
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);

    vec3 r = {.x = a.x * cosTheta - a.y * sinTheta, .y = a.y * cosTheta + a.x * sinTheta, .z = a.z};
    return r;
}

vec3 rotateX(vec3 a, float theta)
{
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);

    vec3 r = {.x = a.x, .y = a.y * cosTheta - a.z * sinTheta, .z = a.z * cosTheta + a.y * sinTheta};
    return r;
}

vec3 dropX(vec3 a)
{
    vec3 r = {.x = 0, .y = a.y, .z = a.z};
    return r;
}

vec3 dropY(vec3 a)
{
    vec3 r = {.x = a.x, .y = 0, .z = a.z};
    return r;
}

vec3 dropZ(vec3 a)
{
    vec3 r = {.x = a.x, .y = a.y, .z = 0};
    return r;
}

vec2 add2(vec2 a, vec2 b)
{
    vec2 r = {.x = a.x + b.x, .y = a.y + b.y};
    return r;
}

vec2 sub2(vec2 a, vec2 b)
{
    vec2 r = {.x = a.x - b.x, .y = a.y - b.y};
    return r;
}

vec2 mul2(vec2 a, float b)
{
    vec2 r = {.x = a.x * b, .y = a.y * b};
    return r;
}

float length2(vec2 a)
{
    return sqrt((double)((a.x * a.x) + (a.y * a.y)));
}

float dot2(vec2 a, vec2 b)
{
    return (a.x * b.x) + (a.y * b.y);
}

float cross2(vec2 a, vec2 b) //magnitude of the cross product
{
    return (a.x * b.y) - (a.y * b.x);
}

vec2 unit2(vec2 a)
{
    float len = length2(a);
    vec2 r = mul2(a,(len != 0 ? 1/len : 0));//check for vector length of 0 to prevent divide by zero error
    return r;
}

vec3 toVec3(vec2 a, float z)
{
    vec3 r = {.x = a.x, .y = a.y, .z = z};
    return r;
}

vec2 perspective2d(vec3 a)
{
    vec2 r = {.x = a.x * FRUSTUM_WIDTH * ((float)WIDTH/2.0) / a.y + (float)WIDTH/2.0, .y = a.z * FRUSTUM_WIDTH * ((float)WIDTH/2.0) / a.y + (float)HEIGHT/2.0};
    return r;
}

bool intersection(vec2 a1, vec2 a2, vec2 b1, vec2 b2, vec2 *result)
{
    vec2 da = sub2(a1,a2);
    vec2 db = sub2(b1,b2);
    float ca = cross2(a1,a2);
    float cb = cross2(b1,b2);
    float base = cross2(da,db);
    if(base == 0)
        return false;
    *result = mul2(sub2(mul2(db,ca),mul2(da,cb)), 1.0/base);
    return true;
}
