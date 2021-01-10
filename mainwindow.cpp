/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#include "mainwindow.h"
#include "ffmpegdriver.h"
#include "renderarea.h"
#include "videostream.h"
#include "worker.h"

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QImageReader>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSlider>
#include <QStatusBar>
#include <QTabBar>
#include <QToolBar>
#include <QVBoxLayout>

#include <QApplication>

#include <cassert>
#include <fstream>
#include <functional>
#include <regex>
#include <type_traits>
#include <dlib/revision.h>

extern const double ZoomInFactor;
extern const double ZoomOutFactor;
extern const int ScrollStep;

namespace {
template <typename T> QImage imgRotate(const QImage &img) {
    const T r(img.width(), img.height());
    QImage res(r.getSize(), img.format());
    for (decltype(img.height()) y{}; y < img.height(); ++y) {
        for (decltype(img.width()) x{}; x < img.width(); ++x) {
            res.setPixelColor(r.getPoint(x, y), img.pixelColor(x, y));
        }
    }
    return res;
}

class Rotate90
{
public:
    constexpr Rotate90(int w, int h) : w_(w), h_(h)
    { }
    constexpr QSize getSize() const {
        return QSize(h_, w_);
    }
    constexpr QPoint getPoint(const int x, const int y) const {
        return QPoint(h_ - 1 - y, x);
    }
private:
    int w_{}, h_{};
};

class Rotate180
{
public:
    constexpr Rotate180(int w, int h) : w_(w), h_(h)
    { }
    constexpr QSize getSize() const {
        return QSize(w_, h_);
    }
    constexpr QPoint getPoint(const int x, const int y) const {
        return QPoint(w_ - 1 - x, h_ - 1 - y);
    }
private:
    int w_{}, h_{};
};

class Rotate270
{
public:
    constexpr Rotate270(int w, int h) : w_(w), h_(h)
    { }
    constexpr QSize getSize() const {
        return QSize(h_, w_);
    }
    constexpr QPoint getPoint(const int x, const int y) const {
        return QPoint(y, w_ - 1 - x);
    }
private:
    int w_{}, h_{};
};

} // namespace unnamed

TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent), renderArea(new RenderArea(nullptr))
{
    QTabBar *tabBar = this->tabBar();
    tabBar->addTab("Rect");
    tabBar->addTab("LBFR");
    addTab(renderArea, "Image");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _gview(new RenderArea(&_scene))/*, renderArea(new RenderArea(&_scene))*/
{
    qRegisterMetaType<CRectArray>("CRectArray&");
    qRegisterMetaType<std::vector<QPointF>>("CPointFArray&");

    setCentralWidget(_gview);
    //setCentralWidget(renderArea);

    QAction *opnAction = new QAction(tr("&Open..."), this);
    opnAction->setShortcut(QKeySequence::Open);
    opnAction->setStatusTip(tr("Open an existing file"));
    connect(opnAction, &QAction::triggered, this, &MainWindow::opnFile);

    QAction *impAction = new QAction(tr("&Import Pts"), this);
    impAction->setShortcut((tr("Ctrl+I")));
    connect(impAction, &QAction::triggered, this, &MainWindow::impFile);

    QAction *expAction = new QAction(tr("&Export Pts"), this);
    expAction->setShortcut((tr("Ctrl+E")));
    connect(expAction, &QAction::triggered, this, &MainWindow::expFile);

    QAction *expRectAction = new QAction(tr("Export &Rect"), this);
    expRectAction->setShortcut((tr("Ctrl+R")));
    connect(expRectAction, &QAction::triggered, this, &MainWindow::expRect);

    QAction *exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcuts(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    const QIcon rectIcon = QIcon(":/images/rect.png");
    QAction *actRectangle = new QAction(rectIcon, tr("Rectangle"), this);
    //actRectangle->setCheckable(true);
    connect(actRectangle, &QAction::triggered, this, &MainWindow::sltAddRect);

    const QIcon faceIcon = QIcon(":/images/face.png");
    QAction *actFaceDetector = new QAction(faceIcon, tr("Face"), this);
    actFaceDetector->setCheckable(false);
    connect(actFaceDetector, &QAction::triggered, this, static_cast<void (MainWindow::*)(bool)>(&MainWindow::sltFaceDetector));
    const QIcon lbfrIcon = QIcon(":/images/lbfr.png");
    QAction *actLBFRDetector = new QAction(lbfrIcon, tr("LBFR"), this);
    actLBFRDetector->setCheckable(false);
    connect(actLBFRDetector, &QAction::triggered, this, static_cast<void (MainWindow::*)(bool)>(&MainWindow::sltLBFRDetector));

    QActionGroup *rotationGroup = new QActionGroup(this);
    QAction *rotation0Act = new QAction(tr("Rotate 0"), this);
    rotation0Act->setCheckable(true);
    connect(rotation0Act, &QAction::triggered, this, &MainWindow::sltRotation0);
    rotationGroup->addAction(rotation0Act);

    QAction *rotation90Act = new QAction(tr("Rotate 90"), this);
    rotation90Act->setCheckable(true);
    connect(rotation90Act, &QAction::triggered, this, &MainWindow::sltRotation90);
    rotationGroup->addAction(rotation90Act);

    QAction *rotation270Act = new QAction(tr("Rotate 270"), this);
    rotation270Act->setCheckable(true);
    connect(rotation270Act, &QAction::triggered, this, &MainWindow::sltRotation270);
    rotationGroup->addAction(rotation270Act);
    rotation0Act->setChecked(true);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(opnAction);
    fileMenu->addSeparator();
    fileMenu->addAction(impAction);
    fileMenu->addAction(expAction);
    fileMenu->addSeparator();
    fileMenu->addAction(expRectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    QMenu *optMenu = menuBar()->addMenu(tr("Options"));
    optMenu->addAction(rotation0Act);
    optMenu->addAction(rotation90Act);
    optMenu->addAction(rotation270Act);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutQtAct = helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    helpMenu->addAction(tr("About"), this, &MainWindow::sltAbout);

    QToolBar *toolsToolBar = addToolBar(tr("Tools"));
    toolsToolBar->addAction(actRectangle);

    QToolBar *optsToolBar = addToolBar(tr("Options"));
    optsToolBar->addAction(actFaceDetector);
    optsToolBar->addAction(actLBFRDetector);

    QToolBar *vidToolBar = addToolBar(tr("Video"));
    QSlider *slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(0, 1000);
    vidToolBar->addWidget(slider);
    QAction *actPrevFrame = new QAction(tr("Prev"), this);
    connect(actPrevFrame, &QAction::triggered, this, &MainWindow::prevFrame);
    vidToolBar->addAction(actPrevFrame);
    QAction *actNextFrame = new QAction(tr("Next"), this);
    connect(actNextFrame, &QAction::triggered, this, &MainWindow::nextFrame);
    vidToolBar->addAction(actNextFrame);

    QStatusBar *statusBar = QMainWindow::statusBar();
    posLabel = new QLabel();
    statusBar->addWidget(posLabel, 6);

    if (AVFormatDll::getInstance().isInited()) {
        AVFormatDll::getInstance().p_av_register_all();
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::loadFile(const QString &filename) {
    if (!filename.isNull()) {
        QFileInfo fi(filename);
        if (0 == fi.completeSuffix().compare("avi")) {
            _safeStream.reset(new VideoStream(filename.toStdString().c_str()));
            this->nextFrame();
        }
        else {
            QImageReader reader(filename);
            reader.setAutoTransform(true);
            QImage newImage = reader.read();
            if (newImage.isNull()) {
                QMessageBox::information(this, QGuiApplication::applicationDisplayName(), tr("Cannot load %1: %2").arg(QDir::toNativeSeparators(filename), reader.errorString()));
                return;
            }
            _image0 = std::move(newImage);
            this->Rotate();
            //item->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsMovable, true);
            //screenCenter = QPointF(image_.width() / 2.f, image_.height() / 2.f);
            //screenScale = std::max(static_cast<decltype(screenScale)>(image_.width()) / this->width(), static_cast<decltype(screenScale)>(image_.height()) / this->height());
        }
    }
//        renderArea->openFile(filename);
}

/*void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_F1:
        {
            QStringList props;
            props.append(QString("MarkerQt: %1-bit").arg(sizeof(void*) * 8));
            props.append("");
            props.append(QString("Qt: %1").arg(qVersion()));
            QMessageBox::about(this, "about", props.join("\n"));
        }
        break;
    case Qt::Key_Plus:
        renderArea->zoom(renderArea->rect().center(), ZoomInFactor);
        break;
    case Qt::Key_Minus:
        renderArea->zoom(renderArea->rect().center(), ZoomOutFactor);
        break;
    case Qt::Key_Left:
        renderArea->scroll(-ScrollStep, 0);
        break;
    case Qt::Key_Right:
        renderArea->scroll(ScrollStep, 0);
        break;
    case Qt::Key_Up:
        renderArea->scroll(0, -ScrollStep);
        break;
    case Qt::Key_Down:
        renderArea->scroll(0, ScrollStep);
        break;
    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}*/

void MainWindow::updateStatusBar(const QString &str)
{
    posLabel->setText(str);
}

void MainWindow::opnFile()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), ""/*QDir::currentPath()*/, tr("Images (*.jpg *.jpeg *.png);;Video (*.avi *.mp4)"));
    /*auto its(_scene.items());
    std::for_each(its.begin(), its.end(), std::bind(&QGraphicsScene::removeItem, &_scene, std::placeholders::_1));*/
    _scene.clear();
    _rects.clear();
    ptNum = 0;
    loadFile(filename);
//        renderArea->openFile(filename);
}

void MainWindow::impFile()
{
    QString filename(QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("PTS (*.pts)")));
    if (!filename.isNull()) {
        std::fstream file(filename.toStdString().c_str());
        if (file.is_open()) {
            std::string parse_templ("%lf %lf"), str;
            if (std::getline(file, str)) {
                qreal x{}, y{};
                if (std::regex_match(str, std::regex("x, +y"))) {
                    parse_templ = "%lf, %lf";
                }
                else if (std::regex_match(str, std::regex("x +y"))) {
                    (void)parse_templ;
                }
                else {
                    const int r(std::sscanf(str.c_str(), parse_templ.c_str(), &x, &y));
                    assert(2 == r);
                    AddPoint(QPointF(x, y));
                }
                while (std::getline(file, str)) {
                    const int r(std::sscanf(str.c_str(), parse_templ.c_str(), &x, &y));
                    assert(2 == r);
                    AddPoint(QPointF(x, y));
                }
            }
            file.close();
        }
    }
}

void MainWindow::expFile()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("PTS (*.pts)"));
    if (!filename.isNull()) {
        std::fstream file(filename.toStdString().c_str(), std::fstream::out);
        if (file.is_open()) {
            auto list = _scene.items();
            std::vector<PointItem*> v;
            for (auto it{std::cbegin(list)}; it != std::cend(list); ++it) {
                if (PointItem::Type == (*it)->type()) {
                    PointItem *p = qgraphicsitem_cast<PointItem*>(*it);
                    v.push_back(p);
                }
            }
            std::sort(std::begin(v), std::end(v), [](const auto &a, const auto &b){ return a->getNum() < b->getNum(); });
            file << "x, y" << std::endl;
            for (auto it = std::cbegin(v); it != std::cend(v); ++it) {
                file << (*it)->x() << ", " << (*it)->y() << std::endl;
            }
            file.close();
        }
    }
}

void MainWindow::expRect()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("Rect (*.txt)"));
    if (!filename.isNull()) {
        std::fstream file(filename.toStdString().c_str(), std::fstream::out);
        if (file.is_open()) {
            file << "x, y, w, h" << std::endl;
            auto list = _scene.items();
            for (auto it{std::cbegin(list)}; it != std::cend(list); ++it) {
                if (RectItem::Type == (*it)->type()) {
                    RectItem *p = qgraphicsitem_cast<RectItem*>(*it);
                    QRect r = p->getRect();
                    file << r.left() << ", " << r.top() << ", " << r.width() << ", " << r.height() << std::endl;
                }
            }
            file.close();
        }
    }
}
void MainWindow::sltFaceDetector(bool bChecked)
{
    Q_UNUSED(bChecked);
    fFaceDetector();
    /*if (_bFaceChecked = bChecked) {
        fFaceDetector();
    }
    else {
        frects.clear();
        _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
    }*/
}

void MainWindow::sltFaceDetector(CRectArray &arr)
{
    //std::for_each(std::cbegin(arr), std::cend(arr), [this](const auto &e){ _rects.push_back(_scene.addRect(e, QPen(Qt::red, 2))); _rects.back()->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsMovable, true); });
    std::for_each(std::cbegin(arr), std::cend(arr), std::bind(&MainWindow::AddRect, this, std::placeholders::_1));
    //_thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
    //std::copy(std::begin(arr), std::end(arr), std::back_inserter(frects));
    /*if (_bFaceChecked) {
        frects = std::move(arr);
        _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
    }*/
}

void MainWindow::fFaceDetector()
{
    auto safeWorker = std::make_unique<TWorker>(TWorker::workerType::wtFaceDetector);
    QThread *thread = new QThread();
    TWorker *worker = safeWorker.release();
    worker->setData(_image);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &TWorker::process);
    connect(worker, &TWorker::finished, thread, &QThread::quit);
    connect(worker ,&TWorker::completeFaceDetector, this, static_cast<void (MainWindow::*)(CRectArray&)>(&MainWindow::sltFaceDetector));
    connect(worker, &TWorker::finished, worker, &TWorker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(worker, &TWorker::noMemory, this, &MainWindow::sltNoMemory);

    thread->start();
}

void MainWindow::sltLBFRDetector(bool bChecked)
{
    Q_UNUSED(bChecked);
    fLBFRDetector();
}

void MainWindow::sltLBFRDetector(CPointFArray &arr)
{
    //std::for_each(std::cbegin(arr), std::cend(arr), [this](const auto &e){ _points.push_back(_scene.addEllipse(QRectF(QPointF(e.x() - 2, e.y() - 2), QSizeF(3, 3)), QPen(Qt::red, 2))); _points.back()->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsMovable, true); });
    std::for_each(std::cbegin(arr), std::cend(arr), std::bind(&MainWindow::AddPoint, this, std::placeholders::_1));
    /*if (_bLBFRChecked) {
        pts = std::move(arr);
        _thread.render(screenCenter, screenScale, this->size(), image_, frects, pts);
    }*/
}

void MainWindow::fLBFRDetector()
{
    auto safeWorker = std::make_unique<TWorker>(TWorker::workerType::wtLBFRDetector);
    QThread *thread = new QThread();
    TWorker *worker = safeWorker.release();
    worker->setData(_image);
    worker->moveToThread(thread);
    auto items = _scene.items();
    for (auto it{std::cbegin(items)}; it != std::cend(items); ++it) {
        if (RectItem::Type == (*it)->type()) {
            worker->setRect(qgraphicsitem_cast<RectItem*>(*it)->getRect());
            break;
        }
    }

    connect(thread, &QThread::started, worker, &TWorker::process);
    connect(worker, &TWorker::finished, thread, &QThread::quit);
    connect(worker, &TWorker::completeLBFRDetector, this, static_cast<void (MainWindow::*)(CPointFArray&)>(&MainWindow::sltLBFRDetector));
    connect(worker, &TWorker::finished, worker, &TWorker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(worker, &TWorker::noMemory, this, &MainWindow::sltNoMemory);

    thread->start();
}

void MainWindow::nextFrame()
{
    if (_safeStream) {
        QImage newImage(static_cast<int>(_safeStream->getWidth()), static_cast<int>(_safeStream->getHeight()), QImage::Format::Format_RGB32);
        if (_safeStream->getNextFrame(newImage)) {
            _image0 = std::move(newImage);
            this->Rotate();
        }
    }
}

void MainWindow::prevFrame()
{
    if (_safeStream && _safeStream->seek(-40000)) {
        this->nextFrame();
    }
}

void MainWindow::sltNoMemory()
{
    QMessageBox::warning(this, "Warning", "No enough memory");
}

void MainWindow::AddPoint(const QPointF &p)
{
    auto item = std::make_unique<PointItem>(ptNum++);
    item->setPos(p);
    item->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsMovable, true);
    item->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsSelectable, true);
    _scene.addItem(item.release());
}

void MainWindow::sltRotation0() {
    if (Rotation::Rot0 != rotation_) {
        rotation_ = Rotation::Rot0;
        Rotate();
    }
}

void MainWindow::sltRotation90() {
    if (Rotation::Rot90 != rotation_) {
        rotation_ = Rotation::Rot90;
        Rotate();
    }
}

void MainWindow::sltRotation270() {
    if (Rotation::Rot270 != rotation_) {
        rotation_ = Rotation::Rot270;
        Rotate();
    }
}

void MainWindow::Rotate() {
    switch (rotation_) {
    case Rotation::Rot90:
        _image = std::move(imgRotate<Rotate90>(_image0));
        break;
    case Rotation::Rot180:
        _image = std::move(imgRotate<Rotate180>(_image0));
        break;
    case Rotation::Rot270:
        _image = std::move(imgRotate<Rotate270>(_image0));
        break;
    case Rotation::Rot0:
    default:
        _image = _image0;
        break;
    };
    QPixmap pixmap;
    pixmap.convertFromImage(_image);
    _scene.clear();
    _scene.addPixmap(pixmap);

}

void MainWindow::sltAddRect() {
    AddRect(QRect(0, 0, 250, 250));
}

void MainWindow::AddRect(const QRect &r)
{
    auto item = std::make_unique<RectItem>(r);
    item->setPos(r.topLeft());
    item->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsMovable, true);
    //item->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsSelectable, true);
    _rects.push_back(item.get());
    _scene.addItem(item.release());
}

void MainWindow::sltAbout() {
    QStringList props;
    props.append(QString("MarkerQt: %1 %2-bit").arg(__DATE__).arg(sizeof(void*) * 8));
    props.append(QString());
    props.append(QString("Qt: %1").arg(qVersion()));
    props.append(QString("Dlib: %1.%2").arg(DLIB_MAJOR_VERSION).arg(DLIB_MINOR_VERSION));
    if (AVUtilDll::getInstance().isInited()) {
        props.append(QString::fromStdString(std::string("FFmpeg: ") + AVUtilDll::getInstance().p_av_version_info()));
    }
    props.append(QString());
    QMessageBox::about(this, "about", props.join("\n"));
}
