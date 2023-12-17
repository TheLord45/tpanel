/*
 * Copyright (C) 2022 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TQEDITLINE_H__
#define __TQEDITLINE_H__

#include <string>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTextEdit;
class QLabel;
class QHBoxLayout;
class TQSingleLine;
class TQMultiLine;
QT_END_NAMESPACE

class TQEditLine : public QWidget
{
    Q_OBJECT

    public:
        TQEditLine(QWidget *widget = nullptr, bool multiline=false);
        TQEditLine(std::string &text, QWidget *widget = nullptr, bool multiline=false);
        ~TQEditLine();

        void setText(std::string &text);
        void setObjectName(const std::string& name);
        void setPasswordChar(uint c);
        std::string& getText() { return mText; }
        void setFixedSize(int w, int h);
        void setFont(QFont &font);
        void setTextColor(QColor col);
        void setPalette(QPalette &pal);
        void setBackgroundPixmap(QPixmap& pixmap);
        void grabGesture(Qt::GestureType type, Qt::GestureFlags flags = Qt::GestureFlags());
        void setPadding(int left, int top, int right, int bottom);
        void setFrameSize(int s);
        void setWordWrapMode(bool mode = false);
        void clear();
        void setInputMask(const std::string& mask);
        void setNumericInput();
#ifndef __ANDROID__
        void setCursor(const QCursor &qc);
        void setClearButtonEnabled(bool state);
#else
        inline void setCursor(const QCursor&) {}
        inline void setClearButtonEnabled(bool) {}
#endif
        void setHandle(ulong handle) { mHandle = handle; }

    signals:
        void inputChanged(ulong handle, std::string& text);
        void cursorPositionChanged(ulong handle, int oldPos, int newPos);
        void focusChanged(ulong handle, bool in);

    protected:
        void hideEvent(QHideEvent *event) override;
        void leaveEvent(QEvent *event) override;
        void closeEvent(QCloseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

        void onTextChanged(const QString &text);
        void onTextAreaChanged();
        void onCursorPositionChangedS(int oldPos, int newPos);
        void onEditingFinished();
        void onFocusChanged(bool in);
        void onKeyPressed(int);

    private:
        void init();
        void _end();

        QHBoxLayout *mLayout{nullptr};
        TQSingleLine *mEdit{nullptr};
        TQMultiLine *mTextArea{nullptr};
        QPixmap mBackground;

        std::string mText;
        ulong mHandle{0};
        bool mChanged{false};
        bool mMultiline{false};

        int mPadLeft{0};
        int mPadTop{0};
        int mPadRight{0};
        int mPadBottom{0};
        int mWidth{0};
        int mHeight{0};
        int mPosX{0};
        int mPosY{0};
};

#endif
