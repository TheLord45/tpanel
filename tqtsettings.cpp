/*
 * Copyright (C) 2020, 2021 by Andreas Theofilu <andreas@theosys.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */
#include <QFileDialog>
#include <QComboBox>
#include <QMessageBox>

#include <unistd.h>

#include "tqtsettings.h"
#include "terror.h"
#include "ui_tqtsettings.h"

#ifdef __ANDROID__
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroid>
#include <QtQml/QQmlFile>
#include <android/log.h>
#endif

#include "tconfig.h"
#include "ttpinit.h"

#define RLOG_INFO           0x00fe
#define RLOG_WARNING        0x00fd
#define RLOG_ERROR          0x00fb
#define RLOG_TRACE          0x00f7
#define RLOG_DEBUG          0x00ef
#define RLOG_PROTOCOL       0x00f8
#define RLOG_ALL            0x00e0

using std::string;
using std::vector;

TQtSettings::TQtSettings(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui::TQtSettings)
{
	DECL_TRACER("TQtSettings::TQtSettings(QWidget *parent)");

    mInitRun = true;
	ui->setupUi(this);

    ui->lineEdit_logFile->setText(TConfig::getLogFile().c_str());
    ui->lineEdit_Controller->setText(TConfig::getController().c_str());
    ui->lineEdit_PType->setText(TConfig::getPanelType().c_str());
    ui->spinBox_Port->setValue(TConfig::getPort());
    ui->spinBox_Channel->setValue(TConfig::getChannel());
    ui->lineEdit_FTPuser->setText(TConfig::getFtpUser().c_str());
    ui->lineEdit_FTPpassword->setText(TConfig::getFtpPassword().c_str());

    TTPInit tinit;
    tinit.setPath(TConfig::getProjectPath());
    vector<string> list = tinit.getFileList(".tp4");
    ui->comboBox_FTPsurface->clear();
    string curSurface = TConfig::getFtpSurface();

    if (list.size() == 0)
        ui->comboBox_FTPsurface->addItem(curSurface.c_str());
    else
    {
        ui->comboBox_FTPsurface->clear();
        vector<string>::iterator iter;
        int idx = 0;
        int newIdx = -1;

        for (iter = list.begin(); iter != list.end(); ++iter)
        {
            ui->comboBox_FTPsurface->addItem(iter->c_str());

            if (iter->compare(curSurface) == 0)
                newIdx = idx;

            idx++;
        }

        if (newIdx != -1)
            ui->comboBox_FTPsurface->setCurrentIndex(newIdx);
    }

    ui->checkBox_FTPpassive->setCheckState((TConfig::getFtpPassive() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));

    mLogLevel = TConfig::getLogLevelBits();
    ui->checkBox_LogInfo->setCheckState((mLogLevel & HLOG_INFO) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->checkBox_LogWarning->setCheckState((mLogLevel & HLOG_WARNING) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->checkBox_LogError->setCheckState((mLogLevel & HLOG_ERROR) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->checkBox_LogTrace->setCheckState((mLogLevel & HLOG_TRACE) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->checkBox_LogDebug->setCheckState((mLogLevel & HLOG_DEBUG) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->checkBox_LogProtocol->setCheckState(((mLogLevel & HLOG_PROTOCOL) == HLOG_PROTOCOL) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->checkBox_LogAll->setCheckState(((mLogLevel & HLOG_ALL) == HLOG_ALL) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    ui->checkBox_Format->setCheckState((TConfig::isLongFormat() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));
    ui->checkBox_Scale->setCheckState((TConfig::getScale() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));
#ifndef __ANDROID__
    ui->checkBox_Scale->setDisabled(true);
#endif
    ui->checkBox_Banner->setCheckState((TConfig::showBanner() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));
#ifdef __ANDROID__
    ui->checkBox_Banner->setDisabled(true);
#endif
    ui->checkBox_Toolbar->setCheckState((TConfig::getToolbarForce() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));

    ui->lineEdit_SIPproxy->setText(TConfig::getSIPproxy().c_str());
    ui->spinBox_SIPport->setValue(TConfig::getSIPport());
    ui->lineEdit_SIPstun->setText(TConfig::getSIPstun().c_str());
    ui->lineEdit_SIPdomain->setText(TConfig::getSIPdomain().c_str());
    ui->lineEdit_SIPuser->setText(TConfig::getSIPuser().c_str());
    ui->lineEdit_SIPpassword->setText(TConfig::getSIPpassword().c_str());
    ui->checkBox_SIPenabled->setCheckState((TConfig::getSIPstatus() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));
#ifdef __ANDROID__
    ui->tabWidget->setPalette(qt_fusionPalette());
    ui->tabCtrl->setPalette(qt_fusionPalette());
    ui->tabLog->setPalette(qt_fusionPalette());
    ui->tabSIP->setPalette(qt_fusionPalette());
    ui->tabView->setPalette(qt_fusionPalette());
#endif
    mInitRun = false;
    mSetChanged = false;
}

TQtSettings::~TQtSettings()
{
	DECL_TRACER("TQtSettings::~TQtSettings()");
	delete ui;
}

#ifdef __ANDROID__
QPalette TQtSettings::qt_fusionPalette()
{
    QColor backGround(239, 239, 239);
    QColor light = backGround.lighter(150);
    QColor mid(backGround.darker(130));
    QColor midLight = mid.lighter(110);
    QColor base = Qt::white;
    QColor disabledBase(backGround);
    QColor dark = backGround.darker(150);
    QColor darkDisabled = QColor(209, 209, 209).darker(110);
    QColor text = Qt::black;
    QColor hightlightedText = Qt::white;
    QColor disabledText = QColor(190, 190, 190);
    QColor button = backGround;
    QColor shadow = dark.darker(135);
    QColor disabledShadow = shadow.lighter(150);

    QPalette fusionPalette(Qt::black,backGround,light,dark,mid,text,base);
    fusionPalette.setBrush(QPalette::Midlight, midLight);
    fusionPalette.setBrush(QPalette::Button, button);
    fusionPalette.setBrush(QPalette::Shadow, shadow);
    fusionPalette.setBrush(QPalette::HighlightedText, hightlightedText);

    fusionPalette.setBrush(QPalette::Disabled, QPalette::Text, disabledText);
    fusionPalette.setBrush(QPalette::Disabled, QPalette::WindowText, disabledText);
    fusionPalette.setBrush(QPalette::Disabled, QPalette::ButtonText, disabledText);
    fusionPalette.setBrush(QPalette::Disabled, QPalette::Base, disabledBase);
    fusionPalette.setBrush(QPalette::Disabled, QPalette::Dark, darkDisabled);
    fusionPalette.setBrush(QPalette::Disabled, QPalette::Shadow, disabledShadow);

    fusionPalette.setBrush(QPalette::Active, QPalette::Highlight, QColor(48, 140, 198));
    fusionPalette.setBrush(QPalette::Inactive, QPalette::Highlight, QColor(48, 140, 198));
    fusionPalette.setBrush(QPalette::Disabled, QPalette::Highlight, QColor(145, 145, 145));
    return fusionPalette;
}
#endif

void TQtSettings::on_kiconbutton_logFile_clicked()
{
    DECL_TRACER("TQtSettings::on_kiconbutton_logFile_clicked()");

    std::string pt = TConfig::getLogFile();
    size_t pos = pt.find_last_of("/");

    if (pos != std::string::npos)
        pt = pt.substr(0, pos);
    else
    {
        char hv0[4096];
        getcwd(hv0, sizeof(hv0));
        pt = hv0;
    }

    QString actPath(pt.c_str());
    QFileDialog fdialog(this, tr("Logfile"), actPath, tr("TPanel.log (*.log)"));
    fdialog.setAcceptMode(QFileDialog::AcceptSave);
    fdialog.setDefaultSuffix("log");
    fdialog.setOption(QFileDialog::DontConfirmOverwrite);
    QString fname;

    if (fdialog.exec())
    {
        QDir dir = fdialog.directory();
        QStringList list = fdialog.selectedFiles();

        if (list.size() > 0)
            fname = dir.absoluteFilePath(list.at(0));
        else
            return;
    }
    else
        return;

#ifdef __ANDROID__
    QString fileName = fname;

    if (fileName.contains("content://"))
    {
        QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
              "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
              QAndroidJniObject::fromString(fileName).object<jstring>());

        fileName =
              QAndroidJniObject::callStaticObjectMethod(
                  "org/qtproject/theosys/UriToPath", "getFileName",
                  "(Landroid/net/Uri;Landroid/content/Context;)Ljava/lang/String;",
                  uri.object(), QtAndroid::androidContext().object()).toString();

        if (fileName.length() > 0)
            fname = fileName;
    }
    else
    {
        MSG_WARNING("Not an Uri? (" << fname.toStdString() << ")");
    }
#endif
    ui->lineEdit_logFile->setText(fname);

    if (TConfig::getLogFile().compare(fname.toStdString()) != 0)
    {
        mSetChanged = true;
        TConfig::saveLogFile(fname.toStdString());
    }
}

template<typename T>
void TQtSettings::scaleObject(T *obj)
{
    DECL_TRACER("TQtSettings::scaleObject(T *obj)");

    QSize size = obj->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    obj->resize(size);
    QRect rect = obj->geometry();
    obj->move(scale(rect.left()), scale(rect.top()));
}

void TQtSettings::doResize()
{
    DECL_TRACER("TQtSettings::doResize()");

    // The main dialog window
    QSize size = this->size();
    QRect rect = this->geometry();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    this->resize(size);
    this->move(scale(rect.left()), scale(rect.top()));
    QWidget *parent = this->parentWidget();

    if (parent)
    {
        rect = parent->geometry();
        this->move(rect.center() - this->rect().center());
    }

    // Layout
    // Iterate through childs and resize them
    QObjectList childs = children();
    QList<QObject *>::Iterator iter;

    for (iter = childs.begin(); iter != childs.end(); ++iter)
    {
        QString name = iter.i->t()->objectName();
        QObject *obj = iter.i->t();

        if (name.startsWith("tabWidget"))
        {
            scaleObject(dynamic_cast<QTabWidget *>(obj));

            QObjectList childsTab = obj->children();
            QList<QObject *>::Iterator iterTab;

            for (iterTab = childsTab.begin(); iterTab != childsTab.end(); ++iterTab)
            {
                QString namet = iterTab.i->t()->objectName();
                QObject *objt = iterTab.i->t();

                if (namet.startsWith("qt_tabwidget_stackedwidget"))
                {
                    QObjectList childsStack = objt->children();
                    QList<QObject *>::Iterator iterStack;

                    for (iterStack = childsStack.begin(); iterStack != childsStack.end(); ++iterStack)
                    {
                        QObjectList tabStack = iterStack.i->t()->children();
                        QList<QObject *>::Iterator tabIter;

                        for (tabIter = tabStack.begin(); tabIter != tabStack.end(); ++tabIter)
                        {
                            QString n = tabIter.i->t()->objectName();
                            QObject *on = tabIter.i->t();

                            if (n.startsWith("kiconbutton"))
                                scaleObject(dynamic_cast<QToolButton *>(on));
                            else if (n.startsWith("checkBox"))
                            {
                                scaleObject(dynamic_cast<QCheckBox *>(on));
#ifdef __ANDROID__
                                QCheckBox *cb = dynamic_cast<QCheckBox *>(on);
                                cb->setPalette(qt_fusionPalette());
#endif
                            }
                            else if (n.startsWith("lineEdit"))
                                scaleObject(dynamic_cast<QLineEdit *>(on));
                            else if (n.startsWith("spinBox"))
                                scaleObject(dynamic_cast<QSpinBox *>(on));
                            else if (n.startsWith("comboBox"))
                                scaleObject(dynamic_cast<QComboBox *>(on));
                            else if (n.startsWith("label"))
                            {
                                scaleObject(dynamic_cast<QLabel *>(on));
#ifdef __ANDROID__
                                QLabel *lb = dynamic_cast<QLabel *>(on);
                                lb->setPalette(qt_fusionPalette());
#endif
                            }
                        }
                    }

//                    if (namet.startsWith("tab"))
//                        scaleObject(dynamic_cast<QWidget *>(objt));
                }
            }
        }
        else if (name.startsWith("buttonBox"))
            scaleObject(dynamic_cast<QDialogButtonBox *>(obj));
    }
}

void TQtSettings::on_lineEdit_logFile_textChanged(const QString &arg1)
{
    DECL_TRACER("TQtSettings::on_lineEdit_logFile_textChanged(const QString &arg1)");

    if (arg1.compare(TConfig::getLogFile().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::saveLogFile(arg1.toStdString());
}

void TQtSettings::on_lineEdit_Controller_textChanged(const QString &arg1)
{
    DECL_TRACER("TQtSettings::on_lineEdit_Controller_textChanged(const QString &arg1)");

    if (arg1.compare(TConfig::getController().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::saveController(arg1.toStdString());
}

void TQtSettings::on_spinBox_Port_valueChanged(int arg1)
{
    DECL_TRACER("TQtSettings::on_spinBox_Port_valueChanged(int arg1)");

    if (arg1 == TConfig::getPort())
        return;

    mSetChanged = true;
    TConfig::savePort(arg1);
}

void TQtSettings::on_spinBox_Channel_valueChanged(int arg1)
{
    DECL_TRACER("TQtSettings::on_spinBox_Channel_valueChanged(int arg1)");

    if (arg1 == TConfig::getChannel())
        return;

    mSetChanged = true;
    TConfig::saveChannel(arg1);
}

void TQtSettings::on_lineEdit_PType_textChanged(const QString &arg1)
{
    DECL_TRACER("TQtSettings::on_lineEdit_PType_textChanged(const QString &arg1)");

    if (arg1.compare(TConfig::getPanelType().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::savePanelType(arg1.toStdString());
}

void TQtSettings::on_checkBox_Format_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_Format_toggled(bool checked)");

    if (TConfig::isLongFormat() == checked)
        return;

    mSetChanged = true;
    TConfig::saveFormat(checked);
}

void TQtSettings::on_checkBox_Scale_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_Scale_toggled(bool checked)");

    if (TConfig::getScale() == checked)
        return;

    mSetChanged = true;
    TConfig::saveScale(checked);
}

void TQtSettings::on_checkBox_Banner_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_Banner_toggled(bool checked)");

    if (TConfig::showBanner() == checked)
        return;

    mSetChanged = true;
    TConfig::saveBanner(!checked);
}

void TQtSettings::on_checkBox_Toolbar_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_Toolbar_toggled(bool checked)");

    if (TConfig::getToolbarForce() == checked)
        return;

    mSetChanged = true;
    TConfig::saveToolbarForce(checked);
}

void TQtSettings::on_checkBox_Profiling_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_Profiling_toggled(bool checked)");

    if (TConfig::getProfiling() == checked)
        return;

    mSetChanged = true;
    TConfig::saveProfiling(checked);
}

void TQtSettings::on_checkBox_LogInfo_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_LogInfo_toggled(bool checked)");

    if (mInitRun)
        return;

    if (checked && !(mLogLevel & HLOG_INFO))
    {
        mLogLevel |= HLOG_INFO;
        mSetChanged = true;
    }
    else if (!checked && (mLogLevel & HLOG_INFO))
    {
        mSetChanged = true;
        mLogLevel &= RLOG_INFO;
    }

    mInitRun = true;
    if ((mLogLevel & HLOG_PROTOCOL) != HLOG_PROTOCOL)
        ui->checkBox_LogProtocol->setCheckState(Qt::CheckState::Unchecked);

    if ((mLogLevel & HLOG_ALL) != HLOG_ALL)
        ui->checkBox_LogAll->setCheckState(Qt::CheckState::Unchecked);

    mInitRun = false;
    TConfig::saveLogLevel(mLogLevel);
}

void TQtSettings::on_checkBox_LogWarning_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_LogWarning_toggled(bool checked)");

    if (mInitRun)
        return;

    if (checked && !(mLogLevel & HLOG_WARNING))
    {
        mLogLevel |= HLOG_WARNING;
        mSetChanged = true;
    }
    else if (!checked && (mLogLevel & HLOG_WARNING))
    {
        mLogLevel &= RLOG_WARNING;
        mSetChanged = true;
    }

    mInitRun = true;
    if ((mLogLevel & HLOG_PROTOCOL) != HLOG_PROTOCOL)
        ui->checkBox_LogProtocol->setCheckState(Qt::CheckState::Unchecked);

    if ((mLogLevel & HLOG_ALL) != HLOG_ALL)
        ui->checkBox_LogAll->setCheckState(Qt::CheckState::Unchecked);

    mInitRun = false;
    TConfig::saveLogLevel(mLogLevel);
}

void TQtSettings::on_checkBox_LogError_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_LogError_toggled(bool checked)");

    if (mInitRun)
        return;

    if (checked && !(mLogLevel & HLOG_ERROR))
    {
        mSetChanged = true;
        mLogLevel |= HLOG_ERROR;
    }
    else if (!checked && (mLogLevel & HLOG_ERROR))
    {
        mLogLevel &= RLOG_ERROR;
        mSetChanged = true;
    }

    mInitRun = true;
    if ((mLogLevel & HLOG_PROTOCOL) != HLOG_PROTOCOL)
        ui->checkBox_LogProtocol->setCheckState(Qt::CheckState::Unchecked);

    if ((mLogLevel & HLOG_ALL) != HLOG_ALL)
        ui->checkBox_LogAll->setCheckState(Qt::CheckState::Unchecked);

    mInitRun = false;
    TConfig::saveLogLevel(mLogLevel);
}

void TQtSettings::on_checkBox_LogTrace_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_LogTrace_toggled(bool checked)");

    if (mInitRun)
        return;

    if (checked && !(mLogLevel & HLOG_TRACE))
    {
        mLogLevel |= HLOG_TRACE;
        mSetChanged = true;
    }
    else if (!checked && (mLogLevel & HLOG_TRACE))
    {
        mLogLevel &= RLOG_TRACE;
        mSetChanged = true;
    }

    mInitRun = true;
    if ((mLogLevel & HLOG_PROTOCOL) != HLOG_PROTOCOL)
        ui->checkBox_LogProtocol->setCheckState(Qt::CheckState::Unchecked);

    if ((mLogLevel & HLOG_ALL) != HLOG_ALL)
        ui->checkBox_LogAll->setCheckState(Qt::CheckState::Unchecked);

    mInitRun = false;
    TConfig::saveLogLevel(mLogLevel);
}

void TQtSettings::on_checkBox_LogDebug_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_LogDebug_toggled(bool checked)");

    if (mInitRun)
        return;

    if (checked && !(mLogLevel & HLOG_DEBUG))
    {
        mLogLevel |= HLOG_DEBUG;
        mSetChanged = true;
    }
    else if (!checked && (mLogLevel & HLOG_DEBUG))
    {
        mLogLevel &= RLOG_DEBUG;
        mSetChanged = true;
    }

    mInitRun = true;
    if ((mLogLevel & HLOG_PROTOCOL) != HLOG_PROTOCOL)
        ui->checkBox_LogProtocol->setCheckState(Qt::CheckState::Unchecked);

    if ((mLogLevel & HLOG_ALL) != HLOG_ALL)
        ui->checkBox_LogAll->setCheckState(Qt::CheckState::Unchecked);

    mInitRun = false;
    TConfig::saveLogLevel(mLogLevel);
}

void TQtSettings::on_checkBox_LogProtocol_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_LogProtocol_toggled(bool checked)");

    if (mInitRun)
        return;

    if (checked && (mLogLevel & HLOG_PROTOCOL) != HLOG_PROTOCOL)
    {
        mLogLevel = HLOG_PROTOCOL;
        mInitRun = true;
        ui->checkBox_LogInfo->setCheckState(Qt::CheckState::Checked);
        ui->checkBox_LogWarning->setCheckState(Qt::CheckState::Checked);
        ui->checkBox_LogError->setCheckState(Qt::CheckState::Checked);
        ui->checkBox_LogTrace->setCheckState(Qt::CheckState::Unchecked);
        ui->checkBox_LogDebug->setCheckState(Qt::CheckState::Unchecked);
        TConfig::saveLogLevel(mLogLevel);
        mInitRun = false;
        mSetChanged = true;
    }
}

void TQtSettings::on_checkBox_LogAll_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_LogAll_toggled(bool checked)");

    if (mInitRun)
        return;

    if (checked && (mLogLevel & HLOG_ALL) != HLOG_ALL)
    {
        mLogLevel = HLOG_ALL;
        mInitRun = true;
        ui->checkBox_LogInfo->setCheckState(Qt::CheckState::Checked);
        ui->checkBox_LogWarning->setCheckState(Qt::CheckState::Checked);
        ui->checkBox_LogError->setCheckState(Qt::CheckState::Checked);
        ui->checkBox_LogTrace->setCheckState(Qt::CheckState::Checked);
        ui->checkBox_LogDebug->setCheckState(Qt::CheckState::Checked);
        TConfig::saveLogLevel(mLogLevel);
        mInitRun = false;
        mSetChanged = true;
    }
}

int TQtSettings::scale(int value)
{
    DECL_TRACER("TQtSettings::scale(int value)");

    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}

void TQtSettings::on_kiconbutton_reset_clicked()
{
    DECL_TRACER("TQtSettings::on_kiconbutton_reset_clicked()");

    char *HOME = getenv("HOME");
    QString logFile = TConfig::getLogFile().c_str();

    if (HOME)
    {
        logFile = HOME;
        logFile += "/tpanel/tpanel.log";
    }

    ui->lineEdit_logFile->setText(logFile);
}

void TQtSettings::on_lineEdit_FTPuser_textChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_lineEdit_FTPuser_textChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getFtpUser().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::saveFtpUser(arg1.toStdString());
}

void TQtSettings::on_lineEdit_FTPpassword_textChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_lineEdit_FTPpassword_textChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getFtpPassword().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::saveFtpPassword(arg1.toStdString());
}

void TQtSettings::on_comboBox_FTPsurface_currentIndexChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_comboBox_FTPsurface_currentIndexChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getFtpSurface().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::saveFtpSurface(arg1.toStdString());
    MSG_DEBUG("Surface was set to " << arg1.toStdString());
}

void TQtSettings::on_toolButton_Download_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButton_Download_clicked()");

    QMessageBox box(this);
    QString text = ui->comboBox_FTPsurface->currentText();

    box.setText("Do you realy want to download and install the surface <b>" + text + "</b>?");
    box.addButton(QMessageBox::Yes);
    box.addButton(QMessageBox::No);
    int ret = box.exec();

    if (ret == QMessageBox::Yes)
    {
        mDownloadForce = true;
        QString qss = QString("background-color: rgba(250, 0, 0, 0.9)");
        ui->toolButton_Download->setStyleSheet(qss);
    }
    else
    {
        mDownloadForce = false;
        QString qss = QString("background-color: rgba(209, 209, 209, 0.9)");
        ui->toolButton_Download->setStyleSheet(qss);
    }

    MSG_DEBUG("mDownloadForce=" << (mDownloadForce ? "YES" : "NO"));
}

void TQtSettings::on_checkBox_FTPpassive_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_FTPpassive_toggled(bool checked)");

    if (TConfig::getFtpPassive() == checked)
        return;

    mSetChanged = true;
    TConfig::saveFtpPassive(checked);
}

void TQtSettings::on_lineEdit_SIPproxy_textChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_lineEdit_SIPproxy_textChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getSIPproxy().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::setSIPproxy(arg1.toStdString());
}

void TQtSettings::on_spinBox_SIPport_valueChanged(int arg1)
{
    DECL_TRACER("TQtSettings::on_spinBox_SIPport_valueChanged(int arg1)");

    if (TConfig::getSIPport() == arg1)
        return;

    mSetChanged = true;
    TConfig::setSIPport(arg1);
}

void TQtSettings::on_lineEdit_SIPstun_textChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_lineEdit_SIPstun_textChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getSIPstun().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::setSIPstun(arg1.toStdString());
}

void TQtSettings::on_lineEdit_SIPdomain_textChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_lineEdit_SIPdomain_textChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getSIPdomain().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::setSIPdomain(arg1.toStdString());
}

void TQtSettings::on_lineEdit_SIPuser_textChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_lineEdit_SIPuser_textChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getSIPuser().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::setSIPuser(arg1.toStdString());
}

void TQtSettings::on_lineEdit_SIPpassword_textChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_lineEdit_SIPpassword_textChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getSIPpassword().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::setSIPpassword(arg1.toStdString());
}

void TQtSettings::on_checkBox_SIPenabled_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_SIPenabled_toggled(bool checked)");

    if (TConfig::getSIPstatus() == checked)
        return;

    mSetChanged = true;
    TConfig::setSIPstatus(checked);
}
