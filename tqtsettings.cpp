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

#define RLOG_INFO           0x00fe
#define RLOG_WARNING        0x00fd
#define RLOG_ERROR          0x00fb
#define RLOG_TRACE          0x00f7
#define RLOG_DEBUG          0x00ef
#define RLOG_PROTOCOL       0x00f8
#define RLOG_ALL            0x00e0

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
    mInitRun = false;
    mSetChanged = false;
}

TQtSettings::~TQtSettings()
{
	DECL_TRACER("TQtSettings::~TQtSettings()");
	delete ui;
}

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
//    QString fileName = QQmlFile::urlToLocalFileOrQrc(fname);
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
    size = ui->scrollArea->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->scrollArea->resize(size);
    rect = ui->scrollArea->geometry();
    ui->scrollArea->move(scale(rect.left()), scale(rect.top()));

    size = ui->scrollAreaWidgetContents->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->scrollAreaWidgetContents->resize(size);
    rect = ui->scrollAreaWidgetContents->geometry();
    ui->scrollAreaWidgetContents->move(scale(rect.left()), scale(rect.top()));

    // labels
    QFont font = ui->label_Channel->font();
    font.setPointSizeF(12.0);
    size = ui->label_Channel->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->label_Channel->resize(size);
    rect = ui->label_Channel->geometry();
    ui->label_Channel->move(scale(rect.left()), scale(rect.top()));
    ui->label_Channel->setFont(font);

    ui->label_Controller->resize(size);
    rect = ui->label_Controller->geometry();
    ui->label_Controller->move(scale(rect.left()), scale(rect.top()));
    ui->label_Controller->setFont(font);

    ui->label_PType->resize(size);
    rect = ui->label_PType->geometry();
    ui->label_PType->move(scale(rect.left()), scale(rect.top()));
    ui->label_PType->setFont(font);

    ui->label_PType_2->resize(size);
    rect = ui->label_PType_2->geometry();
    ui->label_PType_2->move(scale(rect.left()), scale(rect.top()));
    ui->label_PType_2->setFont(font);

    ui->label_Port->resize(size);
    rect = ui->label_Port->geometry();
    ui->label_Port->move(scale(rect.left()), scale(rect.top()));
    ui->label_Port->setFont(font);

    ui->label_logLevel->resize(size);
    rect = ui->label_logLevel->geometry();
    ui->label_logLevel->move(scale(rect.left()), scale(rect.top()));
    ui->label_logLevel->setFont(font);

    ui->label_logFile->resize(size);
    rect = ui->label_logFile->geometry();
    ui->label_logFile->move(scale(rect.left()), scale(rect.top()));
    ui->label_logFile->setFont(font);

    ui->label_logFormats->resize(size);
    rect = ui->label_logFormats->geometry();
    ui->label_logFormats->move(scale(rect.left()), scale(rect.top()));
    ui->label_logFormats->setFont(font);

    // Inputs
    size = ui->lineEdit_Controller->size();
    rect = ui->lineEdit_Controller->geometry();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->lineEdit_Controller->resize(size);
    rect = ui->lineEdit_Controller->geometry();
    ui->lineEdit_Controller->move(scale(rect.left()), scale(rect.top()));
    ui->lineEdit_Controller->setFont(font);

    size = ui->lineEdit_PType->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->lineEdit_PType->resize(size);
    rect = ui->lineEdit_PType->geometry();
    ui->lineEdit_PType->move(scale(rect.left()), scale(rect.top()));
    ui->lineEdit_PType->setFont(font);

    size = ui->lineEdit_logFile->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->lineEdit_logFile->resize(size);
    rect = ui->lineEdit_logFile->geometry();
    ui->lineEdit_logFile->move(scale(rect.left()), scale(rect.top()));
    ui->lineEdit_logFile->setFont(font);

    size = ui->kiconbutton_reset->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->kiconbutton_reset->resize(size);
    rect = ui->kiconbutton_reset->geometry();
    ui->kiconbutton_reset->move(scale(rect.left()), scale(rect.top()));
    size = ui->kiconbutton_reset->iconSize();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->kiconbutton_reset->setIconSize(size);

    size = ui->kiconbutton_logFile->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->kiconbutton_logFile->resize(size);
    rect = ui->kiconbutton_logFile->geometry();
    ui->kiconbutton_logFile->move(scale(rect.left()), scale(rect.top()));
    size = ui->kiconbutton_logFile->iconSize();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->kiconbutton_logFile->setIconSize(size);

    size = ui->spinBox_Channel->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->spinBox_Channel->resize(size);
    rect = ui->spinBox_Channel->geometry();
    ui->spinBox_Channel->move(scale(rect.left()), scale(rect.top()));
    ui->spinBox_Channel->setFont(font);

    size = ui->spinBox_Port->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->spinBox_Port->resize(size);
    rect = ui->spinBox_Port->geometry();
    ui->spinBox_Port->move(scale(rect.left()), scale(rect.top()));
    ui->spinBox_Port->setFont(font);


    size = ui->checkBox_LogInfo->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->checkBox_LogInfo->resize(size);
    rect = ui->checkBox_LogInfo->geometry();
    ui->checkBox_LogInfo->move(scale(rect.left()), scale(rect.top()));
    ui->checkBox_LogInfo->setFont(font);

    size = ui->checkBox_LogWarning->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->checkBox_LogWarning->resize(size);
    rect = ui->checkBox_LogWarning->geometry();
    ui->checkBox_LogWarning->move(scale(rect.left()), scale(rect.top()));
    ui->checkBox_LogWarning->setFont(font);

    size = ui->checkBox_LogError->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->checkBox_LogError->resize(size);
    rect = ui->checkBox_LogError->geometry();
    ui->checkBox_LogError->move(scale(rect.left()), scale(rect.top()));
    ui->checkBox_LogError->setFont(font);

    size = ui->checkBox_LogTrace->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->checkBox_LogTrace->resize(size);
    rect = ui->checkBox_LogTrace->geometry();
    ui->checkBox_LogTrace->move(scale(rect.left()), scale(rect.top()));
    ui->checkBox_LogTrace->setFont(font);

    size = ui->checkBox_LogDebug->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->checkBox_LogDebug->resize(size);
    rect = ui->checkBox_LogDebug->geometry();
    ui->checkBox_LogDebug->move(scale(rect.left()), scale(rect.top()));
    ui->checkBox_LogDebug->setFont(font);

    size = ui->checkBox_LogProtocol->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->checkBox_LogProtocol->resize(size);
    rect = ui->checkBox_LogProtocol->geometry();
    ui->checkBox_LogProtocol->move(scale(rect.left()), scale(rect.top()));
    ui->checkBox_LogProtocol->setFont(font);

    size = ui->checkBox_LogAll->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->checkBox_LogAll->resize(size);
    rect = ui->checkBox_LogAll->geometry();
    ui->checkBox_LogAll->move(scale(rect.left()), scale(rect.top()));
    ui->checkBox_LogAll->setFont(font);

    size = ui->checkBox_Format->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->checkBox_Format->resize(size);
    rect = ui->checkBox_Format->geometry();
    ui->checkBox_Format->move(scale(rect.left()), scale(rect.top()));
    ui->checkBox_Format->setFont(font);

    size = ui->checkBox_Profiling->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->checkBox_Profiling->resize(size);
    rect = ui->checkBox_Profiling->geometry();
    ui->checkBox_Profiling->move(scale(rect.left()), scale(rect.top()));
    ui->checkBox_Profiling->setFont(font);

    size = ui->checkBox_Scale->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->checkBox_Scale->resize(size);
    rect = ui->checkBox_Scale->geometry();
    ui->checkBox_Scale->move(scale(rect.left()), scale(rect.top()));
    ui->checkBox_Scale->setFont(font);

    size = ui->buttonBox->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    ui->buttonBox->resize(size);
    rect = ui->buttonBox->geometry();
    ui->buttonBox->move(scale(rect.left()), scale(rect.top()));
    ui->buttonBox->setFont(font);
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
    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}

void TQtSettings::on_kiconbutton_reset_clicked()
{
    char *HOME = getenv("HOME");
    QString logFile = TConfig::getLogFile().c_str();

    if (HOME)
    {
        logFile = HOME;
        logFile += "/tpanel/tpanel.log";
    }

    ui->lineEdit_logFile->setText(logFile);
}
