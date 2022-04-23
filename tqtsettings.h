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

#ifndef __TQTSETTINGS_H__
#define __TQTSETTINGS_H__

#include <QDialog>

namespace Ui
{
	class TQtSettings;
}

class TQtSettings : public QDialog
{
	Q_OBJECT

	public:
		explicit TQtSettings(QWidget *parent = nullptr);
		~TQtSettings();
        bool hasChanged() { return mSetChanged; }
        void setScaleFactor(double sf) { mScaleFactor = sf; }
        void doResize();
        uint getLogLevel() { return mLogLevel; }
        bool downloadForce() { return mDownloadForce; }

    private slots:
        // Logging
        void on_kiconbutton_logFile_clicked();
        void on_lineEdit_logFile_textChanged(const QString &arg1);
        void on_checkBox_Format_toggled(bool checked);
        void on_checkBox_Profiling_toggled(bool checked);
        void on_checkBox_LogInfo_toggled(bool checked);
        void on_checkBox_LogWarning_toggled(bool checked);
        void on_checkBox_LogError_toggled(bool checked);
        void on_checkBox_LogTrace_toggled(bool checked);
        void on_checkBox_LogDebug_toggled(bool checked);
        void on_checkBox_LogProtocol_toggled(bool checked);
        void on_checkBox_LogAll_toggled(bool checked);
        void on_kiconbutton_reset_clicked();
        // Controller
        void on_lineEdit_Controller_textChanged(const QString &arg1);
        void on_spinBox_Port_valueChanged(int arg1);
        void on_spinBox_Channel_valueChanged(int arg1);
        void on_lineEdit_PType_textChanged(const QString &arg1);
        void on_lineEdit_FTPuser_textChanged(const QString &arg1);
        void on_lineEdit_FTPpassword_textChanged(const QString &arg1);
        void on_comboBox_FTPsurface_currentIndexChanged(const QString &arg1);
        void on_toolButton_Download_clicked();
        void on_checkBox_FTPpassive_toggled(bool checked);
        // SIP
        void on_lineEdit_SIPproxy_textChanged(const QString &arg1);
        void on_spinBox_SIPport_valueChanged(int arg1);
        void on_spinBox_SIPportTLS_valueChanged(int arg1);
        void on_lineEdit_SIPstun_textChanged(const QString &arg1);
        void on_lineEdit_SIPdomain_textChanged(const QString &arg1);
        void on_lineEdit_SIPuser_textChanged(const QString &arg1);
        void on_lineEdit_SIPpassword_textChanged(const QString &arg1);
        void on_checkBox_SIPnetworkIPv4_toggled(bool checked);
        void on_checkBox_SIPnetworkIPv6_toggled(bool checked);
        void on_comboBox_SIPfirewall_currentIndexChanged(int index);
        void on_checkBox_SIPenabled_toggled(bool checked);
        void on_checkBox_SIPiphone_toggled(bool checked);
        // View
        void on_checkBox_Scale_toggled(bool checked);
        void on_checkBox_Banner_toggled(bool checked);
        void on_checkBox_Toolbar_toggled(bool checked);
        void on_checkBox_Rotation_toggled(bool checked);

    private:
        int scale(int value);
#ifdef __ANDROID__
        QPalette qt_fusionPalette();
#endif
        template <typename T>
        void scaleObject(T *obj);

        Ui::TQtSettings *ui{nullptr};
        bool mSetChanged{false};
        double mScaleFactor{1.0};
        uint mLogLevel{0};
        bool mInitRun{false};
        bool mDownloadForce{false};
};

#endif
