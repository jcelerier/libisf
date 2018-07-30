#pragma once
#include <editor/image_surface.hpp>
#include <QUrl>
#include <QMediaPlayer>
#include <QMediaPlaylist>

namespace isf_editor
{
class VideoReader final
    : public image_surface
{
public:
  VideoReader(QImage& m, QString vid, std::mutex& mut):
    image_surface{m, mut}
  {
    m_pl.addMedia(QMediaContent(QUrl::fromLocalFile(vid)));
    m_pl.setPlaybackMode(QMediaPlaylist::PlaybackMode::Loop);
    m_player.setPlaylist(&m_pl);
    m_player.setVideoOutput(this);
    m_player.setMuted(true);
    m_player.play();
  }

private:
  QMediaPlayer m_player;
  QMediaPlaylist m_pl;
};

}
