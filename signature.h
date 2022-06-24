#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <QObject>

class Signature : public QObject
{
    Q_OBJECT
public:
    explicit Signature(QObject *parent = nullptr);

signals:

};

#endif // SIGNATURE_H
