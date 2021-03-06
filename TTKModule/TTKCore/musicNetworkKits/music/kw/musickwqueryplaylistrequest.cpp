#include "musickwqueryplaylistrequest.h"
#include "musicsemaphoreloop.h"
#include "musicnumberutils.h"

MusicKWQueryPlaylistRequest::MusicKWQueryPlaylistRequest(QObject *parent)
    : MusicQueryPlaylistRequest(parent)
{
    m_pageSize = 30;
    m_queryServer = QUERY_KW_INTERFACE;
}

void MusicKWQueryPlaylistRequest::startToSearch(QueryType type, const QString &playlist)
{
    if(type == MusicQuery)
    {
        startToSearch(playlist);
    }
    else
    {
        m_searchText = playlist.isEmpty() ? "167" : playlist;
        startToPage(0);
    }
}

void MusicKWQueryPlaylistRequest::startToPage(int offset)
{
    if(!m_manager)
    {
        return;
    }

    TTK_LOGGER_INFO(QString("%1 startToSearch %2").arg(getClassName()).arg(offset));
    deleteAll();

    const QUrl &musicUrl = MusicUtils::Algorithm::mdII(KW_PLAYLIST_URL, false).arg(m_searchText).arg(offset).arg(m_pageSize);
    m_pageTotal = 0;
    m_interrupt = true;

    QNetworkRequest request;
    request.setUrl(musicUrl);
    request.setRawHeader("User-Agent", MusicUtils::Algorithm::mdII(KW_UA_URL, ALG_UA_KEY, false).toUtf8());
    MusicObject::setSslConfiguration(&request);

    m_reply = m_manager->get(request);
    connect(m_reply, SIGNAL(finished()), SLOT(downLoadFinished()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}

void MusicKWQueryPlaylistRequest::startToSearch(const QString &playlist)
{
    if(!m_manager)
    {
        return;
    }

    TTK_LOGGER_INFO(QString("%1 startToSearch %2").arg(getClassName()).arg(playlist));
    const QUrl &musicUrl = MusicUtils::Algorithm::mdII(KW_PLAYLIST_INFO_URL, false).arg(playlist);
    m_interrupt = true;

    QNetworkRequest request;
    request.setUrl(musicUrl);
    request.setRawHeader("User-Agent", MusicUtils::Algorithm::mdII(KW_UA_URL, ALG_UA_KEY, false).toUtf8());
    MusicObject::setSslConfiguration(&request);

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), SLOT(getDetailsFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}

void MusicKWQueryPlaylistRequest::getPlaylistInfo(MusicResultsItem &item)
{
    if(!m_manager)
    {
        return;
    }

    TTK_LOGGER_INFO(QString("%1 getPlaylistInfo %2").arg(getClassName()).arg(item.m_id));
    const QUrl &musicUrl = MusicUtils::Algorithm::mdII(KW_PLAYLIST_INFO_URL, false).arg(item.m_id);

    QNetworkRequest request;
    request.setUrl(musicUrl);
    request.setRawHeader("User-Agent", MusicUtils::Algorithm::mdII(KW_UA_URL, ALG_UA_KEY, false).toUtf8());
    MusicObject::setSslConfiguration(&request);

    MusicSemaphoreLoop loop;
    QNetworkReply *reply = m_manager->get(request);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &loop, SLOT(quit()));
    loop.exec();

    if(!reply || reply->error() != QNetworkReply::NoError)
    {
        return;
    }

    QJson::Parser parser;
    bool ok;
    const QVariant &data = parser.parse(reply->readAll(), &ok);
    if(ok)
    {
        const QVariantMap &value = data.toMap();
        if(!value.isEmpty())
        {
            item.m_coverUrl = value["pic"].toString();
            item.m_name = value["title"].toString();
            item.m_playCount = QString::number(value["playnum"].toULongLong());
            item.m_description = value["info"].toString();
            item.m_updateTime = QDateTime::fromMSecsSinceEpoch(value["ctime"].toULongLong()*1000).toString(MUSIC_YEAR_FORMAT);
            item.m_nickName = value["uname"].toString();
        }
    }
}

void MusicKWQueryPlaylistRequest::downLoadFinished()
{
    if(!m_reply)
    {
        deleteAll();
        return;
    }

    TTK_LOGGER_INFO(QString("%1 downLoadFinished").arg(getClassName()));
    Q_EMIT clearAllItems();
    m_musicSongInfos.clear();
    m_interrupt = false;

    if(m_reply->error() == QNetworkReply::NoError)
    {
        const QByteArray &bytes = m_reply->readAll();

        QJson::Parser parser;
        bool ok;
        const QVariant &data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            m_pageTotal = value["total"].toString().toLongLong();
            if(value.contains("child"))
            {
                m_tags = value["ninfo"].toMap()["name"].toString();
                const QVariantList &datas = value["child"].toList();
                foreach(const QVariant &var, datas)
                {
                    if(m_interrupt) return;

                    if(var.isNull())
                    {
                        continue;
                    }

                    value = var.toMap();
                    getMorePlaylistDetails(value["sourceid"].toString());
                }
            }
        }
    }

//    Q_EMIT downLoadDataChanged(QString());
    deleteAll();
}

void MusicKWQueryPlaylistRequest::getDetailsFinished()
{
    QNetworkReply *reply = TTKObject_cast(QNetworkReply*, QObject::sender());

    TTK_LOGGER_INFO(QString("%1 getDetailsFinished").arg(getClassName()));
    Q_EMIT clearAllItems();
    m_musicSongInfos.clear();
    m_interrupt = false;

    if(reply && m_manager && reply->error() == QNetworkReply::NoError)
    {
        const QByteArray &bytes = reply->readAll();

        QJson::Parser parser;
        bool ok;
        const QVariant &data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(!value.isEmpty() && value.contains("musiclist"))
            {
                const QVariantList &datas = value["musiclist"].toList();
                foreach(const QVariant &var, datas)
                {
                    if(var.isNull())
                    {
                        continue;
                    }

                    value = var.toMap();
                    MusicObject::MusicSongInformation musicInfo;
                    musicInfo.m_singerName = MusicUtils::String::illegalCharactersReplaced(value["artist"].toString());
                    musicInfo.m_songName = MusicUtils::String::illegalCharactersReplaced(value["name"].toString());
                    musicInfo.m_timeLength = MusicTime::msecTime2LabelJustified(value["duration"].toInt()*1000);

                    musicInfo.m_songId = value["id"].toString();
                    musicInfo.m_artistId = value["artistid"].toString();
                    musicInfo.m_albumId = value["albumid"].toString();
                    musicInfo.m_albumName = MusicUtils::String::illegalCharactersReplaced(value["album"].toString());

                    musicInfo.m_year = QString();
                    musicInfo.m_discNumber = "1";
                    musicInfo.m_trackNumber = "0";

                    if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;
                    readFromMusicSongPic(&musicInfo);
                    if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;
                    musicInfo.m_lrcUrl = MusicUtils::Algorithm::mdII(KW_SONG_LRC_URL, false).arg(musicInfo.m_songId);
                    ///music normal songs urls
                    readFromMusicSongAttribute(&musicInfo, value["formats"].toString(), m_searchQuality, m_queryAllRecords);
                    if(m_interrupt || !m_manager || m_stateCode != MusicObject::NetworkQuery) return;

                    if(musicInfo.m_songAttrs.isEmpty())
                    {
                        continue;
                    }
                    //
                    if(!findUrlFileSize(&musicInfo.m_songAttrs)) return;
                    //
                    MusicSearchedItem item;
                    item.m_songName = musicInfo.m_songName;
                    item.m_singerName = musicInfo.m_singerName;
                    item.m_albumName = musicInfo.m_albumName;
                    item.m_time = musicInfo.m_timeLength;
                    item.m_type = mapQueryServerString();
                    Q_EMIT createSearchedItem(item);
                    m_musicSongInfos << musicInfo;
                }
            }
        }
    }

    Q_EMIT downLoadDataChanged(QString());
}

void MusicKWQueryPlaylistRequest::getMorePlaylistDetailsFinished()
{
    TTK_LOGGER_INFO(QString("%1 getMorePlaylistDetailsFinished").arg(getClassName()));
    QNetworkReply *reply = TTKObject_cast(QNetworkReply*, QObject::sender());

    if(reply && reply->error() == QNetworkReply::NoError)
    {
        QJson::Parser parser;
        bool ok;
        const QVariant &data = parser.parse(reply->readAll(), &ok);
        if(ok)
        {
            const QVariantMap &value = data.toMap();
            if(value["result"].toString() == "ok")
            {
                MusicResultsItem item;
                item.m_tags = m_tags;
                item.m_coverUrl = value["pic"].toString();
                item.m_id = value["id"].toString();
                item.m_name = value["title"].toString();
                item.m_playCount = QString::number(value["playnum"].toULongLong());
                item.m_description = value["info"].toString();
                item.m_updateTime = QDateTime::fromMSecsSinceEpoch(value["ctime"].toULongLong()*1000).toString(MUSIC_YEAR_FORMAT);
                item.m_nickName = value["uname"].toString();
                Q_EMIT createPlaylistItem(item);
            }
        }
    }
}

void MusicKWQueryPlaylistRequest::getMorePlaylistDetails(const QString &pid)
{
    if(!m_manager)
    {
        return;
    }

    TTK_LOGGER_INFO(QString("%1 getMorePlaylistDetails %2").arg(getClassName()).arg(pid));
    const QUrl &musicUrl = MusicUtils::Algorithm::mdII(KW_PLAYLIST_INFO_URL, false).arg(pid);

    QNetworkRequest request;
    request.setUrl(musicUrl);
    request.setRawHeader("User-Agent", MusicUtils::Algorithm::mdII(KW_UA_URL, ALG_UA_KEY, false).toUtf8());
    MusicObject::setSslConfiguration(&request);

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), SLOT(getMorePlaylistDetailsFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}
