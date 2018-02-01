#include "mainwindow.h"
#include "ui_mainwindow.h"

double f1(double x);
double f2(double x);
double f3(double x);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    functions<<"sin(x) * cos(x)"<<"x^2 * tan(x)"<<"x^3+5x";
    ui->comboBoxFunctions->addItems(functions);

    QStringList orders;
    QtConcurrent::run(QThreadPool::globalInstance(),[&]()
    {
        orders<<"stepwise";
        for(int i=1;i<999;i++)
        {
            orders<<QString::number(i);
        }
        ui->comboBoxOrder->addItems(orders);
        ui->comboBoxOrder->setCurrentIndex(0);
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_ButtonCalculate_clicked()
{
    if(getParametrs())
    {
        dir = QFileDialog::getExistingDirectory(this, tr("Save to ..."),"/home",QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty())
        {
            A=new QVector<QPointF>;

            original.setFileName(dir+"\\"+function+"_original.txt");
            if(original.open(QIODevice::WriteOnly| QIODevice::Text))
            {
                switch (funcIndex)
                {
                case 0:
                    pfunc=&f1;
                    break;
                case 1:
                    pfunc=&f2;
                    break;
                case 2:
                    pfunc=&f3;
                    break;
                }

                QRandomGenerator* gener=QRandomGenerator::system();
                M = gener->bounded(N/2,2*N/3);
                B=new QVector<QPointF>;
                hRand=gener->bounded(h)+std::numeric_limits<double>::epsilon();
                X1=X0+(gener->bounded(10)-5)*hRand;

                needed.setFileName(dir+"\\"+function+"_needed.txt");
                neededWithFx.setFileName(dir+"\\"+function+"_neededWithFx.txt");
                if(needed.open(QIODevice::WriteOnly| QIODevice::Text)&&neededWithFx.open(QIODevice::WriteOnly| QIODevice::Text))
                {
                    QTextStream writeOrig(&original),writeNeed(&needed),writeNeedFx(&neededWithFx);

                    writeOrig<<N<<endl;
                    writeNeed<<M<<endl;
                    writeNeedFx<<M<<endl;

                    QFuture<void> futureOrig = QtConcurrent::run(this,&MainWindow::calcAndWrite,pfunc,orig);
                    QFuture<void> futureNeed = QtConcurrent::run(this,&MainWindow::calcAndWrite,pfunc,need);

                    futureOrig.waitForFinished();

                    for(auto i:*A)
                    {
                        writeOrig<<i.x()<<" "<<i.y()<<endl;
                    }

                    futureNeed.waitForFinished();

                    for(auto i:*B)
                    {
                        writeNeed<<i.x()<<endl;
                        writeNeedFx<<i.x()<<" "<<i.y()<<endl;
                    }

                    needed.close();
                    neededWithFx.close();
                }
                else
                    QMessageBox::information(0,"Error","Something's wrong with \"needed\" or \"neededFx\" file.",0,0);

                original.close();
                delete B;
            }
            else
                QMessageBox::information(0,"Error","Something's wrong with \"original\" file.",0,0);

            delete A;
        }
    }
}


void MainWindow::on_ButtonInterpol_clicked()
{
    if(!QString::compare(ui->comboBoxOrder->currentText(),"stepwise",Qt::CaseInsensitive))
        order=0;
    else
    {
        order=ui->comboBoxOrder->currentText().toInt(&errors);
        if(!errors)
        {
            QMessageBox::information(0,"Error","Order must be number.",0,0);
            ui->comboBoxOrder->setCurrentText("1");
            return;
        }
    }
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open \"original.txt\""), dir, tr("Text Files (*.txt)"));
    if(!fileName.isEmpty())
    {
        original.setFileName(fileName);
        if(original.open(QIODevice::ReadOnly| QIODevice::Text))
        {
            QTextStream read(&original);
            read>>N;
            if(N<=0)
            {
                QMessageBox::information(0,"Error","N must be >0.",0,0);
                return;
            }
            A=new QVector<QPointF>;
            double temp=0,temp2=0;
            for(int i=0;i<N;i++)
            {
                read>>temp>>temp2;
                A->push_back(QPointF(temp,temp2));
            }
            original.close();

            fileName = QFileDialog::getOpenFileName(this,tr("Open \"needed.txt\""), dir, tr("Text Files (*.txt)"));
            if(!fileName.isEmpty())
            {
                needed.setFileName(fileName);
                if(needed.open(QIODevice::ReadOnly| QIODevice::Text))
                {
                    QTextStream read(&needed);
                    read>>M;
                    if(M<=0)
                    {
                        QMessageBox::information(0,"Error","M must be >0.",0,0);
                        delete A;
                        return;
                    }
                    if(M<=order)
                    {
                        QMessageBox::information(0,"Error","Order must be less than count of points.\n Max order is"+QString::number(M-1),0,0);
                        ui->comboBoxOrder->setCurrentText(QString::number(M-1));
                        delete A;
                        return;
                    }
                    B=new QVector<QPointF>;
                    double temp=0;
                    for(int i=0;i<M;i++)
                    {
                        read>>temp;
                        B->push_back(QPointF(temp,0));
                    }
                    needed.close();

                    QDir directory(needed.fileName());
                    directory.cdUp();
                    QString st=directory.absolutePath();
                    interpol.setFileName(directory.absolutePath()+"/interpolation.txt");
                    QTextStream write(&interpol);
                    if(interpol.open(QIODevice::WriteOnly| QIODevice::Text))
                    {
                        std::sort(A->begin(),A->end(),[](QPointF A1,QPointF A2)
                            {
                               return A1.x() < A2.x();
                            });

                        std::sort(B->begin(),B->end(),[](QPointF A1,QPointF A2)
                            {
                               return A1.x() < A2.x();
                            });
                        QFuture<void> futureInter = QtConcurrent::run(this,&MainWindow::interpolation);
                        futureInter.waitForFinished();
                        //interpolation();
                        write<<M<<endl;
                        for (auto point:*B)
                        {
                            write<<point.x()<<" "<<point.y()<<endl;
                        }
                        interpol.close();
                    }
                    else
                        QMessageBox::information(0,"Error","Something's wrong with \"interpolation\" file.",0,0);
                    delete B;
                }
                else
                    QMessageBox::information(0,"Error","Something's wrong with \"needed\" file.",0,0);
            }
            delete A;
        }
        else
            QMessageBox::information(0,"Error","Something's wrong with \"original\" file.",0,0);
    }
}

bool MainWindow::getParametrs()
{
    int funcIndex=ui->comboBoxFunctions->currentIndex();
    function=functions.at(funcIndex);
    function=function.replace("*","X");
    X0=ui->lineEditX->text().toDouble(&errors);
    if(!errors)
    {
        QMessageBox::information(0,"Error","X must be number.",0,0);
        ui->lineEditX->setText("0");
        return false;
    }
    N=ui->lineEditN->text().toInt(&errors);
    if(!errors)
    {
        QMessageBox::information(0,"Error","N must be number.",0,0);
        ui->lineEditN->setText("1000");
        return false;
    }
    h=ui->lineEditH->text().toDouble(&errors);
    if(!errors)
    {
        QMessageBox::information(0,"Error","H must be number.",0,0);
        ui->lineEditH->setText("0.01");
        return false;
    }
    N=std::abs(N);
    ui->lineEditN->setText(QString::number(N));
    h=std::abs(h);
    ui->lineEditH->setText(QString::number(h));
    return true;
}

void MainWindow::calcAndWrite(std::function<double(double)> otherFunc, mod Mod)
{
    switch (Mod)
    {
    case orig:
        for (int i=0;i<N;i++)
        {
            A->push_back(QPointF(X0+i*h,otherFunc(X0+i*h)));
        }
        break;
    case need:
        for (int i=0;i<M;i++)
        {
            B->push_back(QPointF(X1+i*hRand,otherFunc(X1+i*hRand)));
        }
        break;
    }
}

void MainWindow::calcAndWrite(std::function<double(double)> otherFunc, mod Mod, QFile& fileFull)
{
    QTextStream writeStreamFull(&fileFull);

    switch (Mod)
    {
    case orig:
        writeStreamFull<<N<<endl;
        for (int i=0;i<N;i++)
        {
            A->push_back(QPointF(X0+i*h,otherFunc(X0+i*h)));
            writeStreamFull<<A->at(i).x()<<" "<<A->at(i).y()<<endl;
        }
        break;
    case need:
        writeStreamFull<<M<<endl;
        for (int i=0;i<M;i++)
        {
            B->push_back(QPointF(X1+i*hRand,otherFunc(X1+i*hRand)));
            writeStreamFull<<B->at(i).x()<<" "<<B->at(i).y()<<endl;
        }
        break;
    }
}

void MainWindow::calcAndWrite(std::function<double(double)> otherFunc, mod Mod, QFile& file, QFile& fileFull)
{
    QTextStream writeStream(&file),writeStreamFull(&fileFull);

    switch (Mod)
    {
    case orig:
        writeStream<<N<<endl;
        writeStreamFull<<N<<endl;
        for (int i=0;i<N;i++)
        {
            A->push_back(QPointF(X0+i*h,otherFunc(X0+i*h)));
            writeStreamFull<<A->at(i).x()<<" "<<A->at(i).y()<<endl;
            writeStream<<A->at(i).x()<<endl;
        }
        break;
    case need:
        writeStream<<M<<endl;
        writeStreamFull<<M<<endl;
        for (int i=0;i<M;i++)
        {
            B->push_back(QPointF(X1+i*hRand,otherFunc(X1+i*hRand)));
            writeStreamFull<<B->at(i).x()<<" "<<B->at(i).y()<<endl;
            writeStream<<B->at(i).x()<<endl;
        }
        break;
    }
}

void MainWindow::interpolation()
{
    int border=0;
    double temp=0, s=0, w=0;
    double* z = new double[order];

    for (int p = 0; p < M; p++)
    {
        while ((A->at(border).x() < B->at(p).x())&&(border!=N))
            border++;

        if(order==0)
        {
            if (std::abs(A->at(border).x()-B->at(p).x())<std::abs(A->at(border+1==N?N-1:border+1).x()-B->at(p).x()))//nearest
                B->replace(p,QPointF(B->at(p).x(),A->at(border).y()));
            else
                B->replace(p,QPointF(B->at(p).x(),A->at(border+1==N?N-1:border+1).y()));
        }
        else
        {
            border -= order / 2;

            if (border < 0)
                border = 0;
            if (border + order >= N)
                border = N - order;

            s = 0, w = 1;

            for (int i = border; i < order + border; i++)
            {
                temp = 1;
                for (int j = border; j < order + border; j++)
                    if (i != j)
                        temp *= (A->at(i).x() - A->at(j).x());
                z[i-border] = A->at(i).y() / temp;
                temp = B->at(p).x() - A->at(i).x();
                if(temp!=0)
                s += z[i- border] / temp;
                w *= temp;
            }
            B->replace(p,QPointF(B->at(p).x(),w*s));
        }
    }
    delete[] z;
}



double f1(double x)
{
    return sin(x)*cos(x);
}

double f2(double x)
{
    return x*x*tan(x);
}

double f3(double x)
{
    return x*x*x+5*x;
}
