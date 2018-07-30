#pragma once
#include <editor/image_surface.hpp>
#include <QObject>
#include <QAudioDecoder>
#include <QQueue>
#include <QAudioBuffer>

namespace isf_editor
{
//! Writes audio data in an image regularly
class audio_reader final
    : public image_surface
{
public:
  audio_reader(QImage& m, QString file, std::mutex& mut):
    image_surface{m, mut}
  {
    connect(&m_decoder, &QAudioDecoder::bufferReady,
            this, [&] {
      m_buffers.push_back(m_decoder.read());
      m_started = true;
    });
    m_decoder.setSourceFilename(file);
    m_decoder.start();

    startTimer(16);
  }

  void timerEvent(QTimerEvent* ev) override
  {
    while(!m_buffers.empty() && m_currentPos + 64 > m_buffers.front().duration())
    {
      m_buffers.push_back(m_buffers.front());
      m_buffers.pop_front();
      m_currentPos = 0;
    }

    if(m_buffers.empty())
    {
      m_currentPos = 0;
      return;
    }

    parseSound(m_buffers.front(), m_currentPos);
    present(m_current);
    m_currentPos += 64;
  }

  void parseSound(QAudioBuffer& frame, int64_t pos)
  {
    m_current.clear();
    std::vector<float>& v = m_current;
    auto f = frame.format();
    auto n = std::min((int64_t)(pos + 64), (int64_t)frame.duration());
    switch(f.sampleType())
    {
      case QAudioFormat::SampleType::Float:
      {
        auto dat = reinterpret_cast<const float*>(frame.data());
        for(auto i = pos; i < n; i++)
        {
          v.push_back(dat[i]);
        }
        break;
      }
      case QAudioFormat::SampleType::SignedInt:
      {
        switch(f.sampleSize())
        {
          case 16:
          {
            auto dat = reinterpret_cast<const int16_t*>(frame.data());
            for(auto i = pos; i < n; i++)
            {
              v.push_back(dat[i] / 32768. + 0.5);
            }
            break;
          }
          case 24:
          {
            auto dat = reinterpret_cast<const int32_t*>(frame.data());
            for(auto i = pos; i < n; i++)
            {
              v.push_back(dat[i] / 8388608. + 0.5);
            }
            break;
          }
        }
        break;
      }
      case QAudioFormat::SampleType::UnSignedInt:
      {
        switch(f.sampleSize())
        {
          case 16:
          {
            auto dat = reinterpret_cast<const uint16_t*>(frame.data());
            for(auto i = pos; i < n; i++)
            {
              v.push_back(dat[i] / 32768.);
            }
            break;
          }
          case 24:
          {
            auto dat = reinterpret_cast<const uint32_t*>(frame.data());
            for(auto i = pos; i < n; i++)
            {
              v.push_back(dat[i] / 8388608. );
            }
            break;
          }
        }
        break;
      }
    }

  }

private:
  QAudioDecoder m_decoder;
  std::vector<float> m_current;
  uint64_t m_currentPos{};
  QQueue<QAudioBuffer> m_buffers;
  QImage m_img;
  bool m_started = false;
};

}
