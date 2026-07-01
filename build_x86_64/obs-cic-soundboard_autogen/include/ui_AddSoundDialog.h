/********************************************************************************
** Form generated from reading UI file 'AddSoundDialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADDSOUNDDIALOG_H
#define UI_ADDSOUNDDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_AddSoundDialog
{
public:
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *lblName;
    QLineEdit *txtName;
    QLabel *lblFile;
    QHBoxLayout *fileLayout;
    QLineEdit *txtFilePath;
    QPushButton *btnBrowse;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *AddSoundDialog)
    {
        if (AddSoundDialog->objectName().isEmpty())
            AddSoundDialog->setObjectName("AddSoundDialog");
        AddSoundDialog->resize(450, 120);
        AddSoundDialog->setModal(true);
        verticalLayout = new QVBoxLayout(AddSoundDialog);
        verticalLayout->setObjectName("verticalLayout");
        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        lblName = new QLabel(AddSoundDialog);
        lblName->setObjectName("lblName");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, lblName);

        txtName = new QLineEdit(AddSoundDialog);
        txtName->setObjectName("txtName");

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, txtName);

        lblFile = new QLabel(AddSoundDialog);
        lblFile->setObjectName("lblFile");

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, lblFile);

        fileLayout = new QHBoxLayout();
        fileLayout->setObjectName("fileLayout");
        txtFilePath = new QLineEdit(AddSoundDialog);
        txtFilePath->setObjectName("txtFilePath");
        txtFilePath->setReadOnly(true);

        fileLayout->addWidget(txtFilePath);

        btnBrowse = new QPushButton(AddSoundDialog);
        btnBrowse->setObjectName("btnBrowse");
        btnBrowse->setMinimumWidth(80);

        fileLayout->addWidget(btnBrowse);


        formLayout->setLayout(1, QFormLayout::ItemRole::FieldRole, fileLayout);


        verticalLayout->addLayout(formLayout);

        buttonBox = new QDialogButtonBox(AddSoundDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(AddSoundDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, AddSoundDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, AddSoundDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(AddSoundDialog);
    } // setupUi

    void retranslateUi(QDialog *AddSoundDialog)
    {
        AddSoundDialog->setWindowTitle(QCoreApplication::translate("AddSoundDialog", "Add Sound", nullptr));
        lblName->setText(QCoreApplication::translate("AddSoundDialog", "Display Name:", nullptr));
        txtName->setPlaceholderText(QCoreApplication::translate("AddSoundDialog", "Enter a name for this sound", nullptr));
        lblFile->setText(QCoreApplication::translate("AddSoundDialog", "Audio File:", nullptr));
        txtFilePath->setPlaceholderText(QCoreApplication::translate("AddSoundDialog", "No file selected", nullptr));
        btnBrowse->setText(QCoreApplication::translate("AddSoundDialog", "Browse...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AddSoundDialog: public Ui_AddSoundDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADDSOUNDDIALOG_H
