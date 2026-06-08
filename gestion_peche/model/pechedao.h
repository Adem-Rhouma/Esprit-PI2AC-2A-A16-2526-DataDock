#ifndef PECHE_DAO_H
#define PECHE_DAO_H

#include <QDate>
#include <QString>

class Peche;

class PecheDAO
{
public:
    PecheDAO();

    bool nextId(int *outId, QString *err = nullptr) const;
    bool insert(const Peche &peche, QString *err = nullptr) const;
    bool update(const Peche &peche, QString *err = nullptr) const;
    bool loadById(int idPeche, Peche *outPeche, QString *err = nullptr) const;
    bool existsDuplicate(const QDate &datePeche,
                         const QString &zonePeche,
                         int idNavire,
                         int excludeId,
                         QString *err = nullptr) const;
};

#endif // PECHE_DAO_H
