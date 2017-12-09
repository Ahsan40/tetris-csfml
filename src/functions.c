#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SFML/System/Clock.h>
#include <SFML/Graphics/RenderWindow.h>
#include <SFML/Graphics/Text.h>
#include <SFML/Graphics/RectangleShape.h>

#include "common.h"
#include "tet_conf.h"
#include "functions.h"

/* Externs from main.c */
extern Game game;
extern Shape active, next;
extern Field fld;

extern sfFont *fontScore;

extern char arrKeys;    // Arrow keys states byte container

extern sfClock *gameTick;
extern sfClock *putTick;
extern sfClock *mTick;
extern sfClock *repPushDown; // Clock for repeat latency when Down key held
extern sfClock *repKeyLeft; // Clock for repeat latency when Left key held
extern sfClock *repKeyRight; // Clock for repeat latency when Left key held

/* Shapes maps */
extern char arrShapeL[4][4];
extern char arrShapeRL[4][4];
extern char arrShapeZ[4][4];
extern char arrShapeS[4][4];
extern char arrShapeB[4][4];
extern char arrShapeI[4][4];
extern char arrShapeT[4][4];


/* Field init routine */
void initFld()
{
    sfVector2f fldCPos[22][10];
    /* Create field */
    for (int j = 0; j < fld.size.y; j++) {
        for (int i = 0; i < fld.size.x; i++) {
            fld.c[j][i].a = 0;    // Inactive = empty
            fldCPos[j][i].x
                = fld.pos.x + (i * (fld.cSize.x + 2 * fld.cOutThick));
            fldCPos[j][i].y
                = fld.pos.y - (j * (fld.cSize.y + 2 * fld.cOutThick));
            fld.p[j][i] = sfRectangleShape_create();
            sfRectangleShape_setFillColor(fld.p[j][i], UIBGCOLOR);
            sfRectangleShape_setSize(fld.p[j][i], fld.cSize);
            sfRectangleShape_setPosition(fld.p[j][i], fldCPos[j][i]);
            sfRectangleShape_setOutlineColor(fld.p[j][i], UIFGACTIVECOLOR);
            sfRectangleShape_setOutlineThickness(fld.p[j][i], fld.cOutThick);
        }
    }
    /* Create next shape field */
    sfVector2f nsPos;
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            nsPos.x = next.x+i*(next.cSize.x+2*fld.cOutThick);
            nsPos.y = next.y-j*(next.cSize.y+2*fld.cOutThick);
            next.p[j][i] = sfRectangleShape_create();
            sfRectangleShape_setSize(next.p[j][i], next.cSize);
            sfRectangleShape_setPosition(next.p[j][i], nsPos);
            sfRectangleShape_setOutlineThickness(next.p[j][i], fld.cOutThick);
        }
    }
    genNextShape();
    resetActiveShape(&active);
}


void valueAfterTextDisplay(int value, List *texts, char *type)
{
    List *l = texts;
    while (l) {
        Text *text = (Text *)l->obj;
        if (text->type)
            if (!strcmp(text->type, type)) {
                char a[64];
                if (text->text) {
                    if (strlen(text->text) > 52) {
                        memset(text->text+52, '\0', 1);
                        strcpy(a, text->text);
                    }
                    sprintf(a, "%s%d", text->text, value);
                    sfText_setString(text->sfText, (char *)&a);
                    break;
                }
            }
        l = l->next;
    }
}

/*
 * Removes line when cells all are in row in it
 *
 */
int rmLines()
{
    int k = 0; // "Filled line" indicator
    int s = 0;
    for (int j = 0; j < 22; j++) {
        for (int i = 0; i < 10; i++)
            if (fld.c[j][i].a != 0)
                k++;
        if (k >= 10) {    // If line is full
            s++;    // Give scores for line
            for (int n = j; n < 22; n++) {    // Drop all lines down
                if (n == 21) {
                    for (int m = 0; m < 10; m++) {
                        fld.c[n][m].a = 0;
                        fld.c[n][m].fColor = UIBGCOLOR;
                    }
                    break;
                }
                for (int m = 0; m < 10; m++) {
                    fld.c[n][m].a = fld.c[n+1][m].a;
                    fld.c[n][m].fColor = fld.c[n+1][m].fColor;
                }
            }
            j--;     // Do not let loop to go to next line because
                // next line go down by itself =)
        }
        k = 0;    // Clear line fill indicator
    }
    return s; // Return number of deleted lines
}

/*
 * Inserts shape into field, runs filled lines searching, puts new shape
 * into the game at the top.
 *
 */
void putShape()
{
    if (outOfFieldCheck(&fld, &active)) {
        gameover(&game);
        return;
    }
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            if (active.c[j][i]) {
                fld.c[j+active.y][i+active.x].a = active.c[j][i];
                if ((j+active.y >= 0) && (i+active.x >= 0))
                    fld.c[j+active.y][i+active.x].fColor = active.fColor;
            }
    int removedLines = rmLines();
    game.lines += removedLines;
    switch (removedLines) {
    case 1:
        game.scoreCurrent += RM_1LINES_SCORE * game.level;
        break;
    case 2:
        game.scoreCurrent += RM_2LINES_SCORE * game.level;
        break;
    case 3:
        game.scoreCurrent += RM_3LINES_SCORE * game.level;
        break;
    case 4:
        game.scoreCurrent += RM_4LINES_SCORE * game.level;
        break;
    }
    resetActiveShape(&active);
    checkLevelUp(&game);
}

int outOfFieldCheck(Field *fld, Shape *active)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (active->c[i][j] && active->y+i >= fld->size.y) {
                gameover(&game);
                return 1;
            }
        }
    }
    return 0;
}

void checkLevelUp(Game *game)
{
    while (game->lines >= LEVELUP_LINES) {
        game->level++;
        game->lines -= LEVELUP_LINES;
        game->moveLatency = getMoveLatencyOfLevel(game->level);
    }
}

int getMoveLatencyOfLevel(int level)
{
    if (level >= 29)
        return L29LATENCY;
    else if (level >= 19)
        return L19LATENCY;
    else if (level >= 16)
        return L16LATENCY;
    else if (level >= 13)
        return L13LATENCY;
    else if (level >= 10)
        return L10LATENCY;
    else if (level == 9)
        return L09LATENCY;
    else if (level == 8)
        return L08LATENCY;
    else if (level == 7)
        return L07LATENCY;
    else if (level == 6)
        return L06LATENCY;
    else if (level == 5)
        return L05LATENCY;
    else if (level == 4)
        return L04LATENCY;
    else if (level == 3)
        return L03LATENCY;
    else if (level == 2)
        return L02LATENCY;
    else if (level == 1)
        return L01LATENCY;
    else if (level == 0)
        return L00LATENCY;
    return L00LATENCY;
}

void resetActiveShape(Shape *active)
{
    genNextShape();
    copyShape(active);
    active->x = 3;
    if (active->t == 6)
        active->y = 19;
    else
        active->y = 18;
    for (;;) {
        if (cellCollisionCheckHere(&fld, active))
            active->y++;
        else
            break;
    }
}

/*
 * Game tick
 *
 */
void tTick()
{     // If tick exceeds current level tick latency
    if (sfClock_getElapsedTime(gameTick).microseconds
            >= game.moveLatency) {
        sfClock_restart(gameTick);
        /* if bottom not reached */
        if ((wallCollisionCheck(DOWN) == 0)
        && (cellCollisionCheck(DOWN) == 0)) {
            active.y--; // Move
            sfClock_restart(putTick);
        } else {
            if (sfClock_getElapsedTime(putTick).microseconds >= PUT_LATENCY) {
                putShape(); // Just put the shape
                sfClock_restart(putTick);
            }
        }
    }
}


/*
 * Rotate matrix left routine
 *
 */
void rotateLeft()
{
    char arr[4][4];
    memcpy(&arr[0][0], &active.c[0][0], sizeof(char)*4*4);
    if (active.t == 5)
        return;
    if (active.t == 6) {
        for (int j = 3; j >= 0; j--)
            for (int i = 0; i < 4; i++)
                active.c[j][i] = arr[3-i][3-j];
        return;
    }
    for (int j = 3; j > 0; j--)
        for (int i = 0; i < 3; i++)
            active.c[j][i] = arr[3-i][j-1];
}


/*
 * Rotate matrix right routine
 *
 */
void rotateRight()
{
    char arr[4][4];
    memcpy(&arr[0][0], &active.c[0][0], sizeof(char)*4*4);
    if (active.t == 5)
        return;
    if (active.t == 6) {
        for (int j = 3; j >= 0; j--)
            for (int i = 0; i < 4; i++)
                active.c[j][i] = arr[3-i][3-j];
        return;
    }
    for (int j = 3; j > 0; j--)
        for (int i = 0; i < 3; i++)
            active.c[j][i] = arr[i+1][3-j];
}


/*
 * Rotates active shape with callling collision checkers
 *
 */
void rotateShape()
{
    rotateRight(); // Try rotate
    if ((wallRotCollisionCheck())
    || (cellRotCollisionCheck()))
        rotateLeft();
}

int cellRotCollisionCheck()
{
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            if (active.c[j][i]
            && fld.c[j+active.y][i+active.x].a)
                return 1; // Collision happens
    return 0; // Else it do not
}


int wallRotCollisionCheck()
{
    if (active.y < 0) //If shape has crossed Bottom border
        for (int i = 0; i < 4; i++)
            if (active.c[-1-active.y][i])
                return 1;    // Collision happens
    if (active.x < 0) //If shape has crossed Left border
        for (int i = 0; i < 4; i++)
            if (active.c[i][-1-active.x])
                return 1;    // Collision happens
    if (active.x > 6) //If shape has crossed Right border
        for (int i = 0; i < 4; i++)
            if (active.c[i][3-(active.x-7)])
                return 1;    // Collision happens
    return 0; // If no conditions are met collision is absent
}

int cellCollisionCheckHere(Field *fld, Shape *active)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (active->y + i < fld->size.y)
                if (active->c[i][j] && fld->c[i+active->y][j+active->x].a)
                    return 1;
    return 0;
}

int cellCollisionCheck(int dir)
{
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            if ((dir & RIGHT) // Check Right
            && (j+active.y >= 0) // Avoiding nonexisting fld cells
            && (i+active.x+1 >= 0) // ---
            && active.c[j][i] // Check activity
            && fld.c[j+active.y][i+active.x+1].a)
                return 1; // Collision happens
            if ((dir & LEFT) // Check Left
            && (j+active.y >= 0) // Avoiding nonexisting fld cells
            && (i+active.x-1 >= 0) // ---
            && active.c[j][i] // Check activity
            && fld.c[j+active.y][i+active.x-1].a)
                return 1; // Collision happens
            if ((dir & DOWN) // Check Bottom
            && (j+active.y-1 >= 0) // Avoiding nonexisting fld cells
            && (i+active.x >= 0) // ---
            && active.c[j][i] // Check activity
            && fld.c[j+active.y-1][i+active.x].a)
                return 1; // Collision happens
        }
    }
    return 0; // Else it do not
}

int wallCollisionCheck(int dir)
{
    if (dir & RIGHT) { // Right collision request
        if (active.x >= 6) // If shape has reached Right boreder
            for (int i = 0 ; i < 4; i++)
                if (active.c[i][3-(active.x-6)])
                    return 1; // Collision happens
    } else if (dir & DOWN) { // Bottom collision request
        if (active.y <= 0) //If shape has reached Bottom border
            for (int i = 0; i < 4; i++)
                if (active.c[-active.y][i])
                    return 1; // Collision happens
    } else if (dir & LEFT) // Left collision request
        if (active.x <= 0) // If shape has reached Left border
            for (int i = 0; i < 4; i++)
                if (active.c[i][-active.x])
                    return 1; // Collision happens
    return 0;
}


/*
 * Keys hold handler
 *
 */
void tKeyCtrl()
{
    /* Up arrow key 'hold' handler */
    if (sfKeyboard_isKeyPressed(sfKeyUp)) {
        if (!(arrKeys & UP)) {
            arrKeys = arrKeys | UP;
            rotateShape();
        }
    } else {
        if ((arrKeys & UP)) {
            arrKeys = arrKeys & ~UP;
        }
    }

    /* Down Arrow Key 'hold' handler */
    if (sfKeyboard_isKeyPressed(sfKeyDown)) {
        if (!(arrKeys & DOWN)) {
            arrKeys = arrKeys | DOWN;
            if (!wallCollisionCheck(DOWN)
            && !cellCollisionCheck(DOWN)) {
                active.y--;
                // Avoid excess move down by gameTick
                sfClock_restart(putTick);
                sfClock_restart(gameTick);
                game.scoreCurrent++;
            }
            repPushDown = sfClock_create();
        } else {
            if (sfClock_getElapsedTime(repPushDown).microseconds
            >= moveRepeatLatency2)
                arrKeys = arrKeys & ~DOWN;
        }
    } else {
        if ((arrKeys & DOWN)) {
            arrKeys = arrKeys & ~DOWN;
        }
    }

    /* Left Arrow Key 'hold' handler */
    if (sfKeyboard_isKeyPressed(sfKeyLeft)
    && !sfKeyboard_isKeyPressed(sfKeyRight)) {
        if (!(arrKeys & LEFT)) {
            arrKeys = arrKeys | LEFT;
            if (!wallCollisionCheck(LEFT)
            && !cellCollisionCheck(LEFT)) {
                active.x--;
                sfClock_restart(putTick);
            }
            repKeyLeft = sfClock_create();
        } else {
            if (!(arrKeys & LEFTHOLD)) {
                if (sfClock_getElapsedTime(repKeyLeft)
                                .microseconds
                >=  moveRepeatLatency1) {
                    arrKeys = arrKeys | LEFTHOLD;
                    arrKeys = arrKeys & ~LEFT;
                }
            } else {
                if (sfClock_getElapsedTime(repKeyLeft)
                                .microseconds
                >=  moveRepeatLatency2)
                    arrKeys = arrKeys & ~LEFT;
            }
        }
    } else if (!sfKeyboard_isKeyPressed(sfKeyLeft)) {
        if ((arrKeys & LEFT)) {
            arrKeys = arrKeys & ~LEFT;
            arrKeys = arrKeys & ~LEFTHOLD;
        }
    }

    /* Right Arrow Key 'hold' handler */
    if (sfKeyboard_isKeyPressed(sfKeyRight)
    && !sfKeyboard_isKeyPressed(sfKeyLeft)) {
        if (!(arrKeys & RIGHT)) {
            arrKeys = arrKeys | RIGHT;
            if (!wallCollisionCheck(RIGHT)
            && !cellCollisionCheck(RIGHT)) {
                active.x++;
                sfClock_restart(putTick);
            }
            repKeyRight = sfClock_create();
        } else {
            if (!(arrKeys & RIGHTHOLD)) {
                if (sfClock_getElapsedTime(repKeyRight).microseconds
                >=  moveRepeatLatency1) {
                    arrKeys = arrKeys | RIGHTHOLD;
                    arrKeys = arrKeys & ~RIGHT;
                }
            } else if (!sfKeyboard_isKeyPressed(sfKeyLeft)) {
                if (sfClock_getElapsedTime(repKeyRight).microseconds
                >=  moveRepeatLatency2) // Wait short time
                    arrKeys = arrKeys & ~RIGHT;
            }
        }
    } else {
        if ((arrKeys & RIGHT)) {
            arrKeys = arrKeys & ~RIGHT;
            arrKeys = arrKeys & ~RIGHTHOLD;
        }
    }
}


/*
 * Colorize active cells of field
 *
 */
void colorizeFld()
{
    for (int j = 0; j < fld.size.y; j++)
        for (int i = 0; i < fld.size.x; i++)
            if (fld.c[j][i].a) {
                sfRectangleShape_setFillColor(fld.p[j][i], fld.c[j][i].fColor);
                sfRectangleShape_setOutlineColor(fld.p[j][i], UIFGACTIVECOLOR);
            } else {
                sfRectangleShape_setFillColor(fld.p[j][i], UIBGCOLOR);
                sfRectangleShape_setOutlineColor(fld.p[j][i], UIFGINACTIVECOLOR);
            }
}

/*
 * Colorize active cells of active shape (overlay only
 * active cells above background of fld)
 *
 */
void colorizeActive()
{
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            if (active.c[j][i] && j+active.y < 22) {
                sfRectangleShape_setFillColor(
                        fld.p[j+active.y][i+active.x],
                        active.fColor);
                sfRectangleShape_setOutlineColor(
                        fld.p[j+active.y][i+active.x],
                        UIFGACTIVECOLOR);
            }
}


/*
 * Draw all fld cells
 *
 */
void drawFld(sfRenderWindow *window)
{
    for (int j = 0; j < fld.size.y; j++)
        for (int i = 0; i < fld.size.x; i++)
            sfRenderWindow_drawRectangleShape(window, fld.p[j][i], NULL);
}


void colorizeRandom(Field *fld)
{
    int a;
    for (int j = 0; j < fld->size.y; j++) {
        for (int i = 0; i < fld->size.x; i++) {
            a = rand()%7+1;
            switch (a) {
                case 1 :
                    sfRectangleShape_setFillColor(fld->p[j][i], LCOLOR);
                    break;
                case 2 :
                    sfRectangleShape_setFillColor(fld->p[j][i], RLCOLOR);
                    break;
                case 3 :
                    sfRectangleShape_setFillColor(fld->p[j][i], ZCOLOR);
                    break;
                case 4 :
                    sfRectangleShape_setFillColor(fld->p[j][i], SCOLOR);
                    break;
                case 5 :
                    sfRectangleShape_setFillColor(fld->p[j][i], BCOLOR);
                    break;
                case 6 :
                    sfRectangleShape_setFillColor(fld->p[j][i], ICOLOR);
                    break;
                case 7 :
                    sfRectangleShape_setFillColor(fld->p[j][i], TCOLOR);
                    break;
            }
            sfRectangleShape_setOutlineColor(fld->p[j][i], UIFGACTIVECOLOR);
        }
    }
}


void gameover(Game *game)
{
    game->isStarted = 0;
    game->scoreCurrent = 0;
    game->level = 0;
    game->moveLatency = L00LATENCY;
    game->lines = 0;
}


void genNextShape()
{
    active.t = next.t;
    next.t = (rand()%7)+1; // Insert new random shape of 7 variants
    copyShape(&next);
    if (next.t == 5)
        for (int j = 0; j < 3; j++)
            for (int i = 0; i < 4; i++)
                next.c[i][j] = next.c[i][j+1];
}


void copyShape(Shape *localSh)
{
    switch (localSh->t) { // Copy cell active/inactive state
        case 1 :
            memcpy(&localSh->c[0][0], &arrShapeL[0][0], sizeof(char)*4*4);
            localSh->fColor = LCOLOR;
            break;
        case 2 :
            memcpy(&localSh->c[0][0], &arrShapeRL[0][0], sizeof(char)*4*4);
            localSh->fColor = RLCOLOR;
            break;
        case 3 :
            memcpy(&localSh->c[0][0], &arrShapeZ[0][0], sizeof(char)*4*4);
            localSh->fColor = ZCOLOR;
            break;
        case 4 :
            memcpy(&localSh->c[0][0], &arrShapeS[0][0], sizeof(char)*4*4);
            localSh->fColor = SCOLOR;
            break;
        case 5 :
            memcpy(&localSh->c[0][0], &arrShapeB[0][0], sizeof(char)*4*4);
            localSh->fColor = BCOLOR;
            break;
        case 6 :
            memcpy(&localSh->c[0][0], &arrShapeI[0][0], sizeof(char)*4*4);
            localSh->fColor = ICOLOR;
            break;
        case 7 :
            memcpy(&localSh->c[0][0], &arrShapeT[0][0], sizeof(char)*4*4);
            localSh->fColor = TCOLOR;
            break;
    }
}


void drawNextShape(sfRenderWindow *window)
{
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            if (next.c[j][i]) {
                sfRectangleShape_setFillColor(next.p[j][i], next.fColor);
                sfRectangleShape_setOutlineColor(next.p[j][i], UIFGACTIVECOLOR);
                sfRenderWindow_drawRectangleShape(window, next.p[j][i], NULL);
            }
}

void freeFld() {
    for (int j = 0; j < fld.size.y; j++)
        for (int i = 0; i < fld.size.x; i++)
            sfRectangleShape_destroy(fld.p[j][i]);
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            sfRectangleShape_destroy(next.p[j][i]);
}
