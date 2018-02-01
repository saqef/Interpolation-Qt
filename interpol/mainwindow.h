#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QFileDialog>
#include <QPointF>
#include <QMessageBox>
#include <QVector>
#include <QTextStream>
#include <QRandomGenerator>
#include <QtConcurrent>

enum mod{orig,need};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool errors=true;
    int N=0,order=0,funcIndex=0;
    qint64 M=0;
    double h=0.001,X0=0,X1=0,hRand=0;

private slots:
    void on_ButtonCalculate_clicked();
    void on_ButtonInterpol_clicked();


private:
    Ui::MainWindow *ui;
    QString dir="",function="";
    QFile original,needed,neededWithFx,interpol;
    QStringList functions;
    QVector<QPointF>* A=nullptr,*B=nullptr;
    std::function<double(double)> pfunc;

    bool getParametrs();
    void interpolation();
    void calcAndWrite(std::function<double(double)> otherFunc, mod Mod);
    void calcAndWrite(std::function<double(double)> otherFunc, mod Mod, QFile &fileFull);
    void calcAndWrite(std::function<double(double)> otherFunc, mod Mod, QFile &file, QFile &fileFull);

};

#endif // MAINWINDOW_H
