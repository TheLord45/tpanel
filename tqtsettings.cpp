/*
 * Copyright (C) 2020, 2023 by Andreas Theofilu <andreas@theosys.at>
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
#ifdef QT5_LINUX
#include <QAudioOutput>
#elif defined(QT6_LINUX) && defined(__ANDROID__)
#include <QtCore/private/qandroidextras_p.h>
#endif
#include <QScreen>
#include <QGuiApplication>
#include <QFont>

#include <unistd.h>

#include "tqtsettings.h"
#include "tdirectory.h"
#include "tresources.h"
#include "tpagemanager.h"
#include "terror.h"
#include "ui_tqtsettings.h"

#ifdef __ANDROID__
#   ifdef QT5_LINUX
#       include <QtAndroidExtras/QAndroidJniObject>
#       include <QtAndroidExtras/QtAndroid>
#   endif
#   include <QtQml/QQmlFile>
#   include <android/log.h>
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
using std::sort;

extern TPageManager *gPageManager;

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
    mFileList = tinit.getFileList(".tp4");
    ui->comboBox_FTPsurface->clear();
    string curSurface = TConfig::getFtpSurface();

    if (mFileList.size() == 0)
        ui->comboBox_FTPsurface->addItem(curSurface.c_str());
    else
    {
        ui->comboBox_FTPsurface->clear();
        vector<TTPInit::FILELIST_t>::iterator iter;
        int idx = 0;
        int newIdx = -1;

        for (iter = mFileList.begin(); iter != mFileList.end(); ++iter)
        {
            ui->comboBox_FTPsurface->addItem(iter->fname.c_str());

            if (iter->fname.compare(curSurface) == 0)
                newIdx = idx;

            idx++;
        }

        if (newIdx != -1)
        {
            ui->comboBox_FTPsurface->setCurrentIndex(newIdx);
            mIndex = newIdx;
        }
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
    ui->checkBox_ToolbarSuppress->setCheckState((TConfig::getToolbarSuppress() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));

    if (TConfig::getToolbarSuppress())
        ui->checkBox_Toolbar->setDisabled(true);

    ui->lineEdit_SIPproxy->setText(TConfig::getSIPproxy().c_str());
    ui->spinBox_SIPport->setValue(TConfig::getSIPport());
    ui->spinBox_SIPportTLS->setValue(TConfig::getSIPportTLS());
    ui->lineEdit_SIPstun->setText(TConfig::getSIPstun().c_str());
    ui->lineEdit_SIPdomain->setText(TConfig::getSIPdomain().c_str());
    ui->lineEdit_SIPuser->setText(TConfig::getSIPuser().c_str());
    ui->lineEdit_SIPpassword->setText(TConfig::getSIPpassword().c_str());
    ui->checkBox_SIPnetworkIPv4->setCheckState(TConfig::getSIPnetworkIPv4() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->checkBox_SIPnetworkIPv6->setCheckState(TConfig::getSIPnetworkIPv6() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->checkBox_SIPiphone->setCheckState(TConfig::getSIPiphone() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    int fwIdx = 0;

    switch(TConfig::getSIPfirewall())
    {
        case TConfig::SIP_NO_FIREWALL:  fwIdx = 0; break;
        case TConfig::SIP_STUN:         fwIdx = 1; break;
        case TConfig::SIP_ICE:          fwIdx = 2; break;
        case TConfig::SIP_UPNP:         fwIdx = 3; break;

        default:
            fwIdx = 0;
    }

    ui->comboBox_SIPfirewall->setCurrentIndex(fwIdx);
    ui->checkBox_SIPenabled->setCheckState((TConfig::getSIPstatus() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));

    // Sound settings
    string sySound = TConfig::getSystemSound();
    size_t pos = sySound.find_last_of(".");
    int numpos = 0;

    if (pos != string::npos)
        numpos = atoi(sySound.substr(pos - 2, 2).c_str());

    dir::TDirectory dir;
    int num = dir.readDir(TConfig::getProjectPath() + "/__system/graphics/sounds");
    vector<string> tmpFiles;

    for (int i = 0; i < num; i++)
    {
        dir::DFILES_T entry = dir.getEntry(i);
        pos = entry.name.find_last_of("/");
        string file;

        if (pos != string::npos)
            file = entry.name.substr(pos + 1);

        if ((pos = file.find_last_of(".")) != string::npos)
            file = file.substr(0, pos);

        if (startsWith(file, "singleBeep"))
            tmpFiles.push_back(file);
    }

    sort(tmpFiles.begin(), tmpFiles.end());
    vector<string>::iterator iter;

    for (iter = tmpFiles.begin(); iter != tmpFiles.end(); ++iter)
        ui->comboBox_SystemSound->addItem(iter->c_str());

    ui->comboBox_SystemSound->setCurrentIndex(numpos);

    string siBeep = TConfig::getSingleBeepSound();
    pos = siBeep.find_last_of(".");

    if (pos != string::npos)
    {
        int num = atoi(siBeep.substr(pos - 2, 2).c_str());

        if (num > 0)
            siBeep = "Single Beep " + std::to_string(num);
    }

    int sel = 0;

    for (int i = 1; i <= 10; i++)
    {
        QString e = QString("Single Beep %1").arg(i);
        ui->comboBox_SingleBeep->addItem(e);

        if (e == QString(siBeep.c_str()))
            sel = i - 1;
    }

    ui->comboBox_SingleBeep->setCurrentIndex(sel);

    string siDBeep = TConfig::getDoubleBeepSound();
    pos = siDBeep.find_last_of(".");

    if (pos != string::npos)
    {
        int num = atoi(siDBeep.substr(pos - 2, 2).c_str());

        if (num > 0)
            siBeep = "Double Beep " + std::to_string(num);
    }

    sel = 0;

    for (int i = 1; i <= 10; i++)
    {
        QString e = QString("Double Beep %1").arg(i);
        ui->comboBox_DoubleBeep->addItem(e);

        if (e == QString(siBeep.c_str()))
            sel = i - 1;
    }

    ui->comboBox_DoubleBeep->setCurrentIndex(sel);
    ui->checkBox_SystemSound->setCheckState((TConfig::getSystemSoundState() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));

    int volume = TConfig::getSystemVolume();
    int gain = TConfig::getSystemGain();
    ui->horizontalSlider_Volume->setValue(volume);
    ui->horizontalSlider_Gain->setValue(gain);

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
    QColor hightlightedText = Qt::darkGray;
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

#ifdef Q_OS_ANDROID
    QString fileName = fname;

    if (fileName.contains("content://"))
    {
#ifdef QT5_LINUX
        QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
              "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
              QAndroidJniObject::fromString(fileName).object<jstring>());

        fileName =
              QAndroidJniObject::callStaticObjectMethod(
                  "org/qtproject/theosys/UriToPath", "getFileName",
                  "(Landroid/net/Uri;Landroid/content/Context;)Ljava/lang/String;",
                  uri.object(), QtAndroid::androidContext().object()).toString();
#else
        QJniObject uri = QJniObject::callStaticObjectMethod(
              "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
              QJniObject::fromString(fileName).object<jstring>());

        fileName =
              QJniObject::callStaticObjectMethod(
                  "org/qtproject/theosys/UriToPath", "getFileName",
                  "(Landroid/net/Uri;Landroid/content/Context;)Ljava/lang/String;",
                  uri.object(), QNativeInterface::QAndroidApplication::context()).toString();
#endif
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
    QFont font = obj->font();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    obj->resize(size);
    QRect rect = obj->geometry();
    obj->move(scale(rect.left()), scale(rect.top()));

    double fontSize = -1.0;
    int pixelSize = -1;

    if (font.pointSizeF() > 0.0 && mRatioFont != 1.0)
        fontSize = font.pointSizeF() * mRatioFont;
    else if (font.pointSize() > 0 && mRatioFont != 1.0)
        fontSize = font.pointSize() * mRatioFont;
    else if (font.pixelSize() > 0 && mRatioFont != 1.0)
        pixelSize = (int)((double)font.pixelSize() * mRatioFont);
    else if (font.pixelSize() >= 8)
        return;

    if (fontSize >= 6.0)
        font.setPointSizeF(fontSize);
    else if (pixelSize >= 8)
        font.setPixelSize(pixelSize);
    else
        font.setPointSizeF(11.0);

    obj->setFont(font);
}

void TQtSettings::doResize()
{
    DECL_TRACER("TQtSettings::doResize()");

    // The main dialog window
    QSize size = this->size();
    QRect rect = this->geometry();
    QRect rectGui = QGuiApplication::primaryScreen()->geometry();
    qreal height = 0.0;
    qreal width = 0.0;

    if (gPageManager->getSettings()->isPortrait())
    {
        height = qMax(rectGui.width(), rectGui.height());
        width = qMin(rectGui.width(), rectGui.height());

        if (rectGui.width() > rectGui.height())
            mScaleFactor = qMin(height / rect.height(), width / rect.width());
    }
    else
    {
        height = qMin(rectGui.width(), rectGui.height());
        width = qMax(rectGui.width(), rectGui.height());

        if (rectGui.width() < rectGui.height())
            mScaleFactor = qMax(height / rect.height(), width / rect.width());
    }

    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    this->resize(size);

    QWidget *parent = this->parentWidget();

    if (rect.width() > rectGui.width() || rect.height() > rectGui.height())
        this->move(0, 0);
    else if (parent && rect.width() < rectGui.width() && rect.height() < rectGui.height())
    {
        rect = parent->geometry();
        this->move(rect.center() - this->rect().center());
    }
    else
        this->move(scale(rect.left()), scale(rect.top()));

    // Font size calculation
    double refWidth = gPageManager->getSettings()->getWidth();
    double refHeight = gPageManager->getSettings()->getHeight();
    double dpi = QGuiApplication::primaryScreen()->logicalDotsPerInch();
    double amxDpi = dpi / ((width > height) ? width : height) * ((refWidth > refHeight) ? refWidth : refHeight);
    mRatioFont = qMin(height * amxDpi / (dpi * refHeight), width * amxDpi / (dpi * refWidth));
//    MSG_PROTOCOL("qMin(" << height << " * " << amxDpi << " / " << "(" << dpi << " * " << refHeight << "), " << width << " * " << amxDpi << " / " << "(" << dpi << " * " << refWidth << "))");
//    MSG_PROTOCOL("Original DPI: " << dpi << ", initial AMX DPI: " << amxDpi << ", font scale factor: " << mRatioFont);
    // Layout
    // Iterate through childs and resize them
    QObjectList childs = children();
    QList<QObject *>::Iterator iter;

    for (iter = childs.begin(); iter != childs.end(); ++iter)
    {
        QObject *obj = *iter;
        QString name = obj->objectName();

        if (name.startsWith("tabWidget"))
        {
            QTabWidget *tw = dynamic_cast<QTabWidget *>(obj);
            scaleObject(tw);
#ifdef __ANDROID__
            QColor txt = qt_fusionPalette().Text;
            tw->setStyleSheet(QString("color: %1").arg(txt.name()));
#endif
            QObjectList childsTab = obj->children();
            QList<QObject *>::Iterator iterTab;

            for (iterTab = childsTab.begin(); iterTab != childsTab.end(); ++iterTab)
            {
                QObject *objt = *iterTab;
                QString namet = objt->objectName();

                if (namet.startsWith("qt_tabwidget_stackedwidget"))
                {
                    QObjectList childsStack = objt->children();
                    QList<QObject *>::Iterator iterStack;

                    for (iterStack = childsStack.begin(); iterStack != childsStack.end(); ++iterStack)
                    {
                        QObject *objStack = *iterStack;
                        QObjectList tabStack = objStack->children();
                        QList<QObject *>::Iterator tabIter;

                        for (tabIter = tabStack.begin(); tabIter != tabStack.end(); ++tabIter)
                        {
                            QObject *on = *tabIter;
                            QString n = on->objectName();

                            if (n.startsWith("kiconbutton") || n.startsWith("toolButton"))
                                scaleObject(dynamic_cast<QToolButton *>(on));
                            else if (n.startsWith("checkBox"))
                            {
                                QCheckBox *cb = dynamic_cast<QCheckBox *>(on);
                                scaleObject(cb);
#ifdef __ANDROID__
                                cb->setPalette(qt_fusionPalette());
                                QSize size = cb->size();
                                QString ss = QString("spacing:%1;indicator:{width:%2px;height:%3px;}").arg(scale(5)).arg(size.height()).arg(size.height());
                                MSG_PROTOCOL("CheckBox style sheet: " << ss.toStdString());
                                cb->setStyleSheet(ss);
#endif
                            }
                            else if (n.startsWith("lineEdit"))
                            {
                                QLineEdit *le = dynamic_cast<QLineEdit *>(on);
                                scaleObject(le);
                            }
                            else if (n.startsWith("spinBox"))
                            {
                                QSpinBox *sb = dynamic_cast<QSpinBox *>(on);
                                scaleObject(sb);
                                QFont font = sb->font();

                                if (font.pixelSize() > 0)
                                    sb->setStyleSheet(QString("font-size: %1px").arg(font.pixelSize()));
                            }
                            else if (n.startsWith("comboBox"))
                            {
                                QComboBox *cb = dynamic_cast<QComboBox *>(on);
                                scaleObject(cb);
                                QFont font = cb->font();
#ifdef __ANDROID__
                                if (font.pixelSize() > 0)
                                {
                                    cb->setPalette(qt_fusionPalette());
                                    QColor txt = qt_fusionPalette().text().color();
                                    QColor back = qt_fusionPalette().button().color();
                                    QColor base = qt_fusionPalette().base().color();
                                    QString ss;

                                    ss = QString("font-size:%1px;color:%2;editable:{background-color:%3};drop-down:{color:%4;background:%5}")
                                            .arg(font.pixelSize())
                                            .arg(txt.name())
                                            .arg(back.name())
                                            .arg(txt.name())
                                            .arg(base.name());

                                    MSG_PROTOCOL("ComboBox style sheet: " << ss.toStdString());
                                    cb->setStyleSheet(ss);
                                }
#endif
                            }
                            else if (n.startsWith("horizontalSlider") || n.startsWith("verticalSlider"))
                                scaleObject(dynamic_cast<QSlider *>(on));
                            else if (n.startsWith("label"))
                            {
                                QLabel *lb = dynamic_cast<QLabel *>(on);
                                scaleObject(lb);
#ifdef __ANDROID__
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

string TQtSettings::getSelectedFtpFile()
{
    DECL_TRACER("TQtSettings::getSelectedFtpFile()");

    if ((size_t)mIndex < mFileList.size())
        return mFileList[mIndex].fname;

    return string();
}

size_t TQtSettings::getSelectedFtpFileSize()
{
    DECL_TRACER("TQtSettings::getSelectedFtpFileSize()");

    if ((size_t)mIndex < mFileList.size())
        return mFileList[mIndex].size;

    return 0;
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

void TQtSettings::on_checkBox_ToolbarSuppress_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_ToolbarSuppress_toggled(bool checked)");

    if (TConfig::getToolbarSuppress() == checked)
        return;

    mSetChanged = true;
    TConfig::saveToolbarSuppress(checked);
    ui->checkBox_Toolbar->setDisabled(checked);
}

void TQtSettings::on_checkBox_Toolbar_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_Toolbar_toggled(bool checked)");

    if (TConfig::getToolbarForce() == checked)
        return;

    mSetChanged = true;
    TConfig::saveToolbarForce(checked);
}

void TQtSettings::on_checkBox_Rotation_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_Rotation_toggled(bool checked)");

    if (TConfig::getRotationFixed() == checked)
        return;

    mSetChanged = true;
    TConfig::setRotationFixed(checked);
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

    if (value <= 0 || mScaleFactor == 1.0 || mScaleFactor < 0.0)
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

void TQtSettings::on_comboBox_FTPsurface_currentTextChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_comboBox_FTPsurface_currentIndexChanged(const QString& arg1)");
MSG_DEBUG("Comparing surface: " << arg1.toStdString() << " with " << TConfig::getFtpSurface());
    if (arg1.compare(TConfig::getFtpSurface().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::saveFtpSurface(arg1.toStdString());
    MSG_DEBUG("Surface was set to " << arg1.toStdString());

    vector<TTPInit::FILELIST_t>::iterator iter;
    int idx = 0;

    for (iter = mFileList.begin(); iter != mFileList.end(); ++iter)
    {
        if (iter->fname.compare(arg1.toStdString()) == 0)
        {
            mIndex = idx;
            break;
        }

        idx++;
    }
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

void TQtSettings::on_spinBox_SIPportTLS_valueChanged(int arg1)
{
    DECL_TRACER("TQtSettings::on_spinBoxTLS_SIPport_valueChanged(int arg1)");

    if (TConfig::getSIPportTLS() == arg1)
        return;

    mSetChanged = true;
    TConfig::setSIPportTLS(arg1);
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

void TQtSettings::on_checkBox_SIPnetworkIPv4_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_SIPnetworkIPv4_toggled(bool checked)");

    if (TConfig::getSIPnetworkIPv4() == checked)
        return;

    mSetChanged = true;
    TConfig::setSIPnetworkIPv4(checked);
}

void TQtSettings::on_checkBox_SIPnetworkIPv6_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_SIPnetworkIPv6_toggled(bool checked)");

    if (TConfig::getSIPnetworkIPv6() == checked)
        return;

    mSetChanged = true;
    TConfig::setSIPnetworkIPv6(checked);
}

void TQtSettings::on_comboBox_SIPfirewall_currentIndexChanged(int index)
{
    DECL_TRACER("TQtSettings::on_comboBox_SIPfirewall_currentIndexChanged(const QString& arg1)");

    TConfig::SIP_FIREWALL_t fw;

    switch(index)
    {
        case 0: fw = TConfig::SIP_NO_FIREWALL; break;
        case 1: fw = TConfig::SIP_STUN; break;
        case 2: fw = TConfig::SIP_ICE; break;
        case 3: fw = TConfig::SIP_UPNP; break;

        default:
            fw = TConfig::SIP_NO_FIREWALL;
    }

    if (TConfig::getSIPfirewall() == fw)
        return;

    mSetChanged = true;
    TConfig::setSIPfirewall(fw);
}

void TQtSettings::on_checkBox_SIPenabled_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_SIPenabled_toggled(bool checked)");

    if (TConfig::getSIPstatus() == checked)
        return;

    mSetChanged = true;
    TConfig::setSIPstatus(checked);
}

void TQtSettings::on_checkBox_SIPiphone_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_SIPiphone_toggled(bool checked)");

    if (TConfig::getSIPiphone() == checked)
        return;

    mSetChanged = true;
    TConfig::setSIPiphone(checked);
}

void TQtSettings::on_comboBox_SystemSound_currentTextChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_comboBox_SystemSound_currentIndexChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getSystemSound().c_str()) == 0)
        return;

    mSetChanged = true;
    TConfig::saveSystemSoundFile(arg1.toStdString() + ".wav");
}

void TQtSettings::on_toolButton_SystemSound_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButton_SystemSound_clicked()");

    QString selected = ui->comboBox_SystemSound->currentText();
    string file = TConfig::getProjectPath() + "/__system/graphics/sounds/" + selected.toStdString() + ".wav";

    if (gPageManager)
        gPageManager->getCallPlaySound()(file);
}

void TQtSettings::on_comboBox_SingleBeep_currentTextChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_comboBox_SingleBeep_currentIndexChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getSingleBeepSound().c_str()) == 0)
        return;

    string file = arg1.toStdString();
    size_t pos = file.find_last_of(" ");

    if (pos != string::npos)
    {
        int num = atoi(file.substr(pos).c_str());
        file = "singleBeep";

        if (num > 0)
        {
            if (num < 10)
                file += "0" + std::to_string(num);
            else
                file += std::to_string(num);
        }

        file += ".wav";
    }

    mSetChanged = true;
    TConfig::saveSingleBeepFile(file);
}

void TQtSettings::on_toolButton_SingleBeep_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButton_SingleBeep_clicked()");

    QString selected = ui->comboBox_SingleBeep->currentText();
    string file = selected.toStdString();
    size_t pos = file.find_last_of(" ");

    if (pos != string::npos)
    {
        int num = atoi(file.substr(pos).c_str());
        file = TConfig::getProjectPath() + "/__system/graphics/sounds/singleBeep";

        if (num > 0)
        {
            if (num < 10)
                file += "0" + std::to_string(num);
            else
                file += std::to_string(num);
        }

        file += ".wav";
    }

//    playFile(file);

    if (gPageManager)
        gPageManager->getCallPlaySound()(file);
}

void TQtSettings::on_comboBox_DoubleBeep_currentTextChanged(const QString& arg1)
{
    DECL_TRACER("TQtSettings::on_comboBox_DoubleBeep_currentIndexChanged(const QString& arg1)");

    if (arg1.compare(TConfig::getDoubleBeepSound().c_str()) == 0)
        return;

    string file = arg1.toStdString();
    size_t pos = file.find_last_of(" ");

    if (pos != string::npos)
    {
        int num = atoi(file.substr(pos).c_str());
        file = "doubleBeep";

        if (num > 0)
        {
            if (num < 10)
                file += "0" + std::to_string(num);
            else
                file += std::to_string(num);
        }

        file += ".wav";
    }

    mSetChanged = true;
    TConfig::saveDoubleBeepFile(file);
}

void TQtSettings::on_toolButton_DoubleBeep_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButton_DoubleBeep_clicked()");

    QString selected = ui->comboBox_DoubleBeep->currentText();
    string file = selected.toStdString();
    size_t pos = file.find_last_of(" ");

    if (pos != string::npos)
    {
        int num = atoi(file.substr(pos).c_str());
        file = TConfig::getProjectPath() + "/__system/graphics/sounds/doubleBeep";

        if (num > 0)
        {
            if (num < 10)
                file += "0" + std::to_string(num);
            else
                file += std::to_string(num);
        }

        file += ".wav";
    }


//    playFile(file);

    if (gPageManager)
        gPageManager->getCallPlaySound()(file);
}

void TQtSettings::on_checkBox_SystemSound_toggled(bool checked)
{
    DECL_TRACER("TQtSettings::on_checkBox_SystemSound_toggled(bool checked)");

    if (TConfig::getSystemSoundState() == checked)
        return;

    mSetChanged = true;
    TConfig::saveSystemSoundState(checked);
}

void TQtSettings::on_toolButton_TestSound_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButton_TestSound_clicked()");

    if (gPageManager)
        gPageManager->getCallPlaySound()(TConfig::getProjectPath() + "/__system/graphics/sounds/audioTest.wav");
}

void TQtSettings::on_horizontalSlider_Volume_valueChanged(int value)
{
    DECL_TRACER("TQtSettings::on_horizontalSlider_Volume_valueChanged(int value)");

    if (TConfig::getSystemVolume() == value)
        return;

    TConfig::saveSystemVolume(value);
}

void TQtSettings::on_horizontalSlider_Gain_valueChanged(int value)
{
    DECL_TRACER("TQtSettings::on_horizontalSlider_Gain_valueChanged(int value)");

    if (TConfig::getSystemGain() == value)
        return;

    TConfig::saveSystemGain(value);
}
