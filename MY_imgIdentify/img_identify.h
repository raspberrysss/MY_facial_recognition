#ifndef IMG_IDENTIFY_H
#define IMG_IDENTIFY_H

#include <QWidget>
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class img_identify;
}

class img_identify : public QWidget
{
    Q_OBJECT

public:
    explicit img_identify(QWidget *parent = nullptr);
    ~img_identify();
public slots://槽函数
    void showCamera(int id, const QImage preview);
    void take_pic();//拍照函数
    void tokenReply(QNetworkReply* reply);//处理应答，获取token
    void begin_baidu();//点击按钮，分析
    void imgReply(QNetworkReply* reply);//处理应答，获取img分析

private:
    Ui::img_identify *ui;
    //QT直接调摄像头，？用指针
    QCamera* camera;//qt+=me...
    QCameraViewfinder* finder;//摄像头取景器，采集到摄像后在取景器查看
    QCameraImageCapture* imgcapture;
    QTimer *refreshTimer;
    QNetworkAccessManager* tokenManager;
    QNetworkAccessManager* imgManager;
    QSslConfiguration sslConfig;
    QString access_token;
    QImage img;
};

#endif // IMG_IDENTIFY_H
