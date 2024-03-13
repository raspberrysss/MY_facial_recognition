#include "img_identify.h"
#include "ui_img_identify.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonParseError>
#include <QJsonObject>
#include <QBuffer>
#include <QJsonArray>


img_identify::img_identify(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::img_identify)
{
    ui->setupUi(this);
    //形成照相机
    camera=new QCamera();
    finder=new QCameraViewfinder();
    camera->setViewfinder(finder);
    imgcapture=new QCameraImageCapture(camera);
    camera->setCaptureMode(QCamera::CaptureStillImage);//静态图像
    imgcapture->setCaptureDestination(QCameraImageCapture::CaptureToFile);//设置照片去向,?去文件？？？，可以指定文件
    //绑定信号,异步程序处理
    connect(imgcapture,&QCameraImageCapture::imageCaptured,this,&img_identify::showCamera);
    connect(ui->pushButton,&QPushButton::clicked,this,&img_identify::begin_baidu);//按钮分析
    camera->start();

    //在ui上显示
    this->resize(800,600);
    //!!!父类不能是this，不然就无法调摄像头？？？有问题
    QVBoxLayout *vboxl=new QVBoxLayout();//垂直盒子
    QVBoxLayout *vboxr=new QVBoxLayout();
    vboxl->addWidget(ui->show);
    vboxl->addWidget(ui->pushButton);
    vboxr->addWidget(finder);
    vboxr->addWidget(ui->textBrowser);
    QHBoxLayout *hbox=new QHBoxLayout(this);//水平控件
    hbox->addLayout(vboxl);
    hbox->addLayout(vboxr);
    //hbox->addWidget(finder);
    this->setLayout(hbox);
    //拍照识别，点击按钮拍照显示，最终目的是实现拍照和取景器保持一致，无间断点击摄像头：用定时器
    //！！！利用定时器
    refreshTimer=new QTimer(this);
    connect(refreshTimer,&QTimer::timeout,this,&img_identify::take_pic);//时间到信号，引发take函数
    refreshTimer->start(20);//20ms一刷新，1s50张，用此判断进行绘制
    //得到照片后进行判断，使用百度接口
    //https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=9ITt0cUOYU9BuYUd9vHWJWtJ&client_secret=NavEvUTeVz9n5h10oPSKNXvHyzQJB9JT
    // "access_token": "24.98013934586623eebfa72e870b783b89.2592000.1712390255.282335-55419660"
    tokenManager =new QNetworkAccessManager(this);
    connect(tokenManager,&QNetworkAccessManager::finished,this,&img_identify::tokenReply);
    qDebug()<<tokenManager->supportedSchemes();//查看支持url格式，不支持https，安装库才支持，到windows system32里去找libcrypto-1_1-x64 libssl到D:\Qt\Qt5.12.2\Tools\QtCreator\bin
    //为什么加了库还是不支持https？？？？？？配好了！！！！！！！！！！！！！！！！！！！！！！！！！！！

    imgManager=new QNetworkAccessManager(this);
    connect(imgManager,&QNetworkAccessManager::finished,this,&img_identify::imgReply);//处理传回来的数据
    //发送请求，拼接url
    QUrl url("https://aip.baidubce.com/oauth/2.0/token");//不includeQurl也行，为什么
    //qDebug()<<url;
    QUrlQuery query;
    query.addQueryItem("grant_type","client_credentials");
    query.addQueryItem("client_id","9ITt0cUOYU9BuYUd9vHWJWtJ");
    query.addQueryItem("client_secret","NavEvUTeVz9n5h10oPSKNXvHyzQJB9JT");
    url.setQuery(query);
    qDebug()<<url;
    //ssl支持
    if(QSslSocket::supportsSsl())
    {
        qDebug()<<"支持ssl"<<endl;
    }
    else
    {
        qDebug()<<"不支持";
    }
    //QString n1=QSslSocket::supportsSsl();
    QString n2=QSslSocket::sslLibraryBuildVersionString();
    //QString n3=QSslSocket::sslLibraryVersionString();
    qDebug()<<n2<<" ";

    //请求协议，ssl配置
     sslConfig=QSslConfiguration::defaultConfiguration();
     sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
     sslConfig.setProtocol(QSsl::TlsV1_2);

    //拼接请求
     QNetworkRequest req;
     req.setUrl(url);
     req.setSslConfiguration(sslConfig);

     tokenManager->get(req);//发送请求，推荐post，get也行，就是不那么安全



}

img_identify::~img_identify()
{
    delete ui;
}

void img_identify::showCamera(int id, const QImage preview)//拍好的照片存储在pre这里
{
    Q_UNUSED(id);
    this->img=preview;
    ui->show->setPixmap(QPixmap::fromImage(preview));
}

void img_identify::take_pic()
{
    imgcapture->capture();//异步进行拍照
}

void img_identify::tokenReply(QNetworkReply *reply)
{
    //错误
    if(reply->error()!=QNetworkReply::NoError)
    {
        qDebug()<<reply->errorString();
        return;
    }

    QByteArray qa=reply->readAll();
    //qDebug()<<qa;
    QJsonParseError err;
    QJsonDocument doc=QJsonDocument::fromJson(qa,&err);
    if(err.error==QJsonParseError::NoError)
    {
        QJsonObject obj=doc.object();
        if(obj.contains("access_token"))
        {
            access_token=obj.take("access_token").toString();
        }
        ui->textBrowser->setText(access_token);
    }
    else {
        qDebug()<<"333"<<err.errorString();
    }
}

void img_identify::begin_baidu()
{
    //照片转换为base64编码
    QByteArray ba;
    QBuffer buff(&ba);//将字节数组加上文件设备io功能，速度快，效率高
    img.save(&buff,"png");//百度ai支持png，保存到buff
    QString base64str=ba.toBase64();//耗时多
    qDebug()<<"111"<<base64str<<endl;

    //拼接json Body参数设置
    QJsonObject postjson;
    QJsonDocument doc;
    postjson.insert("image",base64str);
    postjson.insert("image_type","BASE64");
    postjson.insert("face_field","age,expression,face_shape,gender,glasses,emotion,face_type,mask,spoofing,beauty");//spoofing 是否为合成图
    doc.setObject(postjson);
    QByteArray postdata=doc.toJson(QJsonDocument::Compact);//压缩格式无格式

    //组装图像识别请求
    QUrl url("https://aip.baidubce.com/rest/2.0/face/v3/detect");
    QUrlQuery query;
    query.addQueryItem("access_token",access_token);//设置请求参数
    url.setQuery(query);

    //组装请求
    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));
    req.setUrl(url);
    req.setSslConfiguration(sslConfig);
    imgManager->post(req,postdata);
}

void img_identify::imgReply(QNetworkReply* reply)
{
    if(reply->error()!=QNetworkReply::NoError)
    {
        qDebug()<<"222"<<reply->errorString();
        return;
    }
    const QByteArray replydata=reply->readAll();
    //qDebug()<<replydata;

    //解析json字符串
    QJsonParseError jsonerr;
    QJsonDocument doc=QJsonDocument::fromJson(replydata,&jsonerr);
    if(jsonerr.error==QJsonParseError::NoError)
    {
          QString face_info;
          QJsonObject obj=doc.object();
          if(obj.contains("result"))
          {
             QJsonObject result_obj=obj.take("result").toObject();
             if(result_obj.contains("face_list"))
             {
                QJsonArray face_list=result_obj.take("face_list").toArray();
                QJsonObject faceobj=face_list.at(0).toObject();//face_list的第0个
                //继续取年龄
                if(faceobj.contains("age"))
                {
                    double age=faceobj.take("age").toDouble();
                    face_info.append("年龄：").append(QString::number(age)).append("\r\n");//windows换行，天气预报也用过
                }
                if(faceobj.contains("gender"))
                {
                    QJsonObject gender_obj=faceobj.take("gender").toObject();
                    if(gender_obj.contains("type"))
                    {
                        QString gender=gender_obj.take("type").toString();
                         face_info.append("性别：").append(gender).append("\r\n");
                    }

                }

                if(faceobj.contains("mask"))
                {
                    QJsonObject mask_obj=faceobj.take("mask").toObject();
                    if(mask_obj.contains("type"))
                    {
                        int mask_type=mask_obj.take("type").toInt();
                         face_info.append("是否佩戴口罩：").append(mask_type==0?"未佩戴":"已佩戴").append("\r\n");
                    }

                }
             }
          }
          ui->textBrowser->setText(face_info);
    }
    else {
        qDebug()<<"error:"<<jsonerr.errorString();
    }

}
