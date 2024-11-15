/*
 * Copyright (C) 2021, 2022 by Andreas Theofilu <andreas@theosys.at>
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
#ifndef TTPINIT_H
#define TTPINIT_H

#include <string>
#include <vector>
#include <functional>

#define MAX_TMP_LEN     10

#ifdef __MACH__
typedef off_t off64_t;
#endif

class TTPInit
{
    public:
        TTPInit(const std::string& path);
        TTPInit();

        typedef struct FILELIST_t
        {
            size_t size;
            std::string fname;
        }FILELIST_t;

        void setPath(const std::string& p);
        bool createDirectoryStructure();
        bool loadSurfaceFromController(bool force=false);
        std::vector<FILELIST_t>& getFileList(const std::string& filter);
        bool isSystemDefault();
        bool isVirgin();
        bool makeSystemFiles();
        bool reinitialize();

        int progressCallback(off64_t xfer);
        void setFileSize(off64_t fs) { mFileSize = fs; }
        off64_t getFileSize() { return mFileSize; }
        off64_t getFileSize(const std::string& file);
        void regCallbackProcessEvents(std::function<void ()> pe) { _processEvents = pe; }
        void regCallbackProgressBar(std::function<void (int percent)> pb) { _progressBar = pb; }
        static void setTP5(bool tp) { mIsTP5 = tp; }
        static bool getTP5() { return mIsTP5; }

        static bool haveSystemMarker();

    protected:
        bool testForTp5();

    private:
        std::function<void ()> _processEvents{nullptr};
        std::function<void (int percent)> _progressBar{nullptr};

        bool createPanelConfigs();
        bool createSystemConfigs();
        bool createDemoPage();
        bool _makeDir(const std::string& dir);
        bool copyFile(const std::string& fname);
        std::string getTmpFileName();
        void logging(int level, const std::string& msg);
#ifdef __ANDROID__
        bool askPermissions();
#endif
        std::string mPath;
        std::vector<FILELIST_t> mDirList;
        off64_t mFileSize{0};
        bool mDirStructureCreated{false};
        bool mPanelConfigsCreated{false};
        bool mSystemConfigsCreated{false};
        bool mDemoPageCreated{false};
        static bool mIsTP5;
};

#endif // TTPINIT_H
