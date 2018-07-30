#pragma once
#include <editor/shader_item.hpp>
#include <QObject>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlProperty>
namespace isf_editor
{
//! This class creates a QML control for every meaningful ISF shader parameter
struct control_factory
{
  const isf::input& i;
  const int index;

  QString operator()(const isf::float_input& t)
  {
    auto prop = QString::fromStdString(i.name);
    auto label = QString::fromStdString(i.label);
    return QString(R"_(
                   Text {
                     text: '%2';
                     color: 'white';
                   }
                   Slider {
                     mapFunc: function(v) { return %3 + v * (%4 - %3) };
                     initialValue: (%5 - %3) / (%4 - %3);
                     orientation: Qt.horizontal;
                     height: 30;
                     onValueChanged: s.shader.setControl(%1, value);
                   }
                   )_")
        .arg(index)
        .arg(label.isEmpty() ? prop : label)
        .arg(t.min)
        .arg(t.max)
        .arg(t.def);
  }

  QString operator()(const isf::long_input& t)
  {
    auto prop = QString::fromStdString(i.name);
    auto label = QString::fromStdString(i.label);
    QString model = "[";
    QString values = "[";
    for(std::size_t i = 0; i < t.labels.size(); i++)
    {
      const auto& label_i = t.labels[i];
      auto val_i = QString::number(t.values[i]);

      // Text
      if(!label_i.empty())
        model += '"' + QString::fromStdString(label_i) + '"';
      else
        model += '"' + val_i + '"';

      // Value
      values += val_i;

      if(i != t.labels.size() - 1)
      {
        model += ", ";
        values += ", ";
      }
    }
    model += "]";
    values += "]";

    return QString(R"_(
                   Text {
                     text: '%2';
                     color: 'white';
                   }
                   QC.ComboBox {
                     model: %3;
                     property var values: %4;
                     currentIndex: %5;
                     height: 30;
                     onCurrentIndexChanged: s.shader.setControl(%1, values[currentIndex]);
                   }
                   )_")
        .arg(index)
        .arg(label.isEmpty() ? prop : label)
        .arg(model)
        .arg(values)
        .arg(t.def);
  }

  QString operator()(const isf::bool_input& t)
  {
      auto prop = QString::fromStdString(i.name);
      auto label = QString::fromStdString(i.label);
      return QString(R"_(
                     Text {
                       text: '%2';
                       color: 'white';
                     }
                     Switch {
                       state: %3;
                       onStateChanged: s.shader.setControl(%1, state == 'ON');
                     }
                     )_")
          .arg(index)
          .arg(label.isEmpty() ? prop : label)
          .arg(t.def ? "'ON'" : "'OFF'");
  }

  QString operator()(const isf::event_input& t)
  {
    auto prop = QString::fromStdString(i.name);
    auto label = QString::fromStdString(i.label);
    return QString(
                R"_(
                Text {
                  text: '%3';
                  color: 'white';
                }
                Switch {
                  id: %2_sw;
                  ease: false;
                  Timer { interval: 16; onTriggered: if(%2_sw.state == 'ON') %2_sw.state = 'OFF'; id: tmr }
                  onStateChanged: { s.shader.setControl(%1, state == 'ON'); tmr.start() }
                }
                )_")
        .arg(index)
        .arg(prop)
        .arg(label.isEmpty() ? prop : label);
  }

  QString operator()(const isf::point2d_input& t)
  {
    auto prop = QString::fromStdString(i.name);
    auto label = QString::fromStdString(i.label);
    return QString(
                R"_(
                Text {
                  text: '%2';
                  color: 'white';
                }
                XYPad {
                  property real minX: %3;
                  property real maxX: %4;
                  property real minY: %5;
                  property real maxY: %6;
                  stickX: %7;
                  stickY: %8;

                  onStickXChanged: s.shader.setControl(%1, Qt.point(minX + stickX * (maxX - minX), minY + stickY * (maxY - minY)));
                  onStickYChanged: s.shader.setControl(%1, Qt.point(minX + stickX * (maxX - minX), minY + stickY * (maxY - minY)));
                }
                )_")
        .arg(index)
        .arg(label.isEmpty() ? prop : label)
        .arg(t.min? (*t.min)[0] : 0.)
        .arg(t.max ? (*t.max)[0] : 1.)
        .arg(t.min? (*t.min)[1] : 0.)
        .arg(t.max ? (*t.max)[1] : 1.)
        .arg(t.def? (*t.def)[0] : 0.5)
        .arg(t.def ? (*t.def)[0] : 0.5);
  }

  QString operator()(const isf::point3d_input& i)
  {
    return {};
  }

  QString operator()(const isf::audio_input& i)
  {
    return {};
  }

  QString operator()(const isf::audioFFT_input& i)
  {
    return {};
  }

  QString operator()(const isf::color_input& t)
  {
      auto prop = QString::fromStdString(i.name);
      auto label = QString::fromStdString(i.label);
      return QString(R"_(
                     Text {
                       text: '%2';
                       color: 'white';
                     }
                     RGBSlider {
                       onColorChanged: s.shader.setControl(%1, color);
                     }
                     )_")
          .arg(index)
          .arg(label.isEmpty() ? prop : label);
  }

  QString operator()(const isf::image_input& i)
  {
    return {};
  }
};

}
