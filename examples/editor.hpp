#pragma once

#include <QQuickItem>
namespace isf
{

class ObjectCreator : public QQuickItem
{
    Q_OBJECT
public:
signals:
    void addFloat(QString name, QString label);
private:

};

}
