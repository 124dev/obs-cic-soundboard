/********************************************************************************
** Form generated from reading UI file 'Soundboard.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SOUNDBOARD_H
#define UI_SOUNDBOARD_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Soundboard
{
public:
    QVBoxLayout *mainLayout;
    QListWidget *list;
    QHBoxLayout *horizontalLayout;
    QPushButton *btnPlay;
    QPushButton *btnStop;
    QPushButton *btnAdd;
    QPushButton *btnRemove;

    void setupUi(QWidget *Soundboard)
    {
        if (Soundboard->objectName().isEmpty())
            Soundboard->setObjectName("Soundboard");
        Soundboard->resize(428, 200);
        mainLayout = new QVBoxLayout(Soundboard);
        mainLayout->setSpacing(4);
        mainLayout->setObjectName("mainLayout");
        mainLayout->setContentsMargins(4, 4, 4, 4);
        list = new QListWidget(Soundboard);
        list->setObjectName("list");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(list->sizePolicy().hasHeightForWidth());
        list->setSizePolicy(sizePolicy);

        mainLayout->addWidget(list);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        btnPlay = new QPushButton(Soundboard);
        btnPlay->setObjectName("btnPlay");

        horizontalLayout->addWidget(btnPlay);

        btnStop = new QPushButton(Soundboard);
        btnStop->setObjectName("btnStop");

        horizontalLayout->addWidget(btnStop);

        btnAdd = new QPushButton(Soundboard);
        btnAdd->setObjectName("btnAdd");

        horizontalLayout->addWidget(btnAdd);

        btnRemove = new QPushButton(Soundboard);
        btnRemove->setObjectName("btnRemove");

        horizontalLayout->addWidget(btnRemove);


        mainLayout->addLayout(horizontalLayout);


        retranslateUi(Soundboard);

        QMetaObject::connectSlotsByName(Soundboard);
    } // setupUi

    void retranslateUi(QWidget *Soundboard)
    {
        btnPlay->setText(QCoreApplication::translate("Soundboard", "Play", nullptr));
        btnStop->setText(QCoreApplication::translate("Soundboard", "Stop", nullptr));
        btnAdd->setText(QCoreApplication::translate("Soundboard", "Add", nullptr));
        btnRemove->setText(QCoreApplication::translate("Soundboard", "Remove", nullptr));
        (void)Soundboard;
    } // retranslateUi

};

namespace Ui {
    class Soundboard: public Ui_Soundboard {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SOUNDBOARD_H
