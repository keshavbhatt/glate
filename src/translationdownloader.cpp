#include <QStandardPaths>
#include "translationdownloader.h"
#include "utils.h"

/*
Class used to download audio translation of given text
*/

TranslationDownloader::TranslationDownloader(QObject *parent, QList<QUrl> urls, QString translationUUID) : QObject(parent)
{
    //init download dir
    currentDownloadDir = utils::returnPath("audioCache/"+translationUUID);
    _cache_path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    this->urls = urls;
    total = urls.count();
    current = -1 ;
}

//entry function after class init
void TranslationDownloader::start(){
    if(current < total-1)
    {
        current = current+1;
    }else if(current == total - 1){
        emit downloadComplete();
        return;
    }
    startDownload(urls.at(current));
    emit downloadStarted("<span style='color:green;'>Share: </span>downloading part: "+QString::number(current+1)+" of "+QString::number(total));
}

void TranslationDownloader::startDownload(QUrl url){
    QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
    QNetworkDiskCache* diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(_cache_path);
    m_netwManager->setCache(diskCache);
    connect(m_netwManager,&QNetworkAccessManager::finished,[=](QNetworkReply* rep){
        if(rep->error() == QNetworkReply::NoError){
            emit downloadFinished("Downloaded part: "+QString::number(current+1)+" of "+QString::number(total));
            //save to file
            QFile file(currentDownloadDir+"/"+QString::number(current));
            file.open(QIODevice::WriteOnly);
            file.write(rep->readAll());
            file.close();
            start();
        }else{
            emit downloadError("<span style='color:red;'>Share: </span>downloaded error on part: "+QString::number(current+1)+" "+rep->errorString()+" of "+QString::number(total));
        }
        rep->deleteLater();
        m_netwManager->deleteLater();
    });
    QNetworkRequest request(url);
    m_netwManager->get(request);
}


