#include "game.h"
#include <QKeyEvent>
#include <QGraphicsTextItem>
#include <QIcon>
#include <QFont>

Game::Game(QWidget* parent) : QGraphicsView(parent), score(0), gameState(0) {
    scene = new QGraphicsScene(this);
    setScene(scene);

    setWindowTitle("Ikun牌小鸟");

    QIcon icon(":/assets/images/bluebird-upflap.png");
    setWindowIcon(icon);

    bird = new Bird();
    // 先不添加到场景，等游戏开始再加
    bird->setPos(100, 300);

    // 定时器，控制游戏循环
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Game::gameLoop);

    setFixedSize(400, 600);
    scene->setSceneRect(0, 0, 400, 600);

    scene->setBackgroundBrush(QBrush(QImage(":/assets/images/background-day.png").scaled(400, 600)));

    // 取消滚动条
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 创建并显示分数文本项
    scoreText = new QGraphicsTextItem(QString("分数: %1").arg(score));
    //放在最前面
    scoreText->setZValue(1);
    scoreText->setDefaultTextColor(Qt::white);
    scoreText->setFont(QFont("Microsoft YaHei", 20));
    scoreText->setPos(10, 10);
    scene->addItem(scoreText);

    // 添加开始提示 - 修改为红色
    startText = new QGraphicsTextItem("按空格键开始游戏");
    startText->setDefaultTextColor(Qt::red);  // 改为红色
    startText->setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
    startText->setPos(width()/2 - startText->boundingRect().width()/2,
                      height()/2 - 30);
    scene->addItem(startText);

    // 初始状态为等待开始，定时器不启动
    timer->stop();
}

void Game::startGame() {
    gameState = 1;  // 切换到游戏中状态

    // 移除开始提示（只出现一次）
    if (startText) {
        scene->removeItem(startText);
        delete startText;
        startText = nullptr;  // 设置为空指针，确保只出现一次
    }

    // 将小鸟添加到场景
    if (!bird->scene()) {
        scene->addItem(bird);
    }

    // 启动游戏循环
    timer->start(20);
    bird->reset();
}

void Game::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space) {
        if (gameState == 0) {      // 等待开始状态
            startGame();
        }
        else if (gameState == 1) { // 游戏中状态
            bird->flap();
        }
        else if (gameState == 2) { // 游戏结束状态
            restartGame();
        }
    }
}

void Game::restartGame()
{
    // 清除场景中的管道和文本
    for (Pipe* pipe : pipes) {
        scene->removeItem(pipe);
        delete pipe;
    }
    pipes.clear();

    // 重置小鸟的位置和状态
    bird->setPos(100, 300);
    bird->reset();

    // 重置分数
    score = 0;
    scoreText->setPlainText(QString("分数: %1").arg(score));

    // 移除 Game Over 画面和重新开始提示
    QList<QGraphicsItem*> items = scene->items();
    QList<QGraphicsItem*> itemsToRemove;  // 先收集要删除的项

    for (QGraphicsItem* item : items) {
        if (QGraphicsPixmapItem* pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(item))
        {
            if (pixmapItem->pixmap().cacheKey() == QPixmap(":/assets/images/gameover.png").cacheKey())
            {
                itemsToRemove.append(pixmapItem);
            }
        }
        if (QGraphicsTextItem* textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
            if (textItem->toPlainText() == "按空格键重新开始") {
                itemsToRemove.append(textItem);
            }
        }
    }

    // 安全删除收集的项目
    for (QGraphicsItem* item : itemsToRemove) {
        scene->removeItem(item);
        delete item;
    }

    // 重置游戏状态为游戏中，直接开始游戏
    gameState = 1;

    // 注意：这里不再添加开始提示，确保只出现一次

    // 启动定时器，直接开始游戏
    timer->start(20);
}

void Game::gameLoop() {
    // 只在游戏中状态运行
    if (gameState != 1) {
        return;
    }

    bird->updatePosition();

    // 生成新的管道
    if (pipes.isEmpty() || pipes.last()->x() < 200) {
        Pipe* pipe = new Pipe();
        pipes.append(pipe);
        scene->addItem(pipe);
    }

    // 管道移动与检测碰撞
    auto it = pipes.begin();
    while (it != pipes.end()) {
        Pipe* pipe = *it;
        pipe->movePipe();

        // 检测与小鸟的碰撞
        if (bird->collidesWithItem(pipe)) {
            timer->stop();
            gameState = 2;  // 设置为游戏结束状态

            QGraphicsPixmapItem* gameOverItem = scene->addPixmap(QPixmap(":/assets/images/gameover.png"));
            // 将 Game Over 画面放在中间位置
            gameOverItem->setPos(this->width() / 2 - gameOverItem->pixmap().width() / 2,
                                 this->height() / 2 - gameOverItem->pixmap().height() / 2);

            // 提示按空格重新游戏 - 字体改小，颜色改黑
            QGraphicsTextItem* restartText = new QGraphicsTextItem("按空格键重新开始");
            restartText->setDefaultTextColor(Qt::black);  // 黑色
            restartText->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));  // 字体改小为10号
            // 放在中间
            restartText->setPos(this->width() / 2 - restartText->boundingRect().width() / 2,
                                this->height() / 2 + gameOverItem->pixmap().height() / 2 + 5);  // 位置微调

            scene->addItem(restartText);
            return;
        }

        // 如果小鸟通过了管道（即小鸟的x坐标刚好超过管道的x坐标）
        if (pipe->x() + pipe->boundingRect().width() < bird->x() && !pipe->isPassed) {
            // 增加分数
            score++;
            pipe->isPassed = true;  // 确保不会重复加分

            // 更新分数显示
            scoreText->setPlainText(QString("分数: %1").arg(score));
        }

        // 如果管道移出了屏幕，将其从场景和列表中删除
        if (pipe->x() < -60) {
            scene->removeItem(pipe);
            delete pipe;
            it = pipes.erase(it);  // 从列表中安全移除管道
        }
        else {
            ++it;  // 继续遍历
        }
    }
}
