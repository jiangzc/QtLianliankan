#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "game_model.h"


// 游戏逻辑模型，与界面分离
GameModel::GameModel() :
    gameStatus(PLAYING),
    gameLevel(BASIC)
{

}

GameModel::~GameModel()
{
    if(gameMap)
    {
        free(gameMap);
        free(hintArray);
        gameMap = NULL;
    }
}

void GameModel::startGame(GameLevel level)
{
    // 初始化数组
    gameMap = (int *)malloc(sizeof(int) * MAX_ROW * MAX_COL);
    for (int i = 0; i < MAX_ROW * MAX_COL; i++)
        gameMap[i] = 0;

    hintArray = (int *)malloc(sizeof(int) * 4);
    for (int i = 0; i < 4; i++)
        hintArray[i] = -1;

    gameStatus = PLAYING;
    gameLevel = level;

    int gameLevelNum;
    switch (gameLevel) {
        case BASIC:  gameLevelNum = kBasicNum;      break;
        case MEDIUM: gameLevelNum = kMediumNum;     break;
        case HARD:   gameLevelNum = kHardNum;
    }

    // 填充方块标号
    int iconID = 0;
    for(int i = 0; i < gameLevelNum; i += 2) {
        // 每次填充连着的两个，图片用尽了就循环
        gameMap[i] = iconID % MAX_ICON + 1;
        gameMap[i + 1] = iconID % MAX_ICON + 1;
        iconID++;
    }

    // 打乱方块
    srand((unsigned)time(0));
    for(int i = 0; i < MAX_ROW * MAX_COL; i++) {
        int randomID = rand() % (MAX_ROW * MAX_COL);
        std::swap(gameMap[i], gameMap[randomID]);
    }

    // 初始化判断模式
    isFrozenMode = false;

    // 初始化绘制点
    paintPoints.clear();

    picTotal = gameLevelNum;
}

// 游戏重载
void GameModel::startGame()
{
    startGame(gameLevel);
}

int *GameModel::getGameMap()
{
    return gameMap;
}

// 判断是否成僵局
bool GameModel::isFrozen()
{
    // 暴力法，所有方块两两判断是否可以连接
    // 每次消除后做一次判断
    // 在这个过程中记录提示的方块

    for (int i = 0; i < MAX_ROW * MAX_COL - 1; i++)
        for( int j = i + 1; j < MAX_ROW * MAX_COL; j++) {
            // 得出点的横纵坐标
            int srcX = i % MAX_COL;
            int srcY = i / MAX_COL;
            int dstX = j % MAX_COL;
            int dstY = j / MAX_COL;

            // 若找到可连的两个方块就不为僵局
            isFrozenMode = true;
            if (isCanLink(srcX, srcY, dstX, dstY)) {
                // 记录第一个可以连接的hint，用于提示
                hintArray[0] = srcX;
                hintArray[1] = srcY;
                hintArray[2] = dstX;
                hintArray[3] = dstY;

                isFrozenMode = false;

                return false;
            }
        }
    isFrozenMode = false;

    return true;
}

bool GameModel::isWin()
{
    if (picTotal > 0)
        return false;

    gameStatus = WIN;
    return true;
}

int* GameModel::getHint()
{
    return hintArray;
}

// 最重要的判断连接算法
bool GameModel::canLinkDirectly(int srcX, int srcY, int dstX, int dstY)
{
    // 竖线
    if (srcX == dstX) {
        if (srcY > dstY)
            std::swap(srcY, dstY);

        for (int y = srcY + 1; y < dstY; y++)
            if (gameMap[MAX_COL * y + srcX])
                return false;

        if (!isFrozenMode) {
            // 记录点和路线
            PaintPoint p1(srcX, srcY), p2(dstX, dstY);
            paintPoints.clear();
            paintPoints.push_back(p1);
            paintPoints.push_back(p2);
        }

        return true;
    }

    // 横线
    if (srcY == dstY) {
        if (srcX > dstX)
            std::swap(srcX, dstX);

        for (int x = srcX + 1; x < dstX; x++)
            if (gameMap[MAX_COL * srcY + x])
                return false;

        if (!isFrozenMode) {
            PaintPoint p1(srcX, srcY), p2(dstX, dstY);
            paintPoints.clear();
            paintPoints.push_back(p1);
            paintPoints.push_back(p2);
        }

        return true;
    }

    return false;
}

bool GameModel::canLinkWithOneCorner(int srcX, int srcY, int dstX, int dstY)
{
    // 统一化，方便后续处理
    if (srcX > dstX) {
        std::swap(srcX, dstX);
        std::swap(srcY, dstY);
    }

    // 先确定拐点，再确定直连线路,2种情况，4个点，每种情况逐个试
    if (dstY > srcY) {
        // 右上角
        if (!gameMap[srcY * MAX_COL + dstX] &&
                canLinkDirectly(srcX, srcY, dstX, srcY) &&
                canLinkDirectly(dstX, srcY, dstX, dstY)) {
                // 只有连接模式才记录点
                if (!isFrozenMode) {
                    PaintPoint p1(srcX, srcY), p2(dstX, srcY), p3(dstX, dstY);
                    paintPoints.clear();
                    paintPoints.push_back(p1);
                    paintPoints.push_back(p2);
                    paintPoints.push_back(p3);
                }
                return true;
        }
        // 左下角
        if (!gameMap[dstY * MAX_COL + srcX] &&
                canLinkDirectly(srcX, srcY, srcX, dstY) &&
                canLinkDirectly(srcX, dstY, dstX, dstY)) {
                if (!isFrozenMode) {
                    PaintPoint p1(srcX, srcY), p2(srcX, dstY), p3(dstX, dstY);
                    paintPoints.clear();
                    paintPoints.push_back(p1);
                    paintPoints.push_back(p2);
                    paintPoints.push_back(p3);
                }
                return true;

        }
    } else {
        // 左上角
        if (!gameMap[dstY * MAX_COL + srcX] &&
                canLinkDirectly(srcX, srcY, srcX, dstY) &&
                canLinkDirectly(srcX, dstY, dstX, dstY)) {
                if (!isFrozenMode) {
                    PaintPoint p1(srcX, srcY), p2(srcX, dstY), p3(dstX, dstY);
                    paintPoints.clear();
                    paintPoints.push_back(p1);
                    paintPoints.push_back(p2);
                    paintPoints.push_back(p3);
                }
                return true;

        }
        // 右下角
        if (!gameMap[srcY * MAX_COL + dstX] &&
                canLinkDirectly(srcX, srcY, dstX, srcY) &&
                canLinkDirectly(dstX, srcY, dstX, dstY)) {
                if (!isFrozenMode) {
                    PaintPoint p1(srcX, srcY), p2(dstX, srcY), p3(dstX, dstY);
                    paintPoints.clear();
                    paintPoints.push_back(p1);
                    paintPoints.push_back(p2);
                    paintPoints.push_back(p3);
                }
                return true;
        }
    }

    return false;
}

bool GameModel::canLinkWithTwoCorner(int srcX, int srcY, int dstX, int dstY)
{
    // 统一化，方便后续处理
    if (srcX > dstX) {
        std::swap(srcX, dstX);
        std::swap(srcY, dstY);
    }

    // 两种情况，横向垂线和竖向垂线，以src点作为基准遍历，双折线由直线和一个拐点的折线构成
    // 常规情况，横向垂线
    for (int y = 0; y < MAX_ROW; y++) {
        if (y == srcY || y == dstY)
            continue;

        if (!gameMap[y * MAX_COL + srcX]
                && canLinkDirectly(srcX, srcY, srcX, y)
                && canLinkWithOneCorner(srcX, y, dstX, dstY)) {
            if (!isFrozenMode) {
                PaintPoint p1(srcX, srcY), p2(srcX, y), p3(dstX, y), p4(dstX, dstY);
                paintPoints.clear();
                paintPoints.push_back(p1);
                paintPoints.push_back(p2);
                paintPoints.push_back(p3);
                paintPoints.push_back(p4);
            }
            return true;
        }
    }

    // 常规情况，竖向垂线
    for (int x = 0; x < MAX_COL; x++) {
        if (x == srcX && x == dstX)
            continue;

        if (!gameMap[srcY * MAX_COL + x]
                && canLinkDirectly(srcX, srcY, x, srcY)
                && canLinkWithOneCorner(x, srcY, dstX, dstY)) {
            if (!isFrozenMode) {
                PaintPoint p1(srcX, srcY), p2(x, srcY), p3(x, dstY), p4(dstX, dstY);
                paintPoints.clear();
                paintPoints.push_back(p1);
                paintPoints.push_back(p2);
                paintPoints.push_back(p3);
                paintPoints.push_back(p4);
            }
            return true;
        }
    }

    // 边缘情况，从外边缘连接，方块不一定在边缘
    // 左
    if ((srcX == 0 || gameMap[srcY * MAX_COL + 0] == 0 && canLinkDirectly(srcX, srcY, 0, srcY))
            && (dstX == 0 || gameMap[dstY * MAX_COL + 0] == 0 && canLinkDirectly(0, dstY, dstX, dstY)))
    {
        if (!isFrozenMode) {
            PaintPoint p1(srcX, srcY), p2(-1, srcY), p3(-1, dstY), p4(dstX, dstY);
            paintPoints.clear();
            paintPoints.push_back(p1);
            paintPoints.push_back(p2);
            paintPoints.push_back(p3);
            paintPoints.push_back(p4);

        }
        return true;
    }

    // 右
    if ((srcX == MAX_COL - 1 || gameMap[srcY * MAX_COL + MAX_COL - 1] == 0 && canLinkDirectly(srcX, srcY, MAX_COL - 1, srcY))
            && (dstX == MAX_COL - 1 || gameMap[dstY * MAX_COL + MAX_COL - 1] == 0 && canLinkDirectly(MAX_COL - 1, dstY, dstX, dstY)))
    {
        if (!isFrozenMode) {
            PaintPoint p1(srcX, srcY), p2(MAX_COL, srcY), p3(MAX_COL, dstY), p4(dstX, dstY);
            paintPoints.clear();
            paintPoints.push_back(p1);
            paintPoints.push_back(p2);
            paintPoints.push_back(p3);
            paintPoints.push_back(p4);
        }
        return true;
    }

    // 上
    if ((srcY == 0 || gameMap[srcX] == 0 && canLinkDirectly(srcX, srcY, srcX, 0))
            && (dstY == 0 || gameMap[dstX] == 0 && canLinkDirectly(dstX, 0, dstX, dstY)))
    {
        if (!isFrozenMode) {
            PaintPoint p1(srcX, srcY), p2(srcX, -1), p3(dstX, -1), p4(dstX, dstY);
            paintPoints.clear();
            paintPoints.push_back(p1);
            paintPoints.push_back(p2);
            paintPoints.push_back(p3);
            paintPoints.push_back(p4);
        }
        return true;
    }

    // 下
    if ((srcY == MAX_ROW - 1 || gameMap[(MAX_ROW - 1) * MAX_COL + srcX] == 0 && canLinkDirectly(srcX, srcY, srcX, MAX_ROW - 1))
            && (dstY == MAX_ROW - 1 || gameMap[(MAX_ROW - 1) * MAX_COL + dstX] == 0 && canLinkDirectly(dstX, MAX_ROW - 1, dstX, dstY)))
    {
        if (!isFrozenMode) {
            PaintPoint p1(srcX, srcY), p2(srcX, MAX_ROW), p3(dstX, MAX_ROW), p4(dstX, dstY);
            paintPoints.clear();
            paintPoints.push_back(p1);
            paintPoints.push_back(p2);
            paintPoints.push_back(p3);
            paintPoints.push_back(p4);

        }
        return true;
    }

    return false;
}

bool GameModel::isCanLink(int srcX, int srcY, int dstX, int dstY)
{
    // 首先判断点击的两个方块不是同一个，且不为空，且方块相同
    // 判断方块是否可以连

    if (srcX == dstX && srcY == dstY || !gameMap[srcY * MAX_COL + srcX] || !gameMap[dstY * MAX_COL + dstX]
            || gameMap[srcY * MAX_COL + srcX] != gameMap[dstY * MAX_COL + dstX])
        return false;

    // 横向或者竖向直连可连通,或一次拐弯可以连通，或两次拐弯可以连通
    if (canLinkDirectly(srcX, srcY, dstX, dstY)
            || canLinkWithOneCorner(srcX, srcY, dstX, dstY)
            || canLinkWithTwoCorner(srcX, srcY, dstX, dstY))
        return true;

    return false;
}

// 点击方块进行连接操作，判断是否可连
bool GameModel::linkTwoTiles(int srcX, int srcY, int dstX, int dstY)
{
    if(isCanLink(srcX, srcY, dstX, dstY)) {
        // 值重置
        gameMap[MAX_COL * srcY + srcX] = 0;
        gameMap[MAX_COL * dstY + dstX] = 0;
        picTotal -= 2;
        return true;
    }

    return false;
}

void GameModel::resetMap()
{
    // 打乱方块
    srand((unsigned)time(0));
    for(int i = 0; i < MAX_ROW * MAX_COL; i++) {
        int randomID = rand() % (MAX_ROW * MAX_COL);
        std::swap(gameMap[i], gameMap[randomID]);
    }
}
