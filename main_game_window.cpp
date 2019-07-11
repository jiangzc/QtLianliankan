#include <QSound>
#include <QAction>
#include <QMessageBox>
#include <QPainter>
#include <QLine>
#include "main_game_window.h"
#include "ui_main_game_window.h"

// --------- 全局变量 --------- //
const int kIconSize = 36;
const int kTopMargin = 90;      // 顶部留空
const int kLeftMargin = 130;    // 左留空

const QString kIconReleasedStyle = "";
const QString kIconClickedStyle = "background-color: rgba(255, 0, 0, 255)";  // 点击风格
const QString kIconHintStyle = "background-color: rgba(255, 0, 0, 255)";    // 提示风格

const int kGameTimeTotal = 5 * 60 * 1000; // 总时间
const int kGameTimerInterval = 300;
const int kLinkTimerDelay = 700;

// --------------------------- //


// 游戏主界面
MainGameWindow::MainGameWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainGameWindow),
    preIcon(NULL),
    curIcon(NULL)
{
    ui->setupUi(this);
    // 重载eventfilter安装到当前window
    ui->centralWidget->installEventFilter(this);

    // 关联信号槽
    connect(ui->actionBasic, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));
    connect(ui->actionMedium, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));
    connect(ui->actionHard, SIGNAL(triggered(bool)), this, SLOT(createGameWithLevel()));
    connect(ui->pause_btn, SIGNAL(clicked(bool)), this, SLOT(pauseGame()));
    connect(ui->restart_btn, SIGNAL(clicked(bool)), this, SLOT(restartGame()));
    connect(ui->reset_btn, SIGNAL(clicked(bool)), this, SLOT(reset()));
    connect(ui->hintBtn, SIGNAL(clicked(bool)), this, SLOT(hintBtnGame()));
    connect(ui->quit_btn, SIGNAL(clicked(bool)), this, SLOT(close()));
    // 初始化游戏
    initGame(BASIC);
}

MainGameWindow::~MainGameWindow()
{
    if (game)
        delete game;

    delete ui;
}

// 游戏初始化
void MainGameWindow::initGame(GameLevel level)
{
    // 启动游戏
    game = new GameModel;
    game->startGame(level);

    // 添加button
    for(int i = 0; i < MAX_ROW * MAX_COL; i++) {
        // 新建button对象
        imageButton[i] = new IconButton(this);
        imageButton[i]->setGeometry(kLeftMargin + (i % MAX_COL) * kIconSize, kTopMargin + (i / MAX_COL) * kIconSize, kIconSize, kIconSize);
        // 设置索引
        imageButton[i]->xID = i % MAX_COL;
        imageButton[i]->yID = i / MAX_COL;

        imageButton[i]->hide();

        if (game->getGameMap()[i]) {
            // 有方块就设置图片
            QPixmap iconPix;
            QString fileString;
            fileString.sprintf(":/res/image/youtube-%d.png", game->getGameMap()[i]);
            iconPix.load(fileString);
            QIcon icon(iconPix);
            imageButton[i]->setIcon(icon);
            imageButton[i]->setIconSize(QSize(kIconSize - 8, kIconSize - 8));

            // 添加按下的信号槽
            connect(imageButton[i], SIGNAL(pressed()), this, SLOT(onIconButtonPressed()));
            // 显示
            imageButton[i]->show();
        }
    }

    // 进度条
    ui->timeBar->setMaximum(kGameTimeTotal);
    ui->timeBar->setMinimum(0);
    ui->timeBar->setValue(kGameTimeTotal);

    // 游戏计时器
    gameTimer = new QTimer(this);
    connect(gameTimer, SIGNAL(timeout()), this, SLOT(gameTimerEvent()));
    gameTimer->start(kGameTimerInterval);

    // 连接状态值
    isLinking = false;

    // 播放背景音乐,QMediaPlayer只能播放绝对路径文件
    audioPlayer = new QMediaPlayer(this);
    audioPlayer->setMedia(QUrl("qrc:res/sound/background.mp3"));
    audioPlayer->play();
}

// 点击方块事件
void MainGameWindow::onIconButtonPressed()
{
    // 如果当前有方块在连接，不能点击方块
    if (isLinking)
        return ;

    // 记录当前点击的icon
    curIcon = dynamic_cast<IconButton *>(sender());

    // 若当前是选中的第一个方块
    if(!preIcon) {
        // 播放选择音效
        QSound::play(":/res/sound/select.wav");

        // 如果单击一个icon
        curIcon->setStyleSheet(kIconClickedStyle);
        preIcon = curIcon;

    } else {
        if(curIcon != preIcon) {
            // 如果不是同一个button就都标记,并尝试连接
            curIcon->setStyleSheet(kIconClickedStyle);
            if(game->linkTwoTiles(preIcon->xID, preIcon->yID, curIcon->xID, curIcon->yID)) {
                // 锁住当前状态
                isLinking = true;
                // 播放音效
                QSound::play(":/res/sound/pair.wav");
                // 重绘
                update();
                // 延迟后实现连接效果
                QTimer::singleShot(kLinkTimerDelay, this, SLOT(handleLinkEffect()));                

                // 每次检查一下是否僵局，若成僵局，则自动进行一次重排
                if (game->isFrozen())
                    reset();

                // 检查是否胜利
                if (game->isWin()) {
                    QMessageBox::information(this, "great", "you win");
                    audioPlayer->stop();
                    gameTimer->stop();
                    ui->timeBar->setDisabled(true);
                }

            } else {
                // 播放释放音效
                QSound::play(":/res/sound/release.wav");

                // 消除失败，恢复
                preIcon->setStyleSheet(kIconReleasedStyle);
                curIcon->setStyleSheet(kIconReleasedStyle);

                // 指针置空，用于下次点击判断
                preIcon = NULL;
                curIcon = NULL;
            }
        } else {
            QSound::play(":/res/sound/release.wav");

            preIcon->setStyleSheet(kIconReleasedStyle);
            curIcon->setStyleSheet(kIconReleasedStyle);
            preIcon = NULL;
            curIcon = NULL;
        }
    }
}

void MainGameWindow::handleLinkEffect()
{
    // 消除成功，隐藏掉，并析构
    game->paintPoints.clear();
    preIcon->hide();
    curIcon->hide();

    preIcon = NULL;
    curIcon = NULL;

    // 重绘
    update();

    // 恢复状态
    isLinking = false;
}

bool MainGameWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 重绘时会调用，可以手动调用
    if (event->type() == QEvent::Paint)
    {
        QPainter painter(ui->centralWidget);
        QPen pen;
        //随机颜色画笔
        QColor color(rand() % 256, rand() % 256, rand() % 256);
        pen.setColor(color);
        pen.setWidth(3);
        painter.setPen(pen);

        QString str;
        for (unsigned int i = 0; i < game->paintPoints.size(); i++)
        {
            PaintPoint p = game->paintPoints[i];
            str += "x:" + QString::number(p.x) + "y:" + QString::number(p.y) + "->";
        }

        // 连接各点画线（注，qt中用vector的size好像有点问题，需要类型转换，否则溢出）
        for (int i = 0; i < int(game->paintPoints.size()) - 1; i++)
        {
            PaintPoint p1 = game->paintPoints[i];
            PaintPoint p2 = game->paintPoints[i + 1];

            // 拿到各button的坐标,注意边缘点坐标
            QPoint btn_pos1;
            QPoint btn_pos2;

            // p1
            if (p1.x == -1)
            {
                btn_pos1 = imageButton[p1.y * MAX_COL + 0]->pos();
                btn_pos1 = QPoint(btn_pos1.x() - kIconSize, btn_pos1.y());
            }
            else if (p1.x == MAX_COL)
            {
                btn_pos1 = imageButton[p1.y * MAX_COL + MAX_COL - 1]->pos();
                btn_pos1 = QPoint(btn_pos1.x() + kIconSize, btn_pos1.y());
            }
            else if (p1.y == -1)
            {
                btn_pos1 = imageButton[0 + p1.x]->pos();
                btn_pos1 = QPoint(btn_pos1.x(), btn_pos1.y() - kIconSize);
            }
            else if (p1.y == MAX_ROW)
            {
                btn_pos1 = imageButton[(MAX_ROW - 1) * MAX_COL + p1.x]->pos();
                btn_pos1 = QPoint(btn_pos1.x(), btn_pos1.y() + kIconSize);
            }
            else
                btn_pos1 = imageButton[p1.y * MAX_COL + p1.x]->pos();

            // p2
            if (p2.x == -1)
            {
                btn_pos2 = imageButton[p2.y * MAX_COL + 0]->pos();
                btn_pos2 = QPoint(btn_pos2.x() - kIconSize, btn_pos2.y());
            }
            else if (p2.x == MAX_COL)
            {
                btn_pos2 = imageButton[p2.y * MAX_COL + MAX_COL - 1]->pos();
                btn_pos2 = QPoint(btn_pos2.x() + kIconSize, btn_pos2.y());
            }
            else if (p2.y == -1)
            {
                btn_pos2 = imageButton[0 + p2.x]->pos();
                btn_pos2 = QPoint(btn_pos2.x(), btn_pos2.y() - kIconSize);
            }
            else if (p2.y == MAX_ROW)
            {
                btn_pos2 = imageButton[(MAX_ROW - 1) * MAX_COL + p2.x]->pos();
                btn_pos2 = QPoint(btn_pos2.x(), btn_pos2.y() + kIconSize);
            }
            else
                btn_pos2 = imageButton[p2.y * MAX_COL + p2.x]->pos();

            // 中心点
            QPoint pos1(btn_pos1.x() + kIconSize / 2 , btn_pos1.y() - 8 );
            QPoint pos2(btn_pos2.x() + kIconSize / 2 , btn_pos2.y() - 8 );

            painter.drawLine(pos1, pos2);
        }

        return true;
    }
    else
        return QMainWindow::eventFilter(watched, event);
}

// 进度条计时效果
void MainGameWindow::gameTimerEvent()
{
    if(ui->timeBar->value() == 0) {
        gameTimer->stop();
        QMessageBox::information(this, "game oGameModel.ver", "play again>_<");
        restartGame();
    } else
        ui->timeBar->setValue(ui->timeBar->value() - kGameTimerInterval);
}

// 游戏提示
void MainGameWindow::hintBtnGame()
{
    // 初始时不能获得提示
    for (int i = 0; i < 4;i++)
        if (game->getHint()[i] == -1)
            return ;

    //两个button显示提示
    int srcX = game->getHint()[0];
    int srcY = game->getHint()[1];
    int dstX = game->getHint()[2];
    int dstY = game->getHint()[3];

    IconButton *srcIcon = imageButton[srcY * MAX_COL + srcX];
    IconButton *dstIcon = imageButton[dstY * MAX_COL + dstX];
    srcIcon->setStyleSheet(kIconHintStyle);
    dstIcon->setStyleSheet(kIconHintStyle);
}

void MainGameWindow::createGameWithLevel()
{
    // 先析构之前的
    if (game) {
        delete game;
        for (int i = 0;i < MAX_ROW * MAX_COL; i++) {
            if (imageButton[i])
               delete imageButton[i];
        }
    }

    // 停止音乐
    audioPlayer->stop();

    // 重绘
    update();

    QAction *actionSender = (QAction *)dynamic_cast<QAction *>(sender());

    if (actionSender == ui->actionBasic)
        initGame(BASIC);
    else if (actionSender == ui->actionMedium)
        initGame(MEDIUM);
    else if (actionSender == ui->actionHard)
        initGame(HARD);
}

// 游戏暂停
void MainGameWindow::pauseGame()
{
    if (gameTimer->isActive()) {
        // 改变按钮文字
        ui->pause_btn->setText("继续游戏");
        // 停止背景音乐，进度条，计时器
        audioPlayer->pause();
        ui->timeBar->setDisabled(true);
        gameTimer->stop();     
        // 其他按钮失效
        ui->quit_btn->setDisabled(true);
        ui->restart_btn->setDisabled(true);
        ui->hintBtn->setDisabled(true);
        ui->reset_btn->setDisabled(true);

        // 方块不可点击
        for (int i = 0; i < MAX_ROW * MAX_COL; i++)
            imageButton[i]->setEnabled(false);

    } else {
        ui->pause_btn->setText("暂停游戏");
        audioPlayer->play();
        ui->timeBar->setDisabled(false);
        gameTimer->start();
        ui->quit_btn->setDisabled(false);
        ui->restart_btn->setDisabled(false);
        ui->hintBtn->setDisabled(false);
        ui->reset_btn->setDisabled(false);

        for (int i = 0; i < MAX_ROW * MAX_COL; i++)
            imageButton[i]->setEnabled(true);
    }
}

// 重新开始
void MainGameWindow::restartGame()
{
    audioPlayer->stop();
    MainGameWindow *w = new MainGameWindow();
    w->show();
    this->close();
}

// 重排
void MainGameWindow::reset()
{
    game->resetMap();

    for (int i = 0; i < MAX_ROW *MAX_COL; i++) {
        imageButton[i]->hide();
        imageButton[i]->setStyleSheet(kIconReleasedStyle);
        // 断开槽连接
        disconnect(imageButton[i], SIGNAL(pressed()), this, SLOT(onIconButtonPressed()));
    }

    for(int i = 0; i < MAX_ROW * MAX_COL; i++) {
        // 重置索引
        imageButton[i]->xID = i % MAX_COL;
        imageButton[i]->yID = i / MAX_COL;

        if (game->getGameMap()[i]) {
            // 重置图片
            QPixmap iconPix;
            QString fileString;
            fileString.sprintf(":/res/image/youtube-%d.png", game->getGameMap()[i]-1);
            iconPix.load(fileString);
            QIcon icon(iconPix);
            imageButton[i]->setIcon(icon);
            imageButton[i]->setIconSize(QSize(kIconSize - 8, kIconSize - 8));
            imageButton[i]->show();

            // 连接槽
            connect(imageButton[i], SIGNAL(pressed()), this, SLOT(onIconButtonPressed()));
        }
    }
}
